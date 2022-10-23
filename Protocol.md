# Protocol details

```c
client <=> server // tcp SYN, SYN+ACK, ACK

client -> [request] -> server
client <- [response] <- server

client -/- server // tcp FIN, FIN+ACK, ACK
```

The communication lays on top of TCP and
consists of request-response pairs like so:

```c
request or response {
    uint8_t request_flags;
    uint16_t request_length;
    uint16_t request_id;
    // array<char, size> {
    object a;
    object b;
    object c;
    ...
    // }
}
```

`size` defines amount of bytes allocated for all objects combined on a particular request or response, excluding
the `request_flags`, `request_length` and `request_id`.

## Object structure

Each object is represented by generic key-value structure with following syntax:

```c
object {
    uint16_t object_size;
    // array<char, object_size> {
    object_property a; 
    object_property b; 
    object_property c;
    ...
    // }
}
```

`object_size` defines amount of bytes allocated for all properties combined. Each property is defined like so:

```c
object_property
{
    uint16_t value_size;
    uint8_t key;
    array<char, value_size> value;
}
```

On the documentation below, each object is described like so:
```c
{
    KEY1 "value"
    KEY2 "value1"
    KEY2 "value2"
}
```

An object can have multiple properties with the same key.
On all API calls below, a request-response transaction is documented as follows:
```c
-> [{
    OBJ_PROPERTY_ID "test"
}]
<- [{
    OBJ_PROPERTY_ID "hello"
    OBJ_PROPERTY_TITLE "a"
}, {
    OBJ_PROPERTY_ID "hello"
    OBJ_PROPERTY_TITLE "b"
}]
```

That means the client sent a request with one object,
while server responded with two objects.

* The first object of the request (Request Object) defines the type of the request.
  `OBJ_PROPERTY_ID` property defines the API call of that request.
* If the first object on a response has `OBJ_PROPERTY_ERROR` key, that means
  the whole request has failed and instead of processing regular objects,
  application should process an error path:
  ```c
  -> [{
      OBJ_PROPERTY_ID "haha"
  }]
  <- [{
      OBJ_PROPERTY_ERROR "unknown_api_call"
  }]
  ```

## Inspecting traffic in Wireshark

If you want to inspect traffic for irregularities, see
[Inspecting traffic in Wireshark](./Wireshark.md).
