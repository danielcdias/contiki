#include <stdio.h>

#include "dev/leds.h"
#include "contiki.h"
#include "dev/button-sensor.h"
#include "board.h"
#include "ti-lib.h"

#define LEDS_NENHUM 0
#define LEDS_VERMELHO 1
#define LEDS_VERDE 2
#define LEDS_TODOS 3

#define GPIO_LEDS_NENHUM  0
#define GPIO_LEDS_1  1
#define GPIO_LEDS_2  2
#define GPIO_LEDS_TODOS  3

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

void setGPIOLed(u_int32_t dio) {
    if (dio == GPIO_LEDS_NENHUM) {
        GPIO_clearDio(IOID_29);
        GPIO_clearDio(IOID_30);
    }
    if (dio == GPIO_LEDS_1) {
        GPIO_setDio(IOID_29);
        GPIO_clearDio(IOID_30);
    }
    if (dio == GPIO_LEDS_2) {
        GPIO_clearDio(IOID_29);
        GPIO_setDio(IOID_30);
    }
    if (dio == GPIO_LEDS_TODOS) {
        GPIO_setDio(IOID_29);
        GPIO_setDio(IOID_30);
    }
}

void configureGPIOLed() {
    IOCPinTypeGpioOutput(IOID_29);
    IOCPinTypeGpioOutput(IOID_30);
}

u_int32_t readButton() {
    return GPIO_readDio(IOID_28);
}

void configureGPIOButton() {
    IOCPinTypeGpioInput(IOID_28);
}


// IOCPinTypeGpioInput
// GPIO_setDio
