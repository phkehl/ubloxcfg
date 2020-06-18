// flipflip's serial port library
//
// Copyright (c) 2020 Philippe Kehl (flipflip at oinkzwurgl dot org),
// https://oinkzwurgl.org/hacking/ubloxcfg
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
// even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with this program.
// If not, see <https://www.gnu.org/licenses/>.
//

#ifndef __FF_PORT_H__
#define __FF_PORT_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ********************************************************************************************** */

#define PORT_BAUDRATES      9600,  19200,  38400,  57600,  115200,  230400,  460800,  921600
#ifdef _WIN32
#  define PORT_BAUDVALUES   9600,  19200,  38400,  57600,  115200,  230400,  460800,  921600
#else
#  define PORT_BAUDVALUES  B9600, B19200, B38400, B57600, B115200, B230400, B460800, B921600
#endif

#define PORT_SPEC_MAX_LEN 256

typedef enum PORT_TYPE_e
{
    PORT_TYPE_SER,    // Serial ports: ser://<device>[@baudrate]
    PORT_TYPE_TCP,    // TCP/IP sockets: tcp://<host>:<port>
    PORT_TYPE_TELNET  // TCP/IP sockets with telnet (RFC854 etc.) and com port control (RFC2217): telnet://<host>:<port>[@<baudrate>]
  //PORT_TYPE_HANDLE  // Use existing file handle (or pair of file handles, such as stdin/stdout)
} PORT_TYPE_t;

typedef struct PORT_s
{
    PORT_TYPE_t type;
    uint32_t    numRx;
    uint32_t    numTx;
    bool        portOk;
    int         baudrate;
    char        file[PORT_SPEC_MAX_LEN];
#ifdef _WIN32
    void       *handle;
#else
    int         fd;
#endif
    // tcp
    uint16_t    port;
    uint32_t    lastTime;
    int         lastCount;
    // telnet
    int         tnState;
    uint8_t     tnInband[12];
    int         nTnInband;
    char        tmp[PORT_SPEC_MAX_LEN + 32];

} PORT_t;

bool portInit(PORT_t *port, const char *spec);
bool portOpen(PORT_t *port);
void portClose(PORT_t *port);
bool portWrite(PORT_t *port, const uint8_t *data, const int size);
bool portRead(PORT_t *port, uint8_t *data, const int size, int *read);
bool portCanBaudrate(PORT_t *port);
bool portSetBaudrate(PORT_t *port, const int baudrate);
int portGetBaudrate(PORT_t *port);

/* ********************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_PORT_H__
