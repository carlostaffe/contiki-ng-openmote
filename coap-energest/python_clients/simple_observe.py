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
border_router = '2801:1e:4007:c0da:212:4b00:613:f6d'

uri_energia = 'test/energest'
uri_temperatura = 'sensors/temperature'
uri_all = [uri_energia, uri_temperatura]

# conexion a influxdb
client = InfluxDBClient(host='localhost', port=8086, database='iot_tst')


def influx_post_data(device, region, datos):
    ''' Funcion para almacenar datos en InfluxDB
        Según tenga uno o más valores, el dato es almacenado como
        lectura de temperatura o energía respectivamente.
    '''

    if (len(datos) == 1):
        temp = datos[0]
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
                "delta_lpm": int(lpm),
                "delta_deep_lpm": int(deep_lpm),
                "delta_tx": int(tx),
                "delta_rx": int(rx)
            }
        }]

    # imprimir el dato y guardarlo en InfluxDB
    print("[INFO] Write point: {0}".format(json))
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
            print("[INFO] Nodo encontrado:" + str(node))
            nodes.append(node)
    return nodes


def observation_cb(m):
    ''' Se recibe un aiocoap.Message. Tiene un payload que corresponde a la
        lectura del sensor e información de la petición que permite saber de
        nodo proviene la lectura
    '''
    # breakpoint
    # import pdb; pdb.set_trace()

    # print("[INFO] Mensaje recibido:  %s %s" % (str(m.requested_hostinfo), m.payload))

    datos = str(m.payload).split("'")[1]
    datos = datos.split(";")
    # chequeo que los datos obtenidos tengan la longitud adecuada
    # FIXME: en lugar de terminar, ignorar el dato?
    if (len(datos) not in [1, 7]):
        raise Exception(
            "[FATAL] El dato obtenido no tiene el formato requerido")

    print("[INFO] Recibido: " + str(datos))

    # parseo los ultimos 4 caracteres de la MAC del nodo para etiquetar al dato
    device_id = str(m.remote).split(']:')[0][-4::]

    influx_post_data(device_id, 'Mendoza', datos)
    # datos[device_id].append(vals)
    # print("DATOSPLANOS: " + str(datos[node_id]))
    # pr.observation.cancel()
    # # llegado este punto el diccionario datos tendra una coleccion del tipo
    # datos[node] = [datos_energest, datos_temperatura]
    # aplano la lista para que quede todo en una misma direccion
    # for node in nodes:
    #     node_id = node[-4::]
    #     datos[node_id] = [
    #         item for sublist in datos[node_id] for item in sublist
    #     ]
    #     print("DATOSPLANOS: " + str(datos[node_id]))
    #     # influx_post_data(node_id, 'Mendoza', datos[node_id])
    #     datos[node_id] = []
    # pr.observation.cancel()


async def send_requests(requests, nodes):
    # en este loop se envia cada request a cada nodo notar que como las
    # respuestas son asincronas  no se puede predecir en que orden arriban

    # datos = {}
    # node_id = str(node[-4::])
    # datos[node_id] = []
    # print("[INFO] Control send_request del nodo " + str(node))
    # NOTE: Necesito hacer varias peticiones en simultaneo

    protocol = await aiocoap.Context.create_client_context()
    # lista con objetos aiocoap.protocol.Request
    messages = [protocol.request(request) for request in requests]
    # lista de futuros
    # una vez esperado el futuro se transforma en un aiocoap.Message
    # responses = [protocol.request(request).response for request in requests]

    # breakpoint
    # import pdb; pdb.set_trace()

    for msg in messages:
        msg.observation.register_callback(observation_cb)
        r = await msg.response
        print("[INFO]: Primera respuesta(descartada) \n%r" % (r.payload))

    async for r in msg.observation:
        print("Next result: %s\n%r"%(r, r.payload))
        # msg.observation.cancel()

        # se obtiene el id del mote a parti de la URL del request

        # esto permite etiquetar los datos en el diccionario datos
        # device_id = response.requested_hostinfo[-5:-1]
        # datos[device_id].append(vals)
        # print("DATOSPLANOS: " + str(datos[node_id]))
        # pr.observation.cancel()
        # # llegado este punto el diccionario datos tendra una coleccion del tipo
        # datos[node] = [datos_energest, datos_temperatura]
        # aplano la lista para que quede todo en una misma direccion
        # for node in nodes:
        #     node_id = node[-4::]
        #     datos[node_id] = [
        #         item for sublist in datos[node_id] for item in sublist
        #     ]
        #     print("DATOSPLANOS: " + str(datos[node_id]))
        #     # influx_post_data(node_id, 'Mendoza', datos[node_id])
        #     datos[node_id] = []
        # pr.observation.cancel()


def format_requests(nodes):
    ''' Crea el total de las peticiones.
        Por cada nodo, se crean tantas peticiones como elementos contenga
        la lista uri_all
        Si existen 2 nodos y 3 recursos coap, se crean 6 peticiones.
    '''

    requests = [(aiocoap.Message(
        code=aiocoap.Code.GET, uri='coap://[' + node + ']/' + uri, observe=0))
                for uri in uri_all for node in nodes]
    print("[INFO] Creados %d peticiones. \n \t%d nodos * %d recursos." %
          (len(requests), len(nodes), len(uri_all)))
    return requests


def main():
    # obtener nodos de la red
    # FIXME: no detecta nuevos nodos
    nodes = get_nodes()
    requests = format_requests(nodes)

    loop = asyncio.get_event_loop()

    # es necesario catchear los Ctrl+C porque si no quedan
    # corutinas zombies
    try:
        # obtengo la lista de peticiones para cada nodo
        # request = format_requests(node)
        asyncio.ensure_future(send_requests(requests, nodes))
        loop.run_forever()
        # asyncio.get_event_loop().run_until_complete(
        # send_requests(requests, nodes))

    except KeyboardInterrupt:
        print('[INFO] Interrupción de teclado. Finalizando.')
        try:
            sys.exit(0)
        except SystemExit:
            os._exit(0)


if __name__ == "__main__":
    main()
