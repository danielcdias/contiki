#include <stdio.h>
#include <random.h>

#include "contiki.h"
#include "random.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"
#include "util.h"

#define TAMANHO_MEMORIA 5

/*---------------------------------------------------------------------------*/
PROCESS(lab_4_process, "Lab 4 Process");
AUTOSTART_PROCESSES(&lab_4_process);
/*---------------------------------------------------------------------------*/

u_int8_t sorteiaLed()
{
    static int8_t sorteio;
    sorteio = random_rand()%2;
    if (sorteio == 0) {
        return LEDS_VERMELHO;
    } else {
        return LEDS_VERDE;
    }
}

static struct etimer et;
PROCESS_THREAD(lab_4_process, ev, data)
{
  PROCESS_BEGIN();
  etimer_set(&et, CLOCK_SECOND * 0.5);
  PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
  setLed(LEDS_NENHUM);
  printf("\n--- Exercicio Lab 4 ---\n\n");
  printf("- Jogo da Memoria - Dificil -\n\n");
  printf("Preste atencao em qual led ira acender e apagar.\n");
  printf("Aperte o botao 1 para o led vermelho\n");
  printf("ou botao 2 para o led verde.\n\n");
  printf("A cada sequencia correta, um novo led sera incluido.\n");
  printf("Acerte todas as sequencias ate %d leds para ganhar o jogo.\n", TAMANHO_MEMORIA);
  printf("\n");
  static u_int8_t i, j, k, index = 1, errou = 0;
  static u_int8_t lista_leds[TAMANHO_MEMORIA];
  for (i = 0; i < 5; i++) {
      printf("Comecando em %d...\r", (5-i));
      etimer_set(&et, CLOCK_SECOND * 1);
      PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
  }
  printf("\n");
  for (i = 0; i < TAMANHO_MEMORIA; i++) {
      lista_leds[i] = sorteiaLed();
      printf("\n--- Mostrando sequencia com %d led(s) ---\n", (i +1));
      for (k = 0; k < index; k++) {
          setLed(lista_leds[k]);
          etimer_set(&et, CLOCK_SECOND * 0.5);
          PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
          setLed(LEDS_NENHUM);
          etimer_set(&et, CLOCK_SECOND * 0.5);
          PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
      }
      SENSORS_ACTIVATE(button_sensor);
      printf("### COMECE! ###\n");
      for (j = 0; j < index; j++) {
          PROCESS_YIELD();
          if (ev == sensors_event) {
              if (((data == &button_left_sensor) && (lista_leds[j] == LEDS_VERMELHO)) || ((data == &button_right_sensor) && (lista_leds[j] == LEDS_VERDE))) {
                  printf(">>> Acertou!\n");
              } else {
                  errou = 1;
                  break;
              }
          }
      }
      SENSORS_DEACTIVATE(button_sensor);
      if (errou) {
          break;
      } else {
          index++;
          etimer_set(&et, CLOCK_SECOND * 0.5);
          PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
      }
  }
  if (errou) {
      setLed(LEDS_TODOS);
      printf("\n **** ERROU! *** \n");
      printf("\nVoce errou a sequencia atual. Acertou ate %d.\n", (index - 1));
      printf("A sequencia correta seria:\n");
      for (i = 0; i < index; i++) {
          printf("%d - Botao: %d\n", (i + 1), lista_leds[i]);
      }
  } else {
      printf("\n **** VOCE ACERTOU TODA A SEQUENCIA! PARABENS! *** \n");
      for (i = 0; i < 3; i++) {
          setLed(LEDS_TODOS);
          etimer_set(&et, CLOCK_SECOND * 0.5);
          PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
          setLed(LEDS_NENHUM);
          etimer_set(&et, CLOCK_SECOND * 0.5);
          PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
      }
  }
  printf("\n--- FIM ---\n");
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
