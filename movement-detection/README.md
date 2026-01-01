# ESP32-S3 PIR Motion Detection Camera

Smart security camera with PIR sensor, automatic photo capture, SD card storage, and silent Telegram notifications.

> [!WARNING]
> **AI-Powered Project Disclaimer**
>
> This project was developed with AI assistance. The code is provided "as is" without warranty. The author assumes no responsibility for any damage, data loss, or hardware failure.
>
> **Use at your own risk!** Test thoroughly before deploying.

## Features

- 🎯 **PIR Motion Detection** - Hardware sensor for reliable motion detection
- 📸 **Photo Burst** - Captures 3 photos per motion event (configurable)
- 💾 **SD Card Storage** - Sequential numbering with automatic space management
- 📱 **Silent Telegram Alerts** - Batch photo sending without notification sound
- 🔄 **Smart Recovery** - Continues numbering after restart, skips already-sent photos
- 🚫 **False Trigger Filtering** - Duration check and cooldown to prevent false alarms
- 💡 **LED Indicator** - Visual feedback during capture (SK6812 RGB)
- 🧠 **Dual-Core Processing** - Detection on Core 1, I/O on Core 0

## Hardware Requirements

- **ESP32-S3 SPK** development board with OV2640 camera
- **PIR Motion Sensor** (SR602 or similar) connected to GPIO1
- **SD Card** (FAT32 format, SPI mode)
- **WiFi connection** for Telegram notifications

### Pin Configuration (ESP32-S3 SPK)

Pre-configured for ESP32-S3 SPK board:
- **Camera**: OV2640 via parallel interface
- **SD Card**: SPI mode (MISO=12, MOSI=3, CLK=11, CS=2)
- **PIR Sensor**: GPIO1
- **RGB LED**: GPIO21 (SK6812-EC20)

## Quick Start

### 1. Install Arduino IDE Libraries

Install via Library Manager:
- `UniversalTelegramBot` by Brian Lough
- `ArduinoJson` v6.x
- `Adafruit NeoPixel`

### 2. Configure Settings

Edit **`user_config.ino`**:

```cpp
// WiFi
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

// Telegram Bot (get from @BotFather)
const char* TELEGRAM_BOT_TOKEN = "YOUR_BOT_TOKEN";
const char* TELEGRAM_CHAT_ID = "YOUR_CHAT_ID";

// Camera
const int CAMERA_FRAME_SIZE = FRAMESIZE_SVGA;  // 800x600
const int CAMERA_JPEG_QUALITY = 12;             // 0-63 (lower=better)

// Motion Detection
const int MOTION_COOLDOWN_MS = 10000;           // 10 sec cooldown
const int PIR_TRIGGER_DURATION_MS = 150;        // Filter false triggers
const int PHOTOS_PER_BURST = 3;                 // Photos per event (1-5)
const int PHOTO_BURST_DELAY_MS = 300;           // Delay between photos
```

### 3. Telegram Setup

1. Create bot with `@BotFather` in Telegram → get **bot token**
2. Get your **chat ID** from `@userinfobot`
3. Add credentials to `user_config.ino`

### 4. Upload

- **Board**: ESP32S3 Dev Module
- **PSRAM**: OPI PSRAM
- **Partition Scheme**: Default 4MB with spiffs
- Upload and open Serial Monitor (115200 baud)

## Configuration Options

### Camera Settings
```cpp
CAMERA_FRAME_SIZE    // FRAMESIZE_VGA, FRAMESIZE_SVGA, etc.
CAMERA_JPEG_QUALITY  // 0 (best) - 63 (worst), 8-12 recommended
CAMERA_FB_COUNT      // Frame buffers (1-2)
```

### Motion Detection
```cpp
MOTION_COOLDOWN_MS        // Delay between detections (10000ms = 10 sec)
PIR_TRIGGER_DURATION_MS   // Min trigger time to filter false positives (150ms)
PHOTOS_PER_BURST          // Photos per event (1-5)
PHOTO_BURST_DELAY_MS      // Delay between burst photos (300ms)
```

### Storage & Telegram
```cpp
SD_CARD_ENABLED           // Enable/disable SD card
SD_PHOTO_DIR              // Directory name ("/motion")
TELEGRAM_ENABLED          // Enable/disable Telegram
TELEGRAM_SEND_PHOTO       // Send photos with alerts
```

## How It Works

### Motion Detection Flow

1. **PIR Warm-up**: 30-second calibration on startup
2. **Trigger Check**: PIR signal must stay HIGH for `PIR_TRIGGER_DURATION_MS`
3. **Photo Burst**: Captures N photos with delay between each
4. **SD Save**: Sequential numbering (`photo_000001.jpg`, `photo_000002.jpg`, ...)
5. **Telegram Batch**: Every 10 seconds, sends unsent photos as media group (silent)
6. **Cooldown**: Waits until PIR goes LOW, then additional cooldown period

### Smart Features

**Sequential Numbering with Persistence:**
- Scans SD card on startup, continues from highest number
- Stores counters in NVRAM (survives reboots)
- Tracks last sent photo to avoid re-sending

**Space Management:**
- Auto-deletes oldest photos when <2MB free
- Deletes until 5MB free (max 50 photos per cycle)

**False Trigger Prevention:**
- PIR must stay HIGH for minimum duration
- Cooldown period after each detection
- Waits for PIR to stabilize before next trigger

## Serial Commands

Send via Serial Monitor:

- `STATUS` - Show system info (SD space, photo counts, WiFi status)
- `ERASE_ALL` - Delete all photos from SD card
- `HELP` - Show available commands

## Project Structure

```
movement-detection/
├── movement-detection.ino   # Main sketch with FreeRTOS tasks
├── user_config.ino           # ⚙️ USER CONFIGURATION
├── config.h                  # Hardware pins & constants
├── camera.cpp/h              # Camera initialization & capture
├── sd_storage.cpp/h          # SD operations & space management
├── telegram.cpp/h            # Telegram API (sendMediaGroup)
└── README.md
```

## Troubleshooting

### PIR false triggers
- Increase `PIR_TRIGGER_DURATION_MS` to 200-300ms
- Increase `MOTION_COOLDOWN_MS` to 15000-20000ms
- Ensure PIR sensor is stable, not vibrating
- Keep away from heat sources (ESP32 generates heat after first trigger)

### Photos not sending to Telegram
- Check WiFi connection in serial output
- Verify bot token and chat ID are correct
- Test bot manually in Telegram first
- Check `lastSentNum` in STATUS command

### SD card errors
- Format as FAT32
- Check SPI wiring (MISO=12, MOSI=3, CLK=11, CS=2)
- Try different SD card (some cards incompatible)
- Check serial output during initialization

### Photos start from 1 after reboot
- System now scans SD on startup and continues numbering
- Check serial: "Found N photos on SD card"
- If still resets, NVRAM may be corrupted (use `ERASE_ALL` to reset)

## Technical Details

**Dual-Core Architecture:**
- Core 1: PIR detection loop (high priority)
- Core 0: SD save task + Telegram sender task

**Telegram API:**
- Uses `sendMediaGroup` for photo albums
- Silent mode (`disable_notification: true`)
- Direct WiFiClientSecure with multipart/form-data

**Storage:**
- Sequential naming: `photo_000001.jpg`
- Preferences (NVRAM) stores: `photoNum`, `lastSentNum`
- Circular deletion when space < 2MB

## Performance Tips

- **SVGA (800x600)** - Good balance of quality/speed
- **JPEG Quality 10-12** - Smaller files, faster upload
- **Cooldown 10-15 sec** - Prevents repeat triggers from heat
- **PIR Duration 150-200ms** - Filters electrical noise

## License

Open source - modify and distribute freely.

## Credits

- ESP32 Camera library by Espressif
- UniversalTelegramBot by Brian Lough
- Adafruit NeoPixel library
- Developed for ESP32-S3 SPK platform
