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

#include "ff_ubx.h"
#include "ff_parser.h"
#include "ff_epoch.h"

#include "cfgtool_parse.h"

/* ****************************************************************************************************************** */

const char *parseHelp(void)
{
    return
// -----------------------------------------------------------------------------
"Command 'parse':\n"
"\n"
"    Usage: cfgtool parse [-i <infile>] [-o <outfile>] [-y] [-x] [-e]\n"
"\n"
"    This processes data from the input file through the parser and outputs\n"
"    information on the found messages and optionally a hex dump of the messages.\n"
"    It stops at the end of the file or when SIGINT (e.g. CTRL-C)" NOT_WIN(", SIGHUP") "\n"
"    or SIGTERM is received.\n"
"\n"
"    Add -e to enable epoch detection and to output detected epochs.\n"
"\n";
}

/* ****************************************************************************************************************** */

static bool gAbort;

static void _sigHandler(int signal)
{
    if ( (signal == SIGINT) || (signal == SIGTERM) NOT_WIN(|| (signal == SIGHUP)) )
    {
        PRINT("Aborting...");
        gAbort = true;
    }
}

int parseRun(const bool extraInfo, const bool doEpoch)
{
    uint32_t nEpochs = 0;

    gAbort = false;
    signal(SIGINT, _sigHandler);
    signal(SIGTERM, _sigHandler);
    NOT_WIN( signal(SIGHUP, _sigHandler) );

    PARSER_t parser;
    parserInit(&parser);

    EPOCH_t coll;
    EPOCH_t epoch;
    PARSER_MSG_t msg;
    epochInit(&coll);
    bool done = false;
    while (!(gAbort || done))
    {
        uint8_t buf[1000];
        const int num = ioReadInput(buf, sizeof(buf));
        if (num < 0) // eof
        {
            done = true;
        }
        else if (num == 0) // wait
        {
            SLEEP(5);
            continue;
        }
        if (num > 0)
        {
            parserAdd(&parser, buf, num);
        }

        while (parserProcess(&parser, &msg, true))
        {
            if (doEpoch && epochCollect(&coll, &msg, &epoch))
            {
                nEpochs++;
                ioOutputStr("epoch   %4d, size    0, NONE     EPOCH                %s\n", nEpochs, epoch.str);
            }
            ioOutputStr("message %4u, size %4d, %-8s %-20s %s\n",
                msg.seq, msg.size, parserMsgtypeName(msg.type), msg.name, msg.info != NULL ? msg.info : "n/a");
            if (extraInfo)
            {
                ioAddOutputHexdump(msg.data, msg.size);
            }
            if (!ioWriteOutput(parser.nMsgs == 1 ? false : true))
            {
                break;
            }
        }
    }

    // Anything left in parser?
    if (parserFlush(&parser, &msg))
    {
        ioOutputStr("message %4u, size %4d, %-8s %-20s %s\n",
            msg.seq, msg.size, parserMsgtypeName(msg.type), msg.name, msg.info != NULL ? msg.info : "n/a");
        if (extraInfo)
        {
            ioAddOutputHexdump(msg.data, msg.size);
        }
        ioWriteOutput(true);
    }

    ioOutputStr("stats UBX      count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", parser.nUbx,     parser.nMsgs > 0 ? (double)parser.nUbx     / (double)parser.nMsgs * 1e2 : 0.0, parser.sUbx,     parser.sMsgs > 0 ? (double)parser.sUbx     / (double)parser.sMsgs * 1e2 : 0.0);
    ioOutputStr("stats NMEA     count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", parser.nNmea,    parser.nMsgs > 0 ? (double)parser.nNmea    / (double)parser.nMsgs * 1e2 : 0.0, parser.sNmea,    parser.sMsgs > 0 ? (double)parser.sNmea    / (double)parser.sMsgs * 1e2 : 0.0);
    ioOutputStr("stats RTCM3    count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", parser.nRtcm3,   parser.nMsgs > 0 ? (double)parser.nRtcm3   / (double)parser.nMsgs * 1e2 : 0.0, parser.sRtcm3,   parser.sMsgs > 0 ? (double)parser.sRtcm3   / (double)parser.sMsgs * 1e2 : 0.0);
    ioOutputStr("stats SPARTN   count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", parser.nSpartn,  parser.nMsgs > 0 ? (double)parser.nSpartn  / (double)parser.nMsgs * 1e2 : 0.0, parser.sSpartn,  parser.sMsgs > 0 ? (double)parser.sSpartn  / (double)parser.sMsgs * 1e2 : 0.0);
    ioOutputStr("stats NOVATEL  count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", parser.nUbx,     parser.nMsgs > 0 ? (double)parser.nNovatel / (double)parser.nMsgs * 1e2 : 0.0, parser.sNovatel, parser.sMsgs > 0 ? (double)parser.sNovatel / (double)parser.sMsgs * 1e2 : 0.0);
    ioOutputStr("stats GARBAGE  count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", parser.nGarbage, parser.nMsgs > 0 ? (double)parser.nGarbage / (double)parser.nMsgs * 1e2 : 0.0, parser.sGarbage, parser.sMsgs > 0 ? (double)parser.sGarbage / (double)parser.sMsgs * 1e2 : 0.0);
    ioOutputStr("stats Total    count %6u (100.0%%)  size %10u (100.0%%)\n", parser.nMsgs, parser.sMsgs);
    if (doEpoch)
    {
        ioOutputStr("stats EPOCH    count %6u (%5.1f%%)\n", nEpochs, parser.nMsgs > 0 ? (double)nEpochs / (double)parser.nMsgs * 1e2 : 0.0);
    }

    return ioWriteOutput(true) ? EXIT_SUCCESS : EXIT_OTHERFAIL;
}

/* ****************************************************************************************************************** */
// eof
