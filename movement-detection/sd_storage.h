#ifndef SD_STORAGE_H
#define SD_STORAGE_H

#include "esp_camera.h"
#include "config.h"

// Initialize SD card
bool initSDCard();

// Save photo to SD card
bool savePhotoToSD(camera_fb_t* fb);

// Get SD card info
void printSDCardInfo();

// Check if SD card is mounted
bool isSDCardMounted();

// Get list of unsent photos from SD card
int getUnsentPhotos(String* fileList, int maxFiles);

// Mark photo as sent (rename with .sent suffix)
bool markPhotoAsSent(const String& filename);

// Read photo from SD card
uint8_t* readPhotoFromSD(const String& filename, size_t* size);

// Erase all photos from SD card
int eraseAllPhotos();

// Check free space and delete oldest photos if needed
void checkAndManageSpace();

// Get oldest photo filename
String getOldestPhoto();

// Delete specific photo
bool deletePhoto(const String& filename);

#endif // SD_STORAGE_H
