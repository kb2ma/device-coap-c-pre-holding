Presently `device-coap-c` is a proof of concept. Below are ideas for future development.

* Support for CMake build
* Support for building and running in a container
* Support for more data types, as `device-rest-go` does
* Define security parameters in the EdgeX security microservice rather than command line
* Define libcoap host and ports in device config
* Support a DTLS pre-shared key per client; presently supports a single key shared by all clients

