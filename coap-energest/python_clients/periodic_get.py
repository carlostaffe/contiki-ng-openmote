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
# WARNING; INFO; DEBUG
logging.basicConfig(level=logging.INFO)

# router de borde al cual se le pregunta que nodos conoce
border_router = '2801:1e:4007:c0da:212:4b00:615:a4e1'

# ubicacion de los motes, utilizado para etiquetar los datos
location = 'Mendoza'

# path a los recursos del servidor coap
path_energia = 'test/energest'
path_temperatura = 'sensors/temperature'
path_luz = 'sensors/light'
path_all = [path_energia, path_temperatura, path_luz]

# conexion a influxdb
client = InfluxDBClient(host='localhost', port=8086, database='iot_tst')


def influx_post_data(device, region, datos):
    ''' Funcion para almacenar datos en InfluxDB
        Según tenga uno o más valores, el dato es almacenado como
        lectura de temperatura o energía respectivamente.
    '''

    if (len(datos) == 1):
        datos = datos[0]
        if (datos == 'l:-1' or datos == 't:-1'):
            logging.info("Formato o valor incorrecto")
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
            logging.info("[FATAL] Dato no reconocido: {0}".format(datos))
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
    nodes = []
    response = urllib.request.urlopen('http://[' + border_router + ']/')
    html = response.read()
    parsed_html = BeautifulSoup(html, "lxml")
    li = parsed_html.body.find_all('li')
    for item in li:
        # parsea el list item del html obtenido
        node = item.contents[0].split(' (')[0]
        if (not node.startswith('fe80')):
            logging.info("Nodo encontrado:" + str(node))
            nodes.append(node)
    return nodes


async def send_requests(nodes):
    ''' En esta función se envian las peticiones en paralelo
        Importante notar que no es posible predecir en que orden
        arriban las respuestas.
    '''

    # obtengo una estructura formateada con una lista de request


    while True:
        endpoints = await format_requests(nodes)

        for i in range(len(nodes)):
            messages = endpoints['messages'][i]
            print("Mensajes: {0} \n {1}".format(len(messages), messages))

            for msg in messages:
                try:
                    r = await msg.response
                    datos = str(r.payload).split("'")[1]
                    datos = datos.split(";")
                    print(datos)

                except Exception as e:
                    logging.info('[FATAL] No se pudo obtener el recurso CoAP')
                    print(e)

                else:
                    if (len(datos) not in [1, 7]):
                        raise Exception(
                            "[FATAL] El dato obtenido no tiene el formato requerido: {0}"
                            .format(datos))

                # utilizo los ultimos 4 nros del eui64 como tag de la BD
                device_id = str(r.remote).split(']:')[0][-4::]
                device_id = device_id.replace(':', '0')

                logging.debug("[DEBUG] Obtenido {0} de {1}".format(
                    datos, device_id))
                influx_post_data(device_id, location, datos)
                await asyncio.sleep(5)  # pause 5 seconds

async def format_requests(nodes):
    ''' Crea el total de las peticiones.
        Por cada nodo, se crean tantas peticiones como elementos contenga
        la lista path_all
        Si existen 2 nodos y 3 recursos coap, se crean 6 peticiones.
    '''

    # esta estructura contiene un handler (context) para cada nodo
    # y la lista de mensajes que se enviaran en cada request
    endpoints = {'protocols': [], 'messages': []}

    for i in range(len(nodes)):
        requests = [(aiocoap.Message(code=aiocoap.Code.GET,
                                     uri='coap://[' + nodes[i] + ']/' + path))
                    for path in path_all ]
        endpoints['protocols'].append(await
                                    aiocoap.Context.create_client_context())
        endpoints['messages'].append(
            [endpoints['protocols'][i].request(request) for request in requests])

    logging.info(
        "Creadas {0} peticiones por nodo. \n \t{1} nodos * {2} recursos.".format(
            len(endpoints['messages']), len(nodes), len(path_all)))

    return endpoints


# async def main():
#     # FIXME: no detecta nuevos nodos
#     # obtener nodos de la red
#     nodes = get_nodes()

#     if not nodes:
#         logging.warning('No se han encontrado nodos. Terminando ...')
#         sys.exit(0)

#     # crea y formatea la lista de peticiones
#     requests = await format_requests(nodes)
#     # asyncio.ensure_future(format_requests(nodes)requests)

#     try:
#         # asyncio.ensure_future(send_requests(requests, nodes))
#         task = loop.create_task(send_requests(requests, nodes))
#         # loop.run_forever()
#         loop.run_until_complete(task)
#     except KeyboardInterrupt:
#         logging.warning('Interrupción de teclado. Finalizando.')
#         try:
#             sys.exit(0)
#         except SystemExit:
#             os._exit(0)


def signal_handler(signal, frame):
    logging.warning('Interrupción de teclado. Finalizando.')
    loop.stop()
    loop.close()
    sys.exit(0)


if __name__ == "__main__":
    loop = asyncio.get_event_loop()

    signal.signal(signal.SIGINT, signal_handler)

    nodes = get_nodes()

    if not nodes:
        logging.warning('No se han encontrado nodos. Terminando ...')
        sys.exit(0)

    asyncio.ensure_future(send_requests(nodes))

    loop.run_forever()
    loop.close()
#     requests = await format_requests(nodes)
# try:
#     task = loop.run_until_complete()
# except KeyboardInterrupt:
#     logging.warning('Interrupción de teclado. Finalizando.')
#     try:
#         sys.exit(0)
#     except SystemExit:
#         os._exit(0)
# else:
#     loop.close()
