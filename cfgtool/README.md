# cfgtool -- u-blox positioning receivers configuration tool

Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org) and contributors

https://oinkzwurgl.org/projaeggd/ubloxcfg/

[License](./LICENSE): GNU General Public License (GPL) v3

## Overview

A command line app to configure a receiver from the configuration defined in a human-readable configuration file,
as well as a few other functions.

This uses some gcc/libc stuff ("GNU99").

The `cfgtool` command line tool can do the following:

- Configure a receiver from a configuration text file
- Receiver connection on local serial ports, remote raw TCP/IP ports or telnet (incl. RFC2217) connections
- Retrieve configuration from a receiver
- Convert a config text file into UBX-CFG-VALSET messages, output as binary UBX messages, u-center compatible hex
  dumps, or c code
- Display receiver navigation status
- And more...

Run `cfgtool -h` or see [`cfgtool.txt`](./cfgtool.txt) for more information.

## Build and install

First, build and install the [ubloxcfg library](../ubloxcfg/README.md) and the [ff library](../ff/README.md). Then build
cfgtool:

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
