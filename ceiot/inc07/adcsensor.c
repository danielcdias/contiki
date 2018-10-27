#include "contiki.h"
#include "dev/adc-sensor.h"
#include "lib/sensors.h"

#include <stdio.h> /* For printf() */

static struct etimer et;
static struct sensors_sensor *sensor;

PROCESS(adc_sensor_test, "adc sensor test");
AUTOSTART_PROCESSES(&adc_sensor_test);

PROCESS_THREAD(adc_sensor_test, ev, data)
{
    PROCESS_BEGIN();
    etimer_set(&et, CLOCK_SECOND * 0.5);
    PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
    sensor = sensors_find(ADC_SENSOR);

    static int valuePercent;
    static int valueRead;

    while (1) {
        SENSORS_ACTIVATE(*sensor);
        sensor->configure(ADC_SENSOR_SET_CHANNEL, ADC_COMPB_IN_AUXIO0);
        valueRead = (sensor->value(ADC_SENSOR_VALUE));
        valuePercent = (int)((valueRead == 0) ? 0.0 : (valueRead * 100) / 3300000);
        printf("Valor lido: %i - %i %%\n", valueRead, valuePercent);
        SENSORS_DEACTIVATE(*sensor);
        etimer_set(&et, CLOCK_SECOND * 1);
        PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
    }

    PROCESS_END();
}
