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
- `deviceName` refers to a `device` managed by the CoAP device service.
- `resourceName` refers to the `deviceResource` defined in the `device profile` associated with the given `deviceName`.

The data posted to this endpoint is type validated and type casted (text data only) to the type defined by the specified `device resource`. The resulting value is then sent into EdgeX via the Device SDK's asynchronous `post_readings` capability. Presently only the resourceName `int` is supported, for int32 data.

See the Testing section below for examle use.


## Configuration

This section describes properties in `res/configuration.toml` as used by device-coap-c. See the _Configuration and Registry_ section of the EdgeX documentation for background.

### Driver
`SecurityMode` defines CoAP client-server security. Use `PSK` for pre-shared key, or `NoSec` for no security.

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

## Docker Compose

Below is an example entry for a docker-compose template. The CoAP server listens on the default port, 5683.

```
  device-coap:
    image: kb2ma/device-coap:latest
    ports:
      - "127.0.0.1:49750:49750"
      - "127.0.0.1:5683:5683/udp"
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
   $ coap-client -m post -e 1001 coap://172.17.0.1/a1r/d1/int
```
**PSK**
```
   $ coap-client -m post -u r17 -k nitt4agm2c2tatcy -e 1001 coaps://172.17.0.1/a1r/d1/int
```

  * For DTLS PSK, a CoAP client must include an arbitrary user identity via the `-u` option as well as the same key the server uses. Also notice the protocol in the address is `coaps`. This protocol uses UDP port 5684 rather than 5683 for protocol `coap`.

  * POSTing a text integer value will set the  `Value` of the `Reading` to the string representation of the value as an `Int32`. The POSTed value is verified to be a valid `Int32` value.

  * A 400 error will be returned if the POSTed value fails the `Int32` type verification.

## Development

This section describes how to build and run device-coap-c independent from Docker, for development or debugging.

### Building

device-coap-c depends on libcoap and tinydtls. See [build_deps.sh](scripts/build_deps.sh) to download and build them. As with any C based EdgeX device project, device-coap-c also depends on the EdgeX [C SDK](https://github.com/edgexfoundry/device-sdk-c/blob/master) for its SDK library and headers. Finally, see [build.sh](scripts/build.sh) and [build_debug.sh](scripts/build_debug.sh) to build device-coap-c itself.

### Running

`device-coap-c` supports use of DTLS security over UDP, specifically with a single pre-shared key (PSK). Use the command line `-k` option to define the key, like the example below. The key may be up to 16 characters long.

```
   $ device-coap-c -k nitt4agm2c2tatcy
```

In this case, *all* messaging uses DTLS. To run `device-coap-c` without security (NoSec), do not include the `-k` option.
