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

#ifndef __FF_DEBUG_H__
#define __FF_DEBUG_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef _WIN32
#  ifndef NOGDI
#    define NOGDI
#  endif
#  include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */

typedef enum DEBUG_LEVEL_e
{
    DEBUG_LEVEL_ERROR   = -2,
    DEBUG_LEVEL_WARNING = -1,
    DEBUG_LEVEL_NOTICE  =  0,
    DEBUG_LEVEL_PRINT   =  1,
    DEBUG_LEVEL_DEBUG   =  2,
    DEBUG_LEVEL_TRACE   =  3,
} DEBUG_LEVEL_t;

typedef struct DEBUG_CFG_s DEBUG_CFG_t;

typedef struct DEBUG_CFG_s
{
    DEBUG_LEVEL_t level;
    bool          colour;
    const char   *mark;
    void        (*func)(DEBUG_LEVEL_t level, const char *line, const DEBUG_CFG_t *);
    void         *arg;

} DEBUG_CFG_t;

void debugSetup(const DEBUG_CFG_t *cfg);

void debugGetCfg(DEBUG_CFG_t *cfg);

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

/* ****************************************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_DEBUG_H__
