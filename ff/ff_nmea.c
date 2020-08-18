// flipflip's NMEA protocol stuff
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
#include <stdio.h>

#include "ff_stuff.h"
#include "ff_debug.h"
#include "ff_nmea.h"

/* ********************************************************************************************** */

bool nmeaMessageName(char *name, const int size, const char *msg)
{
    if (name == NULL)
    {
        return false;
    }
    if ( (msg == NULL) || (strlen(msg) < 6) )
    {
        name[0] = '\0';
        return false;
    }
    int res;
    // Talker ID
    char talker[3];
    int offs;
    if (msg[1] == 'P') // Proprietary
    {
        talker[0] = 'P';
        talker[1] = '\0';
        offs = 2;
    }
    else
    {
        talker[0] = msg[1];
        talker[1] = msg[2];
        talker[2] = '\0';
        offs = 3;
    }
    // Formatter
    const char *comma = strchr(msg, ',');
    const int num = comma - &msg[offs];
    char formatter[9];
    if ( (comma == NULL) || (num < 1) || (num > (int)(sizeof(formatter) - 1 - 3)) )
    {
        res = snprintf(name, size, "NMEA-%s-?", talker);
    }
    else
    {
        int ix;
        for (ix = 0; ix < num; ix++)
        {
            formatter[ix] = msg[offs + ix];
        }
        formatter[ix] = '\0';
        // u-blox proprietary formatter, add "_XX"
        if ( (formatter[0] == 'U') && (formatter[1] == 'B') && (formatter[2] == 'X') && (formatter[3] == '\0') )
        {
            formatter[ix + 0] = '_';
            formatter[ix + 1] = comma[1];
            formatter[ix + 2] = comma[2];
            formatter[ix + 3] = '\0';
        }
        res = snprintf(name, size, "NMEA-%s-%s", talker, formatter);
    }

    return res < size;
}

bool nmeaMessageInfo(char *info, const int size, const char *msg)
{
    if (info == NULL)
    {
        return false;
    }
    const int len = strlen(msg);
    if ( (msg == NULL) || (len < 6) )
    {
        info[0] = '\0';
        return false;
    }
    const char *comma = strchr(msg, ',');
    if (comma == NULL)
    {
        info[0] = '\0';
        return false;
    }
    if ( (msg[1] == 'P') && (msg[2] == 'U') && (msg[3] == 'B') && (msg[4] == 'X') && (msg[5] == ',') )
    {
        const char *comma2 = strchr(&comma[1], ',');
        if (comma2 != NULL)
        {
            comma = comma2;
        }
    }
    const int res = snprintf(info, size, "%s", &msg[comma - msg + 1]);
    if (res >= size)
    {
        return false;
    }
    char *star = strchr(info, '*');
    if (star == NULL)
    {
        return false;
    }
    *star = '\0';
    return true;
}

/* ********************************************************************************************** */
// eof
