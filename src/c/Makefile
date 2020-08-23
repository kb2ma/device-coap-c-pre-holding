LIBCOAP ?= libcoap-2-tinydtls
#CSDK_DIR = ../../device-sdk-c/repo/build/release/_CPack_Packages/Linux/TGZ/csdk-1.2.1

pkgconfig = $(shell pkg-config $(1) $(2))
#CFLAGS = -Wall -I$(CSDK_DIR)/include $(call pkgconfig,--cflags,$(LIBCOAP))
CFLAGS = -Wall $(call pkgconfig,--cflags,$(LIBCOAP))
#LDLIBS = -L$(CSDK_DIR)/lib -lcsdk $(call pkgconfig,--libs,$(LIBCOAP))
LDLIBS = $(call pkgconfig,--libs,$(LIBCOAP)) -lcsdk
LINK.o = $(LINK.c)

all: device-coap

device-coap: device-coap.o coap-server.o

clean:
	rm -f *.o device-coap
