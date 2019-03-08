import time
import urllib.request
import asyncio
from bs4 import BeautifulSoup
import datetime

from influxdb import InfluxDBClient
from aiocoap import *

server = '2801:1e:4007:c0da:212:4b00:60d:97ea'
border_router = '2801:1e:4007:c0da:212:4b00:613:f6d'
# uri = '.well-known/core'
uri = 'sensors/temperature'
IDB_URL = 'http://testbed:8086/write?db=iot_tst&precision=s'

def post_data(device,region,temp):
  t = time.time()
  ts = int(t) - (int(t) % 60)
  json_body = [
          {
              "measurement":"temperatura",
              "tags":{
                  "device": device,
                  "region": region
                  },
              "timestamp":datetime.datetime.utcnow().isoformat(),
              "fields":{
                  "temp_raw": temp,
                  "temp_celsius": temp/100
                  }
              }
          ]
  print("[INFO] Write point: {0}".format(json_body))
  client.write_points(json_body)

def get_nodes():
    nodes = []
    response = urllib.request.urlopen('http://['+border_router+']/')
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

# conexion a influxdb
client = InfluxDBClient(host='localhost', port=8086, database='iot_tst')

async def main():
    protocol = await Context.create_client_context()

    # obtener nodos de la red
    nodes = get_nodes()

    # configurar las conexiones coap hacia los nodos
    requests = []
    for node in nodes:
        requests.append(Message(code=GET, uri='coap://['+node+']/'+uri))

    # peticiones coap y consumo de datos
    while True:
       for request in requests:
            try:
                response = await protocol.request(request).response
            except Exception as e:
                print('[ERROR] Recurso no obtenido')
                print(e)
            else:
                temperatura = int(response.payload)
                post_data(requests.index(request),'Mendoza', temperatura)

if __name__ == "__main__":
    asyncio.get_event_loop().run_until_complete(main())
     #asyncio.get_event_loop().run_forever()
