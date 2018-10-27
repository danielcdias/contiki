#include "contiki.h"
#include "random.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"

#include <stdio.h>
#include <random.h>
/*---------------------------------------------------------------------------*/
PROCESS(lab_2_process, "Lab 2 Process");
AUTOSTART_PROCESSES(&lab_2_process);
/*---------------------------------------------------------------------------*/
static struct etimer et;
PROCESS_THREAD(lab_2_process, ev, data)
{
  PROCESS_BEGIN();
  etimer_set(&et, CLOCK_SECOND * 0.5);
  PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
  printf("Exercicio Lab 2:\n");
  SENSORS_ACTIVATE(button_sensor);
  leds_on(LEDS_RED);
  leds_off(LEDS_GREEN);
  while (1) {
      PROCESS_YIELD();
      if (ev == sensors_event) {
          if (data == &button_left_sensor) {
              printf("<BT esq>\n");
              if (etimer_expired(&et)) {
                  etimer_set(&et, CLOCK_SECOND * 2);
              }
          } else if (data == &button_right_sensor) {
              printf("<BT dir>\n");
              etimer_stop(&et);
          }
      }
      if (ev == PROCESS_EVENT_TIMER) {
          leds_toggle(LEDS_RED | LEDS_GREEN);
          etimer_restart(&et);
      }
  }
  SENSORS_DEACTIVATE(button_sensor);
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
