#ifndef TELEGRAM_H
#define TELEGRAM_H

#include "esp_camera.h"
#include "config.h"

// Initialize Telegram bot
bool initTelegram();

// Send text message to Telegram
bool sendTelegramMessage(const char* message);

// Send photo to Telegram
bool sendTelegramPhoto(camera_fb_t* fb, const char* caption);

// Send motion alert with photo
bool sendMotionAlert(camera_fb_t* fb);

// Send multiple photos as a batch (with small delay between)
bool sendPhotosBatch(String* filenames, int count);

#endif // TELEGRAM_H
