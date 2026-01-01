#include <Arduino.h>
#include "camera.h"

// Camera configuration
static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 20000000,        // 20MHz for faster shutter speed
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = (framesize_t)CAMERA_FRAME_SIZE,
    .jpeg_quality = CAMERA_JPEG_QUALITY,
    .fb_count = CAMERA_FB_COUNT,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_LATEST,
};

bool initCamera() {
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.printf("Camera init failed with error 0x%x\n", err);
        }
        return false;
    }

    // Get sensor for advanced settings
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        // Image quality settings
        s->set_brightness(s, 0);     // -2 to 2 (0 = neutral)
        s->set_contrast(s, 0);       // -2 to 2 (0 = neutral)
        s->set_saturation(s, 0);     // -2 to 2 (0 = neutral)
        s->set_sharpness(s, 2);      // -2 to 2 (2 = max sharpness) ← ВАЖЛИВО
        s->set_denoise(s, 0);        // 0 = off (reduce blur from noise reduction)

        // Exposure settings for motion
        s->set_gainceiling(s, GAINCEILING_2X);  // Limit gain to reduce noise
        s->set_aec2(s, 1);           // Auto exposure algorithm
        s->set_ae_level(s, 0);       // -2 to 2 (exposure compensation)
        s->set_aec_value(s, 300);    // Manual exposure (lower = faster shutter)

        // Special modes
        s->set_hmirror(s, 0);        // Horizontal mirror
        s->set_vflip(s, 0);          // Vertical flip
        s->set_awb_gain(s, 1);       // Auto white balance gain
        s->set_agc_gain(s, 0);       // Manual gain (0-30, lower = less noise)
        s->set_bpc(s, 1);            // Black pixel correction
        s->set_wpc(s, 1);            // White pixel correction
        s->set_lenc(s, 1);           // Lens correction

        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("✓ Camera sensor optimized for motion capture");
        }
    }

    if (DEBUG_SERIAL_ENABLED) {
        Serial.println("Camera initialized successfully");
    }

    return true;
}

camera_fb_t* capturePhoto() {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("Camera capture failed");
        }
        return NULL;
    }
    return fb;
}

camera_fb_t* captureGrayscale() {
    // Just capture JPEG - we'll handle it differently
    // Switching formats on-the-fly doesn't work reliably
    return capturePhoto();
}

void releasePhoto(camera_fb_t* fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}
