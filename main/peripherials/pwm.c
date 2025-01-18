#include "pwm.h"

mcpwm_timer_handle_t timer_handle_1 = NULL;
mcpwm_oper_handle_t operator_handle_1 = NULL;
mcpwm_cmpr_handle_t comparator_handle_1 = NULL;
mcpwm_gen_handle_t generator_handle_1 = NULL;
mcpwm_timer_handle_t timer_handle_2 = NULL;
mcpwm_oper_handle_t operator_handle_2 = NULL;
mcpwm_cmpr_handle_t comparator_handle_2 = NULL;
mcpwm_gen_handle_t generator_handle_2 = NULL;

// Fans PWM start filling (%)
int fan_gnd_duty_cycle_percent = 0;
int fan_PWM_duty_cycle_percent = 10;

float pwm_duty_cycle_to_ticks(float duty_cycle_percent)
{
    return ((duty_cycle_percent / 100) * PWM_TIMEBASE_PERIOD);
}

void pwm_set_gnd_duty_cycle(float duty_cycle_percent)
{
    mcpwm_comparator_set_compare_value(comparator_handle_1, pwm_duty_cycle_to_ticks(duty_cycle_percent));
}

void pwm_set_pwm_duty_cycle(float duty_cycle_percent)
{
    mcpwm_comparator_set_compare_value(comparator_handle_2, pwm_duty_cycle_to_ticks(duty_cycle_percent));
}

void pwm_init()
{
    // timer 1
    mcpwm_timer_config_t timer_config1 = {
        .group_id = 0,
        .intr_priority = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = PWM_TIMEBASE_RESOLUTION_HZ,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
        .period_ticks = PWM_TIMEBASE_PERIOD,
    };
    mcpwm_new_timer(&timer_config1, &timer_handle_1);

    // timer 2
    mcpwm_timer_config_t timer_config2 = {
        .group_id = 0,
        .intr_priority = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = PWM_TIMEBASE_RESOLUTION_HZ,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
        .period_ticks = PWM_TIMEBASE_PERIOD,
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
    mcpwm_new_comparator(operator_handle_1, &comparator_config1, &comparator_handle_1);

    // comparator 2
    mcpwm_comparator_config_t comparator_config2 = {
        .flags.update_cmp_on_tez = true,
    };
    mcpwm_new_comparator(operator_handle_2, &comparator_config2, &comparator_handle_2);

    // generator 1
    mcpwm_generator_config_t generator_config1 = {
        .gen_gpio_num = FAN_GND_PIN,
    };
    mcpwm_new_generator(operator_handle_1, &generator_config1, &generator_handle_1);

    // generator 2
    mcpwm_generator_config_t generator_config2 = {
        .gen_gpio_num = FAN_PWM_PIN,
    };
    mcpwm_new_generator(operator_handle_2, &generator_config2, &generator_handle_2);

    // operator 1 and generator 1 connection
    mcpwm_operator_connect_timer(operator_handle_1, timer_handle_1);

    // operator 2 and generator 2 connection
    mcpwm_operator_connect_timer(operator_handle_2, timer_handle_2);

    // other configs 1
    mcpwm_generator_set_action_on_timer_event(generator_handle_1,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_compare_event(generator_handle_1,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator_handle_1, MCPWM_GEN_ACTION_LOW));
    mcpwm_timer_enable(timer_handle_1);
    mcpwm_timer_start_stop(timer_handle_1, MCPWM_TIMER_START_NO_STOP);

    // other configs 2
    mcpwm_generator_set_action_on_timer_event(generator_handle_2,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_compare_event(generator_handle_2,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator_handle_2, MCPWM_GEN_ACTION_LOW));
    mcpwm_timer_enable(timer_handle_2);
    mcpwm_timer_start_stop(timer_handle_2, MCPWM_TIMER_START_NO_STOP);

    // Fan gnd control
    mcpwm_comparator_set_compare_value(comparator_handle_1, pwm_duty_cycle_to_ticks(fan_gnd_duty_cycle_percent));

    // Fan PWM control
    mcpwm_comparator_set_compare_value(comparator_handle_2, pwm_duty_cycle_to_ticks(fan_PWM_duty_cycle_percent));
}