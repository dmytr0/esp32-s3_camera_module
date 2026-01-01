#ifndef _MEMS_RE__H_
#define _MEMS_RE__H_
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "driver/i2s_pdm.h"
#include "esp_err.h"


#define EXAMPLE_STD_BCLK_IO1        GPIO_NUM_39      // I2S bit clock io number
#define EXAMPLE_STD_WS_IO1          GPIO_NUM_40     // I2S word select io number
#define EXAMPLE_STD_DOUT_IO1        -1               // I2S data out io number
#define EXAMPLE_STD_DIN_IO1         GPIO_NUM_38      // I2S data in io number
#define EXAMPLE_BUFF_SIZE           128


extern i2s_chan_handle_t  rx_chan;        // I2S rx channel handler

void MEMS_Init(void);


#define I2S_DEFAULT_CFG()                                                                                                                    \
  {                                                                                                                                          \
    .id = I2S_NUM_AUTO, .role = I2S_ROLE_MASTER, .dma_desc_num = 6, .dma_frame_num = 240, .auto_clear = true, .auto_clear_before_cb = false, \
    .intr_priority = 0                                                                                                                       \
  }

#define I2S_PDM_RX_CHAN_CFG(_sample_rate, _data_bit_width, _slot_mode)                                                               \
  {                                                                                                                                  \
    .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(_sample_rate), .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(_data_bit_width, _slot_mode), \
    .gpio_cfg = {                                                                                                                    \
      .clk = (gpio_num_t)EXAMPLE_STD_BCLK_IO1,                                                                                                    \
      .din = (gpio_num_t)EXAMPLE_STD_DIN_IO1,                                                                                                   \
      .invert_flags =                                                                                                                \
        {                                                                                                                            \
          .clk_inv = false,                                                                                                    \
        },                                                                                                                           \
    },                                                                                                                               \
  }

esp_err_t New_MEM_Init();


#endif