#!/usr/bin/env python3

# This file is part of the Python aiocoap library project.
#
# Copyright (c) 2012-2014 Maciej Wasilak <http://sixpinetrees.blogspot.com/>,
#               2013-2014 Christian Amsüss <c.amsuess@energyharvesting.at>
#
# aiocoap is free software, this file is published under the MIT license as
# described in the accompanying LICENSE file.

#!/usr/bin/env python3

import logging
import asyncio
import sys, os

import aiocoap
from influxdb import InfluxDBClient

# import time
import urllib.request
from bs4 import BeautifulSoup
import datetime

logging.basicConfig(level=logging.INFO)

# router de borde al cual se le pregunta que nodos conoce
border_router = '2801:1e:4007:c0da:212:4b00:615:a4e1'

# ubicacion de los motes, utilizado para etiquetar los datos
location = 'Mendoza'

# path a los recursos del servidor coap
path_energia = 'test/energest'
path_temperatura = 'sensors/temperature'
path_all = [uri_energia, uri_temperatura]

# conexion a influxdb
client = InfluxDBClient(host='localhost', port=8086, database='iot_tst')


def influx_post_data(device, region, datos):
    ''' Funcion para almacenar datos en InfluxDB
        Según tenga uno o más valores, el dato es almacenado como
        lectura de temperatura o energía respectivamente.
    '''

    if (len(datos) == 1):
        temp = datos[0]
        if(temp == '-1'):
            return 
        json = [{
            "measurement": "temperatura",
            "tags": {
                "device": device,
                "region": region
            },
            "timestamp": datetime.datetime.utcnow().isoformat(),
            "fields": {
                "temperatura": int(temp)
            }
        }]
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
                "rel_cpu": int(cpu)/int(time_ticks),
                "delta_lpm": int(lpm),
                "rel_lpm": int(lpm)/int(time_ticks),
                "delta_deep_lpm": int(deep_lpm),
                "rel_deep_lpm": int(deep_lpm)/int(time_ticks),
                "delta_tx": int(tx),
                "rel_tx": int(tx)/int(time_ticks),
                "delta_rx": int(rx),
                "rel_rx": int(rx)/int(time_ticks)
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


def observation_cb(m):
    ''' Se recibe un aiocoap.Message. Tiene un payload que corresponde a la
        lectura del sensor e información de la petición que permite saber de
        nodo proviene la lectura
    '''
    # breakpoint
    # import pdb; pdb.set_trace()

    datos = str(m.payload).split("'")[1]
    datos = datos.split(";")
    # chequeo que los datos obtenidos tengan la longitud adecuada
    # FIXME: en lugar de terminar, ignorar el dato?
    if (len(datos) not in [1, 7]):
        raise Exception(
            "[FATAL] El dato obtenido no tiene el formato requerido")

    logging.debug("Recibido: " + str(datos))

    # parseo los ultimos 4 caracteres de la MAC del nodo para etiquetar al dato
    device_id = str(m.remote).split(']:')[0][-4::]

    influx_post_data(device_id, location, datos)


async def send_requests(requests, nodes):
    ''' En esta función se envian las peticiones en paralelo
        Importante notar que no es posible predecir en que orden
        arriban las respuestas.
    '''

    protocol = await aiocoap.Context.create_client_context()
    # lista con objetos aiocoap.protocol.Request
    messages = [protocol.request(request) for request in requests]

    # breakpoint
    # import pdb; pdb.set_trace()

    # para cada mensaje se registra un callback (funcion que es llamada cuando
    # arriba una respuesta), la cual se encarga de guardar el dato en influxdb.
    # el primer dato se descarta por no respetar el intervalo de observacion
    for msg in messages:
        msg.observation.register_callback(observation_cb)
        r = await msg.response
        logging.info("Primera respuesta(descartada) \n%r" % (r.payload))

    # pass en este caso no significa que se descarta la respuesta, pues la
    # atrapa el callback.
    async for r in msg.observation:
        logging.debug("Proxima observacion: %s\n%r" % (r, r.payload))
        pass


def format_requests(nodes):
    ''' Crea el total de las peticiones.
        Por cada nodo, se crean tantas peticiones como elementos contenga
        la lista uri_all
        Si existen 2 nodos y 3 recursos coap, se crean 6 peticiones.
    '''

    requests = [(aiocoap.Message(
        code=aiocoap.Code.GET, uri='coap://[' + node + ']/' + path, observe=0))
                for path in path_all for node in nodes]
    logging.info("Creados %d peticiones. \n \t%d nodos * %d recursos." %
          (len(requests), len(nodes), len(path_all)))
    return requests


def main():
    # FIXME: no detecta nuevos nodos

    # obtener nodos de la red
    nodes = get_nodes()
    # crea y formatea la lista de peticiones
    requests = format_requests(nodes)

    loop = asyncio.get_event_loop()

    # es necesario catchear los Ctrl+C porque si no quedan
    # corutinas zombies
    try:
        asyncio.ensure_future(send_requests(requests, nodes))
        loop.run_forever()

    except KeyboardInterrupt:
        logging.warning('Interrupción de teclado. Finalizando.')
        try:
            sys.exit(0)
        except SystemExit:
            os._exit(0)


if __name__ == "__main__":
    main()
