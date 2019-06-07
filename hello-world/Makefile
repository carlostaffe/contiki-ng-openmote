CONTIKI_PROJECT = hello-world
all: $(CONTIKI_PROJECT)

CONTIKI = ../..
include $(CONTIKI)/Makefile.include

# Include RPL BR module
# MODULES += os/services/rpl-border-router

CFLAGS += -Wall
ifeq ($(HOST_OS),Darwin)
  RENODE = mono /Applications/Renode.app/Contents/MacOS/bin/Renode.exe
else
  RENODE = renode
endif

SCRIPT ?= $(notdir $(CURDIR)).resc

.PHONY: renode
# habilitar símbolos de depuración
CFLAGS += -g

renode: all
	$(RENODE) $(SCRIPT)
