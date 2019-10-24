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
 *      Example of an observable "on-change" temperature resource
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 * \author
 *      Cristiano De Alti <cristiano_dealti@hotmail.com>
 */

#include "contiki.h"

#if PLATFORM_HAS_TEMPERATURE

#include "coap-engine.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// begin - agregado
#include "dev/sht21.h"
#include "lib/sensors.h"
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP
// end - agregado

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size,
                            int32_t *offset);
static void res_periodic_handler(void);

#define MAX_AGE 60
#define INTERVAL_MIN 5
#define INTERVAL_MAX (MAX_AGE - 1)
#define CHANGE 0.1

static int32_t interval_counter = INTERVAL_MIN;
static int32_t temperature_old = INT_MIN;

static int32_t temperature = 0;
static int32_t humidity = 0;

PERIODIC_RESOURCE(res_temperature,
                  "title=\"Temperature\";rt=\"Temperature\";obs",
                  res_get_handler, NULL, NULL, NULL, 1000,
                  res_periodic_handler);

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size,
                            int32_t *offset) {
  temperature = sht21.value(SHT21_READ_TEMP);
  humidity = sht21.value(SHT21_READ_RHUM);

  LOG_INFO("Valor Temperatura:\t%ld\n", temperature);
  LOG_INFO("Valor Humedad:\t%ld\n", humidity);


  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);

  if (accept == -1 || accept == TEXT_PLAIN) {
    coap_set_header_content_format(response, TEXT_PLAIN);
    snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "t:%ld", temperature);
    coap_set_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  }

  coap_set_header_max_age(response, MAX_AGE);
}

/*
 * Additionally, a handler function named [resource name]_handler must be
 * implemented for each PERIODIC_RESOURCE. It will be called by the coap_manager
 * process with the defined period.
 */
static void res_periodic_handler() {
  /* int temperature = temperature_sensor.value(0); */
  int temperature = sht21.value(SHT21_READ_TEMP);
  ++interval_counter;

  if ((abs(temperature - temperature_old) >= CHANGE &&
       interval_counter >= INTERVAL_MIN) ||
      interval_counter >= INTERVAL_MAX) {
    interval_counter = 0;
    temperature_old = temperature;
    /* Notify the registered observers which will trigger the res_get_handler to
     * create the response. */
    coap_notify_observers(&res_temperature);
  }
}
#endif /* PLATFORM_HAS_TEMPERATURE */
