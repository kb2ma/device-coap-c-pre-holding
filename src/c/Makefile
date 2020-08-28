LIBCOAP ?= libcoap-2-tinydtls
VERSION=\"$(shell cat ../../VERSION || echo 0.0.0)\"

pkgconfig = $(shell pkg-config $(1) $(2))
CFLAGS = -Wall -DVERSION=$(VERSION) -DLIBCOAP=\"$(LIBCOAP)\" $(call pkgconfig,--cflags,$(LIBCOAP))
LDLIBS = $(call pkgconfig,--libs,$(LIBCOAP)) -lcsdk
LINK.o = $(LINK.c)

all: device-coap

device-coap: device-coap.o coap-server.o
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o device-coap
	cp device-coap ../..

clean:
	rm -f *.o device-coap ../../device-coap
