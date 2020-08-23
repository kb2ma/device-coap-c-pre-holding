# device-coap-c

EdgeX device service for CoAP-based REST protocol

This device service provides easy way for 3rd party sensor applications to push data into EdgeX via CoAP's RESTful protocol. This service is modeled after the `device-rest-go` service.

The current implementation is meant for one-way communication into EdgeX via async readings. If future use cases determine a need for `commanding`, i.e. two-way communication, it can be added then.


## REST Endpoints

This device service creates the additional parameterized `REST` endpoint:

```
   /a1r/{deviceName}/{resourceName}
```

- `deviceName` refers to a `device` managed by the REST device service.
- `resourceName`refers to the `device resource` defined in the `device profile` associated with the given `deviceName`. Presently only int32 data is supported.

The data posted to this endpoint is type validated and type casted (text data only) to the type defined by the specified `device resource`. The resulting value is then sent into EdgeX via the Device SDK's `async values` channel.


## Configuration

This device service use the standard configuration defined by the **Device SDK**.

The `DeviceList` configuration is standard except that the `DeviceList.Protocols` can be empty. The following is a sample `DeviceList` that works with the sample device profiles referenced below.

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

## Testing/Simulation

A good way to test this service with simulated data is to use a command line tool like libcoap's `coap-client`.

```
   $ echo -n 1001 |./coap-client -m post coap://172.17.0.1/a1r/d1/int -f -
```

  - POSTing a text integer value will result in the  `Value` of the `Reading` being set to the string representation of the value as an `Int32`. The POSTed value is verified to be a valid `Int32` value.

  - A 400 error will be returned if the POSted value fails the `Int32` type verification.
