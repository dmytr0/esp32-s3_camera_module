#ifndef CONFIG_H
#define CONFIG_H

// External variables from user_config.ino (user configurable)
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;
extern const char* TELEGRAM_BOT_TOKEN;
extern const char* TELEGRAM_CHAT_ID;
extern const int CAMERA_FRAME_SIZE;
extern const int CAMERA_JPEG_QUALITY;
extern const int CAMERA_FB_COUNT;
extern const int MOTION_COOLDOWN_MS;
extern const int PIR_TRIGGER_DURATION_MS;
extern const int PHOTOS_PER_BURST;
extern const int PHOTO_BURST_DELAY_MS;
extern const bool SD_CARD_ENABLED;
extern const char* SD_PHOTO_DIR;
extern const bool TELEGRAM_ENABLED;
extern const bool TELEGRAM_SEND_PHOTO;
extern const char* TELEGRAM_MOTION_MESSAGE;
extern const bool LED_INDICATOR_ENABLED;
extern const int LED_FLASH_DURATION_MS;
extern const bool DEBUG_SERIAL_ENABLED;

// Hardware pin definitions (ESP32-S3 SPK specific - do not change)
// Camera pins for OV2640
#define CAM_PIN_PWDN    -1  // Not used
#define CAM_PIN_RESET   -1  // Not used
#define CAM_PIN_XCLK    33  // MCLK
#define CAM_PIN_SIOD    37  // SDA (I2C Data)
#define CAM_PIN_SIOC    36  // SCL (I2C Clock)

#define CAM_PIN_D7      47  // Y9 (D7)
#define CAM_PIN_D6      48  // Y8 (D6)
#define CAM_PIN_D5      42  // Y7 (D5)
#define CAM_PIN_D4      8   // Y6 (D4)
#define CAM_PIN_D3      6   // Y5 (D3)
#define CAM_PIN_D2      4   // Y4 (D2)
#define CAM_PIN_D1      5   // Y3 (D1)
#define CAM_PIN_D0      7   // Y2 (D0)
#define CAM_PIN_VSYNC   35  // VSYNC
#define CAM_PIN_HREF    34  // HREF
#define CAM_PIN_PCLK    41  // PCLK

// SD Card pins (SPI) - from ESP32-S3 SPK schematic
#define SD_PIN_MISO     12  // DATA0 (D0) -> MISO
#define SD_PIN_MOSI     3   // CMD (DI) -> MOSI
#define SD_PIN_CLK      11  // CLK (SCLK)
#define SD_PIN_CS       2   // CD/DAT3 (CS)

// LED pin (SK6812 RGB LED on GPIO21)
#define LED_PIN         21  // SK6812-EC20 RGB LED

// PIR Motion Sensor pin (SR602 or similar)
#define PIR_PIN         1   // Connect PIR OUT to GPIO1

// Photo burst settings (max buffer size)
#define MAX_PHOTOS_PER_BURST    5   // Maximum photos buffer size (don't change)

// Telegram batch settings
#define TELEGRAM_BATCH_INTERVAL_MS  10000  // Send batch every 10 seconds
#define TELEGRAM_MAX_BATCH_SIZE     10     // Max 10 photos per batch

#endif // CONFIG_H
