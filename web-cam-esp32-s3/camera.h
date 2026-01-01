#ifndef CAMERA_H
#define CAMERA_H

#include "esp_camera.h"
#include "config.h"

// Initialize camera
bool initCamera();

// Get camera configuration
camera_config_t getCameraConfig();

#endif // CAMERA_H
