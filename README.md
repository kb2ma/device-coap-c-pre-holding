# device-coap-c

EdgeX device service for CoAP-based REST protocol

This device service allows a 3rd party sensor application to push data into EdgeX via CoAP. Like HTTP, CoAP provides REST based access to resources, but CoAP is more compact for use in constrained IoT devices.

The `device-coap-c` service is modeled after the HTTP based [device-rest-go](https://github.com/edgexfoundry/device-rest-go) service, and runs over UDP. The current implementation is meant for one-way communication from a device into EdgeX via async readings. Synchronous interaction initiated by EdgeX can be added in the future.


## CoAP Data Resource

This device service creates a parameterized CoAP resource to which data may be posted:

```
   /a1r/{deviceName}/{resourceName}
```

- `a1r` is short for "API v1 resource", as defined by device-rest-go.
- `deviceName` refers to a `device` managed by the CoAP device service. `res/configuration.toml` defines a device named 'd1'.
- `resourceName` refers to the `deviceResource` defined in the device profile associated with the given `deviceName`.
`res/device-profile.yaml` defines integer, string, float, and binary resources.

The data posted to this resource is type validated, and the resulting value then sis ent into EdgeX via the Device SDK's asynchronous `post_readings` capability.

See the _Testing_ section below for example use.


## Configuration

This section describes properties in `res/configuration.toml` as used by device-coap-c. See the _Configuration and Registry_ section of the EdgeX documentation for background.

### Driver
`SecurityMode` defines CoAP client-server security. Use 'PSK' for pre-shared key, or 'NoSec' for no security. See the example below.

```
[Driver]
  SecurityMode = 'PSK'
  # key is up to 16 arbitrary bytes; must be base64 encoded here
  PskKey = '960CWMcJpaM0VLDXPNAf5A=='
```

device-coap-c accepts only a single pre-shared key. `PskKey` is ignored if `SecurityMode` is 'NoSec'.

### DeviceList
The `DeviceList` configuration is standard except that the `DeviceList.Protocols` can be empty. The following is a sample `DeviceList` that works with the sample device profiles referenced below. Also see [configuration.toml](./res/configuration.toml).

```toml
# Pre-define Devices
[[DeviceList]]
  Name = 'd1'
  Profile = 'Coap-Device'
  Description = 'Coap Data Generator Device'
  Labels = [ "coap", "rest" ]
  [DeviceList.Protocols]
    [DeviceList.Protocols.other]
```

## Device Profile

As with all device services, the `device profile` is where the **Device Name**, **Device Resources** and **Device Commands** are defined. The parameterized REST endpoint described above references these definitions. See the sample profile in [device-profile.yaml](./res/device-profile.yaml).

> *Note: The `coreCommands` section is omitted since this device service does not support Commanding.*

> *Note: The `deviceCommands` section only requires the `get` operation.*

## Docker Integration

### Building

You can build a Docker image with the command below from the top level directory of a device-coap-c checkout.

```
   $ docker build -t kb2ma/device-coap -f scripts/Dockerfile.alpine-3.11 .
```

### Compose

Below is an example entry for a docker-compose template with the rest of the EdgeX setup. The CoAP server listens on the default secure port, 5684. It also listens on any interface since the CoAP message likely arrives from an external network.

```
  device-coap:
    image: kb2ma/device-coap:latest
    ports:
      - "127.0.0.1:49750:49750"
      - "0.0.0.0:5684:5684/udp"
    container_name: kb2ma-device-coap
    hostname: kb2ma-device-coap
    networks:
      - edgex-network
    environment:
      <<: *common-variables
      Service_Host: kb2ma-device-coap
    depends_on:
      - metadata
      - data
```

## Testing/Simulation

You can use simulated data to test this service with libcoap's `coap-client` command line tool. The examples below are organized by the SecurityMode defined in the configuration.

**NoSec**
```
   $ coap-client -m post -e 1001 coap://127.0.0.1/a1r/d1/int
```
**PSK**
```
   $ coap-client -m post -u r17 -k nitt4agm2c2tatcy -e 1001 coaps://127.0.0.1/a1r/d1/int
```

  * For DTLS PSK, a CoAP client must include a user identity via the `-u` option as well as the same key the server uses. Presently, the device-coap-c server does not evaluate the identity, only the key. Also notice the protocol in the address is `coaps`. This protocol uses UDP port 5684 rather than 5683 for protocol `coap`.

  * POSTing a text integer value will set the  `Value` of the `Reading` in EdgeX to the string representation of the value as an `Int32`. The POSTed value is verified to be a valid `Int32` value.

  * A 400 error will be returned if the POSTed value fails the `Int32` type verification.

### Zephyr CoAP client

See my Zephyr based [edgex-coap-peer](https://github.com/kb2ma/edgex-coap-peer) repository for a CoAP client usable on an IoT device. The client posts data to `/a1r/d1/int`.

## Development

This section describes how to build and run a device-coap-c executable independent from Docker, for development or debugging.

### Building

device-coap-c depends on libcoap and tinydtls. See [build_deps.sh](scripts/build_deps.sh) to download and build them. As with any C based EdgeX device project, device-coap-c also depends on the EdgeX [C SDK](https://github.com/edgexfoundry/device-sdk-c/blob/master) for its SDK library and headers. Finally, see [build.sh](scripts/build.sh) and [build_debug.sh](scripts/build_debug.sh) to build device-coap-c itself.

_Note_ Service configuration in `configuration.toml` must be customized for a separate device-coap-c executable. For the `Service->Host` parameter, use '172.17.0.1'. For the `Host` parameter in the `Registry` and `Clients` sections, use 'localhost'.

### Running

Simply run the generated executable. The example below was built with the `build_debug.sh` script.

```
   $ build/debug/device-coap-c
```
