/* ************************************************************************************************/ // clang-format off
// u-blox 9 positioning receivers configuration tool
//
// Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org),
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

#include "cfgtool_util.h"

#include "ff_rx.h"
#include "ff_ubx.h"
#include "ff_epoch.h"

#include "cfgtool_dump.h"

/* ****************************************************************************************************************** */

const char *dumpHelp(void)
{
    return
// -----------------------------------------------------------------------------
"Command 'dump':\n"
"\n"
"    Usage: cfgtool dump -p <port> [-o <outfile>] [-y] [-x] [-n]\n"
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
"        cfgtool dump -p COM3\n"
#else
"        cfgtool dump -p /dev/ttyUSB0\n"
"        timeout 20 cfgtool dump -p /dev/ttyACM0\n"
#endif
"\n";
}

/* ****************************************************************************************************************** */

bool gAbort;

static void _sigHandler(int signal)
{
    if ( (signal == SIGINT) || (signal == SIGTERM) NOT_WIN(|| (signal == SIGHUP)) )
    {
        PRINT("Aborting...");
        gAbort = true;
    }
}

int dumpRun(const char *portArg, const bool extraInfo, const bool noProbe)
{
    RX_OPTS_t opts = RX_OPTS_DEFAULT();
    if (noProbe)
    {
        opts.autobaud = false;
        opts.detect   = RX_DET_NONE;
    }

    RX_t *rx = rxInit(portArg, &opts);
    if ( (rx == NULL) || !rxOpen(rx) )
    {
        free(rx);
        return EXIT_RXFAIL;
    }

    gAbort = false;
    signal(SIGINT, _sigHandler);
    signal(SIGTERM, _sigHandler);
    NOT_WIN( signal(SIGHUP, _sigHandler) );

    const uint32_t tOffs = TIME() - timeOfDay(); // Offset between wall clock and parser time reference
    uint32_t nEpochs = 0;
    EPOCH_t coll;
    EPOCH_t epoch;
    epochInit(&coll);

    PRINT("Dumping received data...");
    const PARSER_t *parser = rxGetParser(rx);
    while (!gAbort)
    {
        PARSER_MSG_t *msg = rxGetNextMessage(rx);
        if (msg != NULL)
        {
            const uint32_t latency = (msg->ts - tOffs) % 1000; // Relative to wall clock top of second
            if (epochCollect(&coll, msg, &epoch))
            {
                nEpochs++;
                ioOutputStr("epoch %4u, %s\n", epoch.seq, epoch.str);
                if (!ioWriteOutput(parser->nMsgs == 1 ? false : true))
                {
                    break;
                }
            }
            ioOutputStr("message %4u, dt %4u, size %4d, %-8s %-20s %s\n",
                msg->seq, latency, msg->size, parserMsgtypeName(msg->type), msg->name, msg->info != NULL ? msg->info : "n/a");
            if (extraInfo)
            {
                ioAddOutputHexdump(msg->data, msg->size);
            }
            if (!ioWriteOutput(parser->nMsgs == 1 ? false : true))
            {
                break;
            }
        }
        // No data, yield
        else
        {
            SLEEP(5);
        }
    }

    ioOutputStr("stats UBX      count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", parser->nUbx,     parser->nMsgs > 0 ? (double)parser->nUbx     / (double)parser->nMsgs * 1e2 : 0.0, parser->sUbx,     parser->sMsgs > 0 ? (double)parser->sUbx     / (double)parser->sMsgs * 1e2 : 0.0);
    ioOutputStr("stats NMEA     count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", parser->nNmea,    parser->nMsgs > 0 ? (double)parser->nNmea    / (double)parser->nMsgs * 1e2 : 0.0, parser->sNmea,    parser->sMsgs > 0 ? (double)parser->sNmea    / (double)parser->sMsgs * 1e2 : 0.0);
    ioOutputStr("stats RTCM3    count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", parser->nRtcm3,   parser->nMsgs > 0 ? (double)parser->nRtcm3   / (double)parser->nMsgs * 1e2 : 0.0, parser->sRtcm3,   parser->sMsgs > 0 ? (double)parser->sRtcm3   / (double)parser->sMsgs * 1e2 : 0.0);
    ioOutputStr("stats SPARTN   count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", parser->nSpartn,  parser->nMsgs > 0 ? (double)parser->nSpartn  / (double)parser->nMsgs * 1e2 : 0.0, parser->sSpartn,  parser->sMsgs > 0 ? (double)parser->sSpartn  / (double)parser->sMsgs * 1e2 : 0.0);
    ioOutputStr("stats NOVATEL  count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", parser->nUbx,     parser->nMsgs > 0 ? (double)parser->nNovatel / (double)parser->nMsgs * 1e2 : 0.0, parser->sNovatel, parser->sMsgs > 0 ? (double)parser->sNovatel / (double)parser->sMsgs * 1e2 : 0.0);
    ioOutputStr("stats GARBAGE  count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", parser->nGarbage, parser->nMsgs > 0 ? (double)parser->nGarbage / (double)parser->nMsgs * 1e2 : 0.0, parser->sGarbage, parser->sMsgs > 0 ? (double)parser->sGarbage / (double)parser->sMsgs * 1e2 : 0.0);
    ioOutputStr("stats Total    count %6u (100.0%%)  size %10u (100.0%%)\n", parser->nMsgs, parser->sMsgs);
    ioOutputStr("stats EPOCH    count %6u (%5.1f%%)\n", nEpochs, parser->nMsgs > 0 ? (double)nEpochs / (double)parser->nMsgs * 1e2 : 0.0);

    bool res = ioWriteOutput(true);
    const uint32_t nMsgs = parser->nMsgs;

    rxClose(rx);
    free(rx);

    return res ? (nMsgs > 0 ? EXIT_SUCCESS : EXIT_RXNODATA) : EXIT_OTHERFAIL;
}

/* ****************************************************************************************************************** */
// eof
