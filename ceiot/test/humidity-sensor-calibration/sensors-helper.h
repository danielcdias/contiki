#include "board.h"
#include "dev/adc-sensor.h"
#include "lib/sensors.h"

#ifndef SENSORS_HELPER_H_
#define SENSORS_HELPER_H_

/******************************************************************************
 * Definitions of port numbers and reading intervals
 ******************************************************************************/

#define MOISTURE_SENSOR_1 ADC_COMPB_IN_AUXIO0 // DIO 30
#define MOISTURE_SENSOR_2 ADC_COMPB_IN_AUXIO1 // DIO 29
#define MOISTURE_SENSOR_3 ADC_COMPB_IN_AUXIO2 // DIO 28

#define MIN_VALUE_ACCEPTED 400000
#define MAX_VALUE_ACCEPTED 1500000

#define MAX_READNING_ATTEMPTS 10

/******************************************************************************
 * Functions definitions
 ******************************************************************************/

uint32_t readADSMoistureSensor(u_int32_t port);

#endif
