#include "fuel_cell_control.h"

#include "pwm.h"
#include "adc.h"

// Start value
bool main_valve = 0;  // ================= ustawienie zaworu zasilajacego _ main valve n-mosfet (1 on, 0 off)
bool purge_valve = 0; // ================= ustawienie przedmuchu - purge valve n-mosfet (1 on, 0 off)
bool mosfet = 0;      // ================= ustawienie zwarcia mosfet - driver i n-mosfet (1 on, 0 off)

// Control
// int START_V = 3000;
// int PURGE_V = 1000;

bool start = 0;
bool purge = 0;

void fc_on_loop()
{
    /* TEST
        main_valve = 1;
        purge_valve = 1;
        fan_gnd_duty_cycle_percent = 1;
        fan_PWM_duty_cycle_percent = 30; //testy 3s ????
    */

    // fc_on_loop(); *************************

    /*

    // Fan gnd control
    pwm_set_gnd_duty_cycle(fan_gnd_duty_cycle_percent);

    // Fan PWM control
    pwm_set_pwm_duty_cycle(fan_PWM_duty_cycle_percent);
    // TEST end

    // How to control PEM fuel cell?
    // power main valve, power fans, measure voltage, measure temperature,
    // if temperature is rising from 40*C give higher PWM proportional to temp, if maksimum temp 65*C turn off fuel cell
    // and give flag if voltage falling down in time about 1 min turn purge valve and mosfet on about 1ms , if voltage
    // to low 15V turn off and give flag

    if (button_state_average > 3.0)
    { // 2.8 or 3.4V supplay

        // button_state_average = 0;
        // gpio_set_level(LED_PIN, 1);
        // TODO gpio_set_alarm_led(true);

        if (start != 0)
        {
            main_valve = 0;
        }
        else
            main_valve = 1;
        start = !start;
    }
    else
    {
        // Przedmuchy
        if (button_state_average > 0.9 && button_state_average < 1.2)
        {
            purge_valve = 1;
            mosfet = 1;

            // Dać licznik TODO na previus_button_state

            // gpio_set_level(LED_PIN, 1);
            // gpio_set_level(PURGE_VALVE_PIN, purge_valve);
            vTaskDelay(300 / portTICK_PERIOD_MS); // czas przedmuchu przed zwarciem
            // gpio_set_level(LED_PIN, 0);
            // gpio_set_level(MOSFET_PIN, mosfet);
            vTaskDelay(10 / portTICK_PERIOD_MS); // czas zwarcia - 100ms xd

            purge_valve = 0;
            mosfet = 0;

            // gpio_set_level(MOSFET_PIN, mosfet);
            vTaskDelay(1 / portTICK_PERIOD_MS); // czas przedmuchu przed zwarciem
            // gpio_set_level(PURGE_VALVE_PIN, purge_valve);
        }
        else
        {
            purge_valve = 0;
            mosfet = 0;
        }
    }

    if (start == 1)
    {
        main_valve = 1;
        fan_gnd_duty_cycle_percent = 100;
        fan_PWM_duty_cycle_percent = 40; // testy 3s ????
    }
    else
    {
        // main_valve = 0;
        fan_gnd_duty_cycle_percent = 0;
        fan_PWM_duty_cycle_percent = 0; // testy 3s ????
    }

    // gpio_set_level(PURGE_VALVE_PIN, purge_valve);
    // gpio_set_level(MAIN_VALVE_PIN, main_valve);

    // Chlodzenie
    if (T_average > 40)
        fan_PWM_duty_cycle_percent = T_average - 10; // If you have simple 2 wire cable change fan_PWM to fan_gnd
    else if (T_average > 55)
        fan_PWM_duty_cycle_percent = T_average * 2;
    else
        fan_PWM_duty_cycle_percent = 10;

    if (V_FC_average < 35 && fan_PWM_duty_cycle_percent >= 30)
        fan_PWM_duty_cycle_percent = 30;

    */
}