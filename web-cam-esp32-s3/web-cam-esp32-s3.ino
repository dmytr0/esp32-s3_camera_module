/*
 * ESP32-S3 Web Camera with Audio
 *
 * Features:
 * - Live video streaming (MJPEG)
 * - Bidirectional audio (microphone and speaker)
 * - WebSocket communication
 * - Web interface
 *
 * Hardware: ESP32-S3 SPK with OV2640 camera, MSM261 microphone, NS4168 speaker
 *
 * CONFIGURATION:
 * Edit user_config.ino to change WiFi credentials, camera settings, audio settings, etc.
 */

#include "WiFi.h"
#include "config.h"
#include "camera.h"
#include "audio.h"
#include "webserver.h"
#include "websocket.h"

// Task handles
TaskHandle_t audioRecordTaskHandle = NULL;
TaskHandle_t audioPlaybackTaskHandle = NULL;

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("ESP32-S3 IP Camera with Audio");

    // Initialize camera
    if (!initCamera()) {
        Serial.println("Camera initialization failed!");
        return;
    }

    // Initialize I2S for audio
    setupI2SMicrophone();
    setupI2SSpeaker();

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.setSleep(false);

    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("WiFi connected! IP address: ");
    Serial.println(WiFi.localIP());

    // Start web server
    startCameraServer();

    // Start WebSocket server
    initWebSocket();

    // Create audio tasks with configured settings
    xTaskCreatePinnedToCore(
        audioRecordTask,
        "AudioRecord",
        AUDIO_TASK_STACK_SIZE,
        NULL,
        AUDIO_TASK_PRIORITY,
        &audioRecordTaskHandle,
        AUDIO_TASK_CORE
    );

    xTaskCreatePinnedToCore(
        audioPlaybackTask,
        "AudioPlayback",
        AUDIO_TASK_STACK_SIZE,
        NULL,
        AUDIO_TASK_PRIORITY,
        &audioPlaybackTaskHandle,
        AUDIO_TASK_CORE
    );

    // Print startup information
    Serial.println();
    Serial.println("=================================");
    Serial.println("ESP32-S3 IP Camera Ready!");
    Serial.printf("Web Interface: http://%s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Video Stream:  http://%s/stream\n", WiFi.localIP().toString().c_str());
    Serial.println("=================================");
    Serial.println();
    Serial.println("MICROPHONE ACCESS WORKAROUND:");
    Serial.println("Browser blocks microphone over HTTP.");
    Serial.println("To enable microphone in Chrome/Edge:");
    Serial.println("1. Open: chrome://flags/#unsafely-treat-insecure-origin-as-secure");
    Serial.printf("2. Add: http://%s\n", WiFi.localIP().toString().c_str());
    Serial.println("3. Relaunch browser");
    Serial.println("=================================");
}

void loop() {
    loopWebSocket();
    vTaskDelay(pdMS_TO_TICKS(10));  // Give time to other tasks
}
