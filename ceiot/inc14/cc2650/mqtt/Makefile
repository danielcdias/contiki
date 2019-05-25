all: projeto-final-mqtt

WITH_UIP6=1
UIP_CONF_IPV6=1
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
CONTIKI = ../../../..
CONTIKI_WITH_IPV6 = 1
CFLAGS += -DUIP_CONF_ND6_SEND_NS=1
CFLAGS += -DUIP_CONF_IPV6_RPL
CFLAGS += -g
DEBUG = 1
APPS += powertrace
PROJECT_SOURCEFILES += mqtt-sn.c cetic-6lbr-client.c 
PROJECT_SOURCEFILES += ../../../../../core/net/llsec/noncoresec/noncoresec.c
include $(CONTIKI)/Makefile.include
