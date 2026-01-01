
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "sdkconfig.h"

#include "mems_re.h"


i2s_chan_handle_t  rx_chan;        // I2S rx channel handler

// //mems初始化
// void MEMS_Init(void)
// {

//     i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
//     ESP_ERROR_CHECK(i2s_new_channel(&rx_chan_cfg, NULL, &rx_chan));

//     i2s_std_config_t rx_std_cfg = {
//         .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(44100),
//         .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
//         .gpio_cfg = {
//             .mclk = I2S_GPIO_UNUSED,    // some codecs may require mclk signal, this example doesn't need it
//             .bclk = EXAMPLE_STD_BCLK_IO1,
//             .ws   = EXAMPLE_STD_WS_IO1,
//             .dout = EXAMPLE_STD_DOUT_IO1,
//             .din  = EXAMPLE_STD_DIN_IO1,
//             .invert_flags = {
//                 .mclk_inv = false,
//                 .bclk_inv = false,
//                 .ws_inv   = false,
//             },
//         },
//     };
   
//     rx_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;  //设置为左通道
//     ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &rx_std_cfg));

//     //开始接收数据
//     ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
// }



esp_err_t New_MEM_Init(void)
{
    // I2S configuration
    i2s_chan_config_t chan_cfg = I2S_DEFAULT_CFG();
    esp_err_t ret;
    ret = i2s_new_channel(&chan_cfg, NULL, &rx_chan);
    if (ret != ESP_OK) {
        return false; // or handle error appropriately
    }


    int rate = 44100;
    int bits_cfg = I2S_DATA_BIT_WIDTH_16BIT;
    int ch = I2S_SLOT_MODE_MONO;

    i2s_pdm_rx_config_t i2s_pdf_rx_config = I2S_PDM_RX_CHAN_CFG(rate, bits_cfg, ch);
    if (rx_chan != NULL) {

        ret = i2s_channel_init_pdm_rx_mode(rx_chan, &i2s_pdf_rx_config);
        if (ret != ESP_OK) {
            return false; // or handle error appropriately
        }

        ret = i2s_channel_enable(rx_chan);
        if (ret != ESP_OK) {
            return false; // or handle error appropriately
        }

    }

    return ESP_OK;

}