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

/**
 * \file
 *      Erbium (Er) CoAP Engine example.
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include "coap-engine.h"
#include "contiki.h"
#include "dev/button-hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/
/*                     Librerias para openmote o energest                    */
/*****************************************************************************/
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "lib/sensors.h"
#include "sys/energest.h"
#include "sys/rtimer.h"

#ifdef CONTIKI_TARGET_OPENMOTE_CC2538
#include "adxl346.h"
#include "dev/max44009.h"
#include "dev/sht21.h"
#endif

/* #include "net/netstack.h" */
#include "net/routing/routing.h"
#include "project-conf.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding
 * sub-directory.
 */

#ifdef CONTIKI_TARGET_OPENMOTE_CC2538
extern coap_resource_t res_temperature, res_energest_periodic;
#else
extern coap_resource_t res_energest_periodic;
#endif

/*---------------------------------------------------------------------------*/
PROCESS(er_example_server, "Erbium Example Server");
AUTOSTART_PROCESSES(&er_example_server);
/*---------------------------------------------------------------------------*/

static struct rtimer timer_rtimer;

void rtimer_callback(struct rtimer *timer, void *ptr) {
  /* Re-arm rtimer */
  rtimer_set(&timer_rtimer, RTIMER_NOW() + RTIMER_SECOND / 2, 0,
             rtimer_callback, NULL);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(er_example_server, ev, data) {
  PROCESS_BEGIN();

  PROCESS_PAUSE();

  LOG_INFO("Inicio del pt-coap-server\n");

  /* bind recurso a uri-path */
  coap_activate_resource(&res_energest_periodic, "test/energest");

#ifdef CONTIKI_TARGET_OPENMOTE_CC2538
  coap_activate_resource(&res_temperature, "sensors/temperature");

  /* Initialize the SHT21 sensor */
  uint16_t sht21_present = SENSORS_ACTIVATE(sht21);
  if (sht21_present == SHT21_ERROR) {
    printf("SHT21 sensor is NOT present!\n");
    leds_on(LEDS_RED);
  }
#endif

  /* Initialize Energest Module*/
  /* struct energest_t energy = {0}; */
  energest_flush();

  energy.last_time = ENERGEST_GET_TOTAL_TIME();
  energy.last_cpu = energest_type_time(ENERGEST_TYPE_CPU);
  energy.last_lpm = energest_type_time(ENERGEST_TYPE_LPM);
  energy.curr_tx = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  energy.last_deep_lpm = energest_type_time(ENERGEST_TYPE_DEEP_LPM);
  energy.last_rx = energest_type_time(ENERGEST_TYPE_LISTEN);

  /* Define application-specific events here. */
  while (1) {
    PROCESS_WAIT_EVENT();

    /* inicio un rtimer para que el nodo pueda despertar luego de PM1+ */
    if (NETSTACK_ROUTING.node_is_reachable()) {
      rtimer_set(&timer_rtimer, RTIMER_NOW() + RTIMER_SECOND / 2, 0,
                 rtimer_callback, NULL);
    }
  } /* while (1) */

  PROCESS_END();
}