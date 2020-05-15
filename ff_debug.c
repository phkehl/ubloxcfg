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

#include <string.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdarg.h>
#include <ctype.h>
#ifdef _WIN32
#  define NOGDI
#  include <windows.h>
#endif

#include "ff_debug.h"

/* ********************************************************************************************** */

static FILE       *gLogFile;
static int         gLogVerbosity;
static const char *gLogMark;
static bool        gLogColour;

#define LOG_LEVEL_ERROR   -2
#define LOG_LEVEL_WARNING -1
#define LOG_LEVEL_NOTICE   0
#define LOG_LEVEL_PRINT    0
#define LOG_LEVEL_DEBUG    1
#define LOG_LEVEL_TRACE    2

void debugSetup(FILE *handle, const int verbosity, const bool colour, const char *mark)
{
    gLogFile = handle;
    gLogVerbosity = verbosity;
    if (gLogVerbosity < LOG_LEVEL_ERROR)
    {
        gLogVerbosity = LOG_LEVEL_ERROR;
    }
    if (gLogVerbosity > LOG_LEVEL_TRACE)
    {
        gLogVerbosity = LOG_LEVEL_TRACE;
    }
    gLogColour = colour;
    gLogMark = mark;
}

enum { LOG_COLOUR_ERROR, LOG_COLOUR_WARNING, LOG_COLOUR_NOTICE, LOG_COLOUR_PRINT, LOG_COLOUR_DEBUG, LOG_COLOUR_TRACE, LOG_COLOUR_OFF };

static void _logColour(const int colour)
{
    if ( gLogColour && (gLogFile != NULL) )
    {
        switch (colour)
        {
#ifdef _WIN32
            case LOG_COLOUR_ERROR:
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED |                                      FOREGROUND_INTENSITY);
                break;
            case LOG_COLOUR_WARNING:
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN |                   FOREGROUND_INTENSITY);
                break;
            case LOG_COLOUR_NOTICE:
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                break;
            case LOG_COLOUR_OFF:
            case LOG_COLOUR_PRINT:
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                break;
            case LOG_COLOUR_DEBUG:
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED |                    FOREGROUND_BLUE);
                break;
            case LOG_COLOUR_TRACE:
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),                  FOREGROUND_GREEN | FOREGROUND_BLUE);
                break;
#else
            case LOG_COLOUR_ERROR:
                fputs("\e[1;31m", gLogFile);
                break;
            case LOG_COLOUR_WARNING:
                fputs("\e[1;33m", gLogFile);
                break;
            case LOG_COLOUR_NOTICE:
                fputs("\e[1m", gLogFile);
                break;
            case LOG_COLOUR_PRINT:
                fputs("\e[m", gLogFile);
                break;
            case LOG_COLOUR_DEBUG:
                fputs("\e[0;36m", gLogFile);
                break;
            case LOG_COLOUR_TRACE:
                fputs("\e[0;35m", gLogFile);
                break;
            case LOG_COLOUR_OFF:
                fputs("\e[m", gLogFile);
                break;
#endif
            default: break;
        }
    }
}

static void _log(const int level, const int colour, const char *fmt, va_list args)
{
    if ( (gLogVerbosity < level) || (gLogFile == NULL) )
    {
        return;
    }
    char fmtMod[200];
    fmtMod[0] = '\0';
    int len = 0;
    if (gLogMark != NULL)
    {
        len += snprintf(&fmtMod[len], sizeof(fmtMod) - len, "%s: ", gLogMark);
    }
    snprintf(&fmtMod[len], sizeof(fmtMod) - len, "%s\n", fmt);
    fmtMod[sizeof(fmtMod)-2] = '\n';
    fmtMod[sizeof(fmtMod)-1] = '\0';
    _logColour(colour);
    vfprintf(gLogFile, fmtMod, args);
    _logColour(LOG_COLOUR_OFF);
}

void _hexdump(const int level, const int colour, const void *data, const int size, const char *fmt, va_list args)
{
    if ( (gLogVerbosity < level) || (gLogFile == NULL) )
    {
        return;
    }
    if (fmt != NULL)
    {
        _log(level, colour, fmt, args);
    }

    char mark[200];
    mark[0] = '\0';
    if (gLogMark != NULL)
    {
        snprintf(mark, sizeof(mark), "%s: ", gLogMark);
    }

    const char i2hex[] = "0123456789abcdef";
    const uint8_t *pData = data;
    for (int ix = 0; ix < size; )
    {
        char str[70];
        memset(str, ' ', sizeof(str));
        str[50] = '|';
        str[67] = '|';
        str[68] = '\0';
        for (int ix2 = 0; (ix2 < 16) && ((ix + ix2) < size); ix2++)
        {
            //           1         2         3         4         5         6
            // 012345678901234567890123456789012345678901234567890123456789012345678
            // xx xx xx xx xx xx xx xx  xx xx xx xx xx xx xx xx  |................|\0
            // 0  1  2  3  4  5  6  7   8  9  10 11 12 13 14 15
            const uint8_t c = pData[ix + ix2];
            int pos1 = 3 * ix2;
            int pos2 = 51 + ix2;
            if (ix2 > 7)
            {
                   pos1++;
            }
            str[pos1    ] = i2hex[ (c >> 4) & 0xf ];
            str[pos1 + 1] = i2hex[  c       & 0xf ];

            str[pos2] = isprint((int)c) ? c : '.';
        }
        _logColour(colour);
        fprintf(gLogFile, "%s0x%04"PRIx8" %05d  %s\n", mark, ix, ix, str);
        _logColour(LOG_COLOUR_OFF);
        ix += 16;
    }
}

void ERROR(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(LOG_LEVEL_ERROR, LOG_COLOUR_ERROR, fmt, args);
    va_end(args);
}

void WARNING(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(LOG_LEVEL_WARNING, LOG_COLOUR_WARNING, fmt, args);
    va_end(args);
}

void PRINT(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(LOG_LEVEL_PRINT, LOG_COLOUR_PRINT, fmt, args);
    va_end(args);
}

void NOTICE(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(LOG_LEVEL_PRINT, LOG_COLOUR_NOTICE, fmt, args);
    va_end(args);
}

void DEBUG(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(LOG_LEVEL_DEBUG, LOG_COLOUR_DEBUG, fmt, args);
    va_end(args);
}

bool isDEBUG(void)
{
    return (gLogVerbosity >= LOG_LEVEL_DEBUG);
}

void TRACE(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(LOG_LEVEL_TRACE, LOG_COLOUR_TRACE, fmt, args);
    va_end(args);
}

bool isTRACE(void)
{
    return (gLogVerbosity >= LOG_LEVEL_TRACE);
}

void DEBUG_HEXDUMP(const void *data, const int size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _hexdump(LOG_LEVEL_DEBUG, LOG_COLOUR_DEBUG, data, size, fmt, args);
    va_end(args);
}

void TRACE_HEXDUMP(const void *data, const int size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _hexdump(LOG_LEVEL_TRACE, LOG_COLOUR_TRACE, data, size, fmt, args);
    va_end(args);
}

/* ********************************************************************************************** */
// eof
