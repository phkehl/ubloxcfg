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
//#include <stdlib.h>
//#include <inttypes.h>

#include "ubloxcfg.h"

#include "cfgtool_util.h"

#include "cfgtool_bin2hex.h"

/* ****************************************************************************************************************** */

const char *bin2hexHelp(void)
{
    return
// -----------------------------------------------------------------------------
"Commands 'bin2hex' and 'hex2bin':\n"
"\n"
"    Usage: cfgtool bin2hex [-i <infile>] [-o <outfile>] [-y]\n"
"           cfgtool hex2bin [-i <infile>] [-o <outfile>] [-y]\n"
"\n"
"    Convert data from or to u-center style hex dump.\n"
"\n"
"    Example:\n"
"\n"
"        echo 48 61 6B 75 6E 61 20 4D 61 74 61 74 61 21 0A | cfgtool -q hex2bin\n"
"\n";
}

#define NUM_WORDS (64 / 4)

int bin2hexRun(void)
{
    ioWriteOutput(false);
    while (true)
    {
        uint8_t buf[NUM_WORDS * 4];
        const int num = ioReadInput(buf, sizeof(buf));
        if (num > 0)
        {
            ioAddOutputHex(buf, num, NUM_WORDS, true);
            ioWriteOutput(true);
        }
        else
        {
            break;
        }
    }
    return ioWriteOutput(true) ? EXIT_SUCCESS : EXIT_OTHERFAIL;
}

int hex2binRun(void)
{
    ioWriteOutput(false);
    bool inputOk = true;
    while (inputOk)
    {
        IO_LINE_t *line = ioGetNextInputLine();
        if (line != NULL)
        {
            const char *sep = " ";
            for (char *save = NULL, *tok = strtok_r(line->line, sep, &save); tok != NULL; tok = strtok_r(NULL, sep, &save))
            {
                uint8_t b;
                int n = 0;
                if ( (sscanf(tok, "%2hhx%n", &b, &n) == 1) && (n == 2) )
                {
                    ioAddOutputBin(&b, 1);
                }
                else
                {
                    WARNING("%s:%d: Bad input ('%s')!", line->file, line->lineNr, tok);
                    inputOk = false;
                }
            }
        }
        else
        {
            break;
        }
    }
    const bool writeOk = ioWriteOutput(true);
    return writeOk && inputOk ? EXIT_SUCCESS : EXIT_OTHERFAIL;
}

/* ****************************************************************************************************************** */
// eof
