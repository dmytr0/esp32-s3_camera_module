/*
 * ESP32-S3 SD Card Test (SPI Mode)
 *
 * Alternative test using SPI instead of SD_MMC
 * Use this if SD_MMC mode doesn't work on your board
 *
 * Hardware: ESP32-S3 SPK with SD card slot
 */

#include "FS.h"
#include "SD.h"
#include "SPI.h"

// SPI pins for ESP32-S3 SPK (from schematic)
#define SD_CS    2   // CD/DAT3 (CS)
#define SD_MOSI  3   // CMD (DI) -> MOSI
#define SD_MISO  12  // DATA0 (D0) -> MISO
#define SD_CLK   11  // CLK (SCLK)

// Test configuration
#define TEST_FILE_PATH "/test.txt"
#define TEST_DIR_PATH "/testdir"

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n========================================");
    Serial.println("ESP32-S3 SD Card Test (SPI Mode)");
    Serial.println("========================================\n");

    Serial.printf("Using SPI pins:\n");
    Serial.printf("  CS  : GPIO%d\n", SD_CS);
    Serial.printf("  MOSI: GPIO%d\n", SD_MOSI);
    Serial.printf("  MISO: GPIO%d\n", SD_MISO);
    Serial.printf("  CLK : GPIO%d\n\n", SD_CLK);

    // Initialize SPI
    SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);

    // Test: Try to mount SD card
    Serial.println("TEST: Attempting SD.begin()...");
    if (!SD.begin(SD_CS)) {
        Serial.println("✗ FAILED: SD card mount failed\n");

        Serial.println("========================================");
        Serial.println("SD CARD INITIALIZATION FAILED");
        Serial.println("========================================\n");

        Serial.println("Possible issues:");
        Serial.println("1. SD card not inserted properly");
        Serial.println("2. SD card corrupted or damaged");
        Serial.println("3. SD card not formatted (needs FAT32)");
        Serial.println("4. Wrong SPI pins for your board");
        Serial.println("5. SD card doesn't support SPI mode");
        Serial.println("\nTry adjusting pin numbers at top of sketch");
        Serial.println("Check your board schematic for correct SPI pins");
        return;
    }

    Serial.println("✓ SUCCESS: SD card mounted in SPI mode\n");

    runDiagnostics();
    runTests();
}

void runDiagnostics() {
    Serial.println("--- SD CARD INFORMATION ---");

    uint8_t cardType = SD.cardType();

    Serial.print("Card Type: ");
    if (cardType == CARD_NONE) {
        Serial.println("NONE (No card detected)");
        return;
    } else if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC (Standard Capacity)");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC (High Capacity)");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("Card Size: %llu MB\n", cardSize);

    uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
    Serial.printf("Total Space: %llu MB\n", totalBytes);

    uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);
    Serial.printf("Used Space: %llu MB\n", usedBytes);

    uint64_t freeBytes = (SD.totalBytes() - SD.usedBytes()) / (1024 * 1024);
    Serial.printf("Free Space: %llu MB\n", freeBytes);

    Serial.println("---------------------------\n");
}

void runTests() {
    Serial.println("--- FUNCTIONAL TESTS ---\n");

    // Test 1: List root directory
    Serial.println("TEST: List root directory");
    File root = SD.open("/");
    if (!root) {
        Serial.println("✗ Failed to open root directory");
    } else {
        Serial.println("✓ Root directory opened");

        File file = root.openNextFile();
        int count = 0;
        while (file && count < 10) {
            if (file.isDirectory()) {
                Serial.printf("  [DIR]  %s\n", file.name());
            } else {
                Serial.printf("  [FILE] %s (%d bytes)\n", file.name(), file.size());
            }
            file = root.openNextFile();
            count++;
        }
        if (count == 0) {
            Serial.println("  (empty - no files found)");
        }
        root.close();
    }
    Serial.println();

    // Test 2: Create directory
    Serial.println("TEST: Create test directory");
    if (SD.exists(TEST_DIR_PATH)) {
        Serial.printf("  Directory %s already exists\n", TEST_DIR_PATH);
    } else {
        if (SD.mkdir(TEST_DIR_PATH)) {
            Serial.printf("✓ Created directory: %s\n", TEST_DIR_PATH);
        } else {
            Serial.printf("✗ Failed to create directory: %s\n", TEST_DIR_PATH);
        }
    }
    Serial.println();

    // Test 3: Write test file
    Serial.println("TEST: Write test file");
    File file = SD.open(TEST_FILE_PATH, FILE_WRITE);
    if (!file) {
        Serial.println("✗ Failed to open file for writing");
    } else {
        String testData = "ESP32-S3 SD Card Test (SPI) - ";
        testData += millis();
        testData += "\n";

        size_t bytesWritten = file.print(testData);
        file.close();

        if (bytesWritten > 0) {
            Serial.printf("✓ Wrote %d bytes to %s\n", bytesWritten, TEST_FILE_PATH);
            Serial.printf("  Data: %s", testData.c_str());
        } else {
            Serial.println("✗ Failed to write data");
        }
    }
    Serial.println();

    // Test 4: Read test file
    Serial.println("TEST: Read test file");
    file = SD.open(TEST_FILE_PATH, FILE_READ);
    if (!file) {
        Serial.println("✗ Failed to open file for reading");
    } else {
        Serial.printf("✓ Opened file: %s (%d bytes)\n", TEST_FILE_PATH, file.size());
        Serial.print("  Content: ");
        while (file.available()) {
            Serial.write(file.read());
        }
        file.close();
    }
    Serial.println();

    // Test 5: Append to file
    Serial.println("TEST: Append to test file");
    file = SD.open(TEST_FILE_PATH, FILE_APPEND);
    if (!file) {
        Serial.println("✗ Failed to open file for appending");
    } else {
        String appendData = "Append test - ";
        appendData += millis();
        appendData += "\n";

        size_t bytesWritten = file.print(appendData);
        file.close();

        if (bytesWritten > 0) {
            Serial.printf("✓ Appended %d bytes\n", bytesWritten);
        } else {
            Serial.println("✗ Failed to append");
        }
    }
    Serial.println();

    // Test 6: Delete test file
    Serial.println("TEST: Delete test file");
    if (SD.remove(TEST_FILE_PATH)) {
        Serial.printf("✓ Deleted file: %s\n", TEST_FILE_PATH);
    } else {
        Serial.printf("✗ Failed to delete file: %s\n", TEST_FILE_PATH);
    }
    Serial.println();

    // Test 7: Delete test directory
    Serial.println("TEST: Delete test directory");
    if (SD.rmdir(TEST_DIR_PATH)) {
        Serial.printf("✓ Deleted directory: %s\n", TEST_DIR_PATH);
    } else {
        Serial.printf("✗ Failed to delete directory: %s\n", TEST_DIR_PATH);
    }
    Serial.println();

    Serial.println("========================================");
    Serial.println("ALL TESTS COMPLETED SUCCESSFULLY");
    Serial.println("========================================");
}

void loop() {
    // Nothing to do in loop
    delay(1000);
}
