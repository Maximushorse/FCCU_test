#ifndef PWM_H
#define PWM_H

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

// Fans
#define FAN_GND_PIN 8 // Simple fan have 2 wires and with it we need to use PWM on FAN_GND_PIN

// More complex fans with 3 or 4 wires need to set PWM and have power all the time (A lot of fans starts turning
// without PWM when power is ON)
#define FAN_PWM_PIN 45

// frequncy of clock (each tick)
// 2.5 MHz -> 40 ns
#define PWM_TIMEBASE_RESOLUTION_HZ 25000000 // ------------Sprawdzić oscyloskopem

// number of ticks in each period
// 100 ticks, 40 us;
#define PWM_TIMEBASE_PERIOD 1000

float pwm_duty_cycle_to_ticks(float duty_cycle_percent);
void pwm_set_gnd_duty_cycle(float duty_cycle_percent);
void pwm_set_pwm_duty_cycle(float duty_cycle_percent);

void pwm_init();
void pwm_on_loop();

#endif // PWM_H