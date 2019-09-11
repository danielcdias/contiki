/**
 * File:   test-sensor.c
 * Author: Daniel Carvalho Dias (daniel.dias@gmail.com)
 * Date:   11/05/2019
 *
 * Tests all sensors that can be connected in the CC2650 board, printing results in the serial output.
 *
 */

#include <stdio.h>

#include "contiki.h"
#include "sys/etimer.h"
#include "lib/sensors.h"
#include "button-sensor.h"

#include "sensors-helper.h"

/******************
 * Global variables
 ******************/

static struct etimer et_moistureSensor;

/**********************
 * Processes definition
 **********************/

PROCESS(calibrateMoistureSensor, "calibrateMoistureSensor");
AUTOSTART_PROCESSES(&calibrateMoistureSensor);

/**************************
 * Processes implementation
 **************************/

PROCESS_THREAD(calibrateMoistureSensor, ev, data) {
    PROCESS_BEGIN();

    SENSORS_ACTIVATE(button_sensor);

    etimer_set(&et_moistureSensor, 3 * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && data == &et_moistureSensor);

    printf("********** Starting Moisture Sensors Calibration...\n");

    while (1) {

       PROCESS_YIELD();

       if ((ev == sensors_event) && ((data == &button_left_sensor) || (data == &button_right_sensor))) {
          uint32_t s1, s2, s3;
          s1 = readADSMoistureSensor(MOISTURE_SENSOR_1);
          s2 = readADSMoistureSensor(MOISTURE_SENSOR_2);
          s3 = readADSMoistureSensor(MOISTURE_SENSOR_3);

          if ((s1 > 0) && (s2 > 0) && (s3 > 0)) {
             printf("{'s1':%lu,'s2':%lu,'s3':%lu}\n", s1, s2, s3);
          } else {
             printf("{'message':'reading error'}\n");
          }

       }

    }

    SENSORS_DEACTIVATE(button_sensor);

    PROCESS_END();
}
