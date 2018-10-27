/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
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
 *
 */

/**
 * \file
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "dev/leds.h"
#include "button-sensor.h"

#include <stdio.h> /* For printf() */

#define LED_PING_EVENT 44
#define LED_PONG_EVENT 45

static struct etimer et_hello, et_blink, et_proc3;

char* getProcName(void* proc) {
    return (char*)PROCESS_NAME_STRING((struct process*)proc);
}

/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
PROCESS(blink_process, "Blink process");
PROCESS(proc3_process, "Proc 3 process");
PROCESS(pong_process, "Pong process");
PROCESS(read_button_process, "Read button process");
AUTOSTART_PROCESSES(&hello_world_process, &blink_process, &proc3_process, &pong_process, &read_button_process);
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();
  etimer_set(&et_hello, 4 * CLOCK_SECOND);

  while (1) {
      PROCESS_WAIT_EVENT();
      if (ev == PROCESS_EVENT_TIMER) {
          printf("[%s] Hello, world!\n", getProcName((void*)(&hello_world_process)));
          etimer_reset(&et_hello);
          process_post(&pong_process, LED_PING_EVENT, (void*)(&hello_world_process));
      }
      if (ev == LED_PONG_EVENT) {
          printf("[%s] >>> LED_PONG_EVENT received!\n", getProcName((void*)(&hello_world_process)));
      }
}
  
  PROCESS_END();
}

PROCESS_THREAD(blink_process, ev, data)
{
  PROCESS_BEGIN();
  etimer_set(&et_blink, 2 * CLOCK_SECOND);

  while (1) {
      PROCESS_WAIT_EVENT();
      if (ev == PROCESS_EVENT_TIMER) {
          leds_toggle(LEDS_GREEN);
          etimer_reset(&et_blink);
          process_post(&pong_process, LED_PING_EVENT, (void*)(&blink_process));
      }
      if (ev == LED_PONG_EVENT) {
          printf("[%s] >>> LED_PONG_EVENT received!\n", getProcName((void*)(&blink_process)));
      }
  }

  PROCESS_END();
}

PROCESS_THREAD(proc3_process, ev, data)
{
    PROCESS_BEGIN();
    etimer_set(&et_proc3, 10 * CLOCK_SECOND);

    while (1) {
        PROCESS_WAIT_EVENT();
        if (ev == PROCESS_EVENT_TIMER) {
            printf("[%s] Proc 3 process message!\n", getProcName((void*)(&proc3_process)));
            etimer_reset(&et_proc3);
            process_post(&pong_process, LED_PING_EVENT, (void*)(&proc3_process));
        }
        if (ev == LED_PONG_EVENT) {
            printf("[%s] >>> LED_PONG_EVENT received!\n", getProcName((void*)(&proc3_process)));
        }
    }

    PROCESS_END();
}

PROCESS_THREAD(read_button_process, ev, data)
{
    PROCESS_BEGIN();
    while(1){
        PROCESS_YIELD();

        if(ev == sensors_event){
            if(data == &button_left_sensor){
                printf("[%s] Left Button!\n", getProcName((void*)(&read_button_process)));
                leds_toggle(LEDS_RED);
            }
            else if(data == &button_right_sensor){
                leds_toggle(LEDS_GREEN);
                printf("[%s] Right Button!\n", getProcName((void*)(&read_button_process)));
            }
            process_post(&pong_process, LED_PING_EVENT, (void*)(&read_button_process));
        }
        if (ev == LED_PONG_EVENT) {
            printf("[%s] >>> LED_PONG_EVENT received!\n", getProcName((void*)(&read_button_process)));
        }
    }
    PROCESS_END();
    return 0;
}

PROCESS_THREAD(pong_process, ev, data)
{
    PROCESS_BEGIN();
    while (1) {
        PROCESS_WAIT_EVENT();
        if (ev == LED_PING_EVENT) {
            printf("[%s] >>> LED_PING_EVENT received from %s!\n", getProcName((void*)(&pong_process)), getProcName(data));
            //printf(">>> LED_PING_EVENT received from %s!\n", ((struct process*)data)->name);
            process_post((struct process*)data, LED_PONG_EVENT, NULL);
        }
    }

    PROCESS_END();
}

