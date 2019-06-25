CONTIKI_PROJECT = coap-energest
all: $(CONTIKI_PROJECT)

print-%  : ; @echo $* = $($*)
# Do not try to build on Sky because of code size limitation
PLATFORMS_EXCLUDE = sky z1

# Include the CoAP implementation
MODULES += os/net/app-layer/coap

# Include CoAP resources
MODULES_REL += ./resources

CONTIKI=../..
include $(CONTIKI)/Makefile.include

.PHONY: renode
# habilitar símbolos de depuración
CFLAGS += -ggdb

renode: all
	$(RENODE) $(SCRIPT)
