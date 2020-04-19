#!/usr/bin/env python3

# This file is part of the Python aiocoap library project.
#
# Copyright (c) 2012-2014 Maciej Wasilak <http://sixpinetrees.blogspot.com/>,
#               2013-2014 Christian Amsüss <c.amsuess@energyharvesting.at>
#
# aiocoap is free software, this file is published under the MIT license as
# described in the accompanying LICENSE file.

#!/usr/bin/env python3
import threading
from random import randrange

import logging
import asyncio
import sys, os, signal

import aiocoap
from influxdb import InfluxDBClient

from time import sleep
import urllib.request
from bs4 import BeautifulSoup
import datetime

# en orden decreciente de verbosidad
# CRITICAL; ERROR; WARNING; INFO; DEBUG
logging.basicConfig(level=logging.INFO)

logging_file = logging.FileHandler('periodic_get.log')
logging_file.setLevel(logging.INFO)

# router de borde al cual se le pregunta que nodos conoce
border_router = '2801:1e:4007:c0da:212:4b00:615:a4e1'

# ubicacion de los motes, utilizado para etiquetar los datos
location = 'Mendoza'

# path a los recursos del servidor coap
# FIXME: Interrogar a los motes para descubrir qué recursos encuentran disponibles
path_energia = 'test/energest'
path_temperatura = 'sensors/temperature'
path_luz = 'sensors/light'

path_all = [path_energia, path_temperatura, path_luz]

# conexion a influxdb
client = InfluxDBClient(host='localhost', port=8086, database='iot_tst')

# lista de nodos en la red segun el coordinador
nodes = []

# estructura con un contexto y mensajes para cada endpoint
endpoints = {'protocols': [], 'messages': []}

# requests
requests = []

def influx_post_data(device, region, datos):
    ''' Funcion para almacenar datos en InfluxDB
        Según tenga uno o más valores, el dato es almacenado como
        lectura de temperatura o energía respectivamente.
    '''

    if (len(datos) == 1):
        datos = datos[0]
        if (datos == 'l:-1' or datos == 't:-1'):
            logging.warning("Formato o valor incorrecto")
            return
        if (datos.startswith('l:')):
            json = [{
                "measurement": "luz",
                "tags": {
                    "device": device,
                    "region": region
                },
                "timestamp": datetime.datetime.utcnow().isoformat(),
                "fields": {
                    "luz": int(datos.split(':')[1])
                }
            }]
        elif (datos.startswith('t:')):
            json = [{
                "measurement": "temperatura",
                "tags": {
                    "device": device,
                    "region": region
                },
                "timestamp": datetime.datetime.utcnow().isoformat(),
                "fields": {
                    "temperatura": int(datos.split(':')[1])
                }
            }]
        else:
            logging.error("[FATAL] Dato no reconocido: {0}".format(datos))
            return
    else:
        time, time_ticks, cpu, lpm, deep_lpm, tx, rx = datos

        json = [{
            "measurement": "energia",
            "tags": {
                "device": device,
                "region": region
            },
            "timestamp": datetime.datetime.utcnow().isoformat(),
            "fields": {
                "delta_time": int(time_ticks),
                "delta_cpu": int(cpu),
                "rel_cpu": int(cpu) / int(time_ticks),
                "delta_lpm": int(lpm),
                "rel_lpm": int(lpm) / int(time_ticks),
                "delta_deep_lpm": int(deep_lpm),
                "rel_deep_lpm": int(deep_lpm) / int(time_ticks),
                "delta_tx": int(tx),
                "rel_tx": int(tx) / int(time_ticks),
                "delta_rx": int(rx),
                "rel_rx": int(rx) / int(time_ticks)
            }
        }]

    # imprimir el dato y guardarlo en InfluxDB
    logging.debug("Write point: {0}".format(json))
    client.write_points(json)


def get_nodes():
    ''' Funcion que realiza una peticion al router de borde para que éste
        regrese los nodos que tiene al alcance.
        TODO: Esto se va a romper cuando existan nodos con rank>1
    '''
    response = urllib.request.urlopen('http://[' + border_router + ']/')
    html = response.read()
    parsed_html = BeautifulSoup(html, "lxml")
    li = parsed_html.body.find_all('li')
    logging.info("[INFO] Nodos accesibles :")
    for item in li:
        # parsea el list item del html obtenido
        node = item.contents[0].split(' (')[0]
        if (not node.startswith('fe80')):
            logging.info("\t" + str(node))
            nodes.append(node)


async def send_requests():
    ''' En esta función se envian las peticiones en paralelo
        Importante notar que no es posible predecir en que orden
        arriban las respuestas.
    '''

    while True:
        # los mensajes son consumibles, con lo cual es necesario
        # recrearlos en cada iteracion

        print("Requests: {0}".format( requests ))

        for i in range(len(endpoints['protocols'])):
            messages = endpoints['messages'][i]
            print("Mensajes: {0} ".format(len(messages)))

            for msg in messages:
                try:
                    r = await msg.response
                    datos = str(r.payload).split("'")[1]
                    datos = datos.split(";")

                except Exception as e:
                    logging.error('[FATAL] No se pudo obtener el recurso CoAP')
                    print(e)

                else:
                    if (len(datos) not in [1, 7]):
                        raise Exception(
                            "[FATAL] El dato obtenido no tiene el formato requerido: {0}"
                            .format(datos))

                # utilizo los ultimos 4 nros del eui64 como tag de la BD
                device_id = str(r.remote).split(']:')[0][-4::]
                device_id = device_id.replace(':', '0')

                logging.info("[INFO] Obtenido {0} de {1}".format(
                    datos, device_id))
                influx_post_data(device_id, location, datos)
                await asyncio.sleep(5)  # pause 5 seconds

                # actualizo la lista de nodos con probabilidad del 5%
                if randrange(0, 100) < 5:
                    del nodes[:]
                    get_nodes()
                    init_requests()
                    logging.info("Creadas {0} peticiones".format(
                        len(endpoints['messages']) * len(nodes)))

        for context in endpoints['protocols']:
            endpoints['messages'].append(
                [context.request(request) for request in requests])



async def init_requests():
    ''' Inicializa los objetos Context (representa un endpoint) y
        requests (representa una solicitud al endpoint)
        A diferencia del objeto messages, los dos primeros no son
        consumibles
    '''

    for i in range(len(nodes)):
        global requests = [(aiocoap.Message(code=aiocoap.Code.GET,
                                     uri='coap://[' + nodes[i] + ']/' + path))
                    for path in path_all]

        endpoints['protocols'].append(await
                                      aiocoap.Context.create_client_context())

        for context in endpoints['protocols']:
            endpoints['messages'].append(
                [context.request(request) for request in requests])

        logging.debug("Inicializado request para {0} ".format(nodes[i]))


def signal_handler(signal, frame):
    logging.warning('Interrupción de teclado. Finalizando.')
    loop.run_until_complete(loop.shutdown_asyncgens())
    loop.stop()
    loop.close()
    sys.exit(0)


if __name__ == "__main__":
    loop = asyncio.get_event_loop()

    signal.signal(signal.SIGINT, signal_handler)

    get_nodes()

    if not nodes:
        logging.warning('No se han encontrado nodos. Terminando ...')
        sys.exit(0)

    loop.run_until_complete(init_requests())
    asyncio.ensure_future(send_requests())

    loop.run_forever()
    loop.close()
