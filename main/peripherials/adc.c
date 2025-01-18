#include "adc.h"

// *** Calibration coefficients ***
float adc_3v3_voltage_coefficients[ADC_3V3_VOLTAGE_COEFFICIENTS_COUNT] = {
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

float adv_60v_voltage_coefficients[ADC_60V_VOLTAGE_COEFFICIENTS_COUNT] = {
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
float adc_temperature_coefficients[ADC_TEMPERATURE_COEFFICIENTS_COUNT] = {
    -2.7669277558378205e+001,
    5.9176370002593397e-002,
};

adc_oneshot_unit_handle_t adc1_handle;
adc_oneshot_unit_handle_t adc2_handle;

int V_FC_index = 0;
int V_FC_raw = 0;
float V_FC_calibrated = 0;
float V_FC_samples[ADC_V_FC_SAMPLES_COUNT];
float V_FC_average = 0;

int T_index = 0;
int T_raw = 0;
float T_calibrated = 0;
float T_samples[ADC_T_SAMPLES_COUNT];
float T_average = 0;

int P_index = 0;
int P_raw = 0;
float P_calibrated = 0;
float P_samples[ADC_P_SAMPLES_COUNT];
float P_average = 0;

int button_state_index = 0;
int button_state_raw = 0;
float button_state_calibrated = 0;
float button_state_samples[ADC_BUTTON_SAMPLES_COUNT];
float button_state_average = 0;
float previous_button_state_average = 0;

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

void adc_on_loop()
{
    // adc_oneshot_read(adc2_handle, ADC_12V_CHANNEL, &V_12_raw);
    adc_oneshot_read(adc1_handle, ADC_V_FC_CHANNEL, &V_FC_raw);
    adc_oneshot_read(adc1_handle, ADC_T_CHANNEL, &T_raw);
    adc_oneshot_read(adc1_handle, ADC_P_CHANNEL, &P_raw);
    adc_oneshot_read(adc2_handle, ADC_BUTTON_STATE_CHANNEL, &button_state_raw);

    // V_12_calibrated = adc_apply_calibration(V_12_raw);
    V_FC_calibrated = V_FC_raw; // TODO adc_raw_to_mv_calibrated_60(V_FC_raw);
    T_calibrated = adc_apply_calibration(adc_temperature_coefficients, ADC_TEMPERATURE_COEFF_COUNT, T_raw);
    P_calibrated = adc_apply_calibration(adc_3v3_voltage_coefficients, ADC_3V3_VOLTAGE_COEFF_COUNT, P_raw);
    button_state_calibrated
        = adc_apply_calibration(adc_3v3_voltage_coefficients, ADC_3V3_VOLTAGE_COEFF_COUNT, button_state_raw);

    P_calibrated = adc_map(P_calibrated, 0, 2.995, 0, 300); // Pressure: 0-5 V => 0-300 bar

    // Średnia ruchoma - przerobić na funkcje
    if (V_FC_index <= ADC_V_FC_SAMPLES_COUNT)
    {
        V_FC_samples[V_FC_index] = V_FC_calibrated;
        V_FC_index++;
    }
    else
    {
        V_FC_index = 0;
    }

    for (int i = 0; i < ADC_V_FC_SAMPLES_COUNT; i++)
    {
        V_FC_average = V_FC_average + V_FC_samples[i];
    }
    V_FC_average = (V_FC_average / ADC_V_FC_SAMPLES_COUNT); // ADC input voltage
    // V_FC_average = ((FC_V_average / 20) * 2953) / 60000; // fuel cell 60V => 3V adc input

    // Temperature
    if (T_index <= ADC_T_SAMPLES_COUNT)
    {
        T_samples[T_index] = T_calibrated;
        T_index++;
    }
    else
    {
        T_index = 0;
    }

    for (int i = 0; i < ADC_T_SAMPLES_COUNT; i++)
    {
        T_average = T_average + T_samples[i];
    }
    T_average = (T_average / ADC_T_SAMPLES_COUNT);

    // Pressure
    if (P_index <= ADC_P_SAMPLES_COUNT)
    {
        P_samples[P_index] = P_calibrated;
        P_index++;
    }
    else
    {
        P_index = 0;
    }
    for (int i = 0; i < ADC_P_SAMPLES_COUNT; i++)
    {
        P_average = P_average + P_samples[i];
    }
    P_average = (P_average / ADC_P_SAMPLES_COUNT); // TEST...
    // P_bar = (P_average / 3) * 500; // 500bar sesnor 3V adc inpout = 5V from sensor

    if (button_state_index <= ADC_BUTTON_SAMPLES_COUNT)
    {
        button_state_samples[button_state_index] = button_state_calibrated;
        button_state_index++;
    }
    else
    {
        button_state_index = 0;
    }
    for (int i = 0; i < ADC_BUTTON_SAMPLES_COUNT; i++)
    {
        button_state_average = button_state_average + button_state_samples[i];
    }
    button_state_average = (button_state_average / ADC_BUTTON_SAMPLES_COUNT);

    // zbocze narastajace
    if (button_state_average - previous_button_state_average >= 0.5)
    {
        button_state = 1;
    }
    else
    {
        button_state = 0;
    }

    previous_button_state_average = button_state_average; // TODO
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
}
