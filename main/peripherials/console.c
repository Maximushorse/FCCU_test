#include "console.h"

#include "driver/uart.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
// #include "tinyusb.h"
// #include "tusb_cdc_acm.h"
// #include "tusb_console.h"

#include "adc.h"

#define LED_PIN 33 // 33 led CAN Status
bool led_state = true;
uint8_t buffer_tx[64];

bool is_led_enabled = true;
bool is_serial_output_enabled = true;

void console_parse_command(commands_t command)
{
    switch (command)
    {
        case CMD_TOGGLE_SERIAL_OUTPUT:
            is_serial_output_enabled = !is_serial_output_enabled;
            printf("Toggle serial output, new state: %d\n", is_serial_output_enabled);
            break;

        case CMD_TOGGLE_FAN_PWM_OUTPUT:
            // TODO
            break;

        case CMD_TOGGLE_PWM_GND_OUTPUT:
            // TODO
            break;

        case CMD_TOGGLE_MAIN_VALVE:
            // TODO
            break;

        case CMD_TOGGLE_PURGE_VALVE:
            // TODO
            break;

        case CMD_TOGGLE_MOSFET:
            // TODO
            break;

        case CMD_TOGGLE_LED:
            is_led_enabled = !is_led_enabled;
            printf("Toggle LED, new state: %d\n", is_led_enabled);
            break;

        default:
            break;
    }
}

void console_print_logs()
{
    sprintf((char*) buffer_tx,
        "%2.3f,"
        "%2.3f,"
        "%2.3f,"
        "%2.3f,"
        "%2.3f\n\r",
        V_FC_filtered_raw, V_FC_value, T_value, P_value, button_state_value);

    // uart_write_bytes(CONSOLE_UART_NUM, (const char*) buffer_tx, strlen((char*) buffer_tx));
    printf((const char*) buffer_tx);
}

void console_tx_task(void* pvParameters)
{
    vTaskDelay(100 / portTICK_PERIOD_MS); // Start printing after delay

    while (1)
    {
        if (is_serial_output_enabled)
        {
            console_print_logs();
        }

        // if (is_led_enabled)
        // {
        led_state = !(led_state);
        gpio_set_level(LED_PIN, led_state);
        // }
        // else
        // {
        //     led_state = true;
        // }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void console_rx_task(void* pvParameters)
{
    uint8_t data[128];
    int length = 0;

    vTaskDelay(100 / portTICK_PERIOD_MS); // Start reading after delay

    while (1)
    {
        length = uart_read_bytes(CONSOLE_UART_NUM, data, 1, 10);
        if (length > 0)
        {
            console_parse_command((commands_t) data[0]);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // TODO is it required here?
    }
}

// esp_console ***************
int toggle_led(int argc, char** argv)
{
    is_led_enabled = !is_led_enabled;
    printf("Toggle serial output, new state: %d\n", is_serial_output_enabled);
    return 0;
}

// TinyUSB ***************
// void toggle_led(int itf, cdcacm_event_t* event)
// {
//     uint8_t buffer[64];
//     size_t size = 0;
//     tinyusb_cdcacm_read(itf, buffer, 64, &size);

//     if (size <= 0)
//         return;

//     console_parse_command((commands_t) buffer[0]);
// }

void register_commands()
{
    esp_console_cmd_t command = {
        .command = "led",
        .help = "Toggle debug LED",
        .func = &toggle_led,
        .argtable = NULL,
    };
    esp_console_cmd_register(&command);
}

void console_init()
{
    // UART ********************

    // uart_config_t uart_config = {
    //     .baud_rate = 115200,
    //     .data_bits = UART_DATA_8_BITS,
    //     .parity = UART_PARITY_DISABLE,
    //     .stop_bits = UART_STOP_BITS_1,
    // };
    // ESP_ERROR_CHECK(uart_param_config(CONSOLE_UART_NUM, &uart_config));
    // ESP_ERROR_CHECK(uart_set_pin(CONSOLE_UART_NUM, 43, 44, -1, -1));
    // ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 2048, 2048, 10, NULL, 0));

    // esp_console ********************

    esp_console_repl_t* repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "> ";
    repl_config.max_cmdline_length = 64;
    ESP_ERROR_CHECK(esp_console_register_help_command());
    register_commands();

    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    hw_config.tx_gpio_num = 43;
    hw_config.rx_gpio_num = 44;
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));

    // tinyUSB ********************

    // const tinyusb_config_t tusb_cfg = {
    //     .device_descriptor = NULL,
    //     .string_descriptor = NULL,
    //     .external_phy = false, // In the most cases you need to use a `false` value
    //     .configuration_descriptor = NULL,
    // };

    // ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    // tinyusb_config_cdcacm_t acm_cfg = {
    //     .callback_rx = toggle_led,
    // };
    // ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
    // esp_tusb_init_console(TINYUSB_CDC_ACM_0); // log to usb

    xTaskCreatePinnedToCore(console_tx_task, "console_tx_task", 4096, NULL, 1, NULL, 0);
    // xTaskCreatePinnedToCore(console_rx_task, "console_rx_task", 4096, NULL, 1, NULL, 0);
}