CONTIKI_PROJECT = coap-energest

PREFIX = 2801:1e:4007:c0da::1/64 #esa es la ip de la interfase tun0 de la raspy

MAKE_MAC = MAKE_MAC_TSCH
# MODULES += os/services/shell

all: $(CONTIKI_PROJECT)

# Do not try to build on Sky because of code size limitation
PLATFORMS_EXCLUDE = sky z1

# Include the CoAP implementation
MODULES += os/net/app-layer/coap

# Include CoAP resources
MODULES_REL += ./resources

CONTIKI=../..
include $(CONTIKI)/Makefile.include


# # habilitar símbolos de depuración
# CFLAGS += -ggdb

# renode: all
# 	$(RENODE) $(SCRIPT)
