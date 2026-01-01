#include <Arduino.h>
#include "audio.h"
#include "websocket.h"

// I2S Channels
i2s_chan_handle_t rx_chan = NULL;  // PDM RX (microphone)
i2s_chan_handle_t tx_chan = NULL;  // STD TX (speaker)

// Audio buffers (allocated dynamically)
int16_t* mic_buffer = NULL;
int16_t* spk_buffer = NULL;
size_t audio_buffer_size = 0;

bool audioRecordingEnabled = false;
bool audioPlaybackEnabled = false;

// Initialize PDM Microphone (NEW API)
void setupI2SMicrophone() {
    // Allocate audio buffers
    audio_buffer_size = AUDIO_BUFFER_SIZE;
    mic_buffer = (int16_t*)malloc(audio_buffer_size * sizeof(int16_t));
    spk_buffer = (int16_t*)malloc(audio_buffer_size * sizeof(int16_t));

    if (!mic_buffer || !spk_buffer) {
        Serial.println("ERROR: Failed to allocate audio buffers");
        return;
    }

    memset(mic_buffer, 0, audio_buffer_size * sizeof(int16_t));
    memset(spk_buffer, 0, audio_buffer_size * sizeof(int16_t));

    // Create PDM RX channel
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    chan_cfg.dma_desc_num = 6;
    chan_cfg.dma_frame_num = 240;

    esp_err_t ret = i2s_new_channel(&chan_cfg, NULL, &rx_chan);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: Failed to create RX channel: %d\n", ret);
        return;
    }

    // Configure PDM RX mode
    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = (gpio_num_t)MIC_CLK_PIN,
            .din = (gpio_num_t)MIC_DIN_PIN,
            .invert_flags = {
                .clk_inv = false,
            },
        },
    };

    ret = i2s_channel_init_pdm_rx_mode(rx_chan, &pdm_rx_cfg);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: Failed to init PDM RX mode: %d\n", ret);
        return;
    }

    ret = i2s_channel_enable(rx_chan);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: Failed to enable RX channel: %d\n", ret);
        return;
    }

    Serial.println("✓ PDM Microphone initialized");
}

// Initialize NS4168 Speaker (NEW API)
void setupI2SSpeaker() {
    // Enable speaker amplifier (GPIO 46 as enable pin)
    pinMode(SPK_CTRL_PIN, OUTPUT);
    digitalWrite(SPK_CTRL_PIN, HIGH);

    // Create Standard I2S TX channel
    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    tx_chan_cfg.auto_clear = true;

    esp_err_t ret = i2s_new_channel(&tx_chan_cfg, &tx_chan, NULL);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: Failed to create TX channel: %d\n", ret);
        return;
    }

    // Configure Standard I2S TX mode (MSB, STEREO)
    i2s_std_config_t std_tx_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)SPK_BCLK_PIN,
            .ws = (gpio_num_t)SPK_WS_PIN,
            .dout = (gpio_num_t)SPK_DOUT_PIN,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ret = i2s_channel_init_std_mode(tx_chan, &std_tx_cfg);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: Failed to init STD TX mode: %d\n", ret);
        return;
    }

    ret = i2s_channel_enable(tx_chan);
    if (ret != ESP_OK) {
        Serial.printf("ERROR: Failed to enable TX channel: %d\n", ret);
        return;
    }

    Serial.println("✓ NS4168 Speaker initialized (MSB, STEREO)");
}

// Audio recording task (NEW API)
void audioRecordTask(void *parameter) {
    size_t bytes_read;
    int packet_count = 0;

    Serial.println("Audio record task started");

    while (true) {
        if (audioRecordingEnabled && rx_chan != NULL) {
            // Read from microphone using new API with timeout
            esp_err_t result = i2s_channel_read(rx_chan, mic_buffer, audio_buffer_size * sizeof(int16_t), &bytes_read, 100);

            if (result == ESP_OK && bytes_read > 0) {
                // Send audio data via WebSocket (without level calculation to save CPU)
                broadcastAudioData((uint8_t*)mic_buffer, bytes_read);

                // Debug every 200 packets
                packet_count++;
                if (packet_count % 200 == 0) {
                    Serial.printf("Audio TX: %d bytes, clients: %d\n",
                                  bytes_read, getConnectedClients());
                }
            } else if (result != ESP_OK && result != ESP_ERR_TIMEOUT) {
                Serial.printf("I2S read error: %d\n", result);
            }
        }
        vTaskDelay(2);  // Small delay to prevent watchdog
    }
}

// Audio playback task (NEW API with STEREO conversion)
void audioPlaybackTask(void *parameter) {
    size_t bytes_written;
    int rx_count = 0;
    int16_t* stereo_buffer = (int16_t*)malloc(audio_buffer_size * 2 * sizeof(int16_t));

    if (!stereo_buffer) {
        Serial.println("ERROR: Failed to allocate stereo buffer");
        vTaskDelete(NULL);
        return;
    }

    Serial.println("Audio playback task started");

    while (true) {
        if (audioPlaybackEnabled && tx_chan != NULL) {
            // Simple check for data (check first sample only for speed)
            if (spk_buffer[0] != 0 || spk_buffer[audio_buffer_size/2] != 0) {
                // Convert MONO to STEREO (duplicate each sample)
                for (size_t i = 0; i < audio_buffer_size; i++) {
                    stereo_buffer[i * 2] = spk_buffer[i];      // Left
                    stereo_buffer[i * 2 + 1] = spk_buffer[i];  // Right
                }

                // Write to speaker using new API with timeout
                esp_err_t result = i2s_channel_write(tx_chan, stereo_buffer, audio_buffer_size * 2 * sizeof(int16_t), &bytes_written, 100);

                rx_count++;
                if (rx_count % 200 == 0) {
                    Serial.printf("Audio RX: %d bytes\n", bytes_written);
                }

                if (result != ESP_OK && result != ESP_ERR_TIMEOUT) {
                    Serial.printf("I2S write error: %d\n", result);
                }

                memset(spk_buffer, 0, audio_buffer_size * sizeof(int16_t));
            }
        }
        vTaskDelay(2);  // Small delay to prevent watchdog
    }
}
