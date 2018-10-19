#include "contiki.h"
#include "random.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"

#include <stdio.h>
#include <random.h>
/*---------------------------------------------------------------------------*/
PROCESS(lab_1_process, "Lab 1 Process");
AUTOSTART_PROCESSES(&lab_1_process);
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(lab_1_process, ev, data)
{
  PROCESS_BEGIN();
  printf("Exercicio Lab 1:\n");
  SENSORS_ACTIVATE(button_sensor);
  while (1) {
      PROCESS_YIELD();
      if (ev == sensors_event) {
          if (data == &button_left_sensor) {
              printf("<BT esq>\n");
              leds_toggle(LEDS_RED);
          } else if (data == &button_right_sensor) {
              printf("<BT dir>\n");
              leds_toggle(LEDS_GREEN);
          }
      }
  }
  SENSORS_DEACTIVATE(button_sensor);
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
