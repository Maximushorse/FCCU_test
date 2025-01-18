#ifndef CONSOLE_H
#define CONSOLE_H

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

#define CONSOLE_UART_NUM UART_NUM_0

typedef enum _commands {
    CMD_TOGGLE_SERIAL_OUTPUT = 1,
    CMD_TOGGLE_FAN_PWM_OUTPUT,
    CMD_TOGGLE_PWM_GND_OUTPUT,
    CMD_TOGGLE_MAIN_VALVE,
    CMD_TOGGLE_PURGE_VALVE,
    CMD_TOGGLE_MOSFET,
    CMD_TOGGLE_LED = 0,
} commands_t;

void console_parse_command(commands_t command);
void console_print_logs();
void console_tx_task(void* pvParameters);
void console_rx_task(void* pvParameters);

void console_init();

#endif // CONSOLE_H