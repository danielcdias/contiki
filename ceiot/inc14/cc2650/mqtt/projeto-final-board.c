/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *            uip_ipaddr_copy(&client_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
            client_conn->rport = UIP_UDP_BUF->destport;
 *
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/resolv.h"
#include "dev/leds.h"
#include "ti-lib.h"
#include "lpm.h"

#include <string.h>
#include <stdbool.h>

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#define SEND_INTERVAL       5 * CLOCK_SECOND
#define MAX_PAYLOAD_LEN     40
#define CONN_PORT     8802
#define MDNS 1

#define LED_SET_STATE (0x79)
#define LED_STATE (0x7C)

static char buf[MAX_PAYLOAD_LEN];

static struct uip_udp_conn *client_conn;

static u_int16_t current_duty = 0;
static u_int16_t loadvalue, ticks;

#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN])
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

uint8_t pwm_request_max_pm(void)
{
    return LPM_MODE_DEEP_SLEEP;
}

void sleep_enter(void)
{
    leds_on(LEDS_RED);
}

void sleep_leave(void)
{
    leds_off(LEDS_RED);
}

LPM_MODULE(pwmdrive_module, pwm_request_max_pm, sleep_enter, sleep_leave, LPM_DOMAIN_PERIPH);

u_int16_t pwminit(u_int32_t freq)
{
    u_int32_t load = 0;
    ti_lib_ioc_pin_type_gpio_output(IOID_14);
    ti_lib_ioc_pin_type_gpio_output(IOID_15);
    leds_off(LEDS_RED);

    /* Enable GPT0 clocks under active, sleep, deep sleep */
    ti_lib_prcm_peripheral_run_enable(PRCM_PERIPH_TIMER0);
    ti_lib_prcm_peripheral_sleep_enable(PRCM_PERIPH_TIMER0);
    ti_lib_prcm_peripheral_deep_sleep_enable(PRCM_PERIPH_TIMER0);
    ti_lib_prcm_load_set();
    while(!ti_lib_prcm_load_get());

    /* Register with LPM. This will keep the PERIPH PD powered on
    * during deep sleep, allowing the pwm to keep working while the chip is
    * being power-cycled */
    lpm_register_module(&pwmdrive_module);

    /* Drive the I/O ID with GPT0 / Timer A */
    ti_lib_ioc_port_configure_set(IOID_14, IOC_PORT_MCU_PORT_EVENT0, IOC_STD_OUTPUT);
    ti_lib_ioc_port_configure_set(IOID_15, IOC_PORT_MCU_PORT_EVENT0, IOC_STD_OUTPUT);

    /* GPT0 / Timer A: PWM, Interrupt Enable */
    ti_lib_timer_configure(GPT0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM | TIMER_CFG_B_PWM);

    /* Stop the timers */
    ti_lib_timer_disable(GPT0_BASE, TIMER_A);
    ti_lib_timer_disable(GPT0_BASE, TIMER_B);
    if(freq > 0) {
        load = (GET_MCU_CLOCK / freq);
        ti_lib_timer_load_set(GPT0_BASE, TIMER_A, load);
        ti_lib_timer_match_set(GPT0_BASE, TIMER_A, load-1);
        /* Start */
        ti_lib_timer_enable(GPT0_BASE, TIMER_A);
    }
    return load;
}

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");
AUTOSTART_PROCESSES(&resolv_process,&udp_client_process);
/*---------------------------------------------------------------------------*/
static void set_led(void)
{
    ticks = (current_duty == 0 ? 1 : (current_duty * loadvalue) / 100);
    printf("Current Duty: %i - ticks: %i - loadvalue: %i\n", current_duty, ticks, loadvalue);
    ti_lib_timer_match_set(GPT0_BASE, TIMER_A, loadvalue - ticks);
}
/*---------------------------------------------------------------------------*/
static void tcpip_handler(void)
{
    char i=0;
    #define SEND_ECHO (0xBA)
    if(uip_newdata()) //verifica se novos dados foram recebidos
    {
        char* dados = ((char*)uip_appdata); //este buffer ´e padr˜ao do contiki
        PRINTF("Recebidos %d bytes\n", uip_datalen());
        switch (dados[0])
        {
        case LED_SET_STATE:
        {
            uip_ipaddr_copy(&client_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
            client_conn->rport = UIP_UDP_BUF->destport;
            current_duty = dados[1];
            set_led();
            buf[0] = LED_STATE;
            buf[1] = current_duty;
            uip_udp_packet_send(client_conn, buf, 2);
            PRINTF("Enviando resposta LED_SET_STATE [");
            PRINT6ADDR(&client_conn->ripaddr);
            PRINTF("]:%u\n", UIP_HTONS(client_conn->rport));
            break;
        }
        default:
        {
            PRINTF("Comando Invalido: ");
            for(i=0;i<uip_datalen();i++)
            {
                PRINTF("0x%02X ",dados[i]);
            }
            PRINTF("\n");
            break;
        }
        }
    }
    return;
}
/*---------------------------------------------------------------------------*/
static void
timeout_handler(void)
{
    buf[0] = LED_STATE;
    buf[1] = current_duty;
    if(uip_ds6_get_global(ADDR_PREFERRED) == NULL) {
      PRINTF("Aguardando auto-configuracao de IP\n");
      return;
    }
    PRINTF("Cliente para [");
    PRINT6ADDR(&client_conn->ripaddr);
    PRINTF("]:%u\n", UIP_HTONS(client_conn->rport));
    uip_udp_packet_send(client_conn, buf, 2);
}
/*---------------------------------------------------------------------------*/
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
/*---------------------------------------------------------------------------*/
#if UIP_CONF_ROUTER
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
}
#endif /* UIP_CONF_ROUTER */
/*---------------------------------------------------------------------------*/

#if MDNS

static resolv_status_t
set_connection_address(uip_ipaddr_t *ipaddr)
{
#ifndef UDP_CONNECTION_ADDR
#if RESOLV_CONF_SUPPORTS_MDNS
#define UDP_CONNECTION_ADDR       2804:7f4:3b80:e539:3099:9bf5:5aa4:fc16 //2804:14c:87c4:2003:3099:9bf5:5aa4:fc16
#elif UIP_CONF_ROUTER
#define UDP_CONNECTION_ADDR       fd00:0:0:0:0212:7404:0004:0404
#else
#define UDP_CONNECTION_ADDR       fe80:0:0:0:6466:6666:6666:6666
#endif
#endif /* !UDP_CONNECTION_ADDR */

#define _QUOTEME(x) #x
#define QUOTEME(x) _QUOTEME(x)

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
#endif

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer et;
  uip_ipaddr_t ipaddr;

  PROCESS_BEGIN();
  PRINTF("UDP client process started\n");

  loadvalue = pwminit(5000);

#if UIP_CONF_ROUTER
  //set_global_address();
#endif

  etimer_set(&et, 2*CLOCK_SECOND);
  while(uip_ds6_get_global(ADDR_PREFERRED) == NULL)
  {
      PROCESS_WAIT_EVENT();
      if(etimer_expired(&et))
      {
          PRINTF("Aguardando auto-configuracao de IP\n");
          etimer_set(&et, 2*CLOCK_SECOND);
      }
  }


  print_local_addresses();

#if MDNS
  static resolv_status_t status = RESOLV_STATUS_UNCACHED;
  while(status != RESOLV_STATUS_CACHED) {
      status = set_connection_address(&ipaddr);

      if(status == RESOLV_STATUS_RESOLVING) {
          //PROCESS_WAIT_EVENT_UNTIL(ev == resolv_event_found);
          PROCESS_WAIT_EVENT();
      } else if(status != RESOLV_STATUS_CACHED) {
          PRINTF("Can't get connection address.\n");
          PROCESS_YIELD();
      }
  }
#else
  //c_onfigures the destination IPv6 address
  //uip_ip6addr(&ipaddr, 0xfd00, 0, 0, 0, 0x0212, 0x4b00, 0x0aff, 0x6b01);
  // 2804:14c:8786:8166:f41d:7811:2f8a:1d38
  //uip_ip6addr(&ipaddr, 0x2804, 0x14c, 0x8786, 0x8166, 0xf41d, 0x7811, 0x2f8a, 0x1d38);
  // 2804:14c:8786:8166:3099:9bf5:5aa4:fc16
  // uip_ip6addr(&ipaddr, 0x2804, 0x014c, 0x8786, 0x8166, 0x3099, 0x9bf5, 0x5aa4, 0xfc16);
  // 2804:14c:8786:8166:5979:1073:a316:fb8b
  uip_ip6addr(&ipaddr, 0x2804, 0x014c, 0x8786, 0x8166, 0x5979, 0x1073, 0xa316, 0xfb8b);
  //uip_ip6addr(&ipaddr, 0x2804, 0x014c, 0x8786, 0x8166, 0x756b, 0x998e, 0x5c3a, 0xf76d);
#endif
  /* new connection with remote host */
  client_conn = udp_new(&ipaddr, UIP_HTONS(CONN_PORT), NULL);
  udp_bind(client_conn, UIP_HTONS(CONN_PORT));

  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
    UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

  etimer_set(&et, SEND_INTERVAL);
  while(1) {
    PROCESS_YIELD();
    if(etimer_expired(&et)) {
      timeout_handler();
      etimer_restart(&et);
    } else if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
