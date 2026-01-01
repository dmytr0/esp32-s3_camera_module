#ifndef CONFIG_H
#define CONFIG_H

// External variables from user_config.cpp (user configurable)
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;
extern const int CAMERA_FRAME_SIZE;
extern const int CAMERA_JPEG_QUALITY;
extern const int CAMERA_FB_COUNT;
extern const int AUDIO_SAMPLE_RATE;
extern const int AUDIO_BUFFER_SIZE;
extern const int HTTP_SERVER_PORT;
extern const int WEBSOCKET_PORT;
extern const int AUDIO_TASK_PRIORITY;
extern const int AUDIO_TASK_STACK_SIZE;
extern const int AUDIO_TASK_CORE;

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

// PDM Microphone pins (MSM261S4030H0R)
#define MIC_CLK_PIN     39  // Clock
#define MIC_DIN_PIN     38  // Data In

// NS4168 Speaker pins
#define SPK_BCLK_PIN    10  // BCLK
#define SPK_WS_PIN      45  // WS (Word Select)
#define SPK_DOUT_PIN    9   // Data Out
#define SPK_CTRL_PIN    46  // Enable pin (LRCLK)

#endif // CONFIG_H
