# device-coap-c

EdgeX device service for CoAP-based REST protocol

This device service allows a 3rd party sensor application to push data into EdgeX via CoAP. Like HTTP, CoAP provides REST based access to resources, but CoAP is more compact for use in constrained IoT devices.

The `device-coap-c` service is modeled after the HTTP based [device-rest-go](https://github.com/edgexfoundry/device-rest-go) service, and runs over UDP. The current implementation is meant for one-way communication into EdgeX via async readings. If future use cases determine a need for `commanding`, i.e. two-way communication, it can be added then.


## REST Endpoints

This device service creates the additional parameterized `REST` endpoint:

```
   /a1r/{deviceName}/{resourceName}
```

- `deviceName` refers to a `device` managed by the REST device service.
- `resourceName` refers to the `deviceResource` defined in the `device profile` associated with the given `deviceName`.  Presently only int32 data is supported.

The data posted to this endpoint is type validated and type casted (text data only) to the type defined by the specified `device resource`. The resulting value is then sent into EdgeX via the Device SDK's asynchronous `post_readings` capability.


## Configuration

This device service uses the standard configuration defined by the **Device SDK**.

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

## Building

Presently `device-coap-c` does not include a Docker container build; however, you can run it as a separate exectuable from the rest of EdgeX. Follow the instructions below, which assume you are using a POSIXy environment like Linux or Mac.

Download the [libcoap](https://github.com/obgm/libcoap) source from GitHub and follow the [BUILDING](https://github.com/obgm/libcoap/blob/v4.2.1/BUILDING) instructions. `device-coap-c` is known to work with nosec or tinydtls based PSK security for libcoap v4.2.1.

Next, build the EdgeX [C SDK](https://github.com/edgexfoundry/device-sdk-c/blob/master) for its SDK library and headers. Then you can build `device-coap-c` with its [Makefile](./src/c/Makefile). 

> Note: The `device-coap-c` Makefile assumes libcoap was built with tinydtls support. Otherwise, define a `LIBCOAP`  environment variable with the name of the library. See the Makefile for the default format of this name, and compare with the name of the installed libcoap library.

## Security and Running

`device-coap-c` supports use of DTLS security over UDP, specifically with a single pre-shared key (PSK). Use the command line `-k` option to define the key, like the example below. The key may be up to 16 characters long.

```
   $ device-coap-c -k nitt4agm2c2tatcy
```

In this case, *all* messaging uses DTLS. To run `device-coap-c` without security (nosec), do not include the `-k` option.

## Testing/Simulation

You can use simulated data to test this service with libcoap's `coap-client` command line tool, like the examples below.

```
nosec
   $ coap-client -m post -e 1001 coap://172.17.0.1/a1r/d1/int
PSK
   $ coap-client -m post -u r17 -k nitt4agm2c2tatcy -e 1001 coaps://172.17.0.1/a1r/d1/int
```

  * For DTLS PSK, a CoAP client must include an arbitrary user identity via the `-u` option as well as the same key the server uses. Also notice the protocol in the address is `coaps`. This protocol uses UDP port 5684 rather than 5683 for protocol `coap`.

  * POSTing a text integer value will set the  `Value` of the `Reading` to the string representation of the value as an `Int32`. The POSTed value is verified to be a valid `Int32` value.

  * A 400 error will be returned if the POSTed value fails the `Int32` type verification.
