/**
 * File:   sensors-helper.c
 * Author: Daniel Carvalho Dias (daniel.dias@gmail.com)
 * Date:   28/05/2019
 *
 * Implements the sensors-helper.h interface to configure and read from all
 * sensors used by the CC2650 board.
 *
 * All 5 rain sensors and the temperature sensor will use a GPIO port and the
 * reading will be performed by pooling.
 *
 * The moisture sensor will be used as ADC sensor.
 *
 * The pluviometer will use a interruption event, just like a button's
 * implementation for the CC2650 board.
 * The presence of this sensor will be defined by a jumper. When ON, indicates
 * the pluviometer is installed.
 */

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
#include "temp-sensor-helper.h"

/******************
 * Global variables
 ******************/
static struct sensors_sensor *sensorMoisture;

/**
 * util.h header functions implementation
 */
void configureGPIOSensors() {
  IOCPinTypeGpioInput(RAIN_SENSOR_SURFACE_1);
  IOCPinTypeGpioInput(RAIN_SENSOR_SURFACE_2);
  IOCPinTypeGpioInput(RAIN_SENSOR_SURFACE_3);
  IOCPinTypeGpioInput(RAIN_SENSOR_SURFACE_4);
  IOCPinTypeGpioInput(RAIN_SENSOR_DRAIN);
  IOCPinTypeGpioInput(JUMPER_PLUVIOMETER_INSTALLED);
}

uint32_t readADSMoistureSensor() {
   static uint32_t valueRead;
   sensorMoisture = sensors_find(ADC_SENSOR);
   SENSORS_ACTIVATE(*sensorMoisture);
   sensorMoisture->configure(ADC_SENSOR_SET_CHANNEL, MOISTURE_SENSOR);
   valueRead = (uint32_t)(sensorMoisture->value(ADC_SENSOR_VALUE));
   SENSORS_DEACTIVATE(*sensorMoisture);
   return valueRead;
}

int readTemperatureSensor() {
   (void) ds18b20_probe();
   int result = 0;
   float temp;
   int ret = ds18b20_get_temp(&temp);
   if (ret) {
      result = (uint32_t)(temp * 100);
   }
   return result;
}

u_int32_t readGPIOSensor(u_int32_t dio) {
  return GPIO_readDio(dio);
}
