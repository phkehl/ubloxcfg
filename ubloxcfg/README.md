# ubloxcfg -- u-blox positioning receivers configuration library

Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org) and contributors

https://oinkzwurgl.org/projaeggd/ubloxcfg/

[License](./LICENSE): GNU Lesser General Public License (LGPL)

## Overview

This implements a library (API) do deal with the new configuration interface introduced in u-blox generation 9 positioning receivers.

See the references in [ubloxcfg-50-ublox.jsonc](./ubloxcfg-50-ublox.jsonc).

The ubloxcfg library is thread-safe, free of dynamic memory allocation and written in c (ISO C99 with no further
dependencies).


## Build and install

```sh
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/wherever/you/want/it
cmake --build build
cmake --install build
```

Dev command:

```sh
clear; rm -rf build; cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/tmp/ub && cmake --build build --verbose && cmake --install build
```

## Configuration definitions

The definitions for the configuration items (parameters) have been taken from u-blox manuals and converted into a JSON
file (with comments): [`ubloxcfg-50-ublox.jsonc`](./ubloxcfg-50-ublox.jsonc).

The [`ubloxcfg_gen.pl`](./ubloxcfg_gen.pl) script converts this to c source code:
[`ubloxcfg_gen.h`](./ubloxcfg_gen.h) and [`ubloxcfg_gen.c`](./ubloxcfg_gen.c).

## Configuration library

The configuration library provides the following:

- Type definitions for configuration items (size, data types, IDs)
- Functions to look up information on a configuration item (by name, by ID)
- Functions to help configuring output message rates.
- Helper macros to define lists of key-value pairs
- Functions to encode lists of key-value pairs into configuration data (and the reverse)
- Functions to stringify configuration items
- A function to convert strings into values

See [`ubloxcfg.h`](./ubloxcfg.h) for details and examples.

## See also

- [../README.md](../README.md)
