#include "gpio.h"

void enable_configuration()
{
    gpio_config_t gpio_config1 = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << MAIN_VALVE_PIN),
    };
    gpio_config(&gpio_config1);

    gpio_config_t gpio_config2 = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PURGE_VALVE_PIN),
    };
    gpio_config(&gpio_config2);

    gpio_config_t gpio_config3 = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED_PIN),
    };
    gpio_config(&gpio_config3);

    gpio_config_t gpio_config4 = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << MOSFET_PIN),
    };
    gpio_config(&gpio_config4);
}

void gpio_init()
{
    // gpio_set_level(LED_PIN, led_state);
    gpio_set_level(MAIN_VALVE_PIN, 0);
    gpio_set_level(PURGE_VALVE_PIN, 0);
    gpio_set_level(MOSFET_PIN, 0);
}