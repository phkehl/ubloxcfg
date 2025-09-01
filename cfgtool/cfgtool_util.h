// clang-format off
/* ****************************************************************************************************************** */
// u-blox positioning receivers configuration tool
//
// Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org) and contributors
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

#ifndef __CFGTOOL_UTIL_H__
#define __CFGTOOL_UTIL_H__

/* ****************************************************************************************************************** */

#define EXIT_BADARGS       1
#define EXIT_RXFAIL        2
#define EXIT_RXNODATA      3
#define EXIT_OTHERFAIL    99

typedef struct IO_LINE_s
{
    char       *line;
    int         lineNr;
    int         lineLen;
    const char *file;

} IO_LINE_t;

void ioSetOutput(const char *name, FILE *file, const bool overwrite);
void ioSetInput(const char *name, FILE *file);
IO_LINE_t *ioGetNextInputLine(void);
int  ioReadInput(uint8_t *data, const int size);
void ioOutputStr(const char *fmt, ...);
void ioAddOutputBin(const uint8_t *data, const int size);
void ioAddOutputHex(const uint8_t *data, const int size, const int wordsPerLine, const bool ugly);
void ioAddOutputHexdump(const uint8_t *data, const int size);
void ioAddOutputC(const uint8_t *data, const int size, const int wordsPerLine, const char *indent);
bool ioWriteOutput(const bool append);

bool layersStringToFlags(const char *layers, bool *ram, bool *bbr, bool *flash, bool *def);

/* ****************************************************************************************************************** */
#endif // __CFGTOOL_UTIL_H__
