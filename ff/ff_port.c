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

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/file.h>
#include <sys/types.h>

#ifdef _WIN32
#  define NOGDI
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <windows.h>
#else
#  include <netdb.h>
#  include <sys/socket.h>
#  include <termios.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#endif

#include "ff_debug.h"
#include "ff_stuff.h"
#include "ff_port.h"

/* ****************************************************************************************************************** */

#define PORT_XTRA_TRACE_ENABLE 0 // Set to 1 to enable more trace output

#if PORT_XTRA_TRACE_ENABLE
#  define PORT_XTRA_TRACE(fmt, args...) TRACE("port(%s) " fmt, portSpecStr(port), ## args)
#else
#  define PORT_XTRA_TRACE(fmt, ...) /* nothing */
#endif

#define PORT_WARNING(fmt, ...) WARNING("port(%s) " fmt, portSpecStr(port), ##__VA_ARGS__)
#define PORT_DEBUG(fmt, ...)   DEBUG(  "port(%s) " fmt, portSpecStr(port), ##__VA_ARGS__)
#define PORT_TRACE(fmt, ...)   TRACE(  "port(%s) " fmt, portSpecStr(port), ##__VA_ARGS__)

#define PORT_WARNING_THROTTLE(fmt, ...) do { \
    const uint32_t now = TIME(); if ((now - port->lastWarn) > 1000) { \
    WARNING("port(%s) " fmt, portSpecStr(port), __VA_ARGS__); \
    port->lastWarn = now; } } while (false);

static const char *portSpecStr(PORT_t *port)
{
    if (port == NULL)
    {
        return "?";
    }
    port->tmp[0] = '\0';
    switch (port->type)
    {
        case PORT_TYPE_SER:
            snprintf(port->tmp, sizeof(port->tmp), "ser://%s@%d", port->file, port->baudrate);
            break;
        case PORT_TYPE_TCP:
            snprintf(port->tmp, sizeof(port->tmp), "tcp://%s:%u", port->file, port->port);
            break;
        case PORT_TYPE_TELNET:
            snprintf(port->tmp, sizeof(port->tmp), "telnet://%s:%u@%d", port->file, port->port, port->baudrate);
            break;
    }
    return port->tmp;
}

const char *_portErrStr(PORT_t *port, int err)
{
#ifdef _WIN32
    if (err == 0)
    {
        err = GetLastError();
    }
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, port->tmp, sizeof(port->tmp), NULL);
    const int len = strlen(port->tmp);
    if (len > 2)
    {
        port->tmp[len-2] = '\0';
    }
    snprintf(&port->tmp[len - 2], sizeof(port->tmp) - len, " (%d)", err);
    return port->tmp;
#else
    (void)port;
    return err == 0 ? strerror(errno) : strerror(err);
#endif
}

// ---------------------------------------------------------------------------------------------------------------------

static uint32_t _portBaudrateValue(const int baudrate);

bool portInit(PORT_t *port, const char *spec)
{
    if ( (spec == NULL) || (strlen(spec) > (PORT_SPEC_MAX_LEN - 1 - 5)) )
    {
        return false;
    }

    memset(port, 0, sizeof(*port));
    bool res = true;

    // Create copy of port spec to work on
    char tmp[PORT_SPEC_MAX_LEN];
    snprintf(tmp, sizeof(tmp), "%s", spec);

    // Look for protocol identifier, use "ser://" by default
    char *addr = tmp;
    {
        char *sep = strstr(tmp, "://");
        char *type = tmp;
        if (sep == NULL)
        {
            type = "ser";
        }
        else
        {
            sep[0] = '\0';
            addr = &sep[3];
        }
        if (strcmp(type, "ser") == 0)
        {
            port->type = PORT_TYPE_SER;
        }
        else if (strcmp(type, "tcp") == 0)
        {
            port->type = PORT_TYPE_TCP;
        }
        else if (strcmp(type, "telnet") == 0)
        {
            port->type = PORT_TYPE_TELNET;
        }
        else
        {
            WARNING("%s: Bad port type %s!", spec, type);
            res = false;
        }
    }

    // Parameters
    if (res)
    {
        switch (port->type)
        {
            case PORT_TYPE_SER:
            {
                addr = strtok(addr, "@");
                char *arg = strtok(NULL, "@");
#ifdef _WIN32
                const bool isAcm = false; // FIXME: How to detect?
#else
                char *real = realpath(addr, NULL); // addr might be a symlink..
                const bool isAcm = (real != NULL) && (strstr(real, "ttyACM") != NULL);
                free(real);
#endif
                const int baudrate = arg != NULL ? atoi(arg) : (isAcm ? 921600 : 9600);
                if (_portBaudrateValue(baudrate) == 0)
                {
                    WARNING("%s: Bad baudrate %s!", spec, arg);
                    res = false;
                }
                else
                {
                    strcat(port->file, addr);
                    port->baudrate = baudrate;
                }
                break;
            }
            case PORT_TYPE_TCP:
            {
                addr = strtok(addr, ":");
                char *arg = strtok(NULL, ":");
                const int portnr = arg == NULL ? -1 : atoi(arg);
                if ( (portnr < 1) || (portnr > UINT16_MAX))
                {
                    WARNING("%s: Missing or bad port number!", spec);
                    res = false;
                }
                else
                {
                    port->port = (uint16_t)portnr;
                    strcat(port->file, addr);
                    port->baudrate = 921600;
                }
                break;
            }
            case PORT_TYPE_TELNET:
            {
                addr = strtok(addr, ":");
                char *arg = strtok(NULL, ":@");
                const int portnr = arg == NULL ? -1 : atoi(arg);
                if ( (portnr < 1) || (portnr > UINT16_MAX))
                {
                    WARNING("%s: Missing or bad port number!", spec);
                    res = false;
                }
                else
                {
                    port->port = (uint16_t)portnr;
                    strcat(port->file, addr);
                }
                arg = strtok(NULL, "@");
                const int baudrate = arg != NULL ? atoi(arg) : 9600;
                if (_portBaudrateValue(baudrate) == 0)
                {
                    WARNING("%s: Bad baudrate %s!", spec, arg);
                    res = false;
                }
                else
                {
                    port->baudrate = baudrate;
                }
                break;
            }
        }
    }

    if (res)
    {
        PORT_DEBUG("port init (%s)", spec);
    }

    return res;
}

static bool _portOpenSer(PORT_t *port);
static bool _portOpenTcp(PORT_t *port);
static bool _portOpenTelnet(PORT_t *port);

bool portOpen(PORT_t *port)
{
    bool res = false;
    if (port != NULL)
    {
        switch (port->type)
        {
            case PORT_TYPE_SER:
                res = _portOpenSer(port);
                break;
            case PORT_TYPE_TCP:
                res = _portOpenTcp(port);
                break;
            case PORT_TYPE_TELNET:
                res = _portOpenTelnet(port);
                break;
        }
    }

    // Happy?
    if (res)
    {
        port->portOk = true;
        PORT_DEBUG("connected");
    }
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

static void _portCloseSer(PORT_t *port);
static void _portCloseTcp(PORT_t *port);
static void _portCloseTelnet(PORT_t *port);

void portClose(PORT_t *port)
{
    if (port != NULL)
    {
        switch (port->type)
        {
            case PORT_TYPE_SER:
                _portCloseSer(port);
                break;
            case PORT_TYPE_TCP:
                _portCloseTcp(port);
                break;
            case PORT_TYPE_TELNET:
                _portCloseTelnet(port);
                break;
        }
        PORT_DEBUG("closed (rx=%u, tx=%u)", port->numRx, port->numTx);
        port->portOk = false;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portWriteSer(PORT_t *port, const uint8_t *data, const int size);
static bool _portWriteTcp(PORT_t *port, const uint8_t *data, const int size);
static bool _portWriteTelnet(PORT_t *port, const uint8_t *data, const int size);

bool portWrite(PORT_t *port, const uint8_t *data, const int size)
{
    bool res = false;
    if ( (port != NULL) && port->portOk && (size > 0) )
    {
        switch (port->type)
        {
            case PORT_TYPE_SER:
                res = _portWriteSer(port, data, size);
                break;
            case PORT_TYPE_TCP:
                res = _portWriteTcp(port, data, size);
                break;
            case PORT_TYPE_TELNET:
                res = _portWriteTelnet(port, data, size);
                break;
        }
    }
    PORT_TRACE("write %d %s", size, res ? "ok" : "fail");
    if (res)
    {
        port->numTx += size;
    }
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portReadSer(PORT_t *port, uint8_t *data, const int size, int *nRead);
static bool _portReadTcp(PORT_t *port, uint8_t *data, const int size, int *nRead);
static bool _portReadTelnet(PORT_t *port, uint8_t *data, const int size, int *nRead);

bool portRead(PORT_t *port, uint8_t *data, const int size, int *nRead)
{
    bool res = false;
    *nRead = 0;
    if ( (port != NULL) && (nRead != NULL) && port->portOk && (size > 0) )
    {
        switch (port->type)
        {
            case PORT_TYPE_SER:
                res = _portReadSer(port, data, size, nRead);
                break;
            case PORT_TYPE_TCP:
                res = _portReadTcp(port, data, size, nRead);
                break;
            case PORT_TYPE_TELNET:
                res = _portReadTelnet(port, data, size, nRead);
                break;
        }
    }
    if ((*nRead > 0) || !res)
    {
        PORT_TRACE("read %d -> %d %s", size, *nRead, res ? "ok" : "fail");
    }
    if (res)
    {
        port->numRx += *nRead;
    }
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portCanBaudrateSer(PORT_t *port);
static bool _portCanBaudrateTcp(PORT_t *port);
static bool _portCanBaudrateTelnet(PORT_t *port);

bool portCanBaudrate(PORT_t *port)
{
    bool res = false;
    if ( (port != NULL) && port->portOk )
    {
        switch (port->type)
        {
            case PORT_TYPE_SER:
                res = _portCanBaudrateSer(port);
                break;
            case PORT_TYPE_TCP:
                res = _portCanBaudrateTcp(port);
                break;
            case PORT_TYPE_TELNET:
                res = _portCanBaudrateTelnet(port);
                break;
        }
    }
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portSetBaudrateSer(PORT_t *port, const int baudrate);
static bool _portSetBaudrateTcp(PORT_t *port, const int baudrate);
static bool _portSetBaudrateTelnet(PORT_t *port, const int baudrate);

bool portSetBaudrate(PORT_t *port, const int baudrate)
{
    bool res = false;
    if ( (port != NULL) && port->portOk && (_portBaudrateValue(baudrate) != 0) )
    {
        switch (port->type)
        {
            case PORT_TYPE_SER:
                res = _portSetBaudrateSer(port, baudrate);
                break;
            case PORT_TYPE_TCP:
                res = _portSetBaudrateTcp(port, baudrate);
                break;
            case PORT_TYPE_TELNET:
                res = _portSetBaudrateTelnet(port, baudrate);
                break;
        }
    }
    PORT_TRACE("baudrate %d %s", baudrate, res ? "ok" : "fail");
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

static int _portGetBaudrateSer(PORT_t *port);
static int _portGetBaudrateTcp(PORT_t *port);
static int _portGetBaudrateTelnet(PORT_t *port);

int portGetBaudrate(PORT_t *port)
{
    int res = 0;
    if (port != NULL)
    {
        switch (port->type)
        {
            case PORT_TYPE_SER:
                res = _portGetBaudrateSer(port);
                break;
            case PORT_TYPE_TCP:
                res = _portGetBaudrateTcp(port);
                break;
            case PORT_TYPE_TELNET:
                res = _portGetBaudrateTelnet(port);
                break;
        }
    }
    return res;
}

/* ***** serial ports *************************************************************************** */

static bool _portOpenSer(PORT_t *port)
{
#ifdef _WIN32

    char file[PORT_SPEC_MAX_LEN];
    snprintf(file, sizeof(file), "\\\\.\\%s", port->file);

    HANDLE handle = CreateFileA(file, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        PORT_WARNING("Failed opening device: %s", _portErrStr(port, 0));
        return false;
    }

    char settingsStr[256];
    snprintf(settingsStr, sizeof(settingsStr), "baud=%u data=8 parity=n stop=1",
        _portBaudrateValue(port->baudrate));
    DCB settings;
    memset(&settings, 0, sizeof(settings));
    settings.DCBlength = sizeof(settings);
    if (BuildCommDCBA(settingsStr, &settings) == 0)
    {
        PORT_WARNING("Failed parsing settings: %s", _portErrStr(port, 0));
        CloseHandle(handle);
        return false;
    }
    if (SetCommState(handle, &settings) == 0)
    {
        PORT_WARNING("Failed applying settings: %s", _portErrStr(port, 0));
        CloseHandle(handle);
        return false;
    }

    COMMTIMEOUTS timeouts;
    memset(&timeouts, 0, sizeof(timeouts));
    timeouts.ReadIntervalTimeout = MAXDWORD;
    if (SetCommTimeouts(handle, &timeouts) == 0)
    {
        PORT_WARNING("Failed applying timeouts: %s", _portErrStr(port, 0));
        CloseHandle(handle);
        return false;
    }

    port->handle = (void *)handle;

#else

    int fileno = open(port->file, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fileno < 0)
    {
        PORT_WARNING("Failed opening device: %s", _portErrStr(port, 0));
        return false;
    }

    if (flock(fileno, LOCK_EX | LOCK_NB) != 0)
    {
        PORT_WARNING("Failed locking device: %s", _portErrStr(port, 0));
        close(fileno);
        return false;
    }

    struct termios settings;
    memset(&settings, 0, sizeof(settings));
    settings.c_iflag = IGNBRK | IGNPAR;
    settings.c_cflag = CS8 | CLOCAL | CREAD;
    cfsetispeed(&settings, _portBaudrateValue(port->baudrate));
    cfsetospeed(&settings, _portBaudrateValue(port->baudrate));

    if (tcsetattr(fileno, TCSANOW, &settings) != 0)
    {
        PORT_WARNING("Failed configuring device: %s", _portErrStr(port, 0));
        flock(fileno, LOCK_UN);
        close(fileno);
        return false;
    }

    port->fd = fileno;

#endif

    return true;
}

static uint32_t _portBaudrateValue(const int baudrate)
{
    const int      rates[]  = { PORT_BAUDRATES };
    const uint32_t values[] = { PORT_BAUDVALUES };
    for (int ix = 0; ix < NUMOF(rates); ix++)
    {
        if (baudrate == rates[ix])
        {
            return values[ix];
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------------

static void _portCloseSer(PORT_t *port)
{
#ifdef _WIN32
    CloseHandle((HANDLE)port->handle);
#else
    flock(port->fd, LOCK_UN);
    close(port->fd);
#endif
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portWriteSer(PORT_t *port, const uint8_t *data, const int size)
{
#ifdef _WIN32

    uint32_t num = 0;
    // FIXME: handle ERROR_IO_PENDING?
    const int res = WriteFile((HANDLE)port->handle, data, size, (LPDWORD)&num, NULL);
    PORT_XTRA_TRACE("write %d -> %d %u", size, res, num);
    if ( (res == 0) || ((int)num != size) )
    {
        PORT_WARNING_THROTTLE("write fail (%d, %d): %s", size, res, _portErrStr(port, 0));
        return false;
    }

#else

    const int res = write(port->fd, data, size);
    PORT_XTRA_TRACE("write %d -> %d", size, res);
    // FIXME: handle EAGAIN?
    if ( (res < 0) || (res != size) )
    {
        PORT_WARNING_THROTTLE("write fail (%d, %d): %s", size, res, _portErrStr(port, 0));
        return false;
    }

#endif

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portReadSer(PORT_t *port, uint8_t *data, const int size, int *nRead)
{
#ifdef _WIN32

    uint32_t num = 0;
    // FIXME: handle ERROR_IO_PENDING?
    const int res = ReadFile((HANDLE)port->handle, data, size, (LPDWORD)&num, NULL);
    PORT_XTRA_TRACE("read %d -> %d %u", size, res, num);
    if (res == 0)
    {
        PORT_WARNING_THROTTLE("read fail (%d, %d): %s", size, res, _portErrStr(port, 0));
        return false;
    }
    *nRead = (int)num;

#else

    const int res = read(port->fd, data, size);
    PORT_XTRA_TRACE("read %d -> %d", size, res);
    if (res == -1) // EAGAIN
    {
        *nRead = 0;
        return true;
    }
    if (res < 0)
    {
        PORT_WARNING_THROTTLE("read fail (%d, %d): %s", size, res, _portErrStr(port, 0));
        *nRead = 0;
        return false;
    }
    *nRead = res;

#endif

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portCanBaudrateSer(PORT_t *port)
{
    (void)port;
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portSetBaudrateSer(PORT_t *port, const int baudrate)
{
#ifdef _WIN32

    DCB settings;
    settings.DCBlength = sizeof(settings);
    if (GetCommState((HANDLE)port->handle, &settings) == 0)
    {
        PORT_WARNING("Failed getting port (baudrate) settings: %s", _portErrStr(port, 0));
        return false;
    }
    settings.BaudRate = _portBaudrateValue(baudrate);
    if (SetCommState((HANDLE)port->handle, &settings) == 0)
    {
        PORT_WARNING("Failed applying (baudrate) settings: %s", _portErrStr(port, 0));
        return false;
    }

#else

    struct termios settings;
    if (tcgetattr(port->fd, &settings) != 0)
    {
        PORT_WARNING("tcgetattr fail: %s", _portErrStr(port, 0));
        return false;
    }

    cfsetispeed(&settings, _portBaudrateValue(baudrate));
    cfsetospeed(&settings, _portBaudrateValue(baudrate));

    if (tcsetattr(port->fd, TCSANOW, &settings) != 0)
    {
        PORT_WARNING("tcsetattr fail: %s", _portErrStr(port, 0));
        return false;
    }
    port->baudrate = baudrate;

#endif

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static int _portGetBaudrateSer(PORT_t *port)
{
    return port->baudrate;
}

/* ***** raw TCP/IP ***************************************************************************** */

#define TCPIP_MAX_PACKET_SIZE 256

#ifndef _WIN32
typedef int SOCKET;
#  define INVALID_SOCKET -1
#  define SOCKET_ERROR -1
#endif

#ifdef _WIN32
static int gWinTcpSocketsOpen;

// Initialise Winsock API (once only)
bool _winsockInit(PORT_t *port)
{
    if (gWinTcpSocketsOpen > 0)
    {
        return true;
    }

    // https://docs.microsoft.com/en-us/windows/win32/api/

    WORD versionWanted = MAKEWORD(2, 2);
    WSADATA wsaData;
    const int res = WSAStartup(versionWanted, &wsaData);
    if (res != 0)
    {
        PORT_WARNING("Failed initialising winsock: %s", _portErrStr(port, res));
        return false;
    }
    const int wsockVerMajor = LOBYTE(wsaData.wVersion);
    const int wsockVerMinor = HIBYTE(wsaData.wVersion);
    if ( (wsockVerMajor != 2) || (wsockVerMinor < 2) )
    {
        PORT_WARNING("Unexpected winsock version %d.%d!", wsockVerMajor, wsockVerMinor);
        WSACleanup();
        return false;
    }
    DEBUG("winsock version %d.%d", wsockVerMajor, wsockVerMinor);

    gWinTcpSocketsOpen++;

    return true;
}

void _winsockDeinit(PORT_t *port)
{
    if (gWinTcpSocketsOpen <= 0)
    {
        return;
    }
    (void)port;
    WSACleanup();
    gWinTcpSocketsOpen--;
}

#endif

static bool _portOpenTcp(PORT_t *port)
{
#ifdef _WIN32
    if (!_winsockInit(port))
    {
        return false;
    }
#endif

    // Find address, as per getaddrinfo(3)
    struct addrinfo *result;
    {
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_UNSPEC;  // IPv4 or IPv6
        hints.ai_socktype = SOCK_STREAM;
        char portNrStr[20];
        snprintf(portNrStr, sizeof(portNrStr), "%d", port->port);
        const int res = getaddrinfo(port->file, portNrStr, &hints, &result);
        if (res != 0)
        {
#ifdef _WIN32
            PORT_WARNING("Failed connecting: %s", _portErrStr(port, 0));
            _winsockDeinit(port);
#else
            PORT_WARNING("Failed getting address: %s", gai_strerror(res));
#endif
            return false;
        }
    }

    // Connect
    SOCKET fd = INVALID_SOCKET;
    for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next)
    {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == INVALID_SOCKET)
        {
            continue;
        }

#ifndef _WIN32
#  if 1
        struct timeval timeout;
        timeout.tv_sec  = 5;
        timeout.tv_usec = 0;
        if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != 0)
        {
            PORT_WARNING("Failed setting SO_SNDTIMEO option: %s", _portErrStr(port, 0));
        }
#  else
        const int count = 3;
        if (setsockopt(fd, IPPROTO_TCP, TCP_SYNCNT, &count, sizeof(count)) != 0)
        {
            PORT_WARNING("Failed setting TCP_SYNCNT option: %s", _portErrStr(port, 0));
        }
#  endif
#endif

        int res = connect(fd, rp->ai_addr, rp->ai_addrlen);
        if (res == SOCKET_ERROR)
        {
            PORT_WARNING("Failed connecting: %s", _portErrStr(port, 0));
#ifdef _WIN32
            closesocket(fd);
            _winsockDeinit(port);
#else
            close(fd);
#endif
            return false;
        }
        else
        {
            break;
        }
    }

    // Cleanup
    freeaddrinfo(result);

    // Happy?
    if (fd == INVALID_SOCKET)
    {
        PORT_WARNING("No connection possible!"); // FIXME: but why?
#ifdef _WIN32
        _winsockDeinit(port);
#endif
        return false;
    }

    // Make non-blocking
#ifdef _WIN32
    unsigned long mode = 1;
    if (ioctlsocket(fd, FIONBIO, &mode) != 0)
    {
        PORT_WARNING("Failed setting non-blocking operation: %s", _portErrStr(port, 0));
        closesocket(fd);
        _winsockDeinit(port);
        return false;
    }
#else
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        PORT_WARNING("Failed getting flags: %s", _portErrStr(port, 0));
        close(fd);
        return false;
    }
    int res = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (res < 0)
    {
        PORT_WARNING("Failed setting flags: %s", _portErrStr(port, 0));
        close(fd);
        return false;
    }
#endif

    // Send immediately
#ifdef _WIN32
    const DWORD enable = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&enable, sizeof(enable)) != 0)
    {
        PORT_WARNING("Failed setting TCP_NODELAY option: %s", _portErrStr(port, 0));
        closesocket(fd);
        _winsockDeinit(port);
        return false;
    }
#else
    const int enable = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable)) != 0)
    {
        PORT_WARNING("Failed setting TCP_NODELAY option: %s", _portErrStr(port, 0));
        close(fd);
        return false;
    }
#endif

    // Remember socket
#ifdef _WIN32
    port->handle = (void *)fd;
#else
    port->fd = fd;
#endif

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static void _portCloseTcp(PORT_t *port)
{
#ifdef _WIN32
    closesocket((SOCKET)port->handle);
    _winsockDeinit(port);
#else
    close(port->fd);
#endif
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portWriteTcp(PORT_t *port, const uint8_t *data, const int size)
{
    int rem = size;
    int offs = 0;
    while (rem > 0)
    {
        const int sendSize = rem > TCPIP_MAX_PACKET_SIZE ? TCPIP_MAX_PACKET_SIZE : rem;

        // Throttle. The remote device will not have infinite buffers. So don't send faster than the remote
        // serial port at this baudrate can transmit. Assume 11 bits per character to be on the safe side.
        const int dt = TIME() - port->lastTime;
        const int reqDt = port->baudrate != 0 ? (port->lastCount * 1000 / (port->baudrate / 11)) : 0;
        const int wait = reqDt - dt;
        if (wait > 0)
        {
            SLEEP(wait);
        }

#ifdef _WIN32
        const int res = send((SOCKET)port->handle, (const char *)&data[offs], sendSize, 0);
#else
        const int res = send(port->fd, &data[offs], sendSize, 0);
#endif

        port->lastTime = TIME();
        port->lastCount = sendSize;

        PORT_XTRA_TRACE("tcp send (delay %d) %d -> %d", wait > 0 ? wait : 0, sendSize, res);
        // FIXME: handle EAGAIN?
        if ( (res < 0) || (res != sendSize) )
        {
            PORT_WARNING_THROTTLE("tcp send fail (%d, %d): %s", sendSize, res, _portErrStr(port, 0));
            return false;
        }
        offs += sendSize;
        rem  -= sendSize;
    }
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portReadTcp(PORT_t *port, uint8_t *data, const int size, int *nRead)
{
#ifdef _WIN32
    const int res = recv((SOCKET)port->handle, (char *)data, size, 0);
#else
    const int res = recv(port->fd, data, size, 0);
#endif
    PORT_XTRA_TRACE("tpc recv %d -> %d", size, res);
    // Got data
    if (res > 0)
    {
        *nRead = res;
        return true;
    }
    // No more data at the moment
#ifdef _WIN32
    else if ( (res == -1) && (GetLastError() == WSAEWOULDBLOCK) )
#else
    else if ( (res == -1) && (errno == EAGAIN) )
#endif
    {
        *nRead = 0;
        return true;
    }
    // Error
    else
    {
        PORT_WARNING_THROTTLE("tcp recv fail (%d, %d): %s", size, res, _portErrStr(port, 0));
        *nRead = 0;
        return false;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portCanBaudrateTcp(PORT_t *port)
{
    (void)port;
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portSetBaudrateTcp(PORT_t *port, const int baudrate)
{
    (void)port;
    (void)baudrate;
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static int _portGetBaudrateTcp(PORT_t *port)
{
    (void)port;
    return 0;
}

/* **** telnet + comport control **************************************************************** */

// RFC854: Telnet Protocol Specification (https://tools.ietf.org/html/rfc854.html)
// RFC855: Telnet Option Specifications (https://tools.ietf.org/html/rfc855.html)
#define TELNET_IAC             255 // command escape
#define TELNET_COMMAND_WILL    251 // option negotiation
#define TELNET_COMMAND_WONT    252 // option negotiation
#define TELNET_COMMAND_DO      253 // option negotiation
#define TELNET_COMMAND_DONT    254 // option negotiation
#define TELNET_COMMAND_SB      250 // suboption start
#define TELNET_COMMAND_SE      240 // suboption end
#define TELNET_COMMAND_NOP     241 // no operation
#define TELNET_COMMAND_DM      242 // data mark
#define TELNET_COMMAND_BRK     243 // terminal break
#define TELNET_COMMAND_IP      244 // interrupt process
#define TELNET_COMMAND_AO      245 // abort output
#define TELNET_COMMAND_AYT     246 // are you there
#define TELNET_COMMAND_EC      247 // erase character
#define TELNET_COMMAND_EL      248 // erase line
#define TELNET_COMMAND_GA      249 // go ahead

// RFC856: Telnet Binary Transmission (https://tools.ietf.org/html/rfc856.html)
// default: WON'T TRANSMIT-BINARY, DON'T TRANSMIT-BINARY
#define TELNET_OPTION_TRANSMIT_BINARY      0

// RFC857: Telnet Echo Option (https://tools.ietf.org/html/rfc857.html)
// default: WON'T ECHO, DON'T ECHO
#define TELNET_OPTION_ECHO                 1

// RFC858: Telnet Suppress Go Ahead Option (https://tools.ietf.org/html/rfc858.html)
// default: WON'T SUPPRESS-GO-AHEAD, DON'T SUPPRESS-GO-AHEAD
#define TELNET_OPTION_SUPPRESS_GO_AHEAD    3

// RFC859: Telnet Status Option (https://tools.ietf.org/html/rfc859.html)
// default: DON'T STATUS, WON'T STATUS
#define TELNET_OPTION_STATUS               5

// RFC2217: Telnet Com Port Control Option (https://tools.ietf.org/html/rfc2217.html)
#define TELNET_OPTION_COM_PORT_OPTION     44
#define TELNET_CPCO_SIGNATURE              0 // FIXME: is it 0? The RFC doesn't say..
#define TELNET_CPCO_SET_BAUDRATE           1
#define TELNET_CPCO_SET_DATASIZE           2
#define TELNET_CPCO_SET_PARITY             3
#define TELNET_CPCO_SET_STOPSIZE           4
#define TELNET_CPCO_SET_CONTROL            5
#define TELNET_CPCO_NOTIFY_LINESTATE       6
#define TELNET_CPCO_NOTIFY_MODEMSTATE      7
#define TELNET_CPCO_FLOWCONTROL_SUSPEND    8
#define TELNET_CPCO_FLOWCONTROL_RESUME     9
#define TELNET_CPCO_SET_LINESTATE_MASK    10
#define TELNET_CPCO_SET_MODEMSTATE_MASK   11
#define TELNET_CPCO_PURGE_DATA            12
#define TELNET_CPCO_SERVER_TO_CLIENT_OFFS 100

static const char *_telnetCommandStr(PORT_t *port, const uint8_t command)
{
    switch (command)
    {
        case TELNET_IAC:          return "IAC";
        case TELNET_COMMAND_WILL: return "WILL";
        case TELNET_COMMAND_WONT: return "WONT";
        case TELNET_COMMAND_DO:   return "DO";
        case TELNET_COMMAND_DONT: return "DONT";
        case TELNET_COMMAND_SB:   return "SB";
        case TELNET_COMMAND_SE:   return "SE";
        default:
            snprintf(port->tmp, sizeof(port->tmp), "%d", command);
            return port->tmp;
    }
}

static const char *_telnetOptionStr(PORT_t *port, const uint8_t option)
{
    switch (option)
    {
        case TELNET_OPTION_TRANSMIT_BINARY:   return "TRANSMIT_BINARY";
        case TELNET_OPTION_ECHO:              return "ECHO";
        case TELNET_OPTION_SUPPRESS_GO_AHEAD: return "SUPPRESS_GO_AHEAD";
        case TELNET_OPTION_STATUS:            return "STATUS";
        case TELNET_OPTION_COM_PORT_OPTION:   return "COM_PORT_OPTION";
        default:
            snprintf(port->tmp, sizeof(port->tmp), "%d", option);
            return port->tmp;
    }
}

static const char *_telnetCpcoStr(PORT_t *port, const uint8_t cpco)
{
    const uint8_t _cpco = cpco >= TELNET_CPCO_SERVER_TO_CLIENT_OFFS ?
        cpco - TELNET_CPCO_SERVER_TO_CLIENT_OFFS : cpco;
    switch (_cpco)
    {
        case TELNET_CPCO_SIGNATURE:           return "SIGNATURE";
        case TELNET_CPCO_SET_BAUDRATE:        return "SET_BAUDRATE";
        case TELNET_CPCO_SET_DATASIZE:        return "SET_DATASIZE";
        case TELNET_CPCO_SET_PARITY:          return "SET_PARITY";
        case TELNET_CPCO_SET_STOPSIZE:        return "SET_STOPSIZE";
        case TELNET_CPCO_SET_CONTROL:         return "SET_CONTROL";
        case TELNET_CPCO_NOTIFY_LINESTATE:    return "NOTIFY_LINESTATE";
        case TELNET_CPCO_NOTIFY_MODEMSTATE:   return "NOTIFY_MODEMSTATE";
        case TELNET_CPCO_FLOWCONTROL_SUSPEND: return "FLOWCONTROL_SUSPEND";
        case TELNET_CPCO_FLOWCONTROL_RESUME:  return "FLOWCONTROL_RESUME";
        case TELNET_CPCO_SET_LINESTATE_MASK:  return "SET_LINESTATE_MASK";
        case TELNET_CPCO_SET_MODEMSTATE_MASK: return "SET_MODEMSTATE_MASK";
        case TELNET_CPCO_PURGE_DATA:          return "PURGE_DATA";
        default:
            snprintf(port->tmp, sizeof(port->tmp), "%d", cpco);
            return port->tmp;
    }
}

// options to negotiate
typedef struct TELNET_OPTION_s
{
    uint8_t command;
    uint8_t option;
    bool    agree;
} TELNET_OPTION_t;

typedef struct TELNET_CPCO_s
{
    uint8_t cpco;
    uint8_t arg;
} TELNET_CPCO_t;

// Bug in some serial port server doesn't like more than 8 IACs in a row (in a packet?)
//#define TELNET_SEND_MAX_IAC   8

enum { TELNET_STATE_NORMAL, TELNET_STATE_IAC, TELNET_STATE_COMMAND, TELNET_STATE_SUBOPTION };

void _telnetProcessInband(PORT_t *port, uint8_t *buf, int nIn, int *nOut, TELNET_OPTION_t *options, const int nOptions)
{
    //TRACE_HEXDUMP(buf, nIn, "nIn=%d", nIn);

    int      state   = port->tnState;
    uint8_t *inband  = port->tnInband;
    int      nInband = port->nTnInband;

    const uint8_t *pIn = buf;
    uint8_t *out = buf;
    int nO = 0;
    while (nIn > 0)
    {
        //DEBUG("nIn=%d *pIn=%d 0x%02x", nIn, *pIn, *pIn);
        switch (state)
        {
            case TELNET_STATE_NORMAL:
                if (*pIn == TELNET_IAC)
                {
                    state = TELNET_STATE_IAC;
                }
                else // normal data
                {
                    out[nO++] = *pIn;
                }
                break;
            case TELNET_STATE_IAC:
                switch (*pIn)
                {
                    case TELNET_IAC: // escaped IAC
                        out[nO++] = TELNET_IAC;
                        state = TELNET_STATE_NORMAL; // back to normal
                        break;
                    case TELNET_COMMAND_WILL:
                    case TELNET_COMMAND_WONT:
                    case TELNET_COMMAND_DO:
                    case TELNET_COMMAND_DONT:
                        nInband = 0;
                        inband[nInband++] = *pIn;
                        state = TELNET_STATE_COMMAND; // expecting option code
                        break;
                    case TELNET_COMMAND_SB:
                        nInband = 0;
                        inband[nInband++] = *pIn;
                        state = TELNET_STATE_SUBOPTION; // expection suboptions
                        break;
                    default:
                        PORT_TRACE("inband command %s", _telnetCommandStr(port, *pIn));
                        state = TELNET_STATE_NORMAL;
                        break;
                }
                break;
            case TELNET_STATE_COMMAND:
                inband[nInband++] = *pIn;
                PORT_XTRA_TRACE("they say: %-4s %s", _telnetCommandStr(port, inband[0]), _telnetOptionStr(port, inband[1]));
                for (int ix = 0; ix < nOptions; ix++)
                {
                    if (inband[1] == options[ix].option)
                    {
                        // they say... we say...
                        switch (inband[0])
                        {
                            case TELNET_COMMAND_WILL:
                                switch (options[ix].command)
                                {
                                    case TELNET_COMMAND_DO:   options[ix].agree = true; break;
                                    case TELNET_COMMAND_DONT: options[ix].agree = false; break;
                                }
                                break;
                            case TELNET_COMMAND_WONT:
                                switch (options[ix].command)
                                {
                                    case TELNET_COMMAND_DO:   options[ix].agree = false; break;
                                    case TELNET_COMMAND_DONT: options[ix].agree = true; break;
                                }
                                break;
                            case TELNET_COMMAND_DO:
                                switch (options[ix].command)
                                {
                                    case TELNET_COMMAND_WILL: options[ix].agree = true; break;
                                    case TELNET_COMMAND_WONT: options[ix].agree = false; break;
                                }
                                break;
                            case TELNET_COMMAND_DONT:
                                switch (options[ix].command)
                                {
                                    case TELNET_COMMAND_WILL: options[ix].agree = false; break;
                                    case TELNET_COMMAND_WONT: options[ix].agree = true; break;
                                }
                                break;
                        }
                    }
                }
                state = TELNET_STATE_NORMAL;
                break;
            case TELNET_STATE_SUBOPTION:
                // Ignore second IAC in IAC-IAC sequence
                if ( (*pIn == TELNET_IAC) && (nInband > 0) && (inband[nInband - 1] == TELNET_IAC) )
                {
                    break;
                }
                // We can only digest so and so much.. warn once
                if (nInband == sizeof(port->tnInband))
                {
                    PORT_WARNING("Too much telnet inband data!");
                    nInband = -1;
                }
                // Overflow case: wait for IAC SE sequence that marks the end of the suboption data
                if (nInband < 0)
                {
                    // We have an IAC, so...
                    if ( (nInband == -1) && (*pIn == TELNET_IAC) )
                    {
                        nInband = -2;
                    }
                    else if (nInband == -2)
                    {
                        // ...if the next byte is SE, we're done
                        if (*pIn == TELNET_COMMAND_SE)
                        {
                            state = TELNET_STATE_NORMAL;
                        }
                        // otherwise go back waiting for IAC
                        else if (*pIn != TELNET_IAC)
                        {
                            nInband = -1;
                        }
                        // else: this was another IAC, so try again...
                    }
                }
                // Collect more data until sequence is complete
                else
                {
                    inband[nInband++] = *pIn;
                    if ( (inband[nInband - 2] == TELNET_IAC) && (inband[nInband - 1] == TELNET_COMMAND_SE) )
                    {
                        if (nInband > 2)
                        {
                            nInband -= 2;
                            PORT_XTRA_TRACE("they say: %-4s %s (+%d)", _telnetCommandStr(port, inband[0]), _telnetOptionStr(port, inband[1]), nInband - 2);
                            if (inband[1] == TELNET_OPTION_COM_PORT_OPTION)
                            {
                                PORT_TRACE("inband CPCO %s %d (0x%02x)", _telnetCpcoStr(port, inband[2]), inband[3], inband[3]);
                            }
                        }
                        // else: inband[] overflow, discard
                        state = TELNET_STATE_NORMAL;
                    }
                }
                break;
        }
        nIn--;
        pIn++;
    }

    port->tnState   = state;
    port->nTnInband = nInband;
    *nOut = nO;
}

static bool _portOpenTelnet(PORT_t *port)
{
    if (!_portOpenTcp(port))
    {
        return false;
    }
    port->tnState = TELNET_STATE_NORMAL;
    port->nTnInband = 0;

    uint8_t buf[TCPIP_MAX_PACKET_SIZE];

    // Our offer
    TELNET_OPTION_t telnetOptions[] =
    {
      //{ .agree = false, .command = TELNET_COMMAND_WILL, .option = TELNET_OPTION_SUPPRESS_GO_AHEAD },
        { .agree = false, .command = TELNET_COMMAND_WILL, .option = TELNET_OPTION_TRANSMIT_BINARY },
      //{ .agree = false, .command = TELNET_COMMAND_WONT, .option = TELNET_OPTION_ECHO },
        { .agree = false, .command = TELNET_COMMAND_WILL, .option = TELNET_OPTION_COM_PORT_OPTION },
      //{ .agree = false, .command = TELNET_COMMAND_DO,   .option = TELNET_OPTION_SUPPRESS_GO_AHEAD },
        { .agree = false, .command = TELNET_COMMAND_DO,   .option = TELNET_OPTION_TRANSMIT_BINARY },
      //{ .agree = false, .command = TELNET_COMMAND_DONT, .option = TELNET_OPTION_ECHO },
        { .agree = false, .command = TELNET_COMMAND_DO,   .option = TELNET_OPTION_COM_PORT_OPTION }
    };

    // Announce our offer
    for (int ix = 0; ix < NUMOF(telnetOptions); ix++)
    {
        buf[(ix * 3) + 0] = TELNET_IAC;
        buf[(ix * 3) + 1] = telnetOptions[ix].command;
        buf[(ix * 3) + 2] = telnetOptions[ix].option;
        PORT_XTRA_TRACE("we offer: %-4s %s", _telnetCommandStr(port, telnetOptions[ix].command), _telnetOptionStr(port, telnetOptions[ix].option));
    }
    if (!_portWriteTcp(port, buf, 3 * NUMOF(telnetOptions)))
    {
        _portCloseTcp(port);
        return false;
    }

    // Negotiate, process their answer
    bool happy = false;
    bool timeout = false;
    const uint32_t t0 = TIME();
    const uint32_t t1 = t0 + 1500;
    while (!happy && !timeout)
    {
        if (TIME() > t1)
        {
            timeout = true;
            break;
        }

        int nRead = 0;
        if (!_portReadTcp(port, buf, sizeof(buf), &nRead))
        {
            break;
        }
        if (nRead == 0)
        {
            SLEEP(10);
            continue;
        }

        int nOut = 0;
        _telnetProcessInband(port, buf, nRead, &nOut, telnetOptions, NUMOF(telnetOptions));

        PORT_XTRA_TRACE("State of negotiations:");
        happy = true;
        for (int ix = 0; ix < NUMOF(telnetOptions); ix++)
        {
            PORT_XTRA_TRACE("%d %-4s %-20s %s", ix, _telnetCommandStr(port, telnetOptions[ix].command),
                _telnetOptionStr(port, telnetOptions[ix].option), telnetOptions[ix].agree ? ":-)" : ":-(");
            if (!telnetOptions[ix].agree)
            {
                happy = false;
            }
        }
        PORT_XTRA_TRACE("We %s happy", happy ? "are" : "are not (yet)");
    }
    if (!happy || timeout)
    {
        if (timeout)
        {
            PORT_WARNING("Timeout negotiating telnet options!");
        }
        else
        {
            PORT_WARNING("Failed negotiating telnet options!");
        }
        for (int ix = 0; ix < NUMOF(telnetOptions); ix++)
        {
            if (!telnetOptions[ix].agree)
            {
                PORT_DEBUG("Not agreeing on: %s %s", _telnetCommandStr(port, telnetOptions[ix].command),
                _telnetOptionStr(port, telnetOptions[ix].option));
            }
        }
        _portCloseTcp(port);
        return false;
    }

    // Configure port
    static const TELNET_CPCO_t kTelnetCpco[] =
    {
        { .cpco = TELNET_CPCO_SET_DATASIZE,        .arg = 8 }, // 8
        { .cpco = TELNET_CPCO_SET_PARITY,          .arg = 1 }, // N
        { .cpco = TELNET_CPCO_SET_STOPSIZE,        .arg = 1 }, // 1
        { .cpco = TELNET_CPCO_SET_CONTROL,         .arg = 1 }, // no flow control
        { .cpco = TELNET_CPCO_SET_MODEMSTATE_MASK, .arg = 0 },
        { .cpco = TELNET_CPCO_SET_LINESTATE_MASK,  .arg = 0 }  // FIXME: maybe we should watch some of these?
    };
    for (int ix = 0; ix < NUMOF(kTelnetCpco); ix++)
    {
        buf[(ix * 6) + 0] = TELNET_IAC;
        buf[(ix * 6) + 1] = TELNET_COMMAND_SB;
        buf[(ix * 6) + 2] = TELNET_OPTION_COM_PORT_OPTION;
        buf[(ix * 6) + 3] = kTelnetCpco[ix].cpco;
        buf[(ix * 6) + 4] = kTelnetCpco[ix].arg;
        buf[(ix * 6) + 5] = TELNET_COMMAND_SE;
    }
    if (!_portWriteTcp(port, buf, 6 * NUMOF(kTelnetCpco)) ||
        !_portSetBaudrateTelnet(port, port->baudrate))
    {
        PORT_WARNING("Failed configuring port!");
        _portCloseTcp(port);
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static void _portCloseTelnet(PORT_t *port)
{
    _portCloseTcp(port);
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portWriteTelnet(PORT_t *port, const uint8_t *data, const int size)
{
    uint8_t buf[TCPIP_MAX_PACKET_SIZE];
    int nData = size;
    const uint8_t *pData = data;
    int nBuf = 0;
    int nIac = 0;
    while (nData > 0)
    {
        // Fill buf, escape IAC
        if (*pData == TELNET_IAC)
        {
            // Enough space for 2 x IAC
            if (nBuf < (int)(sizeof(buf) - 1))
            {
                buf[nBuf++] = TELNET_IAC;
                buf[nBuf++] = TELNET_IAC;
                pData++;
                nData--;
#ifdef TELNET_SEND_MAX_IAC
                nIac++;
#endif
            }
            // Only one byte left, trigger send
            else
            {
                nIac = -1;
            }
        }
        // Fill buf, normal byte
        else
        {
            buf[nBuf++] = *pData;
            pData++;
            nData--;
        }

        // Send data now?
        if ( (nBuf >= (int)sizeof(buf)) ||
#ifdef TELNET_SEND_MAX_IAC
             (nIac >= TELNET_SEND_MAX_IAC) ||
#endif
             (nIac < 0) || ((nData <= 0) && (nBuf > 0)) )
        {
            PORT_XTRA_TRACE("telnet send %d (nIac=%d)", nBuf, nIac);
            if (!_portWriteTcp(port, buf, nBuf))
            {
                return false;
            }
            nBuf = 0;
            nIac = 0;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portReadTelnet(PORT_t *port, uint8_t *data, const int size, int *nRead)
{
    int nIn = 0;
    if (_portReadTcp(port, data, size, &nIn))
    {
       int nOut = 0;
        _telnetProcessInband(port, data, nIn, &nOut, NULL, 0);
        *nRead = nOut;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portCanBaudrateTelnet(PORT_t *port)
{
    (void)port;
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portSetBaudrateTelnet(PORT_t *port, const int baudrate)
{
    const uint32_t b = (uint32_t)baudrate;
    uint8_t buf[] =
    {
        TELNET_IAC, TELNET_COMMAND_SB,
        TELNET_OPTION_COM_PORT_OPTION, TELNET_CPCO_SET_BAUDRATE,
        ((b >> 24) & 0xff),
        ((b >> 16) & 0xff),
        ((b >>  8) & 0xff),
        ( b        & 0xff),
        TELNET_IAC, TELNET_COMMAND_SE
    };
    const bool res = _portWriteTcp(port, buf, sizeof(buf));
    if (res)
    {
        port->baudrate = baudrate;
    }
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

static int _portGetBaudrateTelnet(PORT_t *port)
{
    return port->baudrate;
}

#if 0
/* ***** template ******************************************************************************* */

static bool _portOpenXyz(PORT_t *port)
{
    (void)port;
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

static void _portCloseXyz(PORT_t *port)
{
    (void)port;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portWriteXyz(PORT_t *port, const uint8_t *data, const int size)
{
    (void)port;
    (void)data;
    (void)size;
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portReadXyz(PORT_t *port, uint8_t *data, const int size, int *nRead)
{
    (void)port;
    (void)data;
    (void)size;
    (void)nRead;
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portCanBaudrateXyz(PORT_t *port)
{
    (void)port;
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portSetBaudrateXyz(PORT_t *port, const int baudrate)
{
    (void)port;
    (void)baudrate;
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

static int _portGetBaudrateXyz(PORT_t *port)
{
    (void)port;
    return 0;
}

/* ****************************************************************************************************************** */

static bool _portOpenXyz(PORT_t *port)
{
    (void)port;
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

static void _portCloseXyz(PORT_t *port)
{
    (void)port;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portWriteXyz(PORT_t *port, const uint8_t *data, const int size)
{
    (void)port;
    (void)data;
    (void)size;
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portReadXyz(PORT_t *port, uint8_t *data, const int size, int *nRead)
{
    (void)port;
    (void)data;
    (void)size;
    (void)nRead;
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portCanBaudrateXyz(PORT_t *port)
{
    (void)port;
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portSetBaudrateXyz(PORT_t *port, const int baudrate)
{
    (void)port;
    (void)baudrate;
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

static int _portGetBaudrateXyz(PORT_t *port)
{
    (void)port;
    return 0;
}
#endif
/* ****************************************************************************************************************** */
// eof
