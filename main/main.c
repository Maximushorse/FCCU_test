#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "adc.h"
#include "gpio.h"
#include "pwm.h"
#include "console.h"

#include "fuel_cell_control.h"

#include "driver/uart.h"

void app_main()
{
    gpio_init();
    adc_init();
    pwm_init();
    console_init();

    while (1)
    {
        adc_on_loop();
        fc_on_loop();

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
