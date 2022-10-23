# Channels Protocol API

A resource-lightweight protocol for ZX Spectrum on which the client and the server
communicate over the TCP channel, exchanging efficient key-value like objects,
without having to worry about buffers or/and protocol details.

## Overview

The objects use as little memory as possible and do dirty tricks to use even less.
To function, the library uses [Spectranet](https://speccytools.org) on ZX Spectrum and simple
sockets on PC. Ultimately, same POSIX API.

Use cases: multiplayer games, networking programs, for ZX Spectrum.

## How to use

You need to install z88dk and z88dk libraries for Spectranet into your system.
You can read about both on [speccytools.org](https://speccytools.org/).

Project uses CMake both on Server (PC) and Client (ZX Spectrum).
Latest z88dk supports CMake as well, so it should build.

See [terminal example](./examples/terminal/Readme.md) to see how the protocol works, as well the library.

If didn't export `ZCCCFG` variable, do it to something like:
```bash
export ZCCCFG=/usr/local/share/z88dk/lib/config
./build.sh
```

To use this on your project, simply add it as a submodule,
and do `add_subdirectory()` on your CMake project.
See examples on how it's done.

## Protocol details

See [protocol details](./Protocol.md) to see how the protocol and communication works.

## Inspecting traffic in Wireshark

If you want to inspect traffic for irregularities, see
[Inspecting traffic in Wireshark](./Wireshark.md).
