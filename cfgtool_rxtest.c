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

#include "cfgtool.h"

#include "ff_rx.h"
#include "ff_ubx.h"

#include "cfgtool_rxtest.h"

/* ********************************************************************************************** */

const char *rxtestHelp(void)
{
    return
// -----------------------------------------------------------------------------
"Command 'rxtest':\n"
"\n"
"    Usage: cfgtool rxtest -p <port> [-o <outfile>] [-y] [-x]\n"
"\n"
"    Connects to the receiver and outputs information on the received messages\n"
"    and optionally a hex dump of the messages until SIGINT (e.g. CTRL-C)"NOT_WIN(", SIGHUP")"\n"
"    or SIGTERM is received.\n"
"\n"
"    Returns success (0) if receiver was detected and at least one message was\n"
"    received. Otherwise returns "STRINGIFY(EXIT_RXFAIL)" (rx not detected) or "STRINGIFY(EXIT_RXNODATA)" (no messages).\n"
"\n"
"    Examples:\n"
"\n"
#ifdef _WIN32
"        cfgtool rxtest -p COM3\n"
#else
"        cfgtool rxtest -p /dev/ttyUSB0\n"
"        timeout 20 cfgtool rxtest -p /dev/ttyACM0\n"
#endif
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

int rxtestRun(const char *portArg, const bool extraInfo)
{
    RX_t *rx = rxOpen(portArg, NULL);
    if (rx == NULL)
    {
        return EXIT_RXFAIL;
    }

    uint32_t nNmea = 0, sNmea = 0;
    uint32_t nUbx  = 0, sUbx  = 0;
    uint32_t nRtcm = 0, sRtcm = 0;
    uint32_t nGarb = 0, sGarb = 0;
    uint32_t nMsgs = 0, sMsgs = 0;

    gAbort = false;
    signal(SIGINT, _sigHandler);
    signal(SIGTERM, _sigHandler);
    NOT_WIN( signal(SIGHUP, _sigHandler) );

    PRINT("Dumping received data...");
    while (!gAbort)
    {

        PARSER_MSG_t *msg = rxGetNextMessage(rx);
        if (msg != NULL)
        {
            nMsgs++;
            sMsgs += msg->size;
            const char *prot = "?";
            switch (msg->type)
            {
                case PARSER_MSGTYPE_UBX:
                    prot = "UBX";
                    nUbx++;
                    sUbx += msg->size;
                    break;
                case PARSER_MSGTYPE_NMEA:
                    prot = "NMEA";
                    nNmea++;
                    sNmea += msg->size;
                    break;
                case PARSER_MSGTYPE_RTCM3:
                    prot = "RTMC3";
                    nRtcm++;
                    sRtcm += msg->size;
                    break;
                case PARSER_MSGTYPE_GARBAGE:
                    prot = "GARBAGE";
                    nGarb++;
                    sGarb += msg->size;
                    break;
            }
            addOutputStr("message %4u, size %4d, %-8s %-20s %s\n",
                msg->seq, msg->size, prot, msg->name, msg->info != NULL ? msg->info : "n/a");
            if (extraInfo)
            {
                addOutputHexdump(msg->data, msg->size);
            }
            if (!writeOutput(nMsgs == 1 ? false : true))
            {
                break;
            }
        }
        // No data, yield
        else
        {
            SLEEP(100);
        }
    }

    addOutputStr("stats UBX      count %5u (%5.1f%%)  size %10u (%5.1f%%)\n", nUbx,  nMsgs > 0 ? (double)nUbx  / (double)nMsgs * 1e2 : 0.0, sUbx,  sMsgs > 0 ? (double)sUbx  / (double)sMsgs * 1e2 : 0.0);
    addOutputStr("stats NMEA     count %5u (%5.1f%%)  size %10u (%5.1f%%)\n", nNmea, nMsgs > 0 ? (double)nNmea / (double)nMsgs * 1e2 : 0.0, sNmea, sMsgs > 0 ? (double)sNmea / (double)sMsgs * 1e2 : 0.0);
    addOutputStr("stats RTCM3    count %5u (%5.1f%%)  size %10u (%5.1f%%)\n", nUbx,  nMsgs > 0 ? (double)nRtcm / (double)nMsgs * 1e2 : 0.0, sRtcm, sMsgs > 0 ? (double)sRtcm / (double)sMsgs * 1e2 : 0.0);
    addOutputStr("stats GARBAGE  count %5u (%5.1f%%)  size %10u (%5.1f%%)\n", nGarb, nMsgs > 0 ? (double)nGarb / (double)nMsgs * 1e2 : 0.0, sGarb, sMsgs > 0 ? (double)sGarb / (double)sMsgs * 1e2 : 0.0);
    addOutputStr("stats Total    count %5u (100.0%%)  size %10u (100.0%%)\n", nMsgs, sMsgs);
    bool res = writeOutput(true);

    rxClose(rx);

    return res ? (nMsgs > 0 ? EXIT_SUCCESS : EXIT_RXNODATA) : EXIT_OTHERFAIL;
}

/* ********************************************************************************************** */
// eof
