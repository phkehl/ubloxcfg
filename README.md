# u-blox positioning receivers configuration library and tool

Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org) and contributors

https://oinkzwurgl.org/projaeggd/ubloxcfg/

This repository contains:

- The [ubloxcfg](./ubloxcfg/README.md) library to deal with the configuration interface of u-blox position receivers
- The [ff](./ff/README.md) utility library
- The [cfgtool](./cfgtool/README.md) command line app to configure a receiver

This is tested on Linux ([GCC](https://gcc.gnu.org/)).

Some of it might work on Windows (using [Mingw-w64](http://mingw-w64.org)). I wouldn't know...

[![CI](/../../actions/workflows/ci.yml/badge.svg)](/../../actions)

## Building

Install dependencies (see [CI workflow](./actions/workflows/ci.yml/badge.svg) for an up-to-date list).

```sh
apt install gcc libpath-tiny-perl libdata-float-perl
```

Either build and install the individual components (ubloxcfg, ff, cfgtool) or use the top-level CMake:

```sh
cmake -B build -S .                # add -DNO_CODEGEN=ON to skip (re-)generating ubloxcfg_gen.[ch]
cmake --build build --parallel 8
./build/ubloxcfg/ubloxcfg-test
./build/cfgtool/cfgtool -h
```

Build and install:

```sh
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/wherever/you/want/it
cmake --build build --parallel 8
cmake --install build
/wherever/you/want/it/bin/cfgtool -h
```

Alternatively, a Makefile is available. Run `make help` for details.


## Licenses

Se the LICENSE files for the various packages in this repository: [ubloxcfg/LICENSE](./ubloxcfg/LICENSE),
[ff/LICENSE](./ff/LICENSE), [cfgtool/LICENSE](./cfgtool/LICENSE)


