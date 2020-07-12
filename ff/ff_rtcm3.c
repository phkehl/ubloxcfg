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
    const uint32_t type = RTCM3_TYPE(msg);
    int res;
    // u-blox proprietary type 4072 uses a 12-bit sub-type value
    if ( (type == 4072) && (msgSize > (RTCM3_HEAD_SIZE + 2 + 1)) )
    {
        const uint32_t subtype = RTCM3_4072_SUBTYPE(msg);
        res = snprintf(name, size, "RTCM3-TYPE%"PRIu32"_%"PRIu32, type, subtype);
    }
    else
    {
        res = snprintf(name, size, "RTCM3-TYPE%"PRIu32, type);
    }
    return res < size;
}

bool rtcm3MessageInfo(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    if (info == NULL)
    {
        return false;
    }
    if ( (msg == NULL) || (msgSize < (RTCM3_HEAD_SIZE + 2)) )
    {
        info[0] = '\0';
        return false;
    }
    int len = 0;
    const uint32_t type = RTCM3_TYPE(msg);
    switch (type)
    {
        case 1001:
            len = snprintf(info, size, "L1-only GPS RTK observables");
            break;
        case 1002:
            len = snprintf(info, size, "Extended L1-only GPS RTK observables");
            break;
        case 1003:
            len = snprintf(info, size, "L1/L2 GPS RTK observables");
            break;
        case 1004:
            len = snprintf(info, size, "Extended L1/L2 GPS RTK observables");
            break;
        case 1005:
            len = snprintf(info, size, "Stationary RTK reference station ARP");
            break;
        case 1006:
            len = snprintf(info, size, "Stationary RTK reference station ARP with antenna height");
            break;
        case 1007:
            len = snprintf(info, size, "Antenna descriptor");
            break;
        case 1009:
            len = snprintf(info, size, "L1-only GLONASS RTK observables");
            break;
        case 1010:
            len = snprintf(info, size, "Extended L1-only GLONASS RTK observables");
            break;
        case 1011:
            len = snprintf(info, size, "L1/L2 GLONASS RTK observables");
            break;
        case 1012:
            len = snprintf(info, size, "Extended L1/L2 GLONASS RTK observables");
            break;
        case 1030:
            len = snprintf(info, size, "GPS network RTK residual message");
            break;
        case 1031:
            len = snprintf(info, size, "GLONASS network RTK residual message");
            break;
        case 1032:
            len = snprintf(info, size, "Physical reference station position message");
            break;
        case 1033:
            len = snprintf(info, size, "Receiver and Antenna Description");
            break;
        case 1074:
            len = snprintf(info, size, "GPS MSM4 (full C, full L, S)");
            break;
        case 1075:
            len = snprintf(info, size, "GPS MSM5 (full C, full L, S, D)");
            break;
        case 1077:
            len = snprintf(info, size, "GPS MSM7 (ext full C, ext full L, S, D)");
            break;
        case 1084:
            len = snprintf(info, size, "GLONASS MSM4 (full C, full L, S)");
            break;
        case 1085:
            len = snprintf(info, size, "GLONASS MSM5 (full C, full L, S, D)");
            break;
        case 1087:
            len = snprintf(info, size, "GLONASS MSM7 (ext full C, ext full L, S, D)");
            break;
        case 1094:
            len = snprintf(info, size, "Galileo MSM4 (full C, full L, S)");
            break;
        case 1095:
            len = snprintf(info, size, "Galileo MSM5 (full C, full L, S, D)");
            break;
        case 1097:
            len = snprintf(info, size, "Galileo MSM7 (ext full C, ext full L, S, D)");
            break;
        case 1124:
            len = snprintf(info, size, "BeiDou MSM4 (full C, full L, S)");
            break;
        case 1125:
            len = snprintf(info, size, "BeiDou MSM5 (full C, full L, S, D)");
            break;
        case 1127:
            len = snprintf(info, size, "BeiDou MSM7 (ext full C, ext full L, S, D)");
            break;
        case 1230:
            len = snprintf(info, size, "GLONASS code-phase biases");
            break;
        case 4072:
            if (msgSize > (RTCM3_HEAD_SIZE + 2 + 1))
            {
                const uint32_t subtype = RTCM3_4072_SUBTYPE(msg);
                switch (subtype)
                {
                    case 0:
                        len = snprintf(info, size, "u-blox proprietary: Reference station PVT");
                        break;
                    case 1:
                        len = snprintf(info, size, "u-blox proprietary: Additional reference station information");
                        break;
                    default:
                        len = snprintf(info, size, "u-blox proprietary: Unknown sub-type %u", subtype);
                        break;
                }
            }
            break;
    }

    return (len > 0) && (len < size);
}


/* ********************************************************************************************** */
// eof
