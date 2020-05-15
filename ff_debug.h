// flipflip's colourful (printf style) debugging stuff
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
#include <stdio.h>

#ifndef __FF_DEBUG_H__
#define __FF_DEBUG_H__

/* ********************************************************************************************** */

void debugSetup(FILE *handle, const int verbosity, const bool colour, const char *mark);

void ERROR(const char *fmt, ...)    __attribute__ ((format (printf, 1, 2)));
void WARNING(const char *fmt, ...)  __attribute__ ((format (printf, 1, 2)));
void NOTICE(const char *fmt, ...)   __attribute__ ((format (printf, 1, 2)));
void PRINT(const char *fmt, ...)    __attribute__ ((format (printf, 1, 2)));
void DEBUG(const char *fmt, ...)    __attribute__ ((format (printf, 1, 2)));
void TRACE(const char *fmt, ...)    __attribute__ ((format (printf, 1, 2)));

bool isDEBUG(void);
bool isTRACE(void);

void DEBUG_HEXDUMP(const void *data, const int size, const char *fmt, ...) __attribute__ ((format (printf, 3, 4)));
void TRACE_HEXDUMP(const void *data, const int size, const char *fmt, ...) __attribute__ ((format (printf, 3, 4)));

/* ********************************************************************************************** */
#endif // __FF_DEBUG_H__
