#include <Arduino.h>
#include "sd_storage.h"
#include "SD.h"
#include "SPI.h"
#include "FS.h"
#include <Preferences.h>

static bool sdCardMounted = false;
static Preferences preferences;

bool initSDCard() {
    if (!SD_CARD_ENABLED) {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("SD card disabled in config");
        }
        return false;
    }

    if (DEBUG_SERIAL_ENABLED) {
        Serial.println("Attempting to mount SD card (SPI mode)...");
        Serial.printf("Using pins: MISO=%d, MOSI=%d, CLK=%d, CS=%d\n",
                     SD_PIN_MISO, SD_PIN_MOSI, SD_PIN_CLK, SD_PIN_CS);
    }

    // Initialize SPI bus
    SPI.begin(SD_PIN_CLK, SD_PIN_MISO, SD_PIN_MOSI, SD_PIN_CS);

    // Try to mount SD card
    if (!SD.begin(SD_PIN_CS)) {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("SD Card Mount Failed");
            Serial.println("Check: 1) SD card inserted, 2) FAT32 format, 3) Pin connections");
        }
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("No SD Card attached");
        }
        return false;
    }

    sdCardMounted = true;

    // Initialize Preferences for tracking sent photos
    preferences.begin("motion-cam", false);  // false = read/write mode

    // Scan SD card to find maximum photo number
    unsigned long maxPhotoNum = 0;
    File dir = SD.open(SD_PHOTO_DIR);
    if (!dir) {
        // Directory doesn't exist yet - will be created below
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("Photo directory doesn't exist yet");
        }
    } else if (dir.isDirectory()) {
        File file = dir.openNextFile();
        while (file) {
            String filename = String(file.name());

            // Check for photo_XXXXXX.jpg pattern
            if (filename.endsWith(".jpg") && filename.startsWith("photo_")) {
                int startPos = filename.indexOf('_');
                int endPos = filename.indexOf('.');

                if (startPos > 0 && endPos > startPos) {
                    String numStr = filename.substring(startPos + 1, endPos);
                    unsigned long photoNum = numStr.toInt();

                    if (photoNum > maxPhotoNum) {
                        maxPhotoNum = photoNum;
                    }
                }
            }

            file = dir.openNextFile();
        }
        dir.close();
    }

    // Set photoNum to continue from max found number
    if (maxPhotoNum > 0) {
        preferences.putULong("photoNum", maxPhotoNum + 1);
        preferences.putULong("lastSentNum", maxPhotoNum);

        if (DEBUG_SERIAL_ENABLED) {
            Serial.printf("Found %lu photos on SD card\n", maxPhotoNum);
            Serial.printf("Next photo will be: photo_%06lu.jpg\n", maxPhotoNum + 1);
            Serial.printf("Set lastSentNum to: %lu (all existing photos marked as sent)\n", maxPhotoNum);
        }
    } else {
        // No photos on SD card - start from 1
        preferences.putULong("photoNum", 1);
        preferences.putULong("lastSentNum", 0);

        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("No existing photos found - starting from photo_000001.jpg");
        }
    }

    // Create photo directory if it doesn't exist
    if (!SD.exists(SD_PHOTO_DIR)) {
        if (SD.mkdir(SD_PHOTO_DIR)) {
            if (DEBUG_SERIAL_ENABLED) {
                Serial.printf("Created directory: %s\n", SD_PHOTO_DIR);
            }
        } else {
            if (DEBUG_SERIAL_ENABLED) {
                Serial.printf("Failed to create directory: %s\n", SD_PHOTO_DIR);
            }
        }
    }

    if (DEBUG_SERIAL_ENABLED) {
        Serial.println("SD Card initialized successfully");
        printSDCardInfo();
    }

    return true;
}

bool savePhotoToSD(camera_fb_t* fb) {
    if (!sdCardMounted || !fb) {
        return false;
    }

    // Get next photo number from Preferences
    unsigned long photoNumber = preferences.getULong("photoNum", 1);

    // Generate filename with sequential number
    char filename[64];
    snprintf(filename, sizeof(filename), "%s/photo_%06lu.jpg",
             SD_PHOTO_DIR, photoNumber);

    // Open file for writing
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("Failed to open file for writing");
        }
        return false;
    }

    // Write JPEG data
    size_t bytesWritten = file.write(fb->buf, fb->len);
    file.close();

    if (bytesWritten != fb->len) {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("Failed to write complete image");
        }
        return false;
    }

    // Increment photo number for next time
    preferences.putULong("photoNum", photoNumber + 1);

    if (DEBUG_SERIAL_ENABLED) {
        Serial.printf("Photo saved to SD: %s (%d bytes)\n", filename, fb->len);
    }

    return true;
}

void printSDCardInfo() {
    if (!sdCardMounted) {
        return;
    }

    uint8_t cardType = SD.cardType();
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    uint64_t usedSpace = SD.usedBytes() / (1024 * 1024);

    Serial.println("=== SD Card Info ===");
    Serial.print("Type: ");
    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    Serial.printf("Size: %llu MB\n", cardSize);
    Serial.printf("Used: %llu MB\n", usedSpace);
    Serial.printf("Free: %llu MB\n", cardSize - usedSpace);
    Serial.println("====================");
}

bool isSDCardMounted() {
    return sdCardMounted;
}

// Get list of unsent photos (photoNumber > lastSentNum)
int getUnsentPhotos(String* fileList, int maxFiles) {
    if (!sdCardMounted) {
        return 0;
    }

    // Get last sent photo number from Preferences
    unsigned long lastSentNum = preferences.getULong("lastSentNum", 0);

    if (DEBUG_SERIAL_ENABLED) {
        Serial.printf("🔍 getUnsentPhotos: lastSentNum = %lu\n", lastSentNum);
    }

    File dir = SD.open(SD_PHOTO_DIR);
    if (!dir || !dir.isDirectory()) {
        return 0;
    }

    int count = 0;
    File file = dir.openNextFile();

    while (file && count < maxFiles) {
        String filename = String(file.name());

        // Only include .jpg files (photo_XXXXXX.jpg)
        if (filename.endsWith(".jpg") && filename.startsWith("photo_")) {
            // Extract number from filename (photo_000001.jpg)
            int startPos = filename.indexOf('_');
            int endPos = filename.indexOf('.');

            if (startPos > 0 && endPos > startPos) {
                String numStr = filename.substring(startPos + 1, endPos);
                unsigned long photoNum = numStr.toInt();

                // Only include photos newer than last sent
                if (photoNum > lastSentNum) {
                    fileList[count] = String(SD_PHOTO_DIR) + "/" + filename;
                    count++;
                }
            }
        }

        file = dir.openNextFile();
    }

    dir.close();
    return count;
}

// Mark photo as sent by updating lastSentNum in Preferences
bool markPhotoAsSent(const String& filename) {
    if (!sdCardMounted) {
        return false;
    }

    // Extract number from filename
    int startPos = filename.lastIndexOf('_');
    int endPos = filename.lastIndexOf('.');

    if (startPos > 0 && endPos > startPos) {
        String numStr = filename.substring(startPos + 1, endPos);
        unsigned long photoNum = numStr.toInt();

        // Update lastSentNum in Preferences
        preferences.putULong("lastSentNum", photoNum);

        if (DEBUG_SERIAL_ENABLED) {
            Serial.printf("Updated lastSentNum: %lu\n", photoNum);
        }

        return true;
    }

    return false;
}

// Read photo from SD card
uint8_t* readPhotoFromSD(const String& filename, size_t* size) {
    if (!sdCardMounted) {
        return NULL;
    }

    File file = SD.open(filename.c_str(), FILE_READ);
    if (!file) {
        return NULL;
    }

    *size = file.size();
    uint8_t* buffer = (uint8_t*)malloc(*size);

    if (!buffer) {
        file.close();
        return NULL;
    }

    size_t bytesRead = file.read(buffer, *size);
    file.close();

    if (bytesRead != *size) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

// Erase all photos from SD card
int eraseAllPhotos() {
    if (!sdCardMounted) {
        return 0;
    }

    File dir = SD.open(SD_PHOTO_DIR);
    if (!dir || !dir.isDirectory()) {
        return 0;
    }

    int deletedCount = 0;
    File file = dir.openNextFile();

    while (file) {
        String filename = String(SD_PHOTO_DIR) + "/" + String(file.name());

        // Delete all .jpg files (including .sent.jpg)
        if (filename.endsWith(".jpg")) {
            if (SD.remove(filename.c_str())) {
                deletedCount++;
                if (DEBUG_SERIAL_ENABLED) {
                    Serial.printf("Deleted: %s\n", filename.c_str());
                }
            }
        }

        file = dir.openNextFile();
    }

    dir.close();
    return deletedCount;
}

// Delete specific photo
bool deletePhoto(const String& filename) {
    if (!sdCardMounted) {
        return false;
    }

    return SD.remove(filename.c_str());
}

// Get oldest photo filename (lowest photo number)
String getOldestPhoto() {
    if (!sdCardMounted) {
        return "";
    }

    File dir = SD.open(SD_PHOTO_DIR);
    if (!dir || !dir.isDirectory()) {
        return "";
    }

    String oldestFile = "";
    unsigned long oldestNum = 0xFFFFFFFF;

    File file = dir.openNextFile();

    while (file) {
        String filename = String(file.name());

        // Only consider .jpg files (photo_XXXXXX.jpg)
        if (filename.endsWith(".jpg") && filename.startsWith("photo_")) {
            // Extract number from filename (photo_000001.jpg)
            int startPos = filename.indexOf('_');
            int endPos = filename.indexOf('.');

            if (startPos > 0 && endPos > startPos) {
                String numStr = filename.substring(startPos + 1, endPos);
                unsigned long photoNum = numStr.toInt();

                if (photoNum < oldestNum) {
                    oldestNum = photoNum;
                    oldestFile = String(SD_PHOTO_DIR) + "/" + filename;
                }
            }
        }

        file = dir.openNextFile();
    }

    dir.close();
    return oldestFile;
}

// Check free space and delete oldest photos if needed (< 2MB free)
void checkAndManageSpace() {
    if (!sdCardMounted) {
        return;
    }

    uint64_t freeSpace = (SD.cardSize() - SD.usedBytes()) / (1024 * 1024);  // MB

    if (freeSpace < 2) {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.printf("⚠️ Low space: %llu MB free. Deleting oldest photos...\n", freeSpace);
        }

        // Delete oldest photos until we have at least 5MB free
        int deletedCount = 0;
        while (freeSpace < 5 && deletedCount < 50) {  // Max 50 deletions per cycle
            String oldestFile = getOldestPhoto();

            if (oldestFile.length() == 0) {
                break;  // No more photos to delete
            }

            if (deletePhoto(oldestFile)) {
                deletedCount++;
                if (DEBUG_SERIAL_ENABLED) {
                    Serial.printf("Deleted oldest: %s\n", oldestFile.c_str());
                }
            } else {
                break;  // Failed to delete
            }

            // Recalculate free space
            freeSpace = (SD.cardSize() - SD.usedBytes()) / (1024 * 1024);
        }

        if (DEBUG_SERIAL_ENABLED) {
            Serial.printf("✓ Space management: deleted %d photos, %llu MB free\n", deletedCount, freeSpace);
        }
    }
}
