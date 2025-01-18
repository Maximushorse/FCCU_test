#include "console.h"
#include "adc.h"

#define LED_PIN 33 // 33 led CAN Status
bool led_state = true;
void send_esp_logi()
{
    printf("%2.3f,"
           "%2.3f,"
           "%2.3f,"
           "%2.3f\n\r",
        V_FC_average, T_average, P_average, button_state_average);
}

void log_task(void* pvParameters)
{
    while (1)
    {
        send_esp_logi();
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait 1000ms to next
        led_state = !(led_state);
        gpio_set_level(LED_PIN, led_state);
    }
}