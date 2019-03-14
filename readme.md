# Configuración inicial

conectado a la raspy

passwd

sudo su -

apt update

apt install openssh-server

vim /etc/dhcpcd.conf

	# Example static IP configuration:
	interface eth0
	static ip_address=192.168.23.149/24
	static ip6_address=2801:1e:4007:23::c0da:1/64 #gateway a la sub red que nos dio roberto
	#static routers=192.168.0.1
	static domain_name_servers=1.1.1.1 8.8.8.8 fd51:42f8:caae:d92e::1
	
para que rutee ipv6 agregar una entrada en /etc/sysctl.conf

	net.ipv6.conf.all.forwarding = 1

El router de roberto deberia difundir la ruta a la subred 2801:1e:4007:c0da::/64 via  2801:1e:4007:23::c0da:1 ( la raspy) 
	#cosa que NO hace .... Agregar a mano en las PC que queremos acceder a los motes la siguente ruta 

	ip ro add 2801:1e:4007:c0da::/64 via 2801:1e:4007:23::c0da:1


# Instalacion algo de soft

	wget https://github.com/fg2it/grafana-on-raspberry/releases/download/v5.1.4/grafana_5.1.4_armhf.deb
	dpkg -i grafana_5.1.4_armhf.deb
	systemctl enable grafana-server
	systemctl start grafana-server

	curl -sL https://repos.influxdata.com/influxdb.key | sudo apt-key add -
	apt get install influxdb

	apt install vim :-)
	init 6

# A partir de ahora se trabaja remotamente

	ssh pi@192.168.23.149

	sudo apt install build-essential doxygen git curl wireshark python-serial srecord default-jdk ant
	sudo apt install gcc-arm-none-eabi

	mkdir Workspace
	cd Workspace/
	git clone https://github.com/contiki-ng/contiki-ng.git

	cd contiki-ng/
	#permisos al usuario pi para grabar motes
	sudo usermod -a -G plugdev pi
	sudo usermod -a -G dialout pi

'NOTA' para compilación de motes Rev A1 

	make TARGET=openmote-cc2538  BOARD_REVISION=REV_A1 PORT=/dev/ttyUSBXX programa_a_cargar.upload

# compilacion del router de borde (conectado al USB0)

para que el border-router advierta la red publica que nos dio roberto, agregar en el Makefile la siguiente variable
caso contrario lo hace con la red fd00:: 

	PREFIX = 2801:1e:4007:c0da::1/64 #esa es la ip de la interfase tun0 de la raspy

	cd rpl-border-router/
	make distclean
	make TARGET=openmote-cc2538 
	make TARGET=openmote-cc2538 PORT=/dev/ttyUSB0 border-router.upload
	cd ..

En nuestro caso la ip del router de borde es 2801:1e:4007:c0da:212:4b00:613:f6d/64

# compilacion de hello word en motes conectados (al USB1,2 y 3
	cd hello-world/
	make TARGET=openmote-cc2538 PORT=/dev/ttyUSB(1,2 o 3) hello-world.upload
	cd ..

# compilacion de coap en motes(motes conectados al USB1,2 y 3
	cd apps/coap-example-server 
	make TARGET=openmote-cc2538 PORT=/dev/ttyUSB(1,2 o 3) coap-example-server.upload


# en nuestro caso la ip de los otros motes es
	2801:1e:4007:c0da:212:4b00:60d:97ea
	2801:1e:4007:c0da:212:4b00:430:5381
	2801:1e:4007:c0da:212:4b00:615:a4e1


# Conexión al border router 
Hay un tmux corriendo 

	tmux -l  ( una sesion corre el connect-router y la otra la app de python que consume coap y escribe en la influxdb
	make TARGET=openmote-cc2538 PORT=/dev/ttyUSB0 connect-router

TODO:

