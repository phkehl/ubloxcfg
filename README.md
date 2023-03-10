# u-blox 9 positioning receivers configuration library and tool

Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org)

[https://oinkzwurgl.org/hacking/ubloxcfg](https://oinkzwurgl.org/hacking/ubloxcfg)

This implements a library (API) do deal with the new configuration interface introduced in u-blox 9 positioning
receivers (see references in [ubloxcfg-50-ublox.jsonc](./ubloxcfg/ubloxcfg-50-ublox.jsonc)).

A command line `cfgtool` is provided to configure a receiver from the configuration defined in a human-readable
configuration file, as well as a few other functions.

The tool uses a number of small libraries (a UBX/NMEA/RTCM3 message parser, a serial port library, a receiver control
library, and some other things), which could be useful for other projects.

The configuration library is thread-safe, free of dynamic memory allocation and written in c (ISO C99 with no further
dependencies).

The configuration tool and the other libraries use some gcc/libc stuff ("GNU99").

This is tested in Linux ([GCC](https://gcc.gnu.org/)).
Some of it might work on Windows (using [Mingw-w64](http://mingw-w64.org)). I wouldn't know...

[![CI](/../../workflows/CI/badge.svg?branch=master)](/../../actions)

## Building

### Linux

To build and run the tests:

```sh
make test
```

To build the command line tool:

```sh
make cfgtool
```

To build the API documentation:

```sh
make doc
```

To build the tests and the tool for Windows:

```sh
make test.exe
make cfgtool.exe
```

To get a list of all build targets:

```sh
make help
```

See the [`Makefile`](./Makefile) for details.

You may need to install some dependencies:

```sh
sudo apt-get install gcc gcc-multilib perl libpath-tiny-perl libdata-float-perl mingw-w64 doxygen
```

### Windows

GCC, Make, Perl, etc. for Windows is available from [mingw-w64.org](http://mingw-w64.org/doku.php).

### Building as a library (Linux)

Parts of this can be compiled as a shared library:

```sh
make libubloxcfg.so
sudo make install-library  # for installing in /usr/local, or:
make install-library LIBPREFIX=/tmp/some/where
```

Alternatively, use cmake:

```sh
cmake -S cmake -B build/libubloxcfg -DCMAKE_INSTALL_PREFIX=/where/the/library/should/go
make -C build/libubloxcfg
make -C build/libubloxcfg install
```

## Configuration definitions

The definitions for the configuration items (parameters) have been taken from u-blox manuals and converted into a JSON
file (with comments): [`ubloxcfg-50-ublox.jsonc`](./ubloxcfg/ubloxcfg-50-ublox.jsonc).

The [`ubloxcfg_gen.pl`](./ubloxcfg/ubloxcfg_gen.pl) script converts this to c source code:
[`ubloxcfg_gen.h`](./ubloxcfg/ubloxcfg_gen.h) and [`ubloxcfg_gen.c`](./ubloxcfg/ubloxcfg_gen.c).

## Configuration library

The configuration library provides the following:

* Type definitions for configuration items (size, data types, IDs)
* Functions to look up information on a configuration item (by name, by ID)
* Functions to help configuring output message rates.
* Helper macros to define lists of key-value pairs
* Functions to encode lists of key-value pairs into configuration data (and the reverse)
* Functions to stringify configuration items
* A function to convert strings into values

See [`ubloxcfg.h`](./ubloxcfg.h) or the generated HTML documentation for details and examples.

## Tool

The `cfgtool` command line tool can do the following:

* Configure a receiver from a configuration text file
* Receiver connection on local serial ports, remote raw TCP/IP ports or telnet (incl. RFC2217) connections
* Retrieve configuration from a receiver
* Convert a config text file into UBX-CFG-VALSET messages, output as binary UBX messages, u-center compatible hex
  dumps, or c code
* Display receiver navigation status
* And more...

Run `cfgtool -h` or see [`cfgtool.txt`](./cfgtool.txt) for more information.

## GUI

A *very experimental* GUI is available. See [here](./cfggui/README.md).

## Licenses

* Configuration library (`ubloxcfg`): GNU Lesser General Public License (LGPL),
  see [`ubloxcfg/LICENSE`](./ubloxcfg/LICENSE)
* Configuration tool (`cfgtool`), configuration GUI (`cfggui`) and the other libraries (`ff`):
  GNU General Public License (GPL), see [`ff/LICENSE`](./ff/LICENSE)
* Various third-party code comes with its own license, see [`3rdparty/`](./3rdparty) and below

See the individual source files, scripts and other files for details.

## Todo, ideas

* Document ff_* better.
* Fix FIXMEs and TODOs. :)




