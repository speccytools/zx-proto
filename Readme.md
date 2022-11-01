# Channels Protocol API

A resource-lightweight protocol for ZX Spectrum on which the client and the server
communicate over the TCP channel, exchanging efficient key-value like objects,
without having to worry about buffers or/and protocol details.

## Overview

The objects use as little memory as possible and do [dirty tricks](https://github.com/speccytools/zx-proto/blob/master/include/proto_objects.h#L39) to use even less.
To function, the library uses [Spectranet](https://speccytools.org) on ZX Spectrum and simple
sockets on PC. Ultimately, same POSIX API.

Use cases: multiplayer games, networking programs, for ZX Spectrum.

## How to use

You need to install z88dk and z88dk libraries for Spectranet into your system.
You can read about both on [speccytools.org](https://speccytools.org/).

Project uses CMake both on Server (PC) and Client (ZX Spectrum).
Latest z88dk supports CMake as well, so it should build.

When setting things up, make sure to allocate a working buffer of sufficient size,
and supply it via `proto_init(proto, buffer, buffer_size)` call.
The size of the buffer should be enough for one biggest object
on a request/response, and it doesn't have to fit the whole response,
as responses are processed on per-object basis and not as a whole.

See [terminal example](./examples/terminal/Readme.md) to see how the protocol works, as well the library.

If didn't export `ZCCCFG` variable, do it to something like:
```bash
export ZCCCFG=/usr/local/share/z88dk/lib/config
./build.sh
```

To use this on your project, simply add it as a submodule,
and do `add_subdirectory()` on your CMake project.
See examples on how it's done.

```cmake
add_subdirectory("zx-proto")
target_link_libraries(<project> PUBLIC libspectranet.lib libsocket.lib channels_proto)
```

For server use, if you need functionality that uses malloc or/and request-response support, you also have to link `channels_proto_server`.

## Protocol details

See [protocol details](./Protocol.md) to see how the protocol and communication works.

## Inspecting traffic in Wireshark

If you want to inspect traffic for irregularities, see
[Inspecting traffic in Wireshark](./Wireshark.md).
