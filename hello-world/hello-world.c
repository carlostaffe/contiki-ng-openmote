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

// librerias con el HAL para openmote
#include "adxl346.h"
#include "dev/button-hal.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/max44009.h"
#include "dev/sht21.h"
#include "lib/sensors.h"

#include "project-conf.h"
#include "sys/rtimer.h"

/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
static struct rtimer timer_rtimer;

static int16_t accel, light, temperature, humidity;
static uint16_t adxl346_present, sht21_present, max44009_present;

void rtimer_callback(struct rtimer *timer, void *ptr) {
  /* Re-arm rtimer */
  rtimer_set(&timer_rtimer, RTIMER_NOW() + RTIMER_SECOND / 2, 0,
             rtimer_callback, NULL);
}

/* funcion que inicializa los sensores si estan disponibles */
static int sensors_init() {

  /* Initialize and calibrate the ADXL346 sensor */
  adxl346_present = SENSORS_ACTIVATE(adxl346);
  if (adxl346_present == ADXL346_ERROR) {
  } else {
    adxl346.configure(ADXL346_CALIB_OFFSET, 0);
  }

  /* Initialize the MAX44009 sensor */
  max44009_present = SENSORS_ACTIVATE(max44009);
  if (max44009_present == MAX44009_ERROR) {
  }

  /* Initialize the SHT21 sensor */
  sht21_present = SENSORS_ACTIVATE(sht21);
  if (sht21_present == SHT21_ERROR) {
  }
  return 0;
}

/* funcion que lee los sensores e imprime el valor */
static int sensors_read() {
  if (adxl346_present != ADXL346_ERROR) {
    accel = adxl346.value(ADXL346_READ_X_mG);
    accel = adxl346.value(ADXL346_READ_Y_mG);
    accel = adxl346.value(ADXL346_READ_Z_mG);
  }

  if (max44009_present != MAX44009_ERROR) {
    light = max44009.value(MAX44009_READ_LIGHT);
  }

  if (sht21_present != SHT21_ERROR) {
    temperature = sht21.value(SHT21_READ_TEMP);
    humidity = sht21.value(SHT21_READ_RHUM);
  }
  return 0;
}

static struct etimer timer;
/* funcion que inicializa los sensores si estan disponibles */
PROCESS_THREAD(hello_world_process, ev, data) {
  leds_off(LEDS_ALL);
  sensors_init();

  PROCESS_BEGIN();

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 1);
  rtimer_set(&timer_rtimer, RTIMER_NOW() + RTIMER_SECOND / 2, 0,
             rtimer_callback, NULL);

  while (1) {

    /* Wait for the periodic timer to expire and then restart the timer. */
    if (etimer_expired(&timer)) {

      for (int j = 0; j < 1500; j++) {
        sensors_read();
        leds_on(LEDS_YELLOW);
      }
      leds_off(LEDS_YELLOW);
      etimer_reset(&timer);
    }

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
