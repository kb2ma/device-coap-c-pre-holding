Presently `device-coap-c` is a proof of concept. Below are ideas for future development, more or less in priority order.

* Define security parameters in the EdgeX security microservice rather than command line
* Use OpenSSL for TLS library rather than tinydtls; OpenSSL available in Alpine v3.11 Docker base image, so don't have to build it
* Support for more data types, as `device-rest-go` does
* Support a DTLS pre-shared key per client; presently supports a single key shared by all clients
* Define libcoap ports in device config

