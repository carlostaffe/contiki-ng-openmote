import logging
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


# lista de nodos en la red segun el coordinador
nodes = []

interval = 10
