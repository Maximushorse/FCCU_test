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

#include "driver/gpio.h"
#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_gen.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_timer.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/mcpwm_types.h"

#include "adc.h"
#include "gpio.h"
#include "pwm.h"
#include "console.h"

#include "fuel_cell_control.h"

void app_main(void)
{
    gpio_init();
    adc_init();
    pwm_init();

    // FreeRTOS task working separetly - 4096 is the memory slot
    xTaskCreate(log_task, "log-task", 4096, NULL, 1, NULL);

    while (1)
    {
        adc_on_loop();
        fc_on_loop();

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
