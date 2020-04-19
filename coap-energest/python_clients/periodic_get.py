#!/usr/bin/env python3
import threading
from random import randrange
# from blessed import Terminal

import logging
import asyncio
import sys, os, signal

import aiocoap
from influxdb import InfluxDBClient

from time import sleep
import urllib.request
from bs4 import BeautifulSoup
import datetime
import re

from vars import logging_file, border_router, location, path_energia, \
    path_temperatura, path_luz, path_all,  nodes

# term = Terminal()

# conexion a influxdb
idb_client = InfluxDBClient(host='localhost', port=8086, database='iot_tst')


def influx_post_data(device, region, datos):
    """ Funcion para almacenar datos en InfluxDB
        Según sea el formato del dato recibido, se almacena en una u otra
        tabla de la BD.
    """

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
    idb_client.write_points(json)


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


class Client:
    """
    Cada Client representa una conexión a un nodo único.
    Contiene las variables que representan a un nodo.
    Y las funciones necesarias para realizar solicitudes a los nodos.
    """
    def __init__(self, ipv6, interval):
        self.ipv6 = ipv6
        self.id = str(ipv6[-4::]).replace(':', '0')
        self.interval = interval
        self.resources = []

    async def _init(self):
        self.protocol = await aiocoap.Context.create_client_context()
        await self.get_resources()

    async def get_resources(self):
        """
        @brief      Obtiene via CoAP los recursos que sirve cada nodo bajo su
                    well-known
        """
        request = aiocoap.Message(code=aiocoap.Code.GET,
                                  uri='coap://[' + self.ipv6 + ']/' +
                                  '.well-known/core')

        response = await self.protocol.request(request).response

        cadena = str(response.payload)
        coleccion = cadena.split(',')
        coleccion = re.findall('\<[\w\-\/]+\>', cadena)
        print(f"Nodo {self.id} tiene disponibles los recursos {coleccion}")
        for ele in coleccion:
            self.resources.append(ele[1:-1])

    async def format_requests(self):
        """
        @brief      Crea las peticiones N CoAP. Donde N = nodos*recursos de cada nodo.
                    Ademas postea los datos a influxdb.

        """
        requests = [(aiocoap.Message(code=aiocoap.Code.GET,
                                     uri='coap://[' + self.ipv6 + ']' + path))
                    for path in self.resources]
        for request in requests:
            try:
                response = await self.protocol.request(request).response
            except Exception as e:
                print('Failed to fetch resource:')
                print(e)
            else:
                datos = str(response.payload).split("'")[1]
                datos = datos.split(";")
                if (len(datos) not in [1, 7]):
                    raise Exception(
                        f"[FATAL] El dato obtenido no tiene el formato requerido: {datos}"
                    )
                else:
                    # with term.location(0, term.height - 1):
                    # logging.info(term.bold(self.id) + f'>>> Result: {datos}')
                    logging.info(f'{self.id}>>> Result: {datos}')
                    influx_post_data(self.id, location, datos)
                logging.debug(f'{self.id} >>> Result: {datos}')

    async def send_requests(self):
        """
        @brief      Loop infinito que envía las peticiones CoAP y luego espera
                    self.interval segundos.
                    Con una probabilidad de 1% se actualiza la lista de nodos.
        """
        while True:
            try:
                logging.debug(f"Enviando petición desde cliente {self.id}")
                await self.format_requests()
                await asyncio.sleep(self.interval)
                # if randrange(0, 100) <= 1:
                #     get_nodes()
            except Exception as e:
                print(f"send_request except:{e}")

class Context_Manager():
    pass


def signal_handler(signal, frame):
    logging.warning('Interrupción de teclado. Finalizando.')
    asyncio.gather(*asyncio.all_tasks()).cancel()
    sys.exit(0)


async def main():

    # descubre los nodos que conoce el border router
    get_nodes()

    if not bool(nodes):
        logging.warning('No se han encontrado nodos. Terminando ...')
        sys.exit(0)

    clients = []
    # Se instancia la clase Client, uno por cada nodo
    for node in nodes:
        clients.append(Client(node, 10))

    tasks = []
    for client in clients:
        await client._init()
        tasks.append(client.send_requests())

    res = await asyncio.gather(*tasks)


if __name__ == "__main__":
    import pathlib
    import sys

    # algunas funciones asíncronas sólo están disponibles a partir de
    # python3.7
    assert sys.version_info >= (3, 7), "Script requires Python 3.7+."

    signal.signal(signal.SIGINT, signal_handler)
    asyncio.run(main())
