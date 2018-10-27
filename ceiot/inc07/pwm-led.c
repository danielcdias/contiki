#include "contiki.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"
#include "ti-lib.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "lpm.h"

#include <stdio.h> /* For printf() */

static struct etimer et;

uint8_t pwm_request_max_pm(void)
{
    return LPM_MODE_DEEP_SLEEP;
}

void sleep_enter(void)
{
    leds_on(LEDS_RED);
}

void sleep_leave(void)
{
    leds_off(LEDS_RED);
}

LPM_MODULE(pwmdrive_module, pwm_request_max_pm, sleep_enter, sleep_leave, LPM_DOMAIN_PERIPH);

u_int16_t pwminit(u_int32_t freq)
{
    u_int32_t load = 0;
    ti_lib_ioc_pin_type_gpio_output(IOID_21);
    leds_off(LEDS_RED);

    /* Enable GPT0 clocks under active, sleep, deep sleep */
    ti_lib_prcm_peripheral_run_enable(PRCM_PERIPH_TIMER0);
    ti_lib_prcm_peripheral_sleep_enable(PRCM_PERIPH_TIMER0);
    ti_lib_prcm_peripheral_deep_sleep_enable(PRCM_PERIPH_TIMER0);
    ti_lib_prcm_load_set();
    while(!ti_lib_prcm_load_get());

    /* Register with LPM. This will keep the PERIPH PD powered on
    * during deep sleep, allowing the pwm to keep working while the chip is
    * being power-cycled */
    lpm_register_module(&pwmdrive_module);

    /* Drive the I/O ID with GPT0 / Timer A */
    ti_lib_ioc_port_configure_set(IOID_21, IOC_PORT_MCU_PORT_EVENT0, IOC_STD_OUTPUT);

    /* GPT0 / Timer A: PWM, Interrupt Enable */
    ti_lib_timer_configure(GPT0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM | TIMER_CFG_B_PWM);

    /* Stop the timers */
    ti_lib_timer_disable(GPT0_BASE, TIMER_A);
    ti_lib_timer_disable(GPT0_BASE, TIMER_B);
    if(freq > 0) {
        load = (GET_MCU_CLOCK / freq);
        ti_lib_timer_load_set(GPT0_BASE, TIMER_A, load);
        ti_lib_timer_match_set(GPT0_BASE, TIMER_A, load-1);
        /* Start */
        ti_lib_timer_enable(GPT0_BASE, TIMER_A);
    }
    return load;
}

PROCESS(pwmled, "PWM Led");
AUTOSTART_PROCESSES(&pwmled);

PROCESS_THREAD(pwmled, ev, data)
{
    PROCESS_BEGIN();
    etimer_set(&et, CLOCK_SECOND * 0.5);
    PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
    static u_int16_t current_duty = 0;
    static u_int16_t loadvalue, ticks;
    loadvalue = pwminit(5000);
    printf("*** INIT ***\n");
    SENSORS_ACTIVATE(button_sensor);
    while(1) {
        PROCESS_WAIT_EVENT();
        if (ev == sensors_event) {
            if ((data == &button_left_sensor) && current_duty > 0) {
                current_duty -= 10;
            }
            if ((data == &button_right_sensor) && current_duty < 100) {
                current_duty += 10;
            }
        }
        ticks = (current_duty == 0 ? 1 : (current_duty * loadvalue) / 100);
        printf("Current Duty: %i - ticks: %i - loadvalue: %i\n", current_duty, ticks, loadvalue);
        ti_lib_timer_match_set(GPT0_BASE, TIMER_A, loadvalue - ticks);
    }
    SENSORS_DEACTIVATE(button_sensor);
    PROCESS_END();
}

