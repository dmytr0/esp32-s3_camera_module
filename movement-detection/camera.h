#ifndef CAMERA_H
#define CAMERA_H

#include "esp_camera.h"
#include "config.h"

// Initialize camera
bool initCamera();

// Capture photo and return frame buffer (JPEG)
camera_fb_t* capturePhoto();

// Capture grayscale frame for motion detection
camera_fb_t* captureGrayscale();

// Release frame buffer
void releasePhoto(camera_fb_t* fb);

#endif // CAMERA_H
