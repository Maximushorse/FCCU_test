#include "adc.h"
#include "ring_buffer.h"

// *** Calibration coefficients ***
float adc_3v3_coeffs[ADC_3V3_VOLTAGE_COEFF_COUNT] = {
    2.5679088051711859e-004,
    8.1818138383964200e-004,
    4.4561346617176653e-007,
    -1.5785641837443660e-009,
    2.6100902814800432e-012,
    -2.4247998157041470e-015,
    1.3522880124391428e-018,
    -4.6201730839219227e-022,
    9.4673093903828406e-026,
    -1.0679135163074677e-029,
    5.0946016674492631e-034,
};

float adc_60v_coefficients[ADC_60V_VOLTAGE_COEFF_COUNT] = {
    2.5679088051711859e-004,
    8.1818138383964200e-004,
    4.4561346617176653e-007,
    -1.5785641837443660e-009,
    2.6100902814800432e-012,
    -2.4247998157041470e-015,
    1.3522880124391428e-018,
    -4.6201730839219227e-022,
    9.4673093903828406e-026,
    -1.0679135163074677e-029,
    5.0946016674492631e-034,
};

// CALIBRATION SAMPLES
// 813 20.5
// 936 27.7
// 969 29.7
// 1015 32.3
// 1072 35.8
// 1100 37.3
// 1180 42.2
// 1236 45.3
// 1269 47.9
// 1281 48.1
// 1355 52.2
// 1362 53.0
// 1418 56.3
float adc_temperature_coeffs[ADC_TEMPERATURE_COEFF_COUNT] = {
    -2.7669277558378205e+001,
    5.9176370002593397e-002,
};

adc_oneshot_unit_handle_t adc1_handle;
adc_oneshot_unit_handle_t adc2_handle;

uint16_t buffer_V_FC[ADC_V_FC_SAMPLES_COUNT];
uint16_t buffer_T[ADC_T_SAMPLES_COUNT];
uint16_t buffer_P[ADC_P_SAMPLES_COUNT];
uint16_t buffer_button_state[ADC_BUTTON_SAMPLES_COUNT];

ring_buffer_t rb_V_FC;
ring_buffer_t rb_T;
ring_buffer_t rb_P;
ring_buffer_t rb_button_state;

int V_FC_raw = 0;
float V_FC_filtered_raw = 0;
float V_FC_value = 0;

int T_raw = 0;
float T_filtered_raw = 0;
float T_value = 0;

int P_raw = 0;
float P_filtered_raw = 0;
float P_value = 0;

int button_state_raw = 0;
float button_state_filtered_raw = 0;
float button_state_value = 0;
float previous_button_state_value = 0;

bool button_state = 0;
bool previous_button_state = 0;

float adc_map(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float adc_apply_calibration(float coefficients[], uint8_t coeff_count, int adc_raw_sample)
{
    // dac swiezakowi jakiemus (minster cyfryzacji prof. nadzwyczajny dr mgr edukacji wczesnoszkolnej inż. Marek K.
    // (podkarpacianianin)(pomnik czynu rewolucyjnego)) z multimetrem zeby spisal adc_raw i napiecie z woltomierza i
    // ulozyc nowe wspolczynniki dla wielomianu na 1 roku jest to to sobie przecwiczy najlepiej tak ze jak juz bedzie
    // szlo do auta albo bedziemy potrzebowac dokladncyh wynikow kalibracje trzeba dla kazdej plytki zrobic ale chyba
    // nie dla adc1 i adc2 oddzielnie

    // TODO filtr cyfrowy dolnoprzepustowy i ring buffer zamiast średniej ruchomej

    float result = 0;
    for (int i = 0; i < coeff_count; i++)
    {
        result += coefficients[i] * pow(adc_raw_sample, i);
    }

    return result;
}

float adc_apply_filter(ring_buffer_t* rb)
{
    float sum = 0;

    uint16_t sample = 0;
    for (int i = 0; i < rb->size; i++)
    {
        if (ring_buffer_peek(rb, &sample, i))
        {
            sum += sample;
        }
    }

    return sum / (float) rb->count;
}

void adc_on_loop()
{
    // Read raw values from ADC
    adc_oneshot_read(adc1_handle, ADC_V_FC_CHANNEL, &V_FC_raw);
    adc_oneshot_read(adc1_handle, ADC_T_CHANNEL, &T_raw);
    adc_oneshot_read(adc1_handle, ADC_P_CHANNEL, &P_raw);
    adc_oneshot_read(adc2_handle, ADC_BUTTON_STATE_CHANNEL, &button_state_raw);

    // Add new sample to the ring buffer
    ring_buffer_enqueue(&rb_V_FC, V_FC_raw);
    ring_buffer_enqueue(&rb_T, T_raw);
    ring_buffer_enqueue(&rb_P, P_raw);
    ring_buffer_enqueue(&rb_button_state, button_state_raw);

    // Apply filter to the ring buffer
    V_FC_filtered_raw = adc_apply_filter(&rb_V_FC);
    T_filtered_raw = adc_apply_filter(&rb_T);
    P_filtered_raw = adc_apply_filter(&rb_P);
    button_state_filtered_raw = adc_apply_filter(&rb_button_state);

    // Apply calibration curves
    V_FC_value = adc_apply_calibration(adc_60v_coefficients, ADC_60V_VOLTAGE_COEFF_COUNT, V_FC_filtered_raw);
    T_value = adc_apply_calibration(adc_temperature_coeffs, ADC_TEMPERATURE_COEFF_COUNT, T_filtered_raw);
    P_value = adc_apply_calibration(adc_3v3_coeffs, ADC_3V3_VOLTAGE_COEFF_COUNT, P_filtered_raw);
    button_state_value = adc_apply_calibration(adc_3v3_coeffs, ADC_3V3_VOLTAGE_COEFF_COUNT, button_state_filtered_raw);

    // Map to physical values if necessary
    P_value = adc_map(P_value, 0, 2.995, 0, 300); // Pressure: 0-3 V => 0-300 bar

    // TODO move to separate function
    // zbocze narastajace
    if (button_state_value - previous_button_state_value >= 0.5)
    {
        button_state = true;
    }
    else
    {
        button_state = false;
    }
    previous_button_state_value = button_state_value;
}

void adc_init()
{
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };

    adc_oneshot_unit_init_cfg_t init_config2 = {
        .unit_id = ADC_UNIT_2,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    adc_oneshot_new_unit(&init_config2, &adc2_handle);

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_2, &config);
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config);
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_1, &config);
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_2, &config);
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config);

    ring_buffer_init(&rb_V_FC, buffer_V_FC, ADC_V_FC_SAMPLES_COUNT);
    ring_buffer_init(&rb_T, buffer_T, ADC_T_SAMPLES_COUNT);
    ring_buffer_init(&rb_P, buffer_P, ADC_P_SAMPLES_COUNT);
    ring_buffer_init(&rb_button_state, buffer_button_state, ADC_BUTTON_SAMPLES_COUNT);
}
