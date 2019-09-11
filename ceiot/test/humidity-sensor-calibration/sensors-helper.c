#include <stdio.h>
#include <stdint.h>

#include "board.h"
#include "contiki.h"
#include "dev/adc-sensor.h"
#include "gpio-interrupt.h"
#include "lib/sensors.h"
#include "lpm.h"
#include "sys/timer.h"
#include "ti-lib.h"

#include "sensors-helper.h"

/******************
 * Global variables
 ******************/
static struct sensors_sensor *sensorMoisture;

static struct timer reading_timer;

uint32_t internalReading(u_int32_t port) {
   static uint32_t valueRead;
   sensorMoisture = sensors_find(ADC_SENSOR);
   SENSORS_ACTIVATE(*sensorMoisture);
   sensorMoisture->configure(ADC_SENSOR_SET_CHANNEL, port);
   valueRead = (uint32_t)(sensorMoisture->value(ADC_SENSOR_VALUE));
   SENSORS_DEACTIVATE(*sensorMoisture);
   // return (uint32_t)(valueRead/1000);
   return valueRead;
}

uint32_t internalReadingAttemps(u_int32_t port) {
   uint32_t result = -1;
   uint32_t readings[3];
   uint8_t i, r = 0;
   bool failed = true;
   for (i = 0; i < MAX_READNING_ATTEMPTS; i++) {
      uint32_t aux = internalReading(port);
      if ((aux >= MIN_VALUE_ACCEPTED) && (aux <= MAX_VALUE_ACCEPTED)) {
         readings[r] = aux;
         r++;
         if (r == 3) {
            failed = false;
            break;
         }
      }
      timer_set(&reading_timer, CLOCK_SECOND * 0.05);
      while (!timer_expired(&reading_timer));
   }
   if (failed) {
      result = 0;
   } else {
      if (r > 0) {
         uint32_t readings_sum = 0;
         for (i = 0; i < r; i++) {
            readings_sum += readings[i];
         }
         result = (uint32_t)(readings_sum / r);
      }
   }
   return result;
}

uint32_t readADSMoistureSensor(u_int32_t port) {
   return internalReadingAttemps(port);
}
