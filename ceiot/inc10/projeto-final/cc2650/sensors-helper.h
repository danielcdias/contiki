/**
 * File:   sensors-helper.h
 * Author: Daniel Carvalho Dias (daniel.dias@gmail.com)
 * Date:   28/05/2019
 *
 * Defines interface to configure and read from all sensors used by the CC2650 board.
 *
 * All 5 rain sensors will use a GPIO port and the reading will be performed by pooling.
 * The pluviometer will use a interruption event, just like the button implementation
 * for the CC2650 board.
 * The moisture and temperature sensors will be used as ADC sensors.
 *
 */

#include "board.h"
#include "dev/adc-sensor.h"
#include "lib/sensors.h"

#ifndef UTIL_H_
#define UTIL_H_

/******************************************************************************
 * Definitions of port numbers and reading intervals
 ******************************************************************************

Constant for ports are:

> Digital IO ports:
RAIN_SENSOR_           Rain Sensor
  RAIN_SENSOR_SURFACE_ Rain Sensor placed in model surface
  RAIN_SENSOR_DRAIN    Rain Sensor placed in model drain
PLUVIOMETER_SENSOR     PLuviometer

> ADC ports:
MOISTURE_SENSOR        Soil Moisture Sensor
TEMPERATURE_SENSOR     Temperature Sensor

*/
#define RAIN_SENSOR_SURFACE_1 IOID_25
#define RAIN_SENSOR_SURFACE_2 IOID_26
#define RAIN_SENSOR_SURFACE_3 IOID_27
#define RAIN_SENSOR_SURFACE_4 IOID_28

#define RAIN_SENSOR_DRAIN IOID_29

#define PLUVIOMETER_SENSOR IOID_23

#define MOISTURE_SENSOR ADC_COMPB_IN_AUXIO0 // DIO 30

#define TEMPERATURE_SENSOR IOID_24

#define RAIN_SENSORS_READ_INTERVAL 0.1 // seconds

#define MOISTURE_SENSOR_READ_INTERVAL_NO_RAIN 30 // seconds
#define MOISTURE_SENSOR_READ_INTERVAL_RAIN 15 // seconds

#define REPORT_RAIN_SENSORS_ARRAY_INTERVAL 2980 // 298 seconds


#define RAIN_SENSOR_NOT_RAINING 1
#define RAIN_SENSOR_RAINING 0

#define PLUVIOMETER_VALUE 1

/******************************** Sensor IDs **********************************/

/* Rain Sensors */
#define SENSOR_RAIN_SURFACE_1 "CS1"
#define SENSOR_RAIN_SURFACE_2 "CS2"
#define SENSOR_RAIN_SURFACE_3 "CS3"
#define SENSOR_RAIN_SURFACE_4 "CS4"
#define SENSOR_RAIN_DRAIN_1 "CRL"

/* Capacitive soil moisture sensor */
#define SENSOR_CAPACITIVE_SOIL_MOISTURE "SCU"

/* Temperature sensor */
#define SENSOR_TEMPERATURE "TMP"

/* Pluviometer sensor */
#define SENSOR_PLUVIOMETER "PLV"

/******************************************************************************
 * Functions definitions
 ******************************************************************************/
void configureGPIOSensors();

uint32_t readADSMoistureSensor();

int readTemperatureSensor();

u_int32_t readGPIOSensor(u_int32_t dio);

#endif
