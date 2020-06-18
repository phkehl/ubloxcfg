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

#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <limits.h>

#include "cfgtool_util.h"

#define INPUT_MAX_LINE_LEN 2000

static char  gInName[PATH_MAX];
static FILE *gInFile;
int          gInLineNr;
bool         gOutOverwrite;

static char  gOutName[PATH_MAX];
FILE        *gOutFile;
int          gOutLineNr;

void ioSetInput(const char *name, FILE *file)
{
    TRACE("ioSetInput(%s, %p)", name, file);
    if ( (name == NULL) || (name[0] == '\0') )
    {
        return;
    }
    snprintf(gInName, sizeof(gInName), "%s", name);
    gInFile = file;
    gInLineNr = 0;
}

void ioSetOutput(const char *name, FILE *file, const bool overwrite)
{
    TRACE("ioSetOutput(%s, %p, %d)", name, file, overwrite);
    if ( (name == NULL) || (name[0] == '\0') )
    {
        return;
    }
    snprintf(gOutName, sizeof(gOutName), "%s", name);
    gOutFile = file;
    gOutLineNr = 0;
    gOutOverwrite = overwrite;
}

IO_LINE_t *ioGetNextInputLine(void)
{
    static char line[INPUT_MAX_LINE_LEN];
    static IO_LINE_t resLine;
    resLine.line = NULL;
    resLine.lineLen = 0;
    bool res = false;
    // get next useful line from input
    while (fgets(line, sizeof(line), gInFile) != NULL)
    {
        gInLineNr++;
        char *pLine = line;

        // remove comments
        char *comment = strchr(pLine, '#');
        if (comment != NULL)
        {
            *comment = '\0';
        }

        // remove leading space
        while (isspace(*pLine) != 0)
        {
            pLine++;
        }

        // all whitespace?
        if (*pLine == '\0')
        {
            continue;
        }

        // remove trailing whitespace
        int inLineLen = strlen(pLine);
        char *pEnd = pLine + inLineLen - 1;
        while( (pEnd > pLine) && (isspace(*pEnd) != 0) )
        {
            pEnd--;
            inLineLen--;
        }
        pEnd[1] = '\0';

        resLine.line    = pLine;
        resLine.lineLen = inLineLen;
        resLine.lineNr  = gInLineNr;
        resLine.file    = gInName;
        res = true;
        break;
    }

    return res ? &resLine : NULL;
}

int ioReadInput(uint8_t *data, const int size)
{
    int res = 0;
    if ( (data != NULL) && (size > 0) )
    {
        if (feof(gInFile) != 0) // FIXME: why does this flag on /dev/tty... devices?
        {
            res = -1;
        }
        else
        {
             // TODO: only read as much as is currently available
            res = fread(data, 1, size, gInFile);
        }
    }
    return res;
}


static char gOutputBuf[1024 * 1024] = { 0 };
static int gOutputBufSize = 0;

void ioOutputStr(const char *fmt, ...)
{
    const int totSize = sizeof(gOutputBuf);
    if (gOutputBufSize >= totSize)
    {
        return;
    }

    const int remSize = totSize - gOutputBufSize;

    va_list args;
    va_start(args, fmt);
    const int writeSize = vsnprintf(&gOutputBuf[gOutputBufSize], remSize, fmt, args);
    va_end(args);

    if (writeSize > remSize)
    {
        gOutputBufSize = totSize;
    }
    else
    {
        gOutputBufSize += writeSize;
    }
}

void ioAddOutputBin(const uint8_t *data, const int size)
{
    const int totSize = sizeof(gOutputBuf);
    if (gOutputBufSize >= totSize)
    {
        return;
    }

    const int remSize = totSize - gOutputBufSize;
    if (remSize < size)
    {
        return;
    }

    memcpy(&gOutputBuf[gOutputBufSize], data, size);
    gOutputBufSize += size;
}

void ioAddOutputHex(const uint8_t *data, const int size, const int wordsPerLine)
{
    const int bytesPerLine = wordsPerLine * 4;
    for (int ix = 0; ix < size; ix++)
    {
        const int pos = ix % bytesPerLine;
        if (pos > 0)
        {
            ioOutputStr((pos % 4) == 0 ? "  " : " ");
        }
        ioOutputStr("%02"PRIx8, data[ix]);
        if ( (pos == (bytesPerLine - 1)) || (ix == (size - 1)) )
        {
            ioOutputStr("\n");
        }
    }
}

void ioAddOutputC(const uint8_t *data, const int size, const int wordsPerLine, const char *indent)
{
    const int bytesPerLine = wordsPerLine * 4;
    for (int ix = 0; ix < size; ix++)
    {
        const int pos = ix % bytesPerLine;
        if (pos == 0)
        {
            ioOutputStr("%s", indent);
        }
        if (pos > 0)
        {
            ioOutputStr((pos % 4) == 0 ? "  " : " ");
        }
        ioOutputStr("0x%02"PRIx8"%s", data[ix], (ix + 1) == size ? "" : ",");
        if ( (pos == (bytesPerLine - 1)) || (ix == (size - 1)) )
        {
            ioOutputStr("\n");
        }
    }
}

void ioAddOutputHexdump(const uint8_t *data, const int size)
{
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
        ioOutputStr("0x%04"PRIx8" %05d  %s\n", ix, ix, str);
        ix += 16;
    }
}

bool ioWriteOutput(const bool append)
{
    const int totSize = sizeof(gOutputBuf);
    if (gOutputBufSize >= totSize)
    {
        WARNING("Too much output data, not writing to '%s'!", gOutName);
        return false;
    }

    const char *failStr = NULL;
    bool res = true;
    if ( (gOutFile != stdout) && (gOutFile != stderr) )
    {
        if (!append && !gOutOverwrite && (access(gOutName, F_OK) == 0))
        {
            failStr = "File already exists";
            res = false;
        }

        if (res)
        {
            gOutFile = fopen(gOutName, append ? "a" : "w");
            if (gOutFile == NULL)
            {
                failStr = strerror(errno);
                res = false;
            }
        }
    }

    if (res)
    {
        if (!append)
        {
            PRINT("Writing output to '%s'.", gOutName);
        }
        DEBUG("Writing output to '%s' (%d bytes).", gOutName, gOutputBufSize);
        res = (int)fwrite(gOutputBuf, 1, gOutputBufSize, gOutFile) == gOutputBufSize;
    }

    gOutputBufSize = 0;

    if (!res)
    {
        WARNING("Failed writing '%s': %s", gOutName, failStr == NULL ? "Unknown error" : failStr);
    }
    return res;
}

/* ********************************************************************************************** */
// eof
