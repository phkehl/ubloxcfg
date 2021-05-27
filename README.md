# u-blox 9 positioning receivers configuration library and tool

Copyright (c) 2020-2021 Philippe Kehl (flipflip at oinkzwurgl dot org)

[https://oinkzwurgl.org/hacking/ubloxcfg](https://oinkzwurgl.org/hacking/ubloxcfg)

This implements a library (API) do deal with the new configuration interface introduced in u-blox 9 positioning
receivers (e.g. [1]).

A command line `cfgtool` is provided to configure a receiver from the configuration defined in a human-readable
configuration file, as well as a few other functions.

The tool uses a number of small libraries (a UBX/NMEA/RTCM3 message parser, a serial port library, a receiver control
library, and some other things), which could be useful for other projects.

The configuration library is thread-safe, free of dynamic memory allocation and written in c (ISO C99 with no further
dependencies).

The configuration tool and the other libraries use some gcc/libc stuff ("GNU99").

This is tested in Linux ([GCC](https://gcc.gnu.org/)). It should work in Windows ([Mingw-w64](http://mingw-w64.org)).

[![CI](/../../workflows/CI/badge.svg?branch=master)](/../../actions)

## Building

### Building as a library

Program can be compiled as a shared library:

```
make libubloxcfg.so
sudo make install-library
```

### Linux

Note that this probably only works when building on Linux.

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

GCC for Windows is available from [mingw-w64.org](http://mingw-w64.org/doku.php).
Since you might need Perl to generate the configuration definitions (see below) you could also use the
GCC that comes with [Strawberry Perl](http://strawberryperl.com/).

Depending on the availability of other tools (make, rm, etc.) on your system you will be able to use the Makefile.

To manually compile, start a Strawberry Perl shell, navigate to the source code, and use this command to compile:

```
gcc -o output/cfgtool.exe ubloxcfg*.c cfgtool*.c ff*.c crc*.c
```

## Configuration definitions

The definitions for the configuration items (parameters) have been taken from u-blox manuals and converted into a JSON
file (with comments): [`ubloxcfg.json`](./ubloxcfg.json).

The [`ubloxcfg_gen.pl`](./ubloxcfg_gen.pl) script converts this to c source code: [`ubloxcfg_gen.h`](./ubloxcfg_gen.h)
and [`ubloxcfg_gen.c`](./ubloxcfg_gen.c).

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
* Receiver connection on local serial ports, remote raw TCP/IP ports or telnet (inc. RFC2217) connections
* Retrieve configuration from a receiver
* Convert a config text file into UBX-CFG-VALSET messages, output as binary UBX messages, u-center compatible hex
  dumps, or c code
* And more...

Run `cfgtool -h` or see [`cfgtool.txt`](./cfgtool.txt) for more information.

## License

* Configuration library (`ubloxcfg*.[ch]`): GNU Lesser General Public License (LGPL), see [`COPYING.LESSER`](./COPYING.LESSER)
* Configuration tool (`cfgtool*.[ch]`) and the other libraries (`ff_*.[ch]`): GNU General Public License (GPL), see [`COPYING`](./COPYING)
* Various third-party code comes with its own license, see [`3rdparty/`](./3rdparty) and below

See the individual source files and scripts for details.

## Third-party code

The tool uses the following third-party code:

* _CRC-24Q_ routines from [https://gitlab.com/gpsd/](https://gitlab.com/gpsd/)
  See the source code ([`crc24q.c`](./crc24q.c)) and license ([`crc24q.COPYING`](./crc24q.COPYING)).

## Todo, ideas

* Document ff_* better.
* Fix FIXMEs and TODOs. :)

