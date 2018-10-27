#ifndef UTIL_H_
#define UTIL_H_

#define LEDS_NENHUM 0
#define LEDS_VERMELHO 1
#define LEDS_VERDE 2
#define LEDS_TODOS 3

#define GPIO_LEDS_NENHUM  0
#define GPIO_LEDS_1  1
#define GPIO_LEDS_2  2
#define GPIO_LEDS_TODOS  3

void setLed(u_int8_t led);

void setGPIOLed(u_int32_t dio);

void configureGPIOLed();

u_int32_t readButton();

void configureGPIOButton();

#endif
