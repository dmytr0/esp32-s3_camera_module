#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <WebSocketsServer.h>
#include "config.h"

// WebSocket Server
extern WebSocketsServer webSocket;

// Initialize WebSocket server
void initWebSocket();

// WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

// Helper functions for audio module
void broadcastAudioData(uint8_t* data, size_t length);
int getConnectedClients();

// WebSocket loop (call in main loop)
void loopWebSocket();

#endif // WEBSOCKET_H
