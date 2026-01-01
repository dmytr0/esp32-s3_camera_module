#include "ns4168.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_err.h"
// i2s_chan_handle_t tx_handle;
i2s_chan_handle_t tx_handle;

void i2s_init(void)
{
    i2s_chan_config_t tx_handle_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM,I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&tx_handle_cfg,&tx_handle,NULL));
    //初始化i2s
    i2s_std_config_t tx_std_cfg = {
            .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(I2S_FRE),
            .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                        I2S_SLOT_MODE_STEREO),

            .gpio_cfg = {
                    .mclk = -1,    // some codecs may require mclk signal, this example doesn't need it
                    .bclk = NS4168_BCLK,
                    .ws   = NS4168_WS,
                    .dout = NS4168_DOUT,
                    .din  = -1,
                    .invert_flags = {
                            .mclk_inv = false,
                            .bclk_inv = false,
                            .ws_inv   = false,
                    },
            },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &tx_std_cfg));
   // ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));

    //启动右通道
    gpio_config_t cfg = {
        .pin_bit_mask =(1ull<<NS4168_LRCLK) ,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type =0 ,
    };
    gpio_config(&cfg);
    
    gpio_set_level(NS4168_LRCLK,1);

}

