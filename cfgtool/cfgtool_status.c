// u-blox 9 positioning receivers configuration tool
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
#include <signal.h>
#include <unistd.h>

#include "cfgtool_util.h"

#include "ff_rx.h"
#include "ff_ubx.h"
#include "ff_epoch.h"

#include "cfgtool_status.h"

/* ********************************************************************************************** */

const char *statusHelp(void)
{
    return
// -----------------------------------------------------------------------------
"Command 'status':\n"
"\n"
"    Usage: cfgtool status -p <port> [-n] [-x]\n"
"\n";
}

/* ********************************************************************************************** */

bool gAbort;

static void _sigHandler(int signal)
{
    if ( (signal == SIGINT) || (signal == SIGTERM) NOT_WIN(|| (signal == SIGHUP)) )
    {
        PRINT("Aborting...");
        gAbort = true;
    }
}

typedef struct INFO_s
{
    uint32_t       nEpoch;
    uint32_t       nNmea;
    uint32_t       nUbx;
    uint32_t       nRtcm;
    uint32_t       nGarb;
    uint32_t       nMsgs;
} INFO_t;

typedef enum STATUS_COLOUR_e
{
    STATUS_COLOUR_NOFIX, STATUS_COLOUR_MASKED, STATUS_COLOUR_FIXOK,
    STATUS_COLOUR_RTKNONE, STATUS_COLOUR_RTKFLOAT, STATUS_COLOUR_RTKFIXED,
    STATUS_COLOUR_OFF
} STATUS_COLOUR_t;

static void _statusColour(const STATUS_COLOUR_t colour)
{
#ifdef _WIN32
    switch (colour)
    {
        case STATUS_COLOUR_NOFIX:
        case STATUS_COLOUR_RTKNONE:
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED |                                      FOREGROUND_INTENSITY);
            break;
        case STATUS_COLOUR_MASKED:
        case STATUS_COLOUR_RTKFLOAT:
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED |                    FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            break;
        case STATUS_COLOUR_FIXOK:
        case STATUS_COLOUR_RTKFIXED:
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),                  FOREGROUND_GREEN |                   FOREGROUND_INTENSITY);
            break;
        case STATUS_COLOUR_OFF:
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            break;
    }
#else
    switch (colour)
    {
        case STATUS_COLOUR_NOFIX:
        case STATUS_COLOUR_RTKNONE:
            ioOutputStr("\e[1;31m");
            break;
        case STATUS_COLOUR_MASKED:
        case STATUS_COLOUR_RTKFLOAT:
            ioOutputStr("\e[1;36m");
            break;
        case STATUS_COLOUR_FIXOK:
        case STATUS_COLOUR_RTKFIXED:
            ioOutputStr("\e[1;32m");
            break;
        case STATUS_COLOUR_OFF:
            ioOutputStr("\e[m");
            break;
    }
#endif
}

#define COLOUR(col) if (colours) { _statusColour(CONCAT(STATUS_COLOUR_, col)); }

static bool _printInfo(const bool colours, const INFO_t *info, const EPOCH_t *epoch)
{
    ioOutputStr("%4u %4u %4u %4u %5u | ",
        info->nUbx, info->nNmea, info->nRtcm, info->nGarb, epoch != NULL ? epoch->seq : 0);

    const char *epochStr = "no navigation data";

    if (epoch != NULL)
    {
        switch (epoch->fix)
        {
            case EPOCH_FIX_NOFIX:
                COLOUR(NOFIX);
                break;
            case EPOCH_FIX_DRONLY:
            case EPOCH_FIX_2D:
            case EPOCH_FIX_3D:
            case EPOCH_FIX_3D_DR:
            case EPOCH_FIX_TIME:
                if (epoch->fixOk)
                {
                    COLOUR(FIXOK);
                }
                else
                {
                    COLOUR(MASKED);
                }
                break;
        }
    
        epochStr = epoch->str;
    }

    ioOutputStr("%s", epochStr);

    COLOUR(OFF);

    ioOutputStr("\n");
    return ioWriteOutput(true);
}

int statusRun(const char *portArg, const bool extraInfo, const bool noProbe)
{
    RX_ARGS_t args = RX_ARGS_DEFAULT();
    if (noProbe)
    {
        args.autobaud = false;
        args.detect   = false;
    }
    RX_t *rx = rxInit(portArg, &args);
    if ( (rx == NULL) || !rxOpen(rx) )
    {
        free(rx);
        return EXIT_RXFAIL;
    }

    const bool colours = isatty(fileno(stderr)) == 1;

    (void)extraInfo;

    INFO_t info;
    memset(&info, 0, sizeof(info));

    gAbort = false;
    signal(SIGINT, _sigHandler);
    signal(SIGTERM, _sigHandler);
    NOT_WIN( signal(SIGHUP, _sigHandler) );

    EPOCH_t coll;
    EPOCH_t epoch;
    epochInit(&coll);

    PRINT("Dumping receiver status...");

    ioOutputStr("UBX  NMEA RTCM GARB Epoch | %s\n", epochStrHeader());
    ioWriteOutput(false);

    uint32_t lastEpoch = TIME();

    while (!gAbort)
    {
        const uint32_t now = TIME();
        PARSER_MSG_t *msg = rxGetNextMessage(rx);
        if (msg != NULL)
        {
            info.nMsgs++;
            if (epochCollect(&coll, msg, &epoch))
            {
                info.nEpoch++;
                lastEpoch = now;
                if (!_printInfo(colours, &info, &epoch))
                {
                    break;
                }
            }
            switch (msg->type)
            {
                case PARSER_MSGTYPE_UBX:
                    info.nUbx++;
                    break;
                case PARSER_MSGTYPE_NMEA:
                    info.nNmea++;
                    break;
                case PARSER_MSGTYPE_RTCM3:
                    info.nRtcm++;
                    break;
                case PARSER_MSGTYPE_GARBAGE:
                    info.nGarb++;
                    break;
            }
        }
        // No data, yield
        else
        {
            SLEEP(5);
        }
        if ( (now - lastEpoch) > 5000 )
        {
            _printInfo(colours, &info, NULL);
            lastEpoch = now;
        }
    }
    bool res = ioWriteOutput(true);

    rxClose(rx);
    free(rx);

    return res ? (info.nMsgs > 0 ? EXIT_SUCCESS : EXIT_RXNODATA) : EXIT_OTHERFAIL;
}

/* ********************************************************************************************** */
// eof
