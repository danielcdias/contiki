/**
 * File:   util.c
 * Author: Daniel Carvalho Dias (daniel.dias@gmail.com)
 * Date:   11/05/2019
 *
 * Implements the util.h interface to configure and read from all sensors used by the CC2650 board.
 *
 * All 5 rain sensors will use a GPIO port and the reading will be performed by pooling.
 * The pluviometer will use a interruption event, just like the button implementation
 * for the CC2650 board.
 * The moisture sensor will be used as an ADC sensor.
 *
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

#include "util.h"

/******************
 * Global variables
 ******************/

static struct interruption_timer pluv_timer;

static struct sensors_sensor *sensor;

/***************************************************************
 * Functions for pluviometer interruption (event) implementation
 ***************************************************************/

static void pluviometer_event_handler() {

  if (!timer_expired( & pluv_timer.debounce)) {
    return;
  }

  timer_set( & pluv_timer.debounce, INTERRUPTION_DEBOUNCE_DURATION);

  /*
   * Start press duration counter on press (falling), notify on release
   * (rising)
   */
  if (ti_lib_gpio_read_dio(PLUVIOMETER_SENSOR) == 0) {
    pluv_timer.start = clock_time();
    pluv_timer.duration = 0;
  } else {
    pluv_timer.duration = clock_time() - pluv_timer.start;
    sensors_changed(&interruption_sensor);
  }

}

static void config_interuptions(int type, int c, uint32_t key) {
  switch (type) {
  case SENSORS_HW_INIT:
    ti_lib_gpio_clear_event_dio(key);
    ti_lib_rom_ioc_pin_type_gpio_input(key);
    ti_lib_rom_ioc_port_configure_set(key, IOC_PORT_GPIO, INTERRUPTION_GPIO_CFG);
    gpio_interrupt_register_handler(key, pluviometer_event_handler);
    break;
  case SENSORS_ACTIVE:
    if (c) {
      ti_lib_gpio_clear_event_dio(key);
      ti_lib_rom_ioc_pin_type_gpio_input(key);
      ti_lib_rom_ioc_port_configure_set(key, IOC_PORT_GPIO, INTERRUPTION_GPIO_CFG);
      ti_lib_rom_ioc_int_enable(key);
    } else {
      ti_lib_rom_ioc_int_disable(key);
    }
    break;
  default:
    break;
  }
}

static int config_pluviometer(int type, int value) {
  config_interuptions(type, value, PLUVIOMETER_SENSOR);

  return 1;
}

static int status(int type, uint32_t key_io_id) {
  switch (type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    if (ti_lib_rom_ioc_port_configure_get(key_io_id) & IOC_INT_ENABLE) {
      return 1;
    }
    break;
  default:
    break;
  }
  return 0;
}

static int value_interruption(int type) {
  if (type == INTERRUPTION_SENSOR_VALUE_STATE) {
    return
    ti_lib_gpio_read_dio(PLUVIOMETER_SENSOR) == 0 ?
        INTERRUPTION_SENSOR_VALUE_PRESSED : INTERRUPTION_SENSOR_VALUE_RELEASED;
  } else if (type == INTERRUPTION_SENSOR_VALUE_DURATION) {
    return (int) pluv_timer.duration;
  }
  return 0;
}

static int status_interruption(int type) {
  return status(type, PLUVIOMETER_SENSOR);
}

SENSORS_SENSOR(interruption_sensor, INTERRUPTION_SENSOR, value_interruption, config_pluviometer,
  status_interruption);

/**
 * util.h header functions implementation
 */
void configureGPIOSensors() {
  IOCPinTypeGpioInput(RAIN_SENSOR_SURFACE_1);
  IOCPinTypeGpioInput(RAIN_SENSOR_SURFACE_2);
  IOCPinTypeGpioInput(RAIN_SENSOR_SURFACE_3);
  IOCPinTypeGpioInput(RAIN_SENSOR_SURFACE_4);
  IOCPinTypeGpioInput(RAIN_SENSOR_DRAIN);
  IOCPinTypeGpioInput(PLUVIOMETER_SENSOR);
}

int readADSMoistureSensor() {
  sensor = sensors_find(ADC_SENSOR);
  static int valueRead;
  SENSORS_ACTIVATE(*sensor);
  sensor->configure(ADC_SENSOR_SET_CHANNEL, MOISTURE_SENSOR);
  valueRead = (sensor->value(ADC_SENSOR_VALUE));
  SENSORS_DEACTIVATE(*sensor);
  return valueRead;
}

u_int32_t readGPIOSensor(u_int32_t dio) {
  return GPIO_readDio(dio);
}
