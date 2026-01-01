#include <Arduino.h>
#include "websocket.h"
#include "audio.h"

// WebSocket Server (port 81 for regular WS, will work over HTTPS page)
WebSocketsServer webSocket = WebSocketsServer(WEBSOCKET_PORT);

// Initialize WebSocket server
void initWebSocket() {
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    Serial.printf("WebSocket server started on port %d\n", WEBSOCKET_PORT);
    Serial.println("WARNING: WebSocket on port 81 is not encrypted (WS, not WSS)");
    Serial.println("Browsers may block WS connections from HTTPS pages");
}

// WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[WS] Client %u disconnected\n", num);
            audioRecordingEnabled = false;
            audioPlaybackEnabled = false;
            break;

        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[WS] Client %u connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
            Serial.println("[WS] Audio recording ENABLED");
            Serial.println("[WS] Audio playback ENABLED");
            audioRecordingEnabled = true;
            audioPlaybackEnabled = true;
            break;
        }

        case WStype_BIN:
            // Received audio data from browser to play on speaker
            Serial.printf("[WS] Received binary: %d bytes\n", length);
            if (length <= audio_buffer_size * sizeof(int16_t)) {
                memcpy(spk_buffer, payload, length);
            } else {
                Serial.printf("[WS] WARNING: Received %d bytes, buffer size is %zu\n", length, audio_buffer_size * sizeof(int16_t));
            }
            break;

        case WStype_TEXT:
            // Handle text commands if needed
            Serial.printf("[WS] Received text from %u: %s\n", num, payload);
            break;

        case WStype_ERROR:
            Serial.printf("[WS] Error on client %u\n", num);
            break;
    }
}

// Helper function to broadcast audio data
void broadcastAudioData(uint8_t* data, size_t length) {
    webSocket.broadcastBIN(data, length);
}

// Get connected clients count
int getConnectedClients() {
    return webSocket.connectedClients();
}

// WebSocket loop (call in main loop)
void loopWebSocket() {
    webSocket.loop();
}
