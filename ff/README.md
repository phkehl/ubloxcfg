# ff library -- allecheibs utilities

Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org) and contributors

https://oinkzwurgl.org/projaeggd/ubloxcfg/

[License](./LICENSE): GNU General Public License (GPL) v3

## Overview

This is a utility library. It contains a UBX/NMEA/RTCM3 message parser, a serial port library, a receiver control, and
some other things), which could be useful for other projects.

This uses some gcc/libc stuff ("GNU99").

## Build and install

First, build and install the [ubloxcfg library](../ubloxcfg/README.md).

```sh
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/wherever/you/want/it
cmake --build build
cmake --install build
```

Dev command:

```sh
clear; rm -rf build; cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/tmp/ub && cmake --build build --verbose && cmake --install build
```

## See also

- [../README.md](../README.md)
