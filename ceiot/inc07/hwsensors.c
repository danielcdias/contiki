#include "contiki.h"
#include "dev/leds.h"
#include "button-sensor.h"

#include <stdio.h> /* For printf() */

#include "util.h"

#define BUTTON_CHANGED 44

static struct etimer et, et2;
static u_int32_t desc = 0;

PROCESS(hwsensors, "hwsensors");
PROCESS(read_button, "read button");
AUTOSTART_PROCESSES(&hwsensors, &read_button);

PROCESS_THREAD(hwsensors, ev, data)
{
//    PROCESS_BEGIN();
//    etimer_set(&et, CLOCK_SECOND * 0.5);
//    PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
//    SENSORS_ACTIVATE(button_sensor);
//    setLed(LEDS_TODOS);
//    while (1) {
//        PROCESS_YIELD();
//        if (ev == sensors_event) {
//            static u_int8_t i;
//            for (i = 0; i < 4; i++) {
//                setLed(i);
//                etimer_set(&et, CLOCK_SECOND * 1);
//                PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
//            }
//        }
//    }
//    etimer_set(&et, CLOCK_SECOND * 0.5);
//    PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
//    setLed(LEDS_NENHUM);
//    SENSORS_DEACTIVATE(button_sensor);
//    PROCESS_END();
    PROCESS_BEGIN();
    etimer_set(&et, CLOCK_SECOND * 0.5);
    PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
    configureGPIOLed();
    configureGPIOButton();
    setGPIOLed(GPIO_LEDS_TODOS);
    printf("*** Starting *** \n");
    while (1) {
        static u_int8_t i, index;
        printf("Counter started!\n");
        for (i = 0; i < 4; i++) {
            index = (desc % 2 == 0) ? i : (3 - i);
            printf("Counting %s... %i.\n", ((desc % 2 == 0) ? "asc" : "desc"), index);
            setGPIOLed(index);
            etimer_set(&et, CLOCK_SECOND * 1);
            PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
        }
    }
    etimer_set(&et, CLOCK_SECOND * 0.5);
    PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
    setGPIOLed(GPIO_LEDS_NENHUM);
    PROCESS_END();
}

PROCESS_THREAD(read_button, ev, data)
{
    PROCESS_BEGIN();
    static u_int32_t lastState = 1, buttonState;
    while (1) {
        buttonState = readButton();
        if (buttonState != lastState) {
            lastState = buttonState;
            //if (buttonState == 1) {
                desc++;
                printf(">>> Button pressed!\n");
            //}
        }
        etimer_set(&et2, CLOCK_SECOND * 0.1);
        PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
    }
    PROCESS_END();
}
