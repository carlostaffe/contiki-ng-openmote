/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
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
 */

/* * includes -------------------------------------------------*/
#include "coap-engine.h"
#include "contiki.h"
#include "dev/button-hal.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "lib/sensors.h"
#include "sys/energest.h"
#include "sys/stimer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CONTIKI_TARGET_OPENMOTE_CC2538
#include "adxl346.h"
#include "dev/max44009.h"
#include "dev/sht21.h"
#endif

#include "project-conf.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

/* * macros -------------------------------------------------*/

/* Normal mode duration params in seconds */
#define NORMAL_OP_DURATION_DEFAULT 10
#define NORMAL_OP_DURATION_MIN     10
#define NORMAL_OP_DURATION_MAX     60
/*---------------------------------------------------------------------------*/
/* Observer notification period params in seconds */
#define PERIODIC_INTERVAL_DEFAULT  30
#define PERIODIC_INTERVAL_MIN      30
#define PERIODIC_INTERVAL_MAX      86400 /* 1 day */
/*---------------------------------------------------------------------------*/
#define VERY_SLEEPY_MODE_OFF 0
#define VERY_SLEEPY_MODE_ON  1
/*---------------------------------------------------------------------------*/
#define BUTTON_TRIGGER BUTTON_HAL_ID_BUTTON_ZERO
/*---------------------------------------------------------------------------*/
#define MAC_CAN_BE_TURNED_OFF  0
#define MAC_MUST_STAY_ON       1

#define KEEP_MAC_ON_MIN_PERIOD 10 /* secs */
/*---------------------------------------------------------------------------*/
#define PERIODIC_INTERVAL         CLOCK_SECOND
/*---------------------------------------------------------------------------*/
#define POST_STATUS_BAD           0x80
#define POST_STATUS_HAS_MODE      0x40
#define POST_STATUS_HAS_DURATION  0x20
#define POST_STATUS_HAS_INTERVAL  0x10
#define POST_STATUS_NONE          0x00
/*---------------------------------------------------------------------------*/
#define STATE_NORMAL 0
#define STATE_NOTIFY_OBSERVERS 1
#define STATE_VERY_SLEEPY 2

/* * variables globales -------------------------------------*/
static struct stimer st_duration;
static struct stimer st_interval;
/* static struct stimer st_min_mac_on_duration; */
static struct etimer et_periodic;
static process_event_t event_new_config;
static uint8_t state;

/*****************************************************************************/
typedef struct sleepy_config_s {
  unsigned long interval;
  unsigned long duration;
  uint8_t mode;
} sleepy_config_t;

sleepy_config_t config;

/* * funciones locales --------------------------------------*/

/* ** keep_mac_on --------------------------------------*/
static uint8_t
keep_mac_on(void)
{
/* por ahora no hacemos nada */
  return MAC_MUST_STAY_ON;
}
/* ** switch_to_normal --------------------------------------*/
static void
switch_to_normal(void)
{
  state = STATE_NOTIFY_OBSERVERS;

  /*
   * Stay in normal mode for 'duration' secs.
   * Transition back to normal in 'interval' secs, _including_ 'duration'
   */
  stimer_set(&st_duration, config.duration);
  stimer_set(&st_interval, config.interval);
}

/* ** switch_to_very_sleepy --------------------------------------*/
static void
switch_to_very_sleepy(void)
{
  state = STATE_VERY_SLEEPY;
}

/* * main thread --------------------------------------------*/

/* ** preamble ***************************************************************/
PROCESS(er_coap_server, "CoAP Sleepy Energest");
AUTOSTART_PROCESSES(&er_coap_server);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(er_coap_server, ev, data) {

  uint8_t mac_keep_on;
 
  PROCESS_BEGIN();

  config.mode = VERY_SLEEPY_MODE_OFF;
  config.interval = PERIODIC_INTERVAL_DEFAULT;
  config.duration = NORMAL_OP_DURATION_DEFAULT;

  PROCESS_PAUSE();

  state = STATE_NORMAL;

  LOG_INFO("Inicio del pt-coap-server\n");

  /* en esta llamada se inicializan los stimer */
  switch_to_normal();

  etimer_set(&et_periodic, PERIODIC_INTERVAL);
/* ** infinite loop **********************************************************/
  while (1) {
    PROCESS_YIELD();

    if(ev == button_hal_release_event &&
       ((button_hal_button_t *)data)->unique_id == BUTTON_TRIGGER) {
      switch_to_normal();
    }

    if(ev == event_new_config) {
      stimer_set(&st_interval, config.interval);
      stimer_set(&st_duration, config.duration);
    }

    if((ev == PROCESS_EVENT_TIMER && data == &et_periodic) ||
       (ev == button_hal_release_event &&
        ((button_hal_button_t *)data)->unique_id == BUTTON_TRIGGER) ||
       (ev == event_new_config)) {

      /*
       * Determine if the stack is about to do essential network maintenance
       * and, if so, keep the MAC layer on
       */
      mac_keep_on = keep_mac_on();

      if(mac_keep_on == MAC_MUST_STAY_ON || state != STATE_VERY_SLEEPY) {
        leds_on(LEDS_GREEN);
        /* NETSTACK_MAC.on(); */
      }

      /*
       * Next, switch between normal and very sleepy mode depending on config,
       * send notifications to observers as required.
       */
      if(state == STATE_NOTIFY_OBSERVERS) {
        /* coap_notify_observers(&readings_resource); */
        state = STATE_NORMAL;
      }

      if(state == STATE_NORMAL) {
        if(stimer_expired(&st_duration)) {
          stimer_set(&st_duration, config.duration);
          if(config.mode == VERY_SLEEPY_MODE_ON) {
            switch_to_very_sleepy();
          }
        }
      } else if(state == STATE_VERY_SLEEPY) {
        if(stimer_expired(&st_interval)) {
          switch_to_normal();
        }
      }

      if(mac_keep_on == MAC_CAN_BE_TURNED_OFF && state == STATE_VERY_SLEEPY) {
        leds_off(LEDS_GREEN);
        /* NETSTACK_MAC.off(); */
      } else {
        leds_on(LEDS_GREEN);
        /* NETSTACK_MAC.on(); */
      }

      /* Schedule next pass */
      etimer_set(&et_periodic, PERIODIC_INTERVAL);
    }
  
  } /* while (1) */

  PROCESS_END();
}
