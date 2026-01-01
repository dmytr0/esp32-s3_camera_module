/*
 * ESP32-S3 Motion Detection Camera
 *
 * Features:
 * - Motion detection using frame comparison
 * - Save photos to SD card on motion detection
 * - Send alerts to Telegram bot with photos
 * - Configurable sensitivity and cooldown
 *
 * Hardware: ESP32-S3 SPK with OV2640 camera
 *
 * CONFIGURATION:
 * Edit user_config.ino to change all settings:
 * - WiFi credentials
 * - Telegram bot token and chat ID
 * - Motion detection threshold and sensitivity
 * - SD card and Telegram options
 */

#include "WiFi.h"
#include "config.h"
#include "camera.h"
#include "sd_storage.h"
#include "telegram.h"
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include "SD.h"
#include "FS.h"

// RGB LED (SK6812)
Adafruit_NeoPixel led(1, LED_PIN, NEO_GRB + NEO_KHZ800);

// Motion detection state
unsigned long lastCheckTime = 0;

// Queue for photo burst (series of 3 photos)
QueueHandle_t photoBurstQueue = NULL;

// Structure for motion event (burst of photos)
struct PhotoBurst {
    uint8_t* photoData[MAX_PHOTOS_PER_BURST];
    size_t photoSize[MAX_PHOTOS_PER_BURST];
    int photoCount;
};

// Detection task handle
TaskHandle_t detectionTaskHandle = NULL;
TaskHandle_t telegramTaskHandle = NULL;
volatile bool systemReady = false;

// Task for PIR motion detection on Core 1
void motionDetectionTask(void* parameter) {
    // Setup PIR sensor
    pinMode(PIR_PIN, INPUT);
    if (DEBUG_SERIAL_ENABLED) {
        Serial.println("✓ PIR sensor initialized on GPIO" + String(PIR_PIN));
    }

    // PIR sensor warm-up period (30 seconds)
    const unsigned long PIR_WARMUP_MS = 30000;
    unsigned long pirStartTime = 0;  // Will be set when systemReady
    bool warmupMessageShown = false;

    while (true) {
        // Wait for system ready without blocking
        if (!systemReady) {
            delay(100);
            continue;
        }

        // Set warm-up start time once when system becomes ready
        if (pirStartTime == 0) {
            pirStartTime = millis();
        }

        unsigned long currentTime = millis();
        bool pirReady = (currentTime - pirStartTime >= PIR_WARMUP_MS);

        // Check PIR sensor state
        if (digitalRead(PIR_PIN) == HIGH) {
            // PIR triggered - check if warm-up complete
            if (!pirReady) {
                // Still warming up - log and ignore
                if (DEBUG_SERIAL_ENABLED && !warmupMessageShown) {
                    Serial.println("⏳ PIR triggered but still warming up (ignoring for 30 sec)");
                    warmupMessageShown = true;
                }
                delay(100);
                continue;
            }

            // PIR is HIGH - verify it stays HIGH for minimum duration (filter false triggers)
            unsigned long triggerStartTime = millis();
            bool validTrigger = true;

            while (millis() - triggerStartTime < PIR_TRIGGER_DURATION_MS) {
                if (digitalRead(PIR_PIN) == LOW) {
                    // PIR went LOW too quickly - false trigger
                    validTrigger = false;
                    if (DEBUG_SERIAL_ENABLED) {
                        Serial.println("⚠️ PIR false trigger detected (too short)");
                    }
                    break;
                }
                delay(10);  // Check every 10ms
            }

            if (!validTrigger) {
                delay(100);
                continue;
            }

            // Valid trigger - process motion
            if (DEBUG_SERIAL_ENABLED) {
                Serial.println("🔍 PIR sensor triggered!");
            }

            // Turn on LED during photo capture
            setLED(0, 51, 0);  // Green 20%

            // Capture burst of 3 photos
            PhotoBurst burst;
            burst.photoCount = 0;

            for (int i = 0; i < PHOTOS_PER_BURST; i++) {
                camera_fb_t* fbJpeg = capturePhoto();
                if (fbJpeg) {
                    // Copy photo data
                    burst.photoData[i] = (uint8_t*)malloc(fbJpeg->len);
                    if (burst.photoData[i]) {
                        memcpy(burst.photoData[i], fbJpeg->buf, fbJpeg->len);
                        burst.photoSize[i] = fbJpeg->len;
                        burst.photoCount++;
                    }
                    releasePhoto(fbJpeg);
                }
                delay(PHOTO_BURST_DELAY_MS);  // Configurable delay between photos
            }

            ledOff();

            // Send burst to queue
            if (burst.photoCount > 0) {
                if (xQueueSend(photoBurstQueue, &burst, 0) != pdTRUE) {
                    // Queue full - free memory
                    for (int i = 0; i < burst.photoCount; i++) {
                        free(burst.photoData[i]);
                    }
                }
            }

            // Wait for cooldown AND for PIR to go LOW (prevent immediate re-trigger)
            if (DEBUG_SERIAL_ENABLED) {
                Serial.println("⏳ Cooldown period + waiting for PIR to stabilize...");
            }

            unsigned long cooldownStart = millis();
            bool pirWentLow = false;

            // Wait for cooldown period
            while (millis() - cooldownStart < MOTION_COOLDOWN_MS) {
                if (digitalRead(PIR_PIN) == LOW) {
                    pirWentLow = true;
                }
                delay(100);
            }

            // After cooldown, ensure PIR is LOW before allowing next trigger
            if (!pirWentLow || digitalRead(PIR_PIN) == HIGH) {
                if (DEBUG_SERIAL_ENABLED) {
                    Serial.println("⚠️ PIR still HIGH after cooldown - waiting for it to stabilize...");
                }

                // Wait up to 30 seconds for PIR to go LOW
                unsigned long waitStart = millis();
                while (digitalRead(PIR_PIN) == HIGH && (millis() - waitStart < 30000)) {
                    delay(500);
                }

                // Additional 2 second delay after PIR goes LOW
                delay(2000);
            }

            if (DEBUG_SERIAL_ENABLED) {
                Serial.println("✓ Ready for next motion detection");
            }
        }

        // Check PIR every 100ms
        delay(100);
    }
}

// Task for saving photos to SD card on Core 0
void sdSaveTask(void* parameter) {
    PhotoBurst burst;

    while (true) {
        if (xQueueReceive(photoBurstQueue, &burst, portMAX_DELAY)) {
            if (DEBUG_SERIAL_ENABLED) {
                Serial.printf("\n🚨 === MOTION DETECTED: %d photos ===\n", burst.photoCount);
            }

            // Flash LED indicator
            flashLED();

            // Check and manage SD card space before saving
            if (SD_CARD_ENABLED && isSDCardMounted()) {
                checkAndManageSpace();

                for (int i = 0; i < burst.photoCount; i++) {
                    // Create temporary frame buffer structure
                    camera_fb_t tempFb;
                    tempFb.buf = burst.photoData[i];
                    tempFb.len = burst.photoSize[i];
                    tempFb.width = 0;
                    tempFb.height = 0;
                    tempFb.format = PIXFORMAT_JPEG;

                    bool saved = savePhotoToSD(&tempFb);
                    if (DEBUG_SERIAL_ENABLED) {
                        Serial.printf("Photo %d SD save: %s\n", i+1, saved ? "✓ Success" : "✗ Failed");
                    }
                }
            }

            if (DEBUG_SERIAL_ENABLED) {
                Serial.println("=========================\n");
            }

            // Free allocated memory
            for (int i = 0; i < burst.photoCount; i++) {
                free(burst.photoData[i]);
            }
        }
    }
}

// Task for sending photos to Telegram on Core 0
void telegramSenderTask(void* parameter) {
    unsigned long lastSendTime = 0;
    String fileList[50];  // Buffer for file list

    while (true) {
        unsigned long currentTime = millis();

        // Check every 10 seconds if there are new photos to send
        if (TELEGRAM_ENABLED && (currentTime - lastSendTime >= TELEGRAM_BATCH_INTERVAL_MS)) {
            lastSendTime = currentTime;

            if (DEBUG_SERIAL_ENABLED) {
                Serial.println("\n⏰ Telegram sender: checking for unsent photos...");
            }

            if (SD_CARD_ENABLED && isSDCardMounted()) {
                // Get list of unsent photos
                int photoCount = getUnsentPhotos(fileList, 50);

                if (DEBUG_SERIAL_ENABLED) {
                    Serial.printf("📊 Found %d unsent photos\n", photoCount);
                }

                if (photoCount > 0) {

                    // Send photos in batches (max 10 at a time)
                    for (int batch = 0; batch < photoCount; batch += TELEGRAM_MAX_BATCH_SIZE) {
                        int batchSize = min(TELEGRAM_MAX_BATCH_SIZE, photoCount - batch);

                        // Prepare batch file list
                        String batchFiles[TELEGRAM_MAX_BATCH_SIZE];
                        for (int i = 0; i < batchSize; i++) {
                            batchFiles[i] = fileList[batch + i];
                        }

                        // Send batch
                        if (DEBUG_SERIAL_ENABLED) {
                            Serial.printf("📤 Sending batch of %d photos...\n", batchSize);
                        }

                        bool success = sendPhotosBatch(batchFiles, batchSize);

                        if (success) {
                            // Mark all photos in batch as sent
                            for (int i = 0; i < batchSize; i++) {
                                markPhotoAsSent(batchFiles[i]);
                            }
                        } else {
                            if (DEBUG_SERIAL_ENABLED) {
                                Serial.println("✗ Batch sending failed");
                            }
                        }

                        // Delay between batches
                        if (batch + batchSize < photoCount) {
                            delay(3000);  // 3 second delay between batches
                        }
                    }
                }
            }
        }

        delay(1000);  // Check every second
    }
}

// RGB LED control
void setLED(uint8_t r, uint8_t g, uint8_t b) {
    if (LED_INDICATOR_ENABLED) {
        led.setPixelColor(0, led.Color(r, g, b));
        led.show();
    }
}

void ledOff() {
    if (LED_INDICATOR_ENABLED) {
        led.setPixelColor(0, 0);
        led.show();
    }
}

void flashLED() {
    // Green flash at 20% brightness (51/255)
    setLED(0, 51, 0);
    delay(LED_FLASH_DURATION_MS);
    ledOff();
}


void setup() {
    // Initialize serial
    if (DEBUG_SERIAL_ENABLED) {
        Serial.begin(115200);
        delay(500);
        Serial.println("\n\n=================================");
        Serial.println("ESP32-S3 Motion Detection Camera");
        Serial.println("=================================\n");
    }

    // Initialize RGB LED
    if (LED_INDICATOR_ENABLED) {
        led.begin();
        led.setBrightness(255);  // Full brightness (we'll control via RGB values)
        ledOff();
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("✓ RGB LED initialized");
        }
    }

    // Initialize camera
    if (!initCamera()) {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("✗ Camera initialization failed!");
        }
        return;
    }
    if (DEBUG_SERIAL_ENABLED) {
        Serial.println("✓ Camera initialized");
    }

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    if (DEBUG_SERIAL_ENABLED) {
        Serial.print("Connecting to WiFi");
    }

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        if (DEBUG_SERIAL_ENABLED) {
            Serial.print(".");
        }
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println();
            Serial.printf("✓ WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
        }

        // Initialize Telegram bot
        if (TELEGRAM_ENABLED) {
            if (initTelegram()) {
                if (DEBUG_SERIAL_ENABLED) {
                    Serial.println("✓ Telegram bot initialized");
                }
            } else {
                if (DEBUG_SERIAL_ENABLED) {
                    Serial.println("✗ Telegram initialization failed");
                }
            }
        }
    } else {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println();
            Serial.println("✗ WiFi connection failed");
            Serial.println("  Telegram notifications will not work");
        }
    }

    // Initialize SD card
    if (SD_CARD_ENABLED) {
        if (initSDCard()) {
            if (DEBUG_SERIAL_ENABLED) {
                Serial.println("✓ SD Card initialized");
            }
        } else {
            if (DEBUG_SERIAL_ENABLED) {
                Serial.println("✗ SD Card initialization failed");
            }
        }
    }

    // Create photo burst queue
    photoBurstQueue = xQueueCreate(5, sizeof(PhotoBurst));
    if (!photoBurstQueue) {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("✗ Failed to create photo burst queue");
        }
    } else {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("✓ Photo burst queue created");
        }
    }

    // Start SD save task on Core 0
    xTaskCreatePinnedToCore(
        sdSaveTask,             // Task function
        "SDSave",               // Task name
        8192,                   // Stack size
        NULL,                   // Parameters
        1,                      // Priority (low)
        NULL,                   // Task handle
        0                       // Core 0
    );

    // Start Telegram sender task on Core 0
    xTaskCreatePinnedToCore(
        telegramSenderTask,     // Task function
        "TelegramSender",       // Task name
        16384,                  // Stack size (larger for HTTP)
        NULL,                   // Parameters
        1,                      // Priority (low)
        &telegramTaskHandle,    // Task handle
        0                       // Core 0
    );

    // Start motion detection task on Core 1 (fast detection)
    xTaskCreatePinnedToCore(
        motionDetectionTask,    // Task function
        "MotionDetection",      // Task name
        8192,                   // Stack size
        NULL,                   // Parameters
        2,                      // Priority (higher than handler)
        &detectionTaskHandle,   // Task handle
        1                       // Core 1 (separate from handler)
    );

    if (DEBUG_SERIAL_ENABLED) {
        Serial.println("✓ SD save task started (Core 0)");
        Serial.println("✓ Telegram sender task started (Core 0)");
        Serial.println("✓ Motion detection task started (Core 1)");
        Serial.println("\n=================================");
        Serial.println("System ready! Monitoring for motion...");
        Serial.println("=================================\n");
    }

    // Signal that system is ready
    systemReady = true;
}

void loop() {
    // Check for Serial commands
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command == "ERASE_ALL") {
            Serial.println("\n⚠️  ERASING ALL PHOTOS...");

            if (SD_CARD_ENABLED && isSDCardMounted()) {
                int deletedCount = eraseAllPhotos();
                Serial.printf("✓ Deleted %d photos from SD card\n", deletedCount);

                // Reset lastSent in Preferences
                Preferences prefs;
                prefs.begin("motion-cam", false);
                prefs.putULong("lastSent", 0);
                prefs.end();

                Serial.println("✓ Reset sent photo tracking\n");
            } else {
                Serial.println("✗ SD card not available\n");
            }
        }
        else if (command == "STATUS") {
            Serial.println("\n=== SYSTEM STATUS ===");

            if (SD_CARD_ENABLED && isSDCardMounted()) {
                printSDCardInfo();

                // Count photos
                String fileList[100];
                int totalPhotos = 0;
                File dir = SD.open(SD_PHOTO_DIR);
                if (dir && dir.isDirectory()) {
                    File file = dir.openNextFile();
                    while (file) {
                        String filename = String(file.name());
                        if (filename.endsWith(".jpg")) {
                            totalPhotos++;
                        }
                        file = dir.openNextFile();
                    }
                    dir.close();
                }

                int unsentCount = getUnsentPhotos(fileList, 100);

                Serial.printf("Total photos: %d\n", totalPhotos);
                Serial.printf("Unsent photos: %d\n", unsentCount);

                Preferences prefs;
                prefs.begin("motion-cam", true);
                unsigned long lastSent = prefs.getULong("lastSent", 0);
                prefs.end();

                Serial.printf("Last sent timestamp: %lu\n", lastSent);
            } else {
                Serial.println("SD card: Not available");
            }

            Serial.printf("WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
            Serial.printf("Telegram: %s\n", TELEGRAM_ENABLED ? "Enabled" : "Disabled");
            Serial.printf("PIR sensor: Enabled (GPIO%d)\n", PIR_PIN);
            Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
            Serial.println("====================\n");
        }
        else if (command == "HELP") {
            Serial.println("\n=== AVAILABLE COMMANDS ===");
            Serial.println("ERASE_ALL - Delete all photos from SD card");
            Serial.println("STATUS    - Show system status and statistics");
            Serial.println("HELP      - Show this help message");
            Serial.println("==========================\n");
        }
        else {
            Serial.printf("Unknown command: %s\n", command.c_str());
            Serial.println("Type HELP for available commands\n");
        }
    }

    // Keep alive to prevent watchdog
    delay(1000);
}
