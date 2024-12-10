
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
#define MAIN_VALVE_PIN 37
#define PURGE_VALVE_PIN 7
#define MOSFET_PIN 11

//Fans
#define FAN_GND_PIN 8 // Simple fan have 2 wires and with it we need to use PWM on FAN_GND_PIN
#define FAN_PWM_PIN 45  //Complex fans with 3 or 4 wires need to set PWM and have power all the time (A lot of fans starts turning without PWM when power is ON)



//Start value
bool main_valve = 0;   // ================= ustawienie zaworu zasilajacego _ main valve n-mosfet (1 on, 0 off)
bool purge_valve = 0;  // ================= ustawienie przedmuchu - purge valve n-mosfet (1 on, 0 off)
bool mosfet = 0;  // ================= ustawienie zwarcia mosfet - driver i n-mosfet (1 on, 0 off)


// Fans PWM start filling (%)
int fan_gnd_duty_cycle_percent = 100; // PWM A Buck/current control
int fan_PWM_duty_cycle_percent = 10; // PWM B Boost - reverse to voltage: min voltage = 100, max voltage = 40



//Control
int START_V = 3000;
int PURGE_V = 1000;

bool start = 0;
bool purge = 0;


//Average
int FC_numb = 0;
int FC_V_av[11];
int FC_V = 0;

int T_numb = 0;
int T_V_av[21];
int T_V = 0;
int T_C = 0;


int P_numb = 0;
int P_V_av[21];
int P_V = 0;
int P_bar = 0;


//adc
  int adc_raw[2][10];
  int adc_cal[2][10];

// frequncy of clock (each tick)
// 2.5 MHz -> 40 ns
#define SERVO_TIMEBASE_RESOLUTION_HZ 5000000 // ------------Sprawdzić oscyloskopem

void fuel_cell();

// number of ticks in each period
// 100 ticks, 40 us;
#define SERVO_TIMEBASE_PERIOD 10000

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
}

const static char *TAG = "EXAMPLE";
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel,
                                         adc_atten_t atten,
                                         adc_cali_handle_t *out_handle) {
  adc_cali_handle_t handle = NULL;
  esp_err_t ret = ESP_FAIL;
  bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  if (!calibrated) {
    ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
  if (!calibrated) {
    ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

  *out_handle = handle;
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "Calibration Success");
  } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
    ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
  } else {
    ESP_LOGE(TAG, "Invalid arg or no memory");
  }

  return calibrated;
}

adc_oneshot_unit_handle_t adc1_handle;
adc_oneshot_unit_handle_t adc2_handle;
adc_cali_handle_t cali1 = NULL;
adc_cali_handle_t cali2 = NULL;
adc_cali_handle_t cali3 = NULL;
adc_cali_handle_t cali4 = NULL;
adc_cali_handle_t cali5 = NULL;


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
      .atten = ADC_ATTEN_DB_6,
      .bitwidth = ADC_BITWIDTH_12,
  };

  //   adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_2, &config);
  //   adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_4, &config);
  //   adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_8, &config);

  //   adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_6, &config);
  //   adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_7, &config);


  example_adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_0, ADC_ATTEN_DB_2_5, //Start/stop - 3,3V and Purge buttons - 0,9V
                               &cali1);
  example_adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_1, ADC_ATTEN_DB_2_5, //Temperature 
                               &cali2);
  example_adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_2, ADC_ATTEN_DB_2_5, //Analog - Cylinder pressure (Spectronik 0-3V)
                               &cali3);
  example_adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_3, ADC_ATTEN_DB_2_5, //FC_V 0-55V to 0-3V
                               &cali4);
    example_adc_calibration_init(ADC_UNIT_2, ADC_CHANNEL_2, ADC_ATTEN_DB_2_5, //Aditional Voltage mesurment I/O 4 - 12V line
                               &cali5);
}

  
  
  //Logs sending using FreeRTOS xTask

// Creating logs and send
void send_esp_logi() {
    ESP_LOGI("ADC",
             "CAL \tButton_V: %d, \tT_V: %d, %d \t*C, \tP_V: %d, \tFC_V: %d, %d \tV, \t12_V: %d",
             adc_cal[1][0], adc_cal[1][1], T_C, adc_cal[1][2], adc_cal[1][3], FC_V, adc_cal[2][2]);
   }

// Sending logs
void log_task(void *pvParameters) {
    while (1) {
        send_esp_logi();                 
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait 1000ms to next 
    }
}



void app_main(void) {

  //FreeRTOS task working separetly - 4096 is the memory slot
  xTaskCreate(log_task, "Log Task", 4096, NULL, 1, NULL);
  enable_configuration();
  pwm_configuration();
  adc_init();

  
  gpio_set_level(MAIN_VALVE_PIN, main_valve);
  gpio_set_level(PURGE_VALVE_PIN, purge_valve);
  gpio_set_level(MOSFET_PIN, mosfet);

  // Fan gnd control
  mcpwm_comparator_set_compare_value(
      comparator_handle_1, mcpwm_duty_cycle_calculate(fan_gnd_duty_cycle_percent));

  // Fan PWM control
  mcpwm_comparator_set_compare_value(
      comparator_handle_2, mcpwm_duty_cycle_calculate(fan_PWM_duty_cycle_percent));

//   int adc_raw[2][10];
//   int adc_cal[2][10];




  while (1) {       //ADC selection, channel,    tabell int value set
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw[1][0]);
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_1, &adc_raw[1][1]);
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_2, &adc_raw[1][2]);
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &adc_raw[1][3]);
    adc_oneshot_read(adc2_handle, ADC_CHANNEL_2, &adc_raw[2][2]);


                            //mV calculation using integers
    adc_cali_raw_to_voltage(cali1, adc_raw[1][0], &adc_cal[1][0]);
    adc_cali_raw_to_voltage(cali1, adc_raw[1][1], &adc_cal[1][1]);
    adc_cali_raw_to_voltage(cali1, adc_raw[1][2], &adc_cal[1][2]);
    adc_cali_raw_to_voltage(cali1, adc_raw[1][3], &adc_cal[1][3]);
    adc_cali_raw_to_voltage(cali1, adc_raw[2][2], &adc_cal[2][2]);    //////Wszędzie to samo cali1               ?????????????????



    
    //Średnia ruchoma - przerobić na funkcje
    if (FC_numb <= 20){
      FC_V_av[FC_numb] = adc_cal[1][3];
      FC_numb++;
    } else {
        FC_numb=0;
    }
    for(int i=0; i<20; i++){
          FC_V = FC_V + FC_V_av[i];
        }
      FC_V = ((FC_V/20)*2953)/60000; //60V => 3V adc input

        //Temperature
    if (T_numb <= 20){
      T_V_av[T_numb] = adc_cal[1][1];
      T_numb++;
    } else {
        T_numb=0;
    }
    for(int j=0; j<10; j++){
        T_V = T_V + T_V_av[j];
        }
      T_V = (T_V/20); //TEST...
      T_C = 20 + (T_V*1.5)/50; //Sprawdzić .....................??????????????????????????????????




              //Pressure
    if (P_numb <= 20){
      P_V_av[P_numb] = adc_cal[1][2];
      P_numb++;
    } else {
        P_numb=0;
    }
    for(int j=0; j<10; j++){
        P_V = P_V + P_V_av[j];
        }
      P_V = (P_V/20); //TEST...
      P_bar = (P_V/3)*500; //500bar sesnor 3V adc inpout = 5V from sensor



    fuel_cell();
    
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}


void fuel_cell(){
    //How to control PEM fuel cell?
    //power main valve, power fans, mesure voltage, masure temperature, 
    //if temperature is rising from 40*C give higher PWM proportional to temp, if maksimum temp 65*C turn off fuel cell and give flag
    //if voltage folling down in time about 1 min turn purge valve and mosfet on about 1ms , if voltage to low 15V turn off and give flag

        //Start/stop - 3,3V
    if (start == 0 && adc_cal[1][0] >= START_V )
    {
        start = 1;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    } 
    if (start == 1 && adc_cal[1][0] >= START_V)
     {
        start = 0;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    } 



    if (start == 1){
        main_valve = 1;
        fan_gnd_duty_cycle_percent = 100;
        fan_PWM_duty_cycle_percent = 30; //testy 3s ????

    }



        //Przedmuchy
    if (adc_cal[1][0] > 800 && adc_cal[1][0] < 1300)
    {
        purge_valve = 1;
        mosfet = 1;

        gpio_set_level(PURGE_VALVE_PIN, purge_valve);
        vTaskDelay(100 / portTICK_PERIOD_MS);  //czas przedmuchu przed zwarciem
        gpio_set_level(MOSFET_PIN, mosfet);
        vTaskDelay(1 / portTICK_PERIOD_MS); //czas zwarcia - 1ms

        purge_valve = 0;
        mosfet = 0;

        gpio_set_level(MOSFET_PIN, mosfet);
        vTaskDelay(1 / portTICK_PERIOD_MS);  //czas przedmuchu przed zwarciem
        gpio_set_level(PURGE_VALVE_PIN, purge_valve);

    }else
    {
        purge_valve = 0;
        mosfet = 0;
    }



    //Chlodzenie
    if (T_C > 35) fan_PWM_duty_cycle_percent = T_C - 10;   //If you have simple 2 wire cable change fan_PWM to fan_gnd
    else if (T_C > 45) fan_PWM_duty_cycle_percent = T_C*2;
    else fan_PWM_duty_cycle_percent = 10;


}