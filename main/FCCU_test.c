
#include <math.h>
#include <stdio.h>

#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <inttypes.h>

#include "driver/gpio.h"

#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_gen.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_timer.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/mcpwm_types.h"

//Necessary
#define MAIN_VALVE_PIN 37 //37
#define PURGE_VALVE_PIN 7  //7
#define MOSFET_PIN 11     //11

#define LED_PIN 33   //33 led CAN Status
bool led_state = 1;

//Fans
#define FAN_GND_PIN 8 // Simple fan have 2 wires and with it we need to use PWM on FAN_GND_PIN
#define FAN_PWM_PIN 45  //Complex fans with 3 or 4 wires need to set PWM and have power all the time (A lot of fans starts turning without PWM when power is ON)

//ADC channels
#define V12V_CHANNEL 2 //GPIO 13, ADC 2, IO 4 CH 2
#define FC_V_CHANNEL 3 //GPIO 4, ADC 1 CH 3
#define TEMP_SENSOR_CHANNEL 1 //GPIO 2, ADC 1 CH 1
#define PRESSURE_SENSOR_CHANNEL 2 //GPIO 3, ADC 1 CH 2
#define BUTTON_STATE_CHANNEL 0 // GPIO 1, ADC 1 CH 0

//Start value
bool main_valve = 0;   // ================= ustawienie zaworu zasilajacego _ main valve n-mosfet (1 on, 0 off)
bool purge_valve = 0;  // ================= ustawienie przedmuchu - purge valve n-mosfet (1 on, 0 off)
bool mosfet = 0;  // ================= ustawienie zwarcia mosfet - driver i n-mosfet (1 on, 0 off)

// Fans PWM start filling (%)
int fan_gnd_duty_cycle_percent = 0;
int fan_PWM_duty_cycle_percent = 10; 

//Control
//int START_V = 3000;
//int PURGE_V = 1000;

bool start = 0;
bool purge = 0;

//Average
int FC_index = 0;
int FC_raw = 0;
float FC_calibrated = 0;
float FC_V_samples[41];
float FC_V_average = 0;

int T_index = 0;
int T_raw = 0;
float T_calibrated = 0;
float T_samples[41];
float T_average = 0;

int P_index = 0;
int P_raw = 0;
float P_calibrated = 0;
float P_samples[41];
float P_average = 0;

int button_state_index = 0;
int button_state_raw = 0;
float button_state_calibrated = 0;
float button_state_samples[61];  //Drgania styków i dłuzsze przytrzymanie
float button_state_average = 0;
float previous_button_state_average = 0;


// frequncy of clock (each tick)
// 2.5 MHz -> 40 ns
#define SERVO_TIMEBASE_RESOLUTION_HZ 25000000 // ------------Sprawdzić oscyloskopem

void fuel_cell();
void send_esp_logi();

// number of ticks in each period
// 100 ticks, 40 us;
#define SERVO_TIMEBASE_PERIOD 1000

mcpwm_timer_handle_t timer_handle_1 = NULL;
mcpwm_oper_handle_t operator_handle_1 = NULL;
mcpwm_cmpr_handle_t comparator_handle_1 = NULL;
mcpwm_gen_handle_t generator_handle_1 = NULL;
mcpwm_timer_handle_t timer_handle_2 = NULL;
mcpwm_oper_handle_t operator_handle_2 = NULL;
mcpwm_cmpr_handle_t comparator_handle_2 = NULL;
mcpwm_gen_handle_t generator_handle_2 = NULL;

void pwm_configuration() {
  // timer 1
  mcpwm_timer_config_t timer_config1 = {
      .group_id = 0,
      .intr_priority = 0,
      .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
      .resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ,
      .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
      .period_ticks = SERVO_TIMEBASE_PERIOD,
  };
  mcpwm_new_timer(&timer_config1, &timer_handle_1);

  // timer 2
  mcpwm_timer_config_t timer_config2 = {
      .group_id = 0,
      .intr_priority = 0,
      .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
      .resolution_hz = SERVO_TIMEBASE_RESOLUTION_HZ,
      .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
      .period_ticks = SERVO_TIMEBASE_PERIOD,
  };
  mcpwm_new_timer(&timer_config2, &timer_handle_2);

  // operator 1
  mcpwm_operator_config_t operator_config1 = {
      .group_id = 0, // operator must be in the same group to the timer
  };
  mcpwm_new_operator(&operator_config1, &operator_handle_1);

  // operator 2
  mcpwm_operator_config_t operator_config2 = {
      .group_id = 0, // operator must be in the same group to the timer
  };
  mcpwm_new_operator(&operator_config2, &operator_handle_2);

  // comparator 1
  mcpwm_comparator_config_t comparator_config1 = {
      .flags.update_cmp_on_tez = true,
  };
  mcpwm_new_comparator(operator_handle_1, &comparator_config1,
                       &comparator_handle_1);

  // comparator 2
  mcpwm_comparator_config_t comparator_config2 = {
      .flags.update_cmp_on_tez = true,
  };
  mcpwm_new_comparator(operator_handle_2, &comparator_config2,
                       &comparator_handle_2);

  // generator 1
  mcpwm_generator_config_t generator_config1 = {
      .gen_gpio_num = FAN_GND_PIN, 
  };
  mcpwm_new_generator(operator_handle_1, &generator_config1,
                      &generator_handle_1);

  // generator 2
  mcpwm_generator_config_t generator_config2 = {
      .gen_gpio_num = FAN_PWM_PIN,
  };
  mcpwm_new_generator(operator_handle_2, &generator_config2,
                      &generator_handle_2);

  // operator 1 and generator 1 connection
  mcpwm_operator_connect_timer(operator_handle_1, timer_handle_1);

  // operator 2 and generator 2 connection
  mcpwm_operator_connect_timer(operator_handle_2, timer_handle_2);

  // other configs 1
  mcpwm_generator_set_action_on_timer_event(
      generator_handle_1, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                                       MCPWM_TIMER_EVENT_EMPTY,
                                                       MCPWM_GEN_ACTION_HIGH));
  mcpwm_generator_set_action_on_compare_event(
      generator_handle_1,
      MCPWM_GEN_COMPARE_EVENT_ACTION(
          MCPWM_TIMER_DIRECTION_UP, comparator_handle_1, MCPWM_GEN_ACTION_LOW));
  mcpwm_timer_enable(timer_handle_1);
  mcpwm_timer_start_stop(timer_handle_1, MCPWM_TIMER_START_NO_STOP);

  // other configs 2
  mcpwm_generator_set_action_on_timer_event(
      generator_handle_2, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                                       MCPWM_TIMER_EVENT_EMPTY,
                                                       MCPWM_GEN_ACTION_HIGH));
  mcpwm_generator_set_action_on_compare_event(
      generator_handle_2,
      MCPWM_GEN_COMPARE_EVENT_ACTION(
          MCPWM_TIMER_DIRECTION_UP, comparator_handle_2, MCPWM_GEN_ACTION_LOW));
  mcpwm_timer_enable(timer_handle_2);
  mcpwm_timer_start_stop(timer_handle_2, MCPWM_TIMER_START_NO_STOP);
}

float mcpwm_duty_cycle_calculate(float duty_cycle_percent) {
  return ((duty_cycle_percent / 100) * SERVO_TIMEBASE_PERIOD);
}

void enable_configuration() {
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


adc_oneshot_unit_handle_t adc1_handle;
adc_oneshot_unit_handle_t adc2_handle;



void adc_init() {
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


// Sending logs
void log_task(void *pvParameters) {
    while (1) {
        send_esp_logi();                 
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait 1000ms to next 
        led_state = !(led_state);
        gpio_set_level(LED_PIN, led_state);
        
    }
}

float map(float x, float in_min, float in_max, float out_min, float out_max) {
      return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float adc_raw_to_mv_calibrated(int adc_raw)
{
  // dac swiezakowi jakiemus (minster cyfryzacji prof. nadzwyczajny dr mgr edukacji wczesnoszkolnej inż. Marek K. (podkarpacianianin)(pomnik czynu rewolucyjnego)) z multimetrem zeby spisal adc_raw i napiecie z woltomierza i ulozyc nowe wspolczynniki dla wielomianu
  // na 1 roku jest to to sobie przecwiczy
  // najlepiej tak ze jak juz bedzie szlo do auta albo bedziemy potrzebowac dokladncyh wynikow
  // kalibracje trzeba dla kazdej plytki zrobic ale chyba nie dla adc1 i adc2 oddzielnie

  float adc_raw_f = adc_raw;
  //adc_raw_f = adc_raw_f;

  float adc_mV = 0;

  
  float a[11]; //Temperature czajnik calibrated
  a[0] =      2.5679088051711859e-004;
  a[1] =      8.1818138383964200e-004;
  a[2] =      4.4561346617176653e-007;            
  a[3] =     -1.5785641837443660e-009;
  a[4] =      2.6100902814800432e-012;
  a[5] =     -2.4247998157041470e-015;
  a[6] =      1.3522880124391428e-018;
  a[7] =     -4.6201730839219227e-022;
  a[8] =      9.4673093903828406e-026;
  a[9] =     -1.0679135163074677e-029;
  a[10] =     5.0946016674492631e-034;

  for (int i = 0; i < 11; i++)
  {
    for (int j = 0; j < i; j++)
    {
      a[i] *= adc_raw_f;
    }
    adc_mV += a[i];
  }

  return adc_mV;
}


float adc_raw_to_mv_calibrated_60(int adc_raw)
{
  // dac swiezakowi jakiemus (minster cyfryzacji prof. nadzwyczajny dr mgr edukacji wczesnoszkolnej inż. Marek K. (podkarpacianianin)(pomnik czynu rewolucyjnego)) z multimetrem zeby spisal adc_raw i napiecie z woltomierza i ulozyc nowe wspolczynniki dla wielomianu
  // na 1 roku jest to to sobie przecwiczy
  // najlepiej tak ze jak juz bedzie szlo do auta albo bedziemy potrzebowac dokladncyh wynikow
  // kalibracje trzeba dla kazdej plytki zrobic ale chyba nie dla adc1 i adc2 oddzielnie

  float adc_raw_f = adc_raw;
  //adc_raw_f = adc_raw_f;

  float adc_mV = 0;
  

  
  float a[11];    //Voltage 60V calibrated              //TODO ..............................................................!!!!!!
  a[0] =      2.5679088051711859e-004;
  a[1] =      8.1818138383964200e-004;
  a[2] =      4.4561346617176653e-007;
  a[3] =     -1.5785641837443660e-009;
  a[4] =      2.6100902814800432e-012;
  a[5] =     -2.4247998157041470e-015;
  a[6] =      1.3522880124391428e-018;
  a[7] =     -4.6201730839219227e-022;
  a[8] =      9.4673093903828406e-026;
  a[9] =     -1.0679135163074677e-029;
  a[10] =     5.0946016674492631e-034;

  for (int i = 0; i < 11; i++)
  {
    for (int j = 0; j < i; j++)
    {
      a[i] *= adc_raw_f;
    }
    adc_mV += a[i];
  }

  return adc_mV;
}

float adc_raw_to_mv_calibrated_3(int adc_raw)
{
  // dac swiezakowi jakiemus (minster cyfryzacji prof. nadzwyczajny dr mgr edukacji wczesnoszkolnej inż. Marek K. (podkarpacianianin)(pomnik czynu rewolucyjnego)) z multimetrem zeby spisal adc_raw i napiecie z woltomierza i ulozyc nowe wspolczynniki dla wielomianu
  // na 1 roku jest to to sobie przecwiczy
  // najlepiej tak ze jak juz bedzie szlo do auta albo bedziemy potrzebowac dokladncyh wynikow
  // kalibracje trzeba dla kazdej plytki zrobic ale chyba nie dla adc1 i adc2 oddzielnie

  float adc_raw_f = adc_raw;
  //adc_raw_f = adc_raw_f;

  float adc_mV = 0;
  

  
  float a[11];    //Voltage 3V calibrated              //TODO ..............................................................!!!!!!
  a[0] =      2.5679088051711859e-004;
  a[1] =      8.1818138383964200e-004;
  a[2] =      4.4561346617176653e-007;            
  a[3] =     -1.5785641837443660e-009;
  a[4] =      2.6100902814800432e-012;
  a[5] =     -2.4247998157041470e-015;
  a[6] =      1.3522880124391428e-018;
  a[7] =     -4.6201730839219227e-022;
  a[8] =      9.4673093903828406e-026;
  a[9] =     -1.0679135163074677e-029;
  a[10] =     5.0946016674492631e-034;

  for (int i = 0; i < 11; i++)
  {
    for (int j = 0; j < i; j++)
    {
      a[i] *= adc_raw_f;
    }
    adc_mV += a[i];
  }

  return adc_mV;
}


// Creating logs and send
void send_esp_logi() {

printf("%2.3f," 
         "%2.3f,"
         "%2.3f,"
         "%2.3f,"
         "%2.3f\n\r",
         FC_V_average, T_average, adc_raw_to_mv_calibrated(T_raw), P_average, button_state_average);
     
   }

void app_main(void) {

  //FreeRTOS task working separetly - 4096 is the memory slot
  xTaskCreate(log_task, "Log Task", 4096, NULL, 1, NULL);
  enable_configuration();
  pwm_configuration();
  adc_init();

  
  //gpio_set_level(LED_PIN, led_state);
  gpio_set_level(MAIN_VALVE_PIN, main_valve);
  gpio_set_level(PURGE_VALVE_PIN, purge_valve);
  gpio_set_level(MOSFET_PIN, mosfet);

  // Fan gnd control
  mcpwm_comparator_set_compare_value(
      comparator_handle_1, mcpwm_duty_cycle_calculate(fan_gnd_duty_cycle_percent));

  // Fan PWM control
  mcpwm_comparator_set_compare_value(
      comparator_handle_2, mcpwm_duty_cycle_calculate(fan_PWM_duty_cycle_percent));


  while (1) {       //ADC selection, channel
    //adc_oneshot_read(adc2_handle, ADC_CHANNEL_2, &adc_raw[1][2]);
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &FC_raw);
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_1, &T_raw);
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_2, &P_raw);
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &button_state_raw);

    //adc_cal[1][2] = adc_raw_to_mv_calibrated(adc_raw[1][2]);
    FC_calibrated = adc_raw_to_mv_calibrated_60(FC_raw); 
    T_calibrated = adc_raw_to_mv_calibrated(T_raw); 
    P_calibrated = adc_raw_to_mv_calibrated(P_raw);
    button_state_calibrated = adc_raw_to_mv_calibrated_3(button_state_raw);
  
    FC_calibrated = map(FC_calibrated, 0.000, 2.993, 0, 60);
    T_calibrated = map(T_calibrated, 0.638, 1.65, 15.4, 100);
    P_calibrated = map(P_calibrated, 0, 2.995, 0, 300); //Pressure 300bar 0-5V

    // Średnia ruchoma - przerobić na funkcje
    if (FC_index <= 40)
    {
      FC_V_samples[FC_index] = FC_calibrated;
      FC_index++;
    }
    else
    {
      FC_index = 0;
    }
    for (int i = 0; i < 40; i++)
    {
      FC_V_average = FC_V_average + FC_V_samples[i];
    }
    FC_V_average = (FC_V_average / 40); //ADC input voltage
    //FC_V_average = ((FC_V_average / 20) * 2953) / 60000; // fuel cell 60V => 3V adc input

    // Temperature
    if (T_index <= 40)
    {
      T_samples[T_index] = T_calibrated;
      T_index++;
    }
    else
    {
      T_index = 0;
    }
    for (int i = 0; i < 40; i++)
    {
      T_average = T_average + T_samples[i];
    }
    T_average = (T_average / 40);              // TEST...
    //T_C_average = 20 + (T_average * 1.5) / 50; // Sprawdzić .....................??????????????????????????????????

    // Pressure
    if (P_index <= 40)
    {
      P_samples[P_index] = P_calibrated;
      P_index++;
    }
    else
    {
      P_index = 0;
    }
    for (int i = 0; i < 40; i++)
    {
      P_average = P_average + P_samples[i];
    }
    P_average = (P_average / 40);  // TEST...
    //P_bar = (P_average / 3) * 500; // 500bar sesnor 3V adc inpout = 5V from sensor

    if (button_state_index <= 40)
    {
      button_state_samples[button_state_index] = button_state_calibrated;
      button_state_index++;
    }
    else
    {
      button_state_index = 0;
    }
    for (int i = 0; i < 40; i++)
    {
      button_state_average = button_state_average + button_state_samples[i];
    }
    button_state_average = (button_state_average / 40);  



/*
//TEST
            main_valve = 1;
            purge_valve = 1;
        fan_gnd_duty_cycle_percent = 1;
        fan_PWM_duty_cycle_percent = 30; //testy 3s ????
*/
  gpio_set_level(MAIN_VALVE_PIN, main_valve);
  gpio_set_level(PURGE_VALVE_PIN, purge_valve);
  gpio_set_level(MOSFET_PIN, mosfet);

  // Fan gnd control
  mcpwm_comparator_set_compare_value(
      comparator_handle_1, mcpwm_duty_cycle_calculate(fan_gnd_duty_cycle_percent));

  // Fan PWM control
  mcpwm_comparator_set_compare_value(
      comparator_handle_2, mcpwm_duty_cycle_calculate(fan_PWM_duty_cycle_percent));

      //TEST end
     

     fuel_cell();

            
     

    vTaskDelay(10 / portTICK_PERIOD_MS);
    //gpio_set_level(LED_PIN, 0);
  }
}


void fuel_cell(){
  
    //How to control PEM fuel cell?
    //power main valve, power fans, measure voltage, measure temperature, 
    //if temperature is rising from 40*C give higher PWM proportional to temp, if maksimum temp 65*C turn off fuel cell and give flag
    //if voltage falling down in time about 1 min turn purge valve and mosfet on about 1ms , if voltage to low 15V turn off and give flag

    if(button_state_average > 3.0) { //2.8 or 3.4V supplay
      start = !start;
      button_state_average = 0;
      gpio_set_level(LED_PIN, 1);
    }
    //previous_button_state_average = button_state_average;

  
    if (start == 1){
        main_valve = 1;
        fan_gnd_duty_cycle_percent = 100;
        fan_PWM_duty_cycle_percent = 30; //testy 3s ????
    } else
    {
              main_valve = 0;
        fan_gnd_duty_cycle_percent = 0;
        fan_PWM_duty_cycle_percent = 0; //testy 3s ????
    }

    //Przedmuchy
    if(button_state_average > 0.9 && button_state_average < 1.2) 
    {
        purge_valve = 1;
        mosfet = 1;

        gpio_set_level(LED_PIN, 1);
        gpio_set_level(PURGE_VALVE_PIN, purge_valve);
        vTaskDelay(300 / portTICK_PERIOD_MS);  //czas przedmuchu przed zwarciem
        gpio_set_level(LED_PIN, 0);
        gpio_set_level(MOSFET_PIN, mosfet);
        vTaskDelay(10 / portTICK_PERIOD_MS); //czas zwarcia - 100ms xd

        purge_valve = 0;
        mosfet = 0;

        gpio_set_level(MOSFET_PIN, mosfet);
        vTaskDelay(1 / portTICK_PERIOD_MS);  //czas przedmuchu przed zwarciem
        gpio_set_level(PURGE_VALVE_PIN, purge_valve);

    }
    else
    {
        purge_valve = 0;
        mosfet = 0;
    }



    //Chlodzenie
    if (T_average > 35) fan_PWM_duty_cycle_percent = T_average - 10;   //If you have simple 2 wire cable change fan_PWM to fan_gnd
    else if (T_average > 45) fan_PWM_duty_cycle_percent = T_average*2;
    else fan_PWM_duty_cycle_percent = 10;
    

}