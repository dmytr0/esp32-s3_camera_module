#include <Arduino.h>
#include "telegram.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <HTTPClient.h>
#include "SD.h"
#include "FS.h"

static WiFiClientSecure client;
static UniversalTelegramBot* bot = NULL;
static WiFiClientSecure secureClient;

// Buffer for photo data
static const uint8_t* photoBuffer = NULL;
static size_t photoIndex = 0;
static size_t photoSize = 0;
static uint8_t tempBuffer[1024];  // Temporary buffer for getNextBuffer

// Callback functions for sendPhotoByBinary
bool isMoreDataAvailable() {
    return photoIndex < photoSize;
}

byte getNextByte() {
    if (photoIndex < photoSize) {
        return photoBuffer[photoIndex++];
    }
    return 0;
}

uint8_t* getNextBuffer() {
    int remaining = photoSize - photoIndex;
    int toRead = min(1024, remaining);

    if (toRead > 0) {
        memcpy(tempBuffer, photoBuffer + photoIndex, toRead);
        photoIndex += toRead;
        return tempBuffer;
    }

    return NULL;
}

int getContentLength() {
    return photoSize;
}

bool initTelegram() {
    if (!TELEGRAM_ENABLED) {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("Telegram disabled in config");
        }
        return false;
    }

    // Set secure client for Telegram API
    client.setInsecure();  // Skip certificate validation (easier but less secure)

    // Initialize bot
    bot = new UniversalTelegramBot(TELEGRAM_BOT_TOKEN, client);

    if (DEBUG_SERIAL_ENABLED) {
        Serial.println("Telegram bot initialized");
    }

    return true;
}

bool sendTelegramMessage(const char* message) {
    if (!bot || !TELEGRAM_ENABLED) {
        return false;
    }

    bool sent = bot->sendMessage(TELEGRAM_CHAT_ID, message, "");

    if (DEBUG_SERIAL_ENABLED) {
        if (sent) {
            Serial.println("Telegram message sent successfully");
        } else {
            Serial.println("Failed to send Telegram message");
        }
    }

    return sent;
}

bool sendTelegramPhoto(camera_fb_t* fb, const char* caption) {
    if (!bot || !TELEGRAM_ENABLED || !fb) {
        return false;
    }

    if (DEBUG_SERIAL_ENABLED) {
        Serial.printf("Sending photo to Telegram (%d bytes)...\n", fb->len);
    }

    // Set buffer for callbacks
    photoBuffer = fb->buf;
    photoSize = fb->len;
    photoIndex = 0;

    // Send photo using callback functions
    String response = bot->sendPhotoByBinary(
        TELEGRAM_CHAT_ID,
        "image/jpeg",
        fb->len,
        isMoreDataAvailable,
        getNextByte,
        getNextBuffer,
        getContentLength
    );

    bool sent = (response != "");

    if (DEBUG_SERIAL_ENABLED) {
        if (sent) {
            Serial.println("Telegram photo sent successfully");
            Serial.printf("Response: %s\n", response.c_str());
        } else {
            Serial.println("Failed to send Telegram photo");
        }
    }

    // Clear buffer reference
    photoBuffer = NULL;
    photoSize = 0;
    photoIndex = 0;

    return sent;
}

bool sendMotionAlert(camera_fb_t* fb) {
    if (!TELEGRAM_ENABLED) {
        return false;
    }

    bool success = false;

    if (TELEGRAM_SEND_PHOTO && fb) {
        // Send photo with caption
        success = sendTelegramPhoto(fb, TELEGRAM_MOTION_MESSAGE);
    } else {
        // Send text message only
        success = sendTelegramMessage(TELEGRAM_MOTION_MESSAGE);
    }

    return success;
}

// Helper function to create multipart form data boundary
String generateBoundary() {
    return "----WebKitFormBoundary" + String(random(100000, 999999));
}

// Send multiple photos as a batch using sendMediaGroup with direct WiFiClientSecure
bool sendPhotosBatch(String* filenames, int count) {
    if (!TELEGRAM_ENABLED || count == 0 || count > 10) {
        return false;
    }

    if (DEBUG_SERIAL_ENABLED) {
        Serial.printf("📸 Sending media group of %d photos...\n", count);
    }

    // Connect to Telegram API
    secureClient.setInsecure();
    if (!secureClient.connect("api.telegram.org", 443)) {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("✗ Connection failed");
        }
        return false;
    }

    String boundary = generateBoundary();
    String path = "/bot" + String(TELEGRAM_BOT_TOKEN) + "/sendMediaGroup";

    // Build media array (NO escaping - it's a form field, not nested JSON)
    String mediaArray = "[";
    for (int i = 0; i < count; i++) {
        if (i > 0) mediaArray += ",";
        mediaArray += "{\"type\":\"photo\",\"media\":\"attach://photo" + String(i) + "\"";
        if (i == 0) {
            mediaArray += ",\"caption\":\"🚨 Motion detected! (" + String(count) + " photos)\"";
        }
        mediaArray += "}";
    }
    mediaArray += "]";

    // Calculate content length
    size_t contentLength = 0;

    // chat_id part
    String chatIdPart = "--" + boundary + "\r\n";
    chatIdPart += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n";
    chatIdPart += String(TELEGRAM_CHAT_ID) + "\r\n";
    contentLength += chatIdPart.length();

    // disable_notification part (silent mode)
    String notificationPart = "--" + boundary + "\r\n";
    notificationPart += "Content-Disposition: form-data; name=\"disable_notification\"\r\n\r\n";
    notificationPart += "true\r\n";
    contentLength += notificationPart.length();

    // media part
    String mediaPart = "--" + boundary + "\r\n";
    mediaPart += "Content-Disposition: form-data; name=\"media\"\r\n\r\n";
    mediaPart += mediaArray + "\r\n";
    contentLength += mediaPart.length();

    // Photo parts
    for (int i = 0; i < count; i++) {
        File file = SD.open(filenames[i].c_str(), FILE_READ);
        if (!file) {
            secureClient.stop();
            return false;
        }

        size_t fileSize = file.size();
        file.close();

        String photoPart = "--" + boundary + "\r\n";
        photoPart += "Content-Disposition: form-data; name=\"photo" + String(i) + "\"; filename=\"photo.jpg\"\r\n";
        photoPart += "Content-Type: image/jpeg\r\n\r\n";

        contentLength += photoPart.length();
        contentLength += fileSize;
        contentLength += 2; // \r\n
    }

    // End boundary
    String endBoundary = "--" + boundary + "--\r\n";
    contentLength += endBoundary.length();

    // Send HTTP headers
    secureClient.print("POST " + path + " HTTP/1.1\r\n");
    secureClient.print("Host: api.telegram.org\r\n");
    secureClient.print("Content-Type: multipart/form-data; boundary=" + boundary + "\r\n");
    secureClient.print("Content-Length: " + String(contentLength) + "\r\n");
    secureClient.print("Connection: close\r\n\r\n");

    // Send body parts
    secureClient.print(chatIdPart);
    secureClient.print(notificationPart);
    secureClient.print(mediaPart);

    // Send photos
    for (int i = 0; i < count; i++) {
        String photoPart = "--" + boundary + "\r\n";
        photoPart += "Content-Disposition: form-data; name=\"photo" + String(i) + "\"; filename=\"photo.jpg\"\r\n";
        photoPart += "Content-Type: image/jpeg\r\n\r\n";
        secureClient.print(photoPart);

        // Stream photo data
        File file = SD.open(filenames[i].c_str(), FILE_READ);
        if (file) {
            uint8_t buf[512];
            while (file.available()) {
                size_t len = file.read(buf, sizeof(buf));
                secureClient.write(buf, len);
            }
            file.close();
            secureClient.print("\r\n");

            if (DEBUG_SERIAL_ENABLED) {
                Serial.printf("✓ Sent photo %d\n", i + 1);
            }
        }
    }

    secureClient.print(endBoundary);

    // Wait for response
    unsigned long timeout = millis();
    while (secureClient.connected() && !secureClient.available()) {
        if (millis() - timeout > 30000) {
            secureClient.stop();
            if (DEBUG_SERIAL_ENABLED) {
                Serial.println("✗ Timeout");
            }
            return false;
        }
        delay(10);
    }

    // Read response
    String response = "";
    bool headersEnded = false;
    int httpCode = 0;

    while (secureClient.available()) {
        String line = secureClient.readStringUntil('\n');

        if (!headersEnded) {
            if (line.startsWith("HTTP/1.1")) {
                httpCode = line.substring(9, 12).toInt();
            }
            if (line == "\r") {
                headersEnded = true;
            }
        } else {
            response += line;
        }
    }

    secureClient.stop();

    if (DEBUG_SERIAL_ENABLED) {
        Serial.printf("HTTP Code: %d\n", httpCode);
        if (httpCode != 200) {
            Serial.printf("Response: %s\n", response.c_str());
        }
    }

    if (httpCode == 200) {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.printf("✅ Media group sent: %d photos\n", count);
        }
        return true;
    } else {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.printf("✗ Failed (HTTP %d)\n", httpCode);
        }
        return false;
    }
}
