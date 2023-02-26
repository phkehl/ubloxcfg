// flipflip's SPARTN protocol stuff
//
// Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org),
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
#include "ff_spartn.h"

/* ****************************************************************************************************************** */

bool spartnMessageName(char *name, const int size, const uint8_t *msg, const int msgSize)
{
    if ( (name == NULL) || (size < 1) )
    {
        return false;
    }
    if ( (msg == NULL) || (msgSize < (SPARTN_MIN_HEAD_SIZE + 2)) )
    {
        name[0] = '\0';
        return false;
    }
    const uint32_t msgType = (msg[1] & 0xfe) >> 1;
    const uint32_t msgSubType = (msg[4] & 0xf0) >> 4;
    const int res = snprintf(name, size, "SPARTN-TYPE%" PRIu32 "_%" PRIu32, msgType, msgSubType);
    return res < size;
}

// ---------------------------------------------------------------------------------------------------------------------

bool spartnMessageInfo(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    if ( (info == NULL) || (size < 1) )
    {
        return false;
    }

    if ( (msg == NULL) || (msgSize < (SPARTN_MIN_HEAD_SIZE + 2)) )
    {
        info[0] = '\0';
        return false;
    }
    const uint32_t msgType = (msg[1] & 0xfe) >> 1;
    const uint32_t subType = (msg[4] & 0xf0) >> 4;
    const char *desc = spartnTypeDesc(msgType, subType);
    if (desc != NULL)
    {
        const int res = snprintf(info, size, "%s", desc);
        return res < size;
    }
    else
    {
        return false;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

const char *spartnTypeDesc(const int msgType, const int subType)
{
    const char *desc = NULL;
    switch (msgType)
    {
        case 0:
            switch (subType)
            {
                case 0: desc = "Orbits, clock, bias (OCB) GPS";     break;
                case 1: desc = "Orbits, clock, bias (OCB) GLONASS"; break;
                case 2: desc = "Orbits, clock, bias (OCB) Galileo"; break;
                case 3: desc = "Orbits, clock, bias (OCB) BeiDou";  break;
                case 4: desc = "Orbits, clock, bias (OCB) QZSS";    break;
            }
            break;
        case 1:
            switch (subType)
            {
                case 0: desc = "High-precision atmosphere correction (HPAC) GPS";     break;
                case 1: desc = "High-precision atmosphere correction (HPAC) GLONASS"; break;
                case 2: desc = "High-precision atmosphere correction (HPAC) Galileo"; break;
                case 3: desc = "High-precision atmosphere correction (HPAC) BeiDou";  break;
                case 4: desc = "High-precision atmosphere correction (HPAC) QZSS";    break;
            }
            break;
        case 2:
            switch (subType)
            {
                case 0: desc = "Geographic area definition (GAD)"; break;
            }
            break;
        case 3:
            switch (subType)
            {
                case 0: desc = "Basic-precision atmosphere correction (BPAC)"; break;
            }
            break;
        case 4:
            switch (subType)
            {
                case 0: desc = "Dynamic key";          break;
                case 1: desc = "Group authentication"; break;
            }
            break;
        case 120:
            switch (subType)
            {
                case 0: desc = "proprietary test";   break;
                case 1: desc = "u-blox proprietary"; break;
                case 2: desc = "Swift proprietary";  break;
            }
            break;
    }

    return desc;
}


/* ****************************************************************************************************************** */
// eof
