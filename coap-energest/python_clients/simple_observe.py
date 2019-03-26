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

# conexion a influxdb
client = InfluxDBClient(host='localhost', port=8086, database='iot_tst')


def influx_post_data(device, region, temp, time, cpu, lpm, deep_lpm, tx, rx):

    json_energy = [{
        "measurement": "energia",
        "tags": {
            "device": device,
            "region": region
        },
        "timestamp": datetime.datetime.utcnow().isoformat(),
        "fields": {
            "delta_time": time,
            "delta_cpu": cpu,
            "delta_lpm": lpm,
            "delta_deep_lpm": deep_lpm,
            "delta_tx": tx,
            "delta_rx": rx
        }
    }]
    json_temp = [{
        "measurement": "temperatura",
        "tags": {
            "device": device,
            "region": region
        },
        "timestamp": datetime.datetime.utcnow().isoformat(),
        "fields": {
            "temperatura": temp
        }
    }]
    print("[INFO] Write point: {0}".format(json_temp))
    print("[INFO] Write point: {0}".format(json_energy))
    client.write_points(json_temp)
    client.write_points(json_energy)


def get_nodes():
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


async def coap_get(nodes):
    protocol = await aiocoap.Context.create_client_context()

    requests = []
    for node in nodes:
        # request para el recurso energia. devuelve 7 datos
        requests.append(
            aiocoap.Message(
                code=aiocoap.Code.GET,
                uri='coap://[' + node + ']/' + uri_energia))
        # request para el recurso temperatura, devuelve 1 dato
        requests.append(
            aiocoap.Message(
                code=aiocoap.Code.GET,
                uri='coap://[' + node + ']/' + uri_temperatura))

    for request in requests:
        data = []
        try:
            response = await protocol.request(request).response
        except Exception as e:
            print('Failed to fetch resource:')
            print(e)
        else:
            print('Result: %s\n%r' % (response.code, response.payload))
            vals = str(response.payload).split("'")[1]
            vals = vals.split(";")
            print("Vals: "+vals)
            for val in vals:
                if (val == "30"):
                    print("[INFO] Nuevo delta_time" + vals)
                else:
                    data.append(int(val))

        # solo al cabo de dos requests, enviar los datos a influxdb
        # primer request: energia; segundo request: temperatura
        if (not requests.index(request) % 2):
            print("Data: " + str(data))
            # ultimos 4 caracteres de la MAC como node_id
            node_id = nodes[int(
                (requests.index(request) + 1) / 2)].split(":")[-1]
            # influx_post_data(device, region, temp, time, cpu, lpm, deep_lpm, tx, rx)
            influx_post_data(node_id, 'Mendoza', data[0], data[1], data[2],
                             data[3], data[4], data[5], data[6])
            data = []


async def coap_observe(nodes):
    protocol = await aiocoap.Context.create_client_context()

    requests = []
    for node in nodes:
        requests.append(
            aiocoap.Message(
                code=aiocoap.Code.GET,
                uri='coap://[' + node + ']/' + uri_energia,
                observe=0))
        # request = aiocoap.Message(code=aiocoap.Code.GET, uri=uri, observe=0)

    for request in requests:
        pr = protocol.request(request)
        try:
            response = await pr.response
        except Exception as e:
            print('Failed to fetch resource:')
            print(e)
        else:
            print('Result: %s\n%r' % (response.code, response.payload))


def main():
    # asyncio.get_event_loop().run_until_complete(coap_get('coap://localhost/block'))
    # obtener nodos de la red
    nodes = get_nodes()
    asyncio.get_event_loop().run_until_complete(coap_observe(nodes))
    asyncio.get_event_loop().run_until_complete(coap_get(nodes))


if __name__ == "__main__":
    main()
