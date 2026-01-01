#include <Arduino.h>
#include "esp_camera.h"

// ==================== USER CONFIGURATION ====================
// Edit these values according to your needs

// WiFi Settings
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Camera Settings
const int CAMERA_FRAME_SIZE = FRAMESIZE_QVGA;    // Options: FRAMESIZE_QVGA (320x240), FRAMESIZE_VGA (640x480),
                                                  //          FRAMESIZE_SVGA (800x600), FRAMESIZE_HD (1280x720)
const int CAMERA_JPEG_QUALITY = 15;               // 0-63, lower means higher quality (but slower)
const int CAMERA_FB_COUNT = 2;                    // Number of frame buffers (1-2)

// Audio Settings
const int AUDIO_SAMPLE_RATE = 16000;              // Sample rate in Hz (8000, 16000, 44100)
const int AUDIO_BUFFER_SIZE = 512;                // Buffer size in samples (256, 512, 1024)

// Server Settings
const int HTTP_SERVER_PORT = 80;                  // HTTP server port
const int WEBSOCKET_PORT = 81;                    // WebSocket port for audio

// Audio Task Settings
const int AUDIO_TASK_PRIORITY = 5;                // FreeRTOS task priority (1-25, higher = more priority)
const int AUDIO_TASK_STACK_SIZE = 8192;           // Stack size for audio tasks
const int AUDIO_TASK_CORE = 1;                    // CPU core for audio tasks (0 or 1)

// ==================== END USER CONFIGURATION ====================
