#include "contiki.h"
#include "random.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"

#include <stdio.h>
#include <random.h>

/*---------------------------------------------------------------------------*/
PROCESS(lab_3_process, "Lab 3 Process");
AUTOSTART_PROCESSES(&lab_3_process);
/*---------------------------------------------------------------------------*/

u_int8_t sorteiaLed()
{
    static int8_t sorteio;
    sorteio = random_rand()%2;
    if (sorteio == 0) {
        return LEDS_RED;
    } else {
        return LEDS_GREEN;
    }
}

static struct etimer et;
PROCESS_THREAD(lab_3_process, ev, data)
{
  PROCESS_BEGIN();
  etimer_set(&et, CLOCK_SECOND * 0.5);
  PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
  leds_off(LEDS_RED - LEDS_GREEN);
  printf("\n--- Exercicio Lab 3 ---\n\n");
  printf("- Jogo da Memoria - Facil -\n\n");
  printf("Aperte o botao 1 quando o led vermelho acender\n");
  printf("ou o botao 2 quando o led verde acender.\n\n");
  printf("Voce tera 3 segundos para clicar no botao correto.\n\n");
  static u_int8_t i, acertos = 0, erros = 0, timeouts = 0, led;
  for (i = 0; i < 5; i++) {
      printf("Comecando em %d...\r", (5-i));
      etimer_set(&et, CLOCK_SECOND * 1);
      PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
  }
  SENSORS_ACTIVATE(button_sensor);
  printf("\n\n");
  for (i = 0; i < 10; i++) {
      etimer_set(&et, CLOCK_SECOND * 1);
      PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
      printf("Comece agora!\n");
      led = sorteiaLed();
      leds_on(led);
      etimer_set(&et, CLOCK_SECOND * 3);
      PROCESS_YIELD();
      if (ev == sensors_event) {
          if (!etimer_expired(&et)) {
              etimer_stop(&et);
              leds_off(led);
              if (((data == &button_left_sensor) && (led == LEDS_RED)) || ((data == &button_right_sensor) && (led == LEDS_GREEN))) {
                  acertos++;
                  printf("Voce acertou!\n\n");
              } else {
                  erros++;
                  printf("Voce errou!\n\n");
              }
          }
      }
      if (ev == PROCESS_EVENT_TIMER) {
          leds_off(led);
          timeouts++;
          printf("Voce demorou!\n\n");
      }
  }
  printf("\n*** FIM DE JOGO ***\n\n");
  printf("Acertos: %d\n", acertos);
  printf("Erros  : %d\n", erros);
  printf("Demoras: %d\n", timeouts);
  SENSORS_DEACTIVATE(button_sensor);
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
