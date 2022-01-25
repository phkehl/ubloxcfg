/* ************************************************************************************************/ // clang-format off
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
    uint32_t nNmea = 0, sNmea = 0;
    uint32_t nUbx  = 0, sUbx  = 0;
    uint32_t nRtcm = 0, sRtcm = 0;
    uint32_t nGarb = 0, sGarb = 0;
    uint32_t nNova = 0, sNova = 0;
    uint32_t nMsgs = 0, sMsgs = 0;
    uint32_t nEpochs = 0;

    gAbort = false;
    signal(SIGINT, _sigHandler);
    signal(SIGTERM, _sigHandler);
    NOT_WIN( signal(SIGHUP, _sigHandler) );

    PARSER_t parser;
    parserInit(&parser);

    EPOCH_t coll;
    EPOCH_t epoch;
    epochInit(&coll);

    while (!gAbort)
    {
        uint8_t buf[250];
        const int num = ioReadInput(buf, sizeof(buf));
        if (num < 0) // eof
        {
            break;
        }
        else if (num == 0) // wait
        {
            SLEEP(5);
            continue;
        }
        parserAdd(&parser, buf, num);

        PARSER_MSG_t msg;
        while (parserProcess(&parser, &msg, true))
        {
            nMsgs++;
            sMsgs += msg.size;

            if (doEpoch && epochCollect(&coll, &msg, &epoch))
            {
                nEpochs++;
                ioOutputStr("epoch   %4d, size    0, NONE     EPOCH                %s\n", nEpochs, epoch.str);
            }
            const char *prot = "?";
            switch (msg.type)
            {
                case PARSER_MSGTYPE_UBX:
                    prot = "UBX";
                    nUbx++;
                    sUbx += msg.size;
                    break;
                case PARSER_MSGTYPE_NMEA:
                    prot = "NMEA";
                    nNmea++;
                    sNmea += msg.size;
                    break;
                case PARSER_MSGTYPE_RTCM3:
                    prot = "RTCM3";
                    nRtcm++;
                    sRtcm += msg.size;
                    break;
                case PARSER_MSGTYPE_NOVATEL:
                    prot = "NOVATEL";
                    nNova++;
                    sNova += msg.size;
                    break;
                case PARSER_MSGTYPE_GARBAGE:
                    prot = "GARBAGE";
                    nGarb++;
                    sGarb += msg.size;
                    break;
            }
            ioOutputStr("message %4u, size %4d, %-8s %-20s %s\n",
                msg.seq, msg.size, prot, msg.name, msg.info != NULL ? msg.info : "n/a");
            if (extraInfo)
            {
                ioAddOutputHexdump(msg.data, msg.size);
            }
            if (!ioWriteOutput(nMsgs == 1 ? false : true))
            {
                break;
            }
        }
    }

    ioOutputStr("stats UBX      count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", nUbx,  nMsgs > 0 ? (double)nUbx  / (double)nMsgs * 1e2 : 0.0, sUbx,  sMsgs > 0 ? (double)sUbx  / (double)sMsgs * 1e2 : 0.0);
    ioOutputStr("stats NMEA     count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", nNmea, nMsgs > 0 ? (double)nNmea / (double)nMsgs * 1e2 : 0.0, sNmea, sMsgs > 0 ? (double)sNmea / (double)sMsgs * 1e2 : 0.0);
    ioOutputStr("stats RTCM3    count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", nRtcm, nMsgs > 0 ? (double)nRtcm / (double)nMsgs * 1e2 : 0.0, sRtcm, sMsgs > 0 ? (double)sRtcm / (double)sMsgs * 1e2 : 0.0);
    ioOutputStr("stats NOVATEL  count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", nNova, nMsgs > 0 ? (double)nNova / (double)nMsgs * 1e2 : 0.0, sNova, sMsgs > 0 ? (double)sNova / (double)sMsgs * 1e2 : 0.0);
    ioOutputStr("stats GARBAGE  count %6u (%5.1f%%)  size %10u (%5.1f%%)\n", nGarb, nMsgs > 0 ? (double)nGarb / (double)nMsgs * 1e2 : 0.0, sGarb, sMsgs > 0 ? (double)sGarb / (double)sMsgs * 1e2 : 0.0);
    ioOutputStr("stats Total    count %6u (100.0%%)  size %10u (100.0%%)\n", nMsgs, sMsgs);
    if (doEpoch)
    {
        ioOutputStr("stats EPOCH    count %5u (%5.1f%%)\n", nEpochs, nMsgs > 0 ? (double)nEpochs / (double)nMsgs * 1e2 : 0.0);
    }

    return ioWriteOutput(true) ? EXIT_SUCCESS : EXIT_OTHERFAIL;
}

/* ****************************************************************************************************************** */
// eof
