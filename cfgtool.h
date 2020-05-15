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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "ubloxcfg.h"
#include "ff_debug.h"
#include "ff_stuff.h"

#ifndef __CFGTOOL_H__
#define __CFGTOOL_H__

/* ********************************************************************************************** */

#define EXIT_BADARGS       1
#define EXIT_RXFAIL        2
#define EXIT_RXNODATA      3
#define EXIT_OTHERFAIL    99

typedef struct LINE_s
{
    char       *line;
    int         lineNr;
    int         lineLen;
    const char *file;

} LINE_t;

LINE_t *getNextInputLine(void);
int readInput(uint8_t *data, const int size);

void addOutputStr(const char *fmt, ...);
void addOutputBin(const uint8_t *data, const int size);
void addOutputHex(const uint8_t *data, const int size, const int wordsPerLine);
void addOutputHexdump(const uint8_t *data, const int size);
void addOutputC(const uint8_t *data, const int size, const int wordsPerLine, const char *indent);
bool writeOutput(const bool append);

/* ********************************************************************************************** */
#endif // __CFGTOOL_H__
