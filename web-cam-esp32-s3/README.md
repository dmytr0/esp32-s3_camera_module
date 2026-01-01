# ESP32-S3 Web Camera with Audio

A feature-rich web camera project for ESP32-S3 with OV2640 camera, bidirectional audio (microphone and speaker), and real-time streaming over WiFi.

> [!WARNING]
> **AI-Powered Project Disclaimer**
>
> This project was developed with AI assistance. The code is provided "as is" without any warranty or guarantee. The author assumes no responsibility for any damage, data loss, hardware failure, or other issues that may arise from using this code.
>
> **Use at your own risk!** Always test thoroughly before deploying in production environments.

## Features

- 📹 **Live Video Streaming** - MJPEG stream accessible via web browser
- 🎤 **Bidirectional Audio** - Real-time audio from ESP32 microphone to browser and vice versa
- 🌐 **Web Interface** - User-friendly HTML interface with controls
- 🔧 **Configurable** - Easy configuration for WiFi, camera quality, audio settings
- ⚡ **Real-time Communication** - WebSocket for low-latency audio streaming

## Hardware Requirements

- **ESP32-S3 SPK** development board
- **OV2640** camera module
- **MSM261** PDM microphone (MSM261S4030H0R)
- **NS4168** speaker amplifier

### Pin Configuration

The project is pre-configured for ESP32-S3 SPK board. All hardware pins are defined in `config.h` and should not be changed unless using different hardware.

## Software Requirements

- **Arduino IDE** 2.0 or higher
- **ESP32 Board Support** (v3.0.0 or higher)
- **Required Libraries:**
  - ESP32 Camera (built-in with ESP32 board support)
  - WebSockets by Markus Sattler
  - WiFi (built-in)

## Installation

1. **Clone or download** this repository
2. **Open** `web-cam-esp32-s3.ino` in Arduino IDE
3. **Install required libraries** via Arduino Library Manager
4. **Configure your settings** (see Configuration section below)
5. **Select board:** Tools → Board → ESP32 Arduino → ESP32S3 Dev Module
6. **Upload** to your ESP32-S3

### Board Settings in Arduino IDE

- Board: **ESP32S3 Dev Module**
- USB CDC On Boot: **Enabled**
- CPU Frequency: **240MHz**
- Flash Mode: **QIO**
- Flash Size: **16MB**
- Partition Scheme: **Default 4MB with spiffs**
- PSRAM: **OPI PSRAM**

## Configuration

### Quick Start Configuration

Edit the file **`user_config.ino`** to configure:

#### WiFi Settings (Required)
```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

#### Camera Settings (Optional)
```cpp
const int CAMERA_FRAME_SIZE = FRAMESIZE_QVGA;  // Resolution
const int CAMERA_JPEG_QUALITY = 15;            // Quality (0-63, lower = better)
const int CAMERA_FB_COUNT = 2;                 // Frame buffers (1-2)
```

**Available resolutions:**
- `FRAMESIZE_QVGA` - 320x240 (recommended for stable streaming)
- `FRAMESIZE_VGA` - 640x480
- `FRAMESIZE_SVGA` - 800x600
- `FRAMESIZE_HD` - 1280x720

#### Audio Settings (Optional)
```cpp
const int AUDIO_SAMPLE_RATE = 16000;  // Hz (8000, 16000, 44100)
const int AUDIO_BUFFER_SIZE = 512;    // Samples (256, 512, 1024)
```

#### Server Settings (Optional)
```cpp
const int HTTP_SERVER_PORT = 80;      // HTTP server port
const int WEBSOCKET_PORT = 81;        // WebSocket port
```

#### Advanced Settings (Optional)
```cpp
const int AUDIO_TASK_PRIORITY = 5;        // FreeRTOS priority (1-25)
const int AUDIO_TASK_STACK_SIZE = 8192;   // Stack size
const int AUDIO_TASK_CORE = 1;            // CPU core (0 or 1)
```

## Usage

1. **Upload the code** to ESP32-S3
2. **Open Serial Monitor** (115200 baud) to see the IP address
3. **Open your browser** and navigate to `http://ESP32_IP_ADDRESS`
4. **Control the camera:**
   - View live video stream
   - Enable audio to hear ESP32 microphone
   - Enable microphone to send audio to ESP32 speaker

### Microphone Access Workaround

Browsers block microphone access over HTTP by default. To enable browser microphone:

**For Chrome/Edge:**
1. Open: `chrome://flags/#unsafely-treat-insecure-origin-as-secure`
2. Add your ESP32 IP address (e.g., `http://192.168.1.100`)
3. Set to "Enabled"
4. Relaunch browser

**Note:** Audio from ESP32 microphone works without this workaround.

## Project Structure

```
web-cam-esp32-s3/
├── web-cam-esp32-s3.ino    # Main sketch file
├── user_config.ino          # ⚙️ USER CONFIGURATION - Edit this!
├── config.h                 # Hardware pin definitions
├── camera.cpp/h             # Camera module
├── audio.cpp/h              # Audio I2S module
├── webserver.cpp/h          # HTTP server
├── websocket.cpp/h          # WebSocket server
├── webpage.h                # HTML interface
└── README.md                # This file
```

## Troubleshooting

### Camera initialization failed
- Check camera connections
- Verify pin configuration in `config.h`
- Try lower resolution in `user_config.ino`

### WiFi connection issues
- Verify SSID and password in `user_config.ino`
- Check WiFi signal strength
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)

### Audio not working
- Check I2S pin connections
- Verify sample rate compatibility
- Try lower buffer size in `user_config.ino`

### Compilation errors
- Ensure all required libraries are installed
- Check ESP32 board support version (3.0.0+)
- Verify board settings match requirements

## Performance Tips

- **Lower resolution** for faster streaming (QVGA recommended)
- **Higher JPEG quality number** = lower quality but faster (15-20 recommended)
- **Lower audio sample rate** for reduced bandwidth (16000 Hz is optimal)
- Use **stronger WiFi signal** for better streaming quality

## Technical Details

- **Video Format:** MJPEG stream via HTTP
- **Audio Format:** PCM 16-bit mono
- **Audio Transport:** WebSocket binary frames
- **Microphone:** PDM (Pulse Density Modulation)
- **Speaker:** I2S Standard mode (stereo)

## License

This project is open source. Feel free to modify and distribute.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## Credits

- ESP32 Camera library by Espressif
- WebSockets library by Markus Sattler
- Developed for ESP32-S3 SPK hardware platform
