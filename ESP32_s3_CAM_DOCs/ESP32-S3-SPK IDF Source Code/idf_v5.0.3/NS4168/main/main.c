#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ns4168.h"

void app_main(void)
{
    i2s_init();
    i2s_play_music();
}
