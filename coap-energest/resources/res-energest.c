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

static void simple_energest_step(void);

static unsigned long to_permil(unsigned long delta_metric, unsigned long delta_time);

  /* A simple getter example. */
  RESOURCE(res_energest,
           "title=\"Energest Vals (supports JSON)\";rt=\"Energest\"",
           res_get_handler, NULL, NULL, NULL);

  static void res_get_handler(coap_message_t * request,
                              coap_message_t * response, uint8_t * buffer,
                              uint16_t preferred_size, int32_t * offset) {
    /* uint16_t light_solar = light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR); */
    // FIXME : Crear estructura si es que no sirve la que esta global
    simple_energest_step();

    unsigned int accept = -1;
    coap_get_header_accept(request, &accept);

    /* LOG_INFO("--- Period summary #%u (%lu seconds)\n", count++, */
             /* energy.delta_time / ENERGEST_SECOND); */
    /* LOG_INFO("Total time  : %10lu\n", energy.delta_time); */
    /* LOG_INFO("CPU         : %10lu/%10lu (%lu permil)\n", energy.delta_cpu, */
             /* energy.delta_time, to_permil(energy.delta_cpu, energy.delta_time)); */
    /* LOG_INFO("LPM         : %10lu/%10lu (%lu permil)\n", energy.delta_lpm, */
    /*          delta_time, to_permil(delta_lpm, delta_time)); */
    /* LOG_INFO("Deep LPM    : %10lu/%10lu (%lu permil)\n", */
    /* energy.delta_deep_lpm, */
             /* delta_time, to_permil(delta_deep_lpm, delta_time)); */
    /* LOG_INFO("Radio Tx    : %10lu/%10lu (%lu permil)\n", energy.delta_tx, */
             /* delta_time, to_permil(delta_tx, delta_time)); */
    /* LOG_INFO("Radio Rx    : %10lu/%10lu (%lu permil)\n", energy.delta_rx, */
             /* delta_time, to_permil(delta_rx, delta_time)); */
    /* LOG_INFO("Radio total : %10lu/%10lu (%lu permil)\n", */
             /* energy.delta_tx + delta_rx, delta_time, */
             /* to_permil(delta_tx + delta_rx, delta_time)); */

    if (accept == -1 || accept == TEXT_PLAIN) {
      coap_set_header_content_format(response, TEXT_PLAIN);
      snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "%lu;%lu;%lu;%lu;%lu;%lu;%lu;%lu;%lu;%lu;%lu;%lu;%lu;%lu;%lu;%lu;%lu;%lu;%lu;%lu",
               energy.delta_time / ENERGEST_SECOND, energy.delta_time,
               energy.delta_cpu, energy.delta_time,
               to_permil(energy.delta_cpu, energy.delta_time), energy.delta_lpm,
               energy.delta_time, to_permil(energy.delta_lpm, energy.delta_time),
               energy.delta_deep_lpm, energy.delta_time,
               to_permil(energy.delta_deep_lpm, energy.delta_time), energy.delta_tx,
               energy.delta_time, to_permil(energy.delta_tx, energy.delta_time),
               energy.delta_rx, energy.delta_time,
               to_permil(energy.delta_rx, energy.delta_time), energy.delta_tx + energy.delta_rx,
               energy.delta_time, to_permil(energy.delta_tx + energy.delta_rx, energy.delta_time));

      coap_set_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
    } else if (accept == APPLICATION_XML) {
      coap_set_header_content_format(response, APPLICATION_XML);
      snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE,
               "<energest delta_time=\"%lu\" solar=\"%lu\"/>",
               energy.delta_time, energy.delta_tx);

      coap_set_payload(response, buffer, strlen((char *)buffer));
    } else if (accept == APPLICATION_JSON) {
      coap_set_header_content_format(response, APPLICATION_JSON);
      snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE,
               "{'energest':{'delta_time':%lu,'delta':%lu}}", energy.delta_time,
               energy.delta_tx);

      coap_set_payload(response, buffer, strlen((char *)buffer));
    } else {
      coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
      const char *msg = "Supporting content-types text/plain, application/xml, "
                        "and application/json";
      coap_set_payload(response, msg, strlen(msg));
    }
  }

  /* struct energest_t energy; */
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
