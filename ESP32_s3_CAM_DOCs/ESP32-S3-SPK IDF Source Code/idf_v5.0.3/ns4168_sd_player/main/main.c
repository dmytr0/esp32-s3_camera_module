#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ns4168.h"
#include "esp_err.h"
#include "../components/sdspi/include/sdspi_driver.h"

// defines
#define REBOOT_WAIT 5000            // reboot after 5 seconds
#define AUDIO_BUFFER 2048           // buffer size for reading the wav file and sending to i2s
#define WAV_FILE "/sdcard/xtd.wav" // wav file to play

// constants
const char *TAG = "esp-idf-wav-player";

#define volume_factor 0.2 //调节音量

esp_err_t play_wav(char *fp)
{
  FILE *fh = fopen(fp, "rb");
  if (fh == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file");
    return ESP_ERR_INVALID_ARG;
  }

  // skip the header...//跳过wav文件的开头
  fseek(fh, 44, SEEK_SET);

  // create a writer buffer
  int16_t *buf = calloc(AUDIO_BUFFER, sizeof(int16_t));
  size_t bytes_read = 0;
  size_t bytes_written = 0;

  //从文件中读取AUDIO_BUFFER个int16_t类型的数据，存储到buf缓冲区中,并返回实际成功读取的字节数
  bytes_read = fread(buf, sizeof(int16_t), AUDIO_BUFFER, fh);

  i2s_channel_enable(tx_handle);

  while (bytes_read > 0)
  {
    // adjust the volume of the buffer
    for (int i = 0; i < bytes_read; i++) {
      buf[i] *= volume_factor;
    }
    // write the buffer to the i2s
    i2s_channel_write(tx_handle, buf, bytes_read * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    bytes_read = fread(buf, sizeof(int16_t), AUDIO_BUFFER, fh);
    ESP_LOGV(TAG, "Bytes read: %d", bytes_read);
  }

  i2s_channel_disable(tx_handle);
  free(buf);

  return ESP_OK;
}
void audio_play(void *param)
{
  sdspi_init();
  i2s_init();
  // play the wav file
  ESP_LOGI(TAG, "Playing wav file");
  ESP_ERROR_CHECK(play_wav(WAV_FILE));

  // that'll do pig... that'll do
  i2s_del_channel(tx_handle); // delete the channel

  // reboot
  ESP_LOGI(TAG, "Rebooting in %d ms...", REBOOT_WAIT);
  vTaskDelay(REBOOT_WAIT / portTICK_PERIOD_MS);

  vTaskDelete(NULL);
}
void app_main(void)
{

  xTaskCreatePinnedToCore(audio_play,"audio_play",1024*4,NULL,6,NULL,0);
}
