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

/* ****************************************************************************************************************** */

static void _debugLog(const DEBUG_LEVEL_t level, const char *line, const DEBUG_CFG_t *cfg)
{
    (void)cfg;
    (void)level;
    fputs(line, stderr);
}

static DEBUG_CFG_t gCfg =
{
    .level  = DEBUG_LEVEL_PRINT,
    .colour = false,
    .mark   = NULL,
    .func   = _debugLog,
    .arg    = NULL,
};

void debugSetup(const DEBUG_CFG_t *cfg)
{
    memcpy(&gCfg, cfg, sizeof(gCfg));
    if (gCfg.func == NULL)
    {
        gCfg.func = _debugLog;
    }
    if (gCfg.level < DEBUG_LEVEL_ERROR)
    {
        gCfg.level = DEBUG_LEVEL_ERROR;
    }
    if (gCfg.level > DEBUG_LEVEL_TRACE)
    {
        gCfg.level = DEBUG_LEVEL_TRACE;
    }
}

void debugGetCfg(DEBUG_CFG_t *cfg)
{
    if (cfg != NULL)
    {
        memcpy(cfg, &gCfg, sizeof(gCfg));
    }
}

typedef enum DEBUG_COLOUR_e
{
    LOG_COLOUR_ERROR, LOG_COLOUR_WARNING, LOG_COLOUR_NOTICE, LOG_COLOUR_PRINT,
    LOG_COLOUR_DEBUG, LOG_COLOUR_TRACE, LOG_COLOUR_OFF
} DEBUG_COLOUR_t;

static void _logColour(const DEBUG_LEVEL_t level, const DEBUG_COLOUR_t colour)
{
    if (gCfg.colour)
    {
#ifdef _WIN32
        switch (colour)
        {
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
        }
        gCfg.func(level, "", &gCfg);
#else
        switch (colour)
        {
            case LOG_COLOUR_ERROR:
                gCfg.func(level, "\e[1;31m", &gCfg);
                break;
            case LOG_COLOUR_WARNING:
                gCfg.func(level, "\e[1;33m", &gCfg);
                break;
            case LOG_COLOUR_NOTICE:
                gCfg.func(level, "\e[1m", &gCfg);
                break;
            case LOG_COLOUR_PRINT:
                gCfg.func(level, "\e[m", &gCfg);
                break;
            case LOG_COLOUR_DEBUG:
                gCfg.func(level, "\e[0;36m", &gCfg);
                break;
            case LOG_COLOUR_TRACE:
                gCfg.func(level, "\e[0;35m", &gCfg);
                break;
            case LOG_COLOUR_OFF:
                gCfg.func(level, "\e[m", &gCfg);
                break;
        }
#endif
    }
}

static void _log(const DEBUG_LEVEL_t level, const DEBUG_COLOUR_t colour, const char *fmt, va_list args)
{
    if (gCfg.level < level)
    {
        return;
    }
    char fmtMod[200];
    fmtMod[0] = '\0';
    int len = 0;
    if ( (gCfg.mark != NULL) && (gCfg.mark[0] != '\0') )
    {
        len += snprintf(&fmtMod[len], sizeof(fmtMod) - len, "%s: ", gCfg.mark);
    }
    snprintf(&fmtMod[len], sizeof(fmtMod) - len, "%s\n", fmt);
    fmtMod[sizeof(fmtMod)-2] = '\n';
    fmtMod[sizeof(fmtMod)-1] = '\0';
    _logColour(level, colour);
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmtMod, args);
    gCfg.func(level, buf, &gCfg);
    _logColour(level, LOG_COLOUR_OFF);
}

void _hexdump(const DEBUG_LEVEL_t level, const DEBUG_COLOUR_t colour, const void *data, const int size, const char *fmt, va_list args)
{
    if (gCfg.level < level)
    {
        return;
    }
    if (fmt != NULL)
    {
        _log(level, colour, fmt, args);
    }

    char mark[200];
    mark[0] = '\0';
    if ( (gCfg.mark != NULL) && (gCfg.mark[0] != '\0') )
    {
        snprintf(mark, sizeof(mark), "%s: ", gCfg.mark);
    }

    const char i2hex[] = "0123456789abcdef";
    const uint8_t *pData = (const uint8_t *)data;
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
        _logColour(level, colour);
        char buf[1024];
        snprintf(buf, sizeof(buf), "%s0x%04" PRIx8 " %05d  %s\n", mark, ix, ix, str);
        gCfg.func(level, buf, &gCfg);
        _logColour(level, LOG_COLOUR_OFF);
        ix += 16;
    }
}

void ERROR(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(DEBUG_LEVEL_ERROR, LOG_COLOUR_ERROR, fmt, args);
    va_end(args);
}

void WARNING(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(DEBUG_LEVEL_WARNING, LOG_COLOUR_WARNING, fmt, args);
    va_end(args);
}

void PRINT(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(DEBUG_LEVEL_PRINT, LOG_COLOUR_PRINT, fmt, args);
    va_end(args);
}

void NOTICE(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(DEBUG_LEVEL_NOTICE, LOG_COLOUR_NOTICE, fmt, args);
    va_end(args);
}

void DEBUG(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(DEBUG_LEVEL_DEBUG, LOG_COLOUR_DEBUG, fmt, args);
    va_end(args);
}

bool isDEBUG(void)
{
    return (gCfg.level >= DEBUG_LEVEL_DEBUG);
}

void TRACE(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _log(DEBUG_LEVEL_TRACE, LOG_COLOUR_TRACE, fmt, args);
    va_end(args);
}

bool isTRACE(void)
{
    return (gCfg.level >= DEBUG_LEVEL_TRACE);
}

void DEBUG_HEXDUMP(const void *data, const int size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _hexdump(DEBUG_LEVEL_DEBUG, LOG_COLOUR_DEBUG, data, size, fmt, args);
    va_end(args);
}

void TRACE_HEXDUMP(const void *data, const int size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _hexdump(DEBUG_LEVEL_TRACE, LOG_COLOUR_TRACE, data, size, fmt, args);
    va_end(args);
}

/* ****************************************************************************************************************** */
// eof
