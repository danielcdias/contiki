/**
 * File:  projeto-final-cc2650.c
 * Author: Daniel Carvalho Dias (daniel.dias@gmail.com)
 * Date:   25/05/2019
 *
 * Main C file for the Terraço Verde CWB project.
 * Implements a set of sensor readings to collect data regarding
 * the influence of green terraces in rain absortion.
 * Each CC2650 board will control a set of 5 rain sensors, 1 capacitive
 * soil moisture sensor and 1 pluviometer sensor.
 *
 */

/******************************************************************************
 * Constants and includes
 ******************************************************************************/

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "lib/random.h"
#include "sys/timer.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-nameserver.h"
#include "mqtt-sn.h"
#include "rpl.h"
#include "net/ip/resolv.h"
#include "net/rime/rime.h"
#include "simple-udp.h"
#include "ti-lib.h"
#include "dev/leds.h"

#define DEBUG 1

#include "net/ip/uip-debug.h"

#include "sensors-helper.h"
#include "pluv-interruption.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#define MDNS 1

#define UDP_PORT 1883

#define REQUEST_RETRIES 4
#define DEFAULT_SEND_INTERVAL (10 * CLOCK_SECOND)
#define REPLY_TIMEOUT (3 * CLOCK_SECOND)
#define INACTIVITY_TIMEOUT (300 * CLOCK_SECOND)

#define TOPIC_STA_SENSOR "/tvcwb1299/mmm/sta/%02X%02X/%s"
#define TOPIC_STA_GENERAL "/tvcwb1299/mmm/sta/%02X%02X"
#define TOPIC_CMD "/tvcwb1299/mmm/cmd/%02X%02X"

#ifndef UDP_CONNECTION_ADDR
#if RESOLV_CONF_SUPPORTS_MDNS
#define UDP_CONNECTION_ADDR       danieldias.mooo.com // pksr.eletrica.eng.br
#elif UIP_CONF_ROUTER
#define UDP_CONNECTION_ADDR       fd00:0:0:0:0212:7404:0004:0404
#else
#define UDP_CONNECTION_ADDR       fe80:0:0:0:6466:6666:6666:6666
#endif
#endif /* !UDP_CONNECTION_ADDR */

#define _QUOTEME(x) #x
#define QUOTEME(x) _QUOTEME(x)

#define GREEN_LED_SENDING_MESSAGE 1
#define GREEN_LED_NO_MESSAGE 2
#define GREEN_LED_OFF 3
#define GREEN_LED_OFF_REBOOTING 4

#define RED_LED_CONNECTING 1
#define RED_LED_CONNECTED 2
#define RED_LED_OFF 3
#define RED_LED_OFF_REBOOTING 4

/******************************************************************************
 * Global variables
 ******************************************************************************/

static struct mqtt_sn_connection mqtt_sn_c;
static char mqtt_client_id[17];
static char ctrl_topic[24] = "\0";
static char pub_topic_general[24] = "\0";
static char pub_topic_sensor[28] = "\0";
static uint16_t ctrl_topic_id;
static uint16_t publisher_topic_id;
static publish_packet_t incoming_packet;
static uint16_t ctrl_topic_msg_id;
static uint16_t reg_topic_msg_id;
static uint16_t mqtt_keep_alive=10;
static int8_t qos = 1;
static uint8_t retain = FALSE;
static clock_time_t send_interval;
static mqtt_sn_subscribe_request subreq;
static mqtt_sn_register_request regreq;
static bool is_connected = false;

static bool is_raining = false;

static uint8_t green_led_state = GREEN_LED_OFF;
static uint8_t red_led_state = RED_LED_CONNECTING;

static enum mqttsn_connection_status connection_state = MQTTSN_DISCONNECTED;

static struct ctimer connection_timer;
static process_event_t connection_timeout_event;

static process_event_t mqttsn_connack_event, network_inactivity_timeout_reset;

/******************************************************************************
 * Processes definition
 ******************************************************************************/

PROCESS(mqttsn_process, "Configure Connection and Topic Registration");
PROCESS(publish_process, "Register topic and publish data");
PROCESS(ctrl_subscription_process, "Subscribe to a device control channel");
PROCESS(inactivity_watchdog_process, "Monitor for network inactivity");
PROCESS(reboot_process, "Reboots the board");
PROCESS(green_led_process, "Controls green led indicator");
PROCESS(red_led_process, "Controls ref led indicator");

PROCESS(rain_sensors_process, "Reads from all rain sensors");
PROCESS(moisture_sensor_process, "Reads from capacitive soil moisture sensor");
PROCESS(pluviometer_sensor_process, "Receives events from pluviometer sensor");

AUTOSTART_PROCESSES(&mqttsn_process);

/******************************************************************************
 * Functions implementation
 ******************************************************************************/

/*********************************** General **********************************/

void reset_board()
{
  process_start(&reboot_process, 0);
}

void loop_forever()
{
  while (true);
}

void set_green_led(uint8_t value ) {
   green_led_state = value;
}

void set_red_led(uint8_t value) {
   red_led_state = value;
}

/************************************ MQTT ************************************/

static void
puback_receiver(struct mqtt_sn_connection *mqc, const uip_ipaddr_t *source_addr, const uint8_t *data, uint16_t datalen)
{
  PRINTF("[E] Puback received\n");
  set_green_led(GREEN_LED_NO_MESSAGE);
}

static void
connack_receiver(struct mqtt_sn_connection *mqc, const uip_ipaddr_t *source_addr, const uint8_t *data, uint16_t datalen)
{
  uint8_t connack_return_code;
  connack_return_code = *(data + 3);
  PRINTF("[E] Connack received\n");
  if (connack_return_code == ACCEPTED) {
    process_post(&mqttsn_process, mqttsn_connack_event, NULL);
  } else {
    PRINTF("[E] Connack error: %s\n", mqtt_sn_return_code_string(connack_return_code));
  }
}

static void
regack_receiver(struct mqtt_sn_connection *mqc, const uip_ipaddr_t *source_addr, const uint8_t *data, uint16_t datalen)
{
  regack_packet_t incoming_regack;
  memcpy(&incoming_regack, data, datalen);
  PRINTF("[E] Regack received\n");
  if (incoming_regack.message_id == reg_topic_msg_id) {
    if (incoming_regack.return_code == ACCEPTED) {
      publisher_topic_id = uip_htons(incoming_regack.topic_id);
    } else {
      PRINTF("[E] Regack error: %s\n", mqtt_sn_return_code_string(incoming_regack.return_code));
    }
  }
}

static void
suback_receiver(struct mqtt_sn_connection *mqc, const uip_ipaddr_t *source_addr, const uint8_t *data, uint16_t datalen)
{
  suback_packet_t incoming_suback;
  memcpy(&incoming_suback, data, datalen);
  PRINTF("[E] Suback received\n");
  if (incoming_suback.message_id == ctrl_topic_msg_id) {
    if (incoming_suback.return_code == ACCEPTED) {
      ctrl_topic_id = uip_htons(incoming_suback.topic_id);
    } else {
      PRINTF("[E] Suback error: %s\n", mqtt_sn_return_code_string(incoming_suback.return_code));
    }
  }
}

static void
publish_receiver(struct mqtt_sn_connection *mqc, const uip_ipaddr_t *source_addr, const uint8_t *data, uint16_t datalen)
{
  //publish_packet_t* pkt = (publish_packet_t*)data;
  memcpy(&incoming_packet, data, datalen);
  incoming_packet.data[datalen-7] = 0x00;
  PRINTF("[E] Published message received: %s\n", incoming_packet.data);
  //see if this message corresponds to ctrl channel subscription request
  if (uip_htons(incoming_packet.topic_id) == ctrl_topic_id) {
    // TODO Tratar dados recebidos
    // Verificar tamanho dos dados (quantidade de dígitos do número recebido)
    // e utilizar tipo de variavel correta.
    //current_duty = atoi(incoming_packet.data);
    // TODO Incluir código para comando recebido
  } else {
    PRINTF("[E] Unknown publication received.\n");
  }

}

static void
pingreq_receiver(struct mqtt_sn_connection *mqc, const uip_ipaddr_t *source_addr, const uint8_t *data, uint16_t datalen)
{
  PRINTF("[E] PingReq received\n");
}

static void
disconnect_receiver(struct mqtt_sn_connection *mqc, const uip_ipaddr_t *source_addr, const uint8_t *data, uint16_t datalen)
{
  PRINTF("[E] Disconnection received\n");
  is_connected = false;
  reset_board();
}

static const struct mqtt_sn_callbacks mqtt_sn_call = {
  publish_receiver,
  pingreq_receiver,
  NULL,
  connack_receiver,
  regack_receiver,
  puback_receiver,
  suback_receiver,
  disconnect_receiver,
  NULL
};

static void publish_status(const char* sensor_id, u_int16_t data)
{
  set_green_led(GREEN_LED_SENDING_MESSAGE);
  static char buf[20];
  static uint8_t buf_len;
  sprintf(pub_topic_sensor, TOPIC_STA_SENSOR, linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7], sensor_id);
  sprintf(buf, "%" PRIu16, data);
  PRINTF("Publishing at topic: %s -> msg: %s\n", pub_topic_sensor, buf);
  buf_len = strlen(buf);
  uint16_t result = mqtt_sn_send_publish(&mqtt_sn_c, publisher_topic_id,
                       MQTT_SN_TOPIC_TYPE_NORMAL, buf, buf_len, qos, retain);
  process_post(&inactivity_watchdog_process, network_inactivity_timeout_reset, NULL);
  if (result == 0) {
     PRINTF("** Error publishing message **");
  }
}


static void connection_timer_callback(void *mqc)
{
  process_post(&mqttsn_process, connection_timeout_event, NULL);
}

static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Client IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
    }
  }
}

static resolv_status_t
set_connection_address(uip_ipaddr_t *ipaddr)
{

  resolv_status_t status = RESOLV_STATUS_ERROR;

  if(uiplib_ipaddrconv(QUOTEME(UDP_CONNECTION_ADDR), ipaddr) == 0) {
    uip_ipaddr_t *resolved_addr = NULL;
    status = resolv_lookup(QUOTEME(UDP_CONNECTION_ADDR),&resolved_addr);
    if(status == RESOLV_STATUS_UNCACHED || status == RESOLV_STATUS_EXPIRED) {
      PRINTF("Attempting to look up %s\n",QUOTEME(UDP_CONNECTION_ADDR));
      resolv_query(QUOTEME(UDP_CONNECTION_ADDR));
      status = RESOLV_STATUS_RESOLVING;
    } else if(status == RESOLV_STATUS_CACHED && resolved_addr != NULL) {
      PRINTF("Lookup of \"%s\" succeded!\n",QUOTEME(UDP_CONNECTION_ADDR));
    } else if(status == RESOLV_STATUS_RESOLVING) {
      PRINTF("Still looking up \"%s\"...\n",QUOTEME(UDP_CONNECTION_ADDR));
    } else {
      PRINTF("Lookup of \"%s\" failed. status = %d\n",QUOTEME(UDP_CONNECTION_ADDR),status);
    }
    if(resolved_addr)
      uip_ipaddr_copy(ipaddr, resolved_addr);
  } else {
    status = RESOLV_STATUS_CACHED;
  }

  return status;
}

/******************************************************************************
 * Processes implementation
 ******************************************************************************/

PROCESS_THREAD(publish_process, ev, data)
{
  static uint8_t registration_tries;
  static struct etimer send_timer;
  static mqtt_sn_register_request *rreq = &regreq;

  PROCESS_BEGIN();

  send_interval = DEFAULT_SEND_INTERVAL;
  sprintf(pub_topic_general,TOPIC_STA_GENERAL,linkaddr_node_addr.u8[6],linkaddr_node_addr.u8[7]);
  PRINTF("Registering topic %s.\n", pub_topic_general);
  registration_tries =0;
  while (registration_tries < REQUEST_RETRIES)
  {

    reg_topic_msg_id = mqtt_sn_register_try(rreq,&mqtt_sn_c,pub_topic_general,REPLY_TIMEOUT);
    //PROCESS_WAIT_EVENT_UNTIL(mqtt_sn_request_returned(rreq));
    etimer_set(&send_timer, 5*CLOCK_SECOND);
    PROCESS_WAIT_EVENT();
    if (mqtt_sn_request_success(rreq)) {
      registration_tries = 4;
      PRINTF("Registration acked.\n");
    }
    else {
      registration_tries++;
      if (rreq->state == MQTTSN_REQUEST_FAILED) {
          PRINTF("Regack error: %s\n", mqtt_sn_return_code_string(rreq->return_code));
      }
    }
  }
  if (mqtt_sn_request_success(rreq)){
    //start topic publishing to topic at regular intervals
    etimer_set(&send_timer, send_interval);
    while(1)
    {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
      etimer_set(&send_timer, send_interval);
    }
  } else {
    PRINTF("Unable to register topic.\n");
  }
  PROCESS_END();
}

PROCESS_THREAD(ctrl_subscription_process, ev, data)
{
  static uint8_t subscription_tries;
  static mqtt_sn_subscribe_request *sreq = &subreq;
  static struct etimer periodic_timer;
  PROCESS_BEGIN();
  subscription_tries = 0;
  sprintf(ctrl_topic,TOPIC_CMD,linkaddr_node_addr.u8[6],linkaddr_node_addr.u8[7]);
  PRINTF("Requesting subscription\n");
  while(subscription_tries < REQUEST_RETRIES)
  {
      PRINTF("Subscribing... topic: %s\n", ctrl_topic);
      ctrl_topic_msg_id = mqtt_sn_subscribe_try(sreq,&mqtt_sn_c,ctrl_topic,0,REPLY_TIMEOUT);

      //PROCESS_WAIT_EVENT_UNTIL(mqtt_sn_request_returned(sreq));
      etimer_set(&periodic_timer, 5*CLOCK_SECOND);
      PROCESS_WAIT_EVENT();
      if (mqtt_sn_request_success(sreq)) {
          subscription_tries = 4;
          PRINTF("Subscription acked\n");
      }
      else {
          subscription_tries++;
          if (sreq->state == MQTTSN_REQUEST_FAILED) {
              PRINTF("Suback error: %s\n", mqtt_sn_return_code_string(sreq->return_code));
          }
      }
  }
  PROCESS_END();
}

PROCESS_THREAD(inactivity_watchdog_process, ev, data)
{
  static struct etimer inactivity_timer;
  PROCESS_BEGIN();
  network_inactivity_timeout_reset = process_alloc_event();
  etimer_set(&inactivity_timer, INACTIVITY_TIMEOUT);
  while (true)
  {
    PROCESS_WAIT_EVENT();
    if (ev == PROCESS_EVENT_TIMER) {
        if (etimer_expired(&inactivity_timer)) {
          reset_board();
        }
    }
    if (ev == network_inactivity_timeout_reset) {
      etimer_restart(&inactivity_timer);
    }
  }
  PROCESS_END();
}

PROCESS_THREAD(reboot_process, ev, data)
{
   static struct etimer et;
   PROCESS_BEGIN();
   set_red_led(RED_LED_OFF_REBOOTING);
   set_green_led(GREEN_LED_OFF_REBOOTING);
   etimer_set(&et, CLOCK_SECOND * 0.8);
   PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
   leds_off(LEDS_ALL);
   PRINTF("***** Watchdog detected network inactivity/disconnection. REBOOTING board...\n\n\n");
   static uint8_t i = 0;
   while (i < 6) {
     leds_toggle(LEDS_ALL);
     etimer_set(&et, CLOCK_SECOND * 0.8);
     PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
     leds_toggle(LEDS_ALL);
     etimer_set(&et, CLOCK_SECOND * 0.5);
     PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
     i++;
   }
   loop_forever();
   PROCESS_END();
}

PROCESS_THREAD(green_led_process, ev, data)
{
   static struct etimer et;
   PROCESS_BEGIN();
   leds_off(LEDS_GREEN);
   while (true) {
      if (green_led_state == GREEN_LED_SENDING_MESSAGE) {
         leds_off(LEDS_GREEN);
      } else if (green_led_state == GREEN_LED_NO_MESSAGE) {
         leds_on(LEDS_GREEN);
      } else if(green_led_state == GREEN_LED_OFF) {
         leds_off(LEDS_GREEN);
      } else if (green_led_state == GREEN_LED_OFF_REBOOTING) {
         break;
      }
      etimer_set(&et, CLOCK_SECOND * 0.1);
      PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
   }
   PROCESS_END();
}

PROCESS_THREAD(red_led_process, ev, data)
{
   static struct etimer et;
   PROCESS_BEGIN();
   leds_off(LEDS_RED);
   while (true) {
      if (red_led_state == RED_LED_CONNECTING) {
         leds_toggle(LEDS_RED);
      } else if (red_led_state == RED_LED_CONNECTED) {
         leds_on(LEDS_RED);
      } else if (red_led_state == RED_LED_OFF) {
         leds_off(LEDS_RED);
      } else if (red_led_state == RED_LED_OFF_REBOOTING) {
         break;
      }
      etimer_set(&et, CLOCK_SECOND * 0.6);
      PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);
   }
   PROCESS_END();
}

PROCESS_THREAD(mqttsn_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer et;
  static uip_ipaddr_t broker_addr,google_dns;
  static uint8_t connection_retries = 0;
  static resolv_status_t status;
  char contiki_hostname[16];

  PROCESS_BEGIN();

  process_start(&green_led_process, 0);
  process_start(&red_led_process, 0);

  etimer_set(&et, CLOCK_SECOND * 1);
  PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_TIMER);

  sprintf(contiki_hostname,"node%02X%02X",linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);
  resolv_set_hostname(contiki_hostname);
  PRINTF("----- Setting hostname to %s\n",contiki_hostname);

  mqttsn_connack_event = process_alloc_event();

  mqtt_sn_set_debug(1);
  uip_ip6addr(&google_dns, 0x2001, 0x4860, 0x4860, 0x0, 0x0, 0x0, 0x0, 0x8888);

  set_red_led(RED_LED_CONNECTING);

  etimer_set(&periodic_timer, 2*CLOCK_SECOND);
  while(uip_ds6_get_global(ADDR_PREFERRED) == NULL)
  {
    PROCESS_WAIT_EVENT();
    if(etimer_expired(&periodic_timer))
    {
        PRINTF("Waiting for IP auto configutation...\n");
        etimer_set(&periodic_timer, 2*CLOCK_SECOND);
    }
  }

  print_local_addresses();

  rpl_dag_t *dag = rpl_get_any_dag();
  if(dag) {
    uip_nameserver_update(&google_dns, UIP_NAMESERVER_INFINITE_LIFETIME);
  }

  status = RESOLV_STATUS_UNCACHED;
  while(status != RESOLV_STATUS_CACHED) {
    status = set_connection_address(&broker_addr);

    if(status == RESOLV_STATUS_RESOLVING) {
      PROCESS_WAIT_EVENT();
    } else if(status != RESOLV_STATUS_CACHED) {
      PRINTF("Can't get connection address.\n");
      etimer_set(&periodic_timer, 2*CLOCK_SECOND);
      PROCESS_WAIT_EVENT();
    }
  }

  mqtt_sn_create_socket(&mqtt_sn_c,UDP_PORT, &broker_addr, UDP_PORT);
  (&mqtt_sn_c)->mc = &mqtt_sn_call;

  sprintf(mqtt_client_id,"sens%02X%02X%02X%02X",linkaddr_node_addr.u8[4],linkaddr_node_addr.u8[5],linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);

  PRINTF("Requesting connection...\n");
  connection_timeout_event = process_alloc_event();
  connection_retries = 0;
  ctimer_set(&connection_timer, REPLY_TIMEOUT, connection_timer_callback, NULL);
  mqtt_sn_send_connect(&mqtt_sn_c,mqtt_client_id,mqtt_keep_alive);
  connection_state = MQTTSN_WAITING_CONNACK;
  while (connection_retries < 11)
  {
    PROCESS_WAIT_EVENT();
    if (ev == mqttsn_connack_event) {
      PRINTF("Connection acked.\n");
      ctimer_stop(&connection_timer);
      connection_state = MQTTSN_CONNECTED;
      connection_retries = 15;//using break here may mess up switch statement of proces
    }
    if (ev == connection_timeout_event) {
      connection_state = MQTTSN_CONNECTION_FAILED;
      connection_retries++;
      PRINTF("Connection timeout (%i).\n", connection_retries);
      ctimer_restart(&connection_timer);
      if (connection_retries < 15) {
        mqtt_sn_send_connect(&mqtt_sn_c,mqtt_client_id,mqtt_keep_alive);
        connection_state = MQTTSN_WAITING_CONNACK;
      }
    }
  }
  ctimer_stop(&connection_timer);
  if (connection_state == MQTTSN_CONNECTED){
    is_connected = true;
    set_red_led(RED_LED_CONNECTED);
    set_green_led(GREEN_LED_NO_MESSAGE);
    process_start(&ctrl_subscription_process, 0);
    etimer_set(&periodic_timer, 3*CLOCK_SECOND);
    while(!etimer_expired(&periodic_timer))
        PROCESS_WAIT_EVENT();
    process_start(&publish_process, 0);
    process_start(&inactivity_watchdog_process, NULL);
    process_start(&rain_sensors_process, NULL);
    process_start(&moisture_sensor_process, NULL);
    process_start(&pluviometer_sensor_process, NULL);
    etimer_set(&et, 2*CLOCK_SECOND);
    while(1)
    {
      PROCESS_WAIT_EVENT();
      if(etimer_expired(&et)) {
        etimer_restart(&et);
      }
    }
  } else {
    PRINTF("Unable to connect!\n");
    reset_board();
  }
  PROCESS_END();
}

PROCESS_THREAD(rain_sensors_process, ev, data)
{
   static struct etimer et;

   PROCESS_BEGIN();

   configureGPIOSensors();
   while (is_connected) {
       static int valueRead[5];
       valueRead[0] = readGPIOSensor(RAIN_SENSOR_SURFACE_1);
       valueRead[1] = readGPIOSensor(RAIN_SENSOR_SURFACE_2);
       valueRead[2] = readGPIOSensor(RAIN_SENSOR_SURFACE_3);
       valueRead[3] = readGPIOSensor(RAIN_SENSOR_SURFACE_4);
       valueRead[4] = readGPIOSensor(RAIN_SENSOR_DRAIN);

       if ((!is_raining) && (valueRead[0] == RAIN_SENSOR_RAINING)) {
          is_raining = true;
          publish_status(SENSOR_RAIN_SURFACE_1, RAIN_SENSOR_RAINING);
       }
       if ((!is_raining) && (valueRead[1] == RAIN_SENSOR_RAINING)) {
          is_raining = true;
          publish_status(SENSOR_RAIN_SURFACE_2, RAIN_SENSOR_RAINING);
       }
       if ((!is_raining) && (valueRead[2] == RAIN_SENSOR_RAINING)) {
          is_raining = true;
          publish_status(SENSOR_RAIN_SURFACE_3, RAIN_SENSOR_RAINING);
       }
       if ((!is_raining) && (valueRead[3] == RAIN_SENSOR_RAINING)) {
          is_raining = true;
          publish_status(SENSOR_RAIN_SURFACE_4, RAIN_SENSOR_RAINING);
       }

       if ((is_raining) && (valueRead[4] == RAIN_SENSOR_RAINING)) {
          publish_status(SENSOR_RAIN_DRAIN_1, RAIN_SENSOR_RAINING);
       }

       if ((valueRead[0] + valueRead[1] + valueRead[2] + valueRead[3]) == (RAIN_SENSOR_NOT_RAINING * 4)) {
          is_raining = false;
       }

       etimer_set(&et, RAIN_SENSORS_READ_INTERVAL * CLOCK_SECOND);
       PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && data == &et);
   }

   PROCESS_END();
}

PROCESS_THREAD(moisture_sensor_process, ev, data)
{
   static struct etimer et;

   PROCESS_BEGIN();

   while (is_connected) {
      int sensorRead = readADSMoistureSensor();
      printf("##### Moisture sensor - value read: %i\n", sensorRead);
      publish_status(SENSOR_CAPACITIVE_SOIL_MOISTURE, sensorRead);
      if (is_raining) {
         etimer_set(&et, (MOISTURE_SENSOR_READ_INTERVAL_RAIN  * CLOCK_SECOND));
       } else {
         etimer_set(&et, (MOISTURE_SENSOR_READ_INTERVAL_NO_RAIN  * CLOCK_SECOND));
      }
      PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && data == &et);
   }

   PROCESS_END();
}

PROCESS_THREAD(pluviometer_sensor_process, ev, data)
{
   PROCESS_BEGIN();
   SENSORS_ACTIVATE(pluviometer_sensor);

   while(is_connected) {
       PROCESS_YIELD();

       if(ev == sensors_event) {
           if(data == &pluviometer_sensor) {
               printf("##### Pluviometer Sensor event received!\n");
               publish_status(SENSOR_PLUVIOMETER, PLUVIOMETER_VALUE);
           }
       }
   }

   SENSORS_DEACTIVATE(pluviometer_sensor);
   PROCESS_END();
}
