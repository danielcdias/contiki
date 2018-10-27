#include <stdio.h>

#include "dev/leds.h"
#include "contiki.h"
#include "dev/button-sensor.h"

#define LEDS_NENHUM 0
#define LEDS_VERMELHO 1
#define LEDS_VERDE 2
#define LEDS_TODOS 3

void setLed(u_int8_t led) {
    if (led == LEDS_NENHUM) {
        leds_off(LEDS_RED | LEDS_GREEN);
    }
    if (led == LEDS_VERDE) {
        leds_off(LEDS_RED);
        leds_on(LEDS_GREEN);
    }
    if (led == LEDS_VERMELHO) {
        leds_off(LEDS_GREEN);
        leds_on(LEDS_RED);
    }
    if (led == LEDS_TODOS) {
        leds_on(LEDS_RED | LEDS_GREEN);
    }
}
