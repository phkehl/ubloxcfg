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

/* ****************************************************************************************************************** */

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


    if ( (len == 0) && ((type == 1005) || (type == 1006) || (type == 1032)) )
    {
        RTCM3_ARP_t arp;
        if (rtcm3GetArp(msg, &arp))
        {
            len = snprintf(info, size, "(#%d) %.2f %.2f %.2f - ", arp.refStaId, arp.X, arp.Y, arp.Z);
        }
        if (len < size)
        {
            switch (type)
            {
                case 1005:
                    len = snprintf(&info[len], size - len, "Stationary RTK reference station ARP");
                    break;
                case 1006:
                    len = snprintf(&info[len], size - len, "Stationary RTK reference station ARP with antenna height");
                    break;
                case 1032:
                    len = snprintf(&info[len], size - len, "Physical reference station position message");
                    break;
            }
        }
    }

    if ( (len == 0) && ((type == 1007) || (type == 1008) || (type == 1033)) )
    {
        RTCM3_ANT_t ant;
        if (rtcm3GetAnt(msg, &ant))
        {
            len = snprintf(info, size, "(#%d) [%s] [%s] %u [%s] [%s] [%s] - ",
                ant.refStaId, ant.antDesc, ant.antSerial, ant.refStaId, ant.rxType, ant.rxFw, ant.rxSerial);
        }
        if (len < size)
        {
            switch (type)
            {
                case 1007:
                    len = snprintf(&info[len], size - len, "Antenna descriptor");
                    break;
                case 1008:
                    len = snprintf(&info[len], size - len, "Antenna descriptor and serial number");
                    break;
                case 1033:
                    len = snprintf(&info[len], size - len, "Receiver and antenna descriptors");
                    break;
            }
        }
    }

    const int gnss = ((type - 1000) / 10) * 10;
    const int msm  = type % 10;

    if ( (len == 0) && (gnss >= RTCM3_MSM_GNSS_GPS) && (gnss <= RTCM3_MSM_GNSS_BDS) &&
         (msm >= RTCM3_MSM_TYPE_1) && (msm <= RTCM3_MSM_TYPE_7) )
    {
        RTCM3_MSM_HEADER_t header;
        if (rtcm3GetMsmHeader(msg, &header))
        {
            len = snprintf(info, size, "(#%d) %010.3f (%d * %d) - ",
                header.refStaId, header.anyTow, header.numSat, header.numSig);
        }

        if (len < size)
        {
            switch (type)
            {
                case 1074:
                    len = snprintf(&info[len], size - len, "GPS MSM4 (full C, full L, S)") + len;
                    break;
                case 1075:
                    len = snprintf(&info[len], size - len, "GPS MSM5 (full C, full L, S, D)") + len;
                    break;
                case 1077:
                    len = snprintf(&info[len], size - len, "GPS MSM7 (ext full C, ext full L, S, D)") + len;
                    break;
                case 1084:
                    len = snprintf(&info[len], size - len, "GLONASS MSM4 (full C, full L, S)") + len;
                    break;
                case 1085:
                    len = snprintf(&info[len], size - len, "GLONASS MSM5 (full C, full L, S, D)") + len;
                    break;
                case 1087:
                    len = snprintf(&info[len], size - len, "GLONASS MSM7 (ext full C, ext full L, S, D)") + len;
                    break;
                case 1094:
                    len = snprintf(&info[len], size - len, "Galileo MSM4 (full C, full L, S)") + len;
                    break;
                case 1095:
                    len = snprintf(&info[len], size - len, "Galileo MSM5 (full C, full L, S, D)") + len;
                    break;
                case 1097:
                    len = snprintf(&info[len], size - len, "Galileo MSM7 (ext full C, ext full L, S, D)") + len;
                    break;
                case 1124:
                    len = snprintf(&info[len], size - len, "BeiDou MSM4 (full C, full L, S)") + len;
                    break;
                case 1125:
                    len = snprintf(&info[len], size - len, "BeiDou MSM5 (full C, full L, S, D)") + len;
                    break;
                case 1127:
                    len = snprintf(&info[len], size - len, "BeiDou MSM7 (ext full C, ext full L, S, D)") + len;
                    break;
            }
        }
    }


    return (len > 0) && (len < size);
}

// ---------------------------------------------------------------------------------------------------------------------

bool rtcm3typeToMsm(int msgType, RTCM3_MSM_GNSS_t *gnss, RTCM3_MSM_TYPE_t *msm)
{
    const int gnssVal = ((msgType - 1000) / 10) * 10;
    const int msmVal  = msgType % 10;
    if ( (msmVal >= RTCM3_MSM_TYPE_1) && (msmVal <= RTCM3_MSM_TYPE_7) &&
         ( (gnssVal == RTCM3_MSM_GNSS_GPS)  || (gnssVal == RTCM3_MSM_GNSS_GLO)  || (gnssVal == RTCM3_MSM_GNSS_GAL) ||
           (gnssVal == RTCM3_MSM_GNSS_SBAS) || (gnssVal == RTCM3_MSM_GNSS_QZSS) || (gnssVal == RTCM3_MSM_GNSS_BDS) ) )
    {
        *msm  = (RTCM3_MSM_TYPE_t)msmVal;
        *gnss = (RTCM3_MSM_GNSS_t)gnssVal;
        return true;
    }
    else
    {
        return false;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

static uint64_t bits(const uint8_t *data, const int offs, const int size)
{
    uint64_t val = 0;
    for (int bo = 0; bo < size; bo++)
    {
        val <<= 1;
        const int bit = offs + bo;
        val |= ( data[bit / 8] >> (7 - (bit % 8)) ) & 0x01;
    }
    return val;
}

static int countbits(uint64_t mask)
{
    int cnt = 0;
    for (; mask != 0; mask >>= 1)
    {
        cnt += mask & 0x1;
    }
    return cnt;
}

bool rtcm3GetMsmHeader(const uint8_t *msg, RTCM3_MSM_HEADER_t *header)
{
    memset(header, 0, sizeof(*header));
    const uint8_t *data = &msg[RTCM3_HEAD_SIZE];

    header->msgType = bits(data, 0, 12); // DF002
    if (!rtcm3typeToMsm(header->msgType, &header->gnss, &header->msm))
    {
        return false; // Not an MSM message
    }

    header->refStaId = bits(data, 12, 12); // DF003
    if (header->gnss == RTCM3_MSM_GNSS_GLO)
    {
        const int dow = bits(data, 24,  3); // DF416
        const int tod = bits(data, 27, 27); // DF034
        header->gloTow = ((double)dow * 86400.0) + ((double)tod * 1e-3);
    }
    else
    {
        header->anyTow   = (double)bits(data, 24, 30) * 1e-3; // DF004, DF416, DF248, DF427
    }

    header->multiMsgBit = bits(data,  54,  1); // DF393
    header->iods        = bits(data,  55,  3); // DF409
    // bit(7) reserved // DF001
    header->clkSteering = bits(data,  65,  2); // DF411
    header->extClock    = bits(data,  67,  2); // DF412
    header->smooth      = bits(data,  69,  1); // DF417
    header->smoothInt   = bits(data,  70,  3); // DF418
    header->satMask     = bits(data,  73, 64); // DF394
    header->sigMask     = bits(data, 137, 32); // DF395

    header->numSat   = countbits(header->satMask);
    header->numSig   = countbits(header->sigMask);
    header->numCell  = header->numSat * header->numSig;
    header->cellMask = bits(data, 169, header->numCell); // DF396

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

bool rtcm3GetArp(const uint8_t *msg, RTCM3_ARP_t *arp)
{
    memset(arp, 0, sizeof(*arp));
    const uint8_t *data = &msg[RTCM3_HEAD_SIZE];

    const int msgType = bits(data, 0, 12); // DF002
    bool res = true;
    switch (msgType)
    {
        case 1005:
        case 1006:
            arp->refStaId = bits(data,  12, 12);          // DF003
            arp->X        = bits(data,  34, 38) * 0.0001; // DF025
            arp->Y        = bits(data,  74, 38) * 0.0001; // DF026
            arp->Z        = bits(data, 114, 38) * 0.0001; // DF027
            break;
        case 1032:
            arp->refStaId = bits(data,  12, 12);          // DF003
            arp->X        = bits(data,  42, 38) * 0.0001; // DF025
            arp->Y        = bits(data,  80, 38) * 0.0001; // DF026
            arp->Z        = bits(data, 118, 38) * 0.0001; // DF027
            break;
        default:
            res = false;
            break;
    }
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

bool rtcm3GetAnt(const uint8_t *msg, RTCM3_ANT_t *ant)
{
    memset(ant, 0, sizeof(*ant));
    const uint8_t *data = &msg[RTCM3_HEAD_SIZE];

    const int msgType = bits(data, 0, 12); // DF002
    bool res = true;
    if ( (msgType == 1007) || (msgType == 1008) || (msgType == 1033) )
    {
        int offs = 12;
        ant->refStaId = bits(data, offs, 12);        // DF003
        offs += 12;
        const int n   = bits(data, offs, 8);         // DF029
        offs += 8;
        for (int ix = 0; (ix < n) && (ix < ((int)sizeof(ant->antDesc)-1)); ix++)
        {
            ant->antDesc[ix] = bits(data, offs, 8);  // DF030
            offs += 8;
        }
        ant->antSetupId = bits(data, offs, 8);       // DF031
        offs += 8;

        if ( (msgType == 1008) || (msgType == 1033) )
        {
            const int m = bits(data, offs, 8);           // DF032
            offs += 8;
            for (int ix = 0; (ix < m) && (ix < ((int)sizeof(ant->antSerial)-1)); ix++)
            {
                ant->antSerial[ix] = bits(data, offs, 8);  // DF033
                offs += 8;
            }
        }

        if ( msgType == 1033 )
        {
            const int i = bits(data, offs, 8);           // DF227
            offs += 8;
            for (int ix = 0; (ix < i) && (ix < ((int)sizeof(ant->rxType)-1)); ix++)
            {
                ant->rxType[ix] = bits(data, offs, 8);   // DF228
                offs += 8;
            }
            const int j = bits(data, offs, 8);           // DF229
            offs += 8;
            for (int ix = 0; (ix < j) && (ix < ((int)sizeof(ant->rxFw)-1)); ix++)
            {
                ant->rxFw[ix] = bits(data, offs, 8);     // DF230
                offs += 8;
            }
            const int k = bits(data, offs, 8);           // DF231
            offs += 8;
            for (int ix = 0; (ix < k) && (ix < ((int)sizeof(ant->rxSerial)-1)); ix++)
            {
                ant->rxSerial[ix] = bits(data, offs, 8); // DF232
                offs += 8;
            }
        }
    }
    else
    {
        res = false;
    }
    return res;
}

/* ****************************************************************************************************************** */
// eof
