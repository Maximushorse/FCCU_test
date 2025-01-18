#ifndef ADC_H
#define ADC_H

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

// ADC channels
#define ADC_12V_CHANNEL          ADC_CHANNEL_2 // GPIO 13, ADC 2, IO 4 CH 2
#define ADC_V_FC_CHANNEL         ADC_CHANNEL_3 // GPIO 4, ADC 1 CH 3
#define ADC_T_CHANNEL            ADC_CHANNEL_1 // GPIO 2, ADC 1 CH 1
#define ADC_P_CHANNEL            ADC_CHANNEL_2 // GPIO 3, ADC 1 CH 2
#define ADC_BUTTON_STATE_CHANNEL ADC_CHANNEL_2 // I/O 4 -- GPIO 13 ADC 2 CH 2
// #define ADC_MCU_TEST_CHANNEL   ADC_CHANNEL_1 // MCU test GPIO 1, ADC 1 CH 0

#define ADC_V_FC_SAMPLES_COUNT   32
#define ADC_T_SAMPLES_COUNT      32
#define ADC_P_SAMPLES_COUNT      32
#define ADC_BUTTON_SAMPLES_COUNT 16

#define ADC_60V_VOLTAGE_COEFF_COUNT 11
#define ADC_3V3_VOLTAGE_COEFF_COUNT 11
#define ADC_TEMPERATURE_COEFF_COUNT 2

extern float V_FC_filtered_raw;
extern float V_FC_value;

extern float T_filtered_raw;
extern float T_value;

extern float P_filtered_raw;
extern float P_value;

extern float button_state_filtered_raw;
extern float button_state_value;
extern float previous_button_state_value;

extern bool button_state;
extern bool previous_button_state;

float adc_map(float x, float in_min, float in_max, float out_min, float out_max);
float adc_apply_calibration(float coefficients[], uint8_t coeff_count, int adc_raw);

void adc_on_loop();
void adc_init();

#endif // ADC_H