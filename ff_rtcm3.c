// flipflip's RTCM3 protocol stuff
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
#include <inttypes.h>

#include "ff_stuff.h"
#include "ff_rtcm3.h"

/* ********************************************************************************************** */

bool rtcm3MessageName(char *name, const int size, const uint8_t *msg, const int msgSize)
{
    if (name == NULL)
    {
        return false;
    }
    if ( (msg == NULL) || (msgSize < (RTCM3_HEAD_SIZE + 2)) )
    {
        name[0] = '\0';
        return false;
    }
    uint32_t type = (msg[RTCM3_HEAD_SIZE + 0] << 4) | (msg[RTCM3_HEAD_SIZE + 1] & 0x0f);
    int res;
    // u-blox proprietary type 1072 uses a 12-bit sub-type value
    if ( (type == 1072) && (msgSize > (RTCM3_HEAD_SIZE + 2 + 1)) )
    {
        uint32_t subtype = (msg[RTCM3_HEAD_SIZE + 1] & 0x0f) | (msg[RTCM3_HEAD_SIZE + 2]);
        res = snprintf(name, size, "RTCM3-TYPE%"PRIu32"_%"PRIu32, type, subtype);
    }
    else
    {
        res = snprintf(name, size, "RTCM3-TYPE%"PRIu32, type);
    }
    return res < size;
}

/* ********************************************************************************************** */
// eof
