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
 *      Erbium (Er) example project configuration.
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

<<<<<<< HEAD
// CONFIGURACION ENERGIA
#define ENERGEST_CONF_ON 1
#define LPM_CONF_STATS 1
#define LPM_CONF_MAX_PM 2
#define ENERGEST_CONF_ON 1
// deshabilitar para habilitar PM1+
// #define USB_SERIAL_CONF_ENABLE 0
// #define UART_CONF_ENABLE 0
//#define CC2538_CONF_QUIET 1

// CONFIGURACION LOG LEVEL
=======

//CONFIGURACION ENERGIA
#define ENERGEST_CONF_ON 1
#define LPM_CONF_STATS 1
#define LPM_CONF_MAX_PM  2

// deshabilito perifericos para ahorrar energia
// #define USB_SERIAL_CONF_ENABLE 0
// #define UART_CONF_ENABLE 0
//#define CC2538_CONF_QUIET 1


#define LOG_CONF_LEVEL_MAIN  LOG_LEVEL_DBG
>>>>>>> cf746dbfa8185742048002fbf631cdc98cc8c6dd
#define LOG_LEVEL_APP LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_COAP LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_MAIN LOG_LEVEL_DBG

#define PLATFORM_HAS_TEMPERATURE 1
typedef struct {
	unsigned long last_tx, last_rx, last_time, last_cpu, last_lpm,
	         last_deep_lpm;
	unsigned long delta_tx, delta_rx, delta_time, delta_cpu, delta_lpm,
			             delta_deep_lpm;
	unsigned long curr_tx, curr_rx, curr_time, curr_cpu, curr_lpm,
				       curr_deep_lpm;

} energest_t;

extern energest_t energy;
#endif /* PROJECT_CONF_H_ */
