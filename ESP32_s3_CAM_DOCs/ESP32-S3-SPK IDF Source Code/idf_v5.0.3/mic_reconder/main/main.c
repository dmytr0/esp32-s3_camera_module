#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdspi_driver.h"
#include "driver/gpio.h"
#include "sdmmc_cmd.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "format_wav.h"
#include "../components/mems261/src/mems/mems_re.h"

static const char *TAG = "std_rec_example";

#define NUM_CHANNELS        (2) // For mono recording only!
#define BYTE_RATE           (44100 * (16 / 8))*NUM_CHANNELS
#define SAMPLE_SIZE         (16 * 1024)

static int16_t i2s_readraw_buff[SAMPLE_SIZE];
size_t bytes_read;
const int WAVE_HEADER_SIZE = 44;
#define gain    20  //增益
void record_wav(uint32_t rec_time)
{
    // Use POSIX and C standard library functions to work with files.
    int flash_wr_size = 0;
    ESP_LOGI(TAG, "Opening file");

    uint32_t flash_rec_time = BYTE_RATE * rec_time;
    //文件头
    const wav_header_t wav_header =
        WAV_HEADER_PCM_DEFAULT(flash_rec_time, 16, 44100, 1);

    // First check if file exists before creating a new file.
    struct stat st;
    if (stat(MOUNT_POINT"/test11.wav", &st) == 0) {
        // Delete it if it exists
        unlink(MOUNT_POINT"/test11.wav");
    }

    // Create new WAV file
    FILE *f = fopen(MOUNT_POINT"/test11.wav", "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    // Write the header to the WAV file
    fwrite(&wav_header, sizeof(wav_header), 1, f);

    // // Start recording
    // while (flash_wr_size < flash_rec_time) {
    //     // Read the RAW samples from the microphone
    //     if (i2s_channel_read(rx_chan,i2s_readraw_buff, SAMPLE_SIZE, &bytes_read, 1000) == ESP_OK) {
    //         // Write the samples to the WAV file
    //         fwrite(i2s_readraw_buff, bytes_read, 1, f);
    //         flash_wr_size += bytes_read;
    //     } else {
    //         printf("Read Failed!\n");
    //     }
    // }

    // Start recording
    while (flash_wr_size < flash_rec_time) {
        // Read the RAW samples from the microphone
        if (i2s_channel_read(rx_chan,i2s_readraw_buff, SAMPLE_SIZE, &bytes_read, 1000) == ESP_OK) {
        // Apply gain to the samples
        int sample_count = bytes_read / sizeof(int16_t);
        printf("sample_count = %d\n",sample_count);
        for (int i = 0; i < sample_count; i++) {
            int16_t sample = ((int16_t *)i2s_readraw_buff)[i];
            // Apply gain adjustment
            sample = sample * gain;
            // Write the adjusted sample back to buffer
            ((int16_t *)i2s_readraw_buff)[i] = sample;
        }

        // Write the adjusted samples to the WAV file
        fwrite(i2s_readraw_buff, bytes_read, 1, f);
        flash_wr_size += bytes_read;
        } else {
            printf("Read Failed!\n");
        }
    }

    ESP_LOGI(TAG, "Recording done!");
    fclose(f);
    ESP_LOGI(TAG, "File written on SDCard");

}

void audio_reconder(void *param)
{
    sdspi_init();
    New_MEM_Init();


    record_wav(10);
    // Stop I2S driver and destroy
    ESP_ERROR_CHECK(i2s_channel_disable(rx_chan));
    ESP_ERROR_CHECK(i2s_del_channel(rx_chan));

  vTaskDelete(NULL);
}
void app_main(void)
{
xTaskCreatePinnedToCore(audio_reconder,"audio_reconder",1024*4,NULL,6,NULL,1);
}
