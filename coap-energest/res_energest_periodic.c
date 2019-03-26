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
 *      Example resource
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include "contiki.h"
#include <stdio.h>
#include <string.h>
#include "coap-engine.h"
#include "sys/energest.h"
#include <stdio.h>
#include <limits.h>

// FIXME: Necesario para incluir la variable global energy
#include "../project-conf.h"

energest_t energy;
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Energest"
#define LOG_LEVEL LOG_LEVEL_INFO

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_periodic_handler(void);

/* funcion para actualizar la estructura energy con */
/* los ultimos datos de consumo */
static void simple_energest_step(void);

/* funcion que devuelve el por mil del consumo de energia */
/* solo utilizada para imprimir por puerto serie */
static unsigned long to_permil(unsigned long delta_metric, unsigned long delta_time);


PERIODIC_RESOURCE(res_energest_periodic,
                  "title=\"Energest Vals \";rt=\"Energest\"",
                  res_get_handler,
                  NULL,
                  NULL,
                  NULL,
                  5000,
                  res_periodic_handler);

/*
 * Use local resource state that is accessed by res_get_handler() and altered by res_periodic_handler() or PUT or POST.
 */
static int32_t event_counter = 0;


  static void res_get_handler(coap_message_t * request,
                              coap_message_t * response, uint8_t * buffer,
                              uint16_t preferred_size, int32_t * offset) {

    simple_energest_step();

    unsigned int accept = -1;
    coap_get_header_accept(request, &accept);

    coap_set_header_content_format(response, TEXT_PLAIN);
      snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "%lu;%lu;%lu;%lu;%lu;%lu",
               energy.delta_time / ENERGEST_SECOND,
               energy.delta_cpu,
               energy.delta_lpm,
               energy.delta_deep_lpm,
               energy.delta_tx, 
               energy.delta_rx);
      coap_set_header_max_age(response, res_push.periodic->period / CLOCK_SECOND);
      coap_set_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  }

static void
res_periodic_handler()
{
  /* Do a periodic task here, e.g., sampling a sensor. */
  ++event_counter;

  /* Usually a condition is defined under with subscribers are notified, e.g., large enough delta in sensor reading. */
  if(1) {
    /* Notify the registered observers which will trigger the res_get_handler to create the response. */
    coap_notify_observers(&res_push);
  }
}

  /*---------------------------------------------------------------------------*/
  static unsigned long to_permil(unsigned long delta_metric,
                                 unsigned long delta_time) {
    return (1000ul * (delta_metric)) / delta_time;
  }
  /*---------------------------------------------------------------------------*/
  static void simple_energest_step(void) {
    static unsigned count = 0;
    /* extern struct energest_t energy; */
    energest_flush();

    energy.curr_time = ENERGEST_GET_TOTAL_TIME();
    energy.curr_cpu = energest_type_time(ENERGEST_TYPE_CPU);
    energy.curr_lpm = energest_type_time(ENERGEST_TYPE_LPM);
    energy.curr_deep_lpm = energest_type_time(ENERGEST_TYPE_DEEP_LPM);
    energy.curr_tx = energest_type_time(ENERGEST_TYPE_TRANSMIT);
    energy.curr_rx = energest_type_time(ENERGEST_TYPE_LISTEN);

    energy.delta_time = energy.curr_time - energy.last_time;
    energy.delta_cpu = energy.curr_cpu - energy.last_cpu;
    energy.delta_lpm = energy.curr_lpm - energy.last_lpm;
    energy.delta_deep_lpm = energy.curr_deep_lpm - energy.last_deep_lpm;
    energy.delta_tx = energy.curr_tx - energy.last_tx;
    energy.delta_rx = energy.curr_rx - energy.last_rx;

    energy.last_time = energy.curr_time;
    energy.last_cpu = energy.curr_cpu;
    energy.last_lpm = energy.curr_lpm;
    energy.last_deep_lpm = energy.curr_deep_lpm;
    energy.last_tx = energy.curr_tx;
    energy.last_rx = energy.curr_rx;

    LOG_INFO("--- Period summary #%u (%lu seconds)\n", count++,
             energy.delta_time / ENERGEST_SECOND);
    LOG_INFO("Total time  : %10lu\n", energy.delta_time);
    LOG_INFO("CPU         : %10lu/%10lu (%lu permil)\n", energy.delta_cpu,
             energy.delta_time, to_permil(energy.delta_cpu, energy.delta_time));
    LOG_INFO("LPM         : %10lu/%10lu (%lu permil)\n", energy.delta_lpm,
             energy.delta_time, to_permil(energy.delta_lpm, energy.delta_time));
    LOG_INFO("Deep LPM    : %10lu/%10lu (%lu permil)\n",
    energy.delta_deep_lpm,
             energy.delta_time, to_permil(energy.delta_deep_lpm, energy.delta_time));
    LOG_INFO("Radio Tx    : %10lu/%10lu (%lu permil)\n", energy.delta_tx,
             energy.delta_time, to_permil(energy.delta_tx, energy.delta_time));
    LOG_INFO("Radio Rx    : %10lu/%10lu (%lu permil)\n", energy.delta_rx,
             energy.delta_time, to_permil(energy.delta_rx, energy.delta_time));
    LOG_INFO("Radio total : %10lu/%10lu (%lu permil)\n",
             energy.delta_tx + energy.delta_rx, energy.delta_time,
             to_permil(energy.delta_tx + energy.delta_rx, energy.delta_time));
  }
