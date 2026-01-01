#ifndef AUDIO_H
#define AUDIO_H

#include "driver/i2s_pdm.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "config.h"

// I2S Channels
extern i2s_chan_handle_t rx_chan;  // PDM RX (microphone)
extern i2s_chan_handle_t tx_chan;  // STD TX (speaker)

// Audio buffers (dynamically sized based on AUDIO_BUFFER_SIZE from main file)
extern int16_t* mic_buffer;
extern int16_t* spk_buffer;
extern size_t audio_buffer_size;

// Audio state
extern bool audioRecordingEnabled;
extern bool audioPlaybackEnabled;

// Initialize audio hardware
void setupI2SMicrophone();
void setupI2SSpeaker();

// Audio tasks
void audioRecordTask(void *parameter);
void audioPlaybackTask(void *parameter);

#endif // AUDIO_H
