import logging
import asyncio

from aiocoap import *

logging.basicConfig(level=logging.INFO)
server = '2801:1e:4007:c0da:212:4b00:60d:97ea'
# uri = '.well-known/core'
uri = 'sensors/temperature'
IDB_URL = 'http://testbed:8086/write?db=iot_tst&precision=s'

async def main():
    protocol = await Context.create_client_context()

    request = Message(code=GET, uri='coap://['+server+']/'+uri)
    print("Trying: "+ 'coap://['+server+']/'+uri)
    try:
        response = await protocol.request(request).response
    except Exception as e:
        print('Failed to fetch resource:')
        print(e)
    else:
        print('Result: %s\n%r'%(response.code, response.payload))
        temperatura = int(response.payload)
        print('Temperatura : %d'%(temperatura))

if __name__ == "__main__":
    asyncio.get_event_loop().run_until_complete(main())
