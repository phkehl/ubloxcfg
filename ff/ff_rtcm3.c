// flipflip's RTCM3 protocol stuff
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
#include "ff_rtcm3.h"

/* ****************************************************************************************************************** */

bool rtcm3MessageName(char *name, const int size, const uint8_t *msg, const int msgSize)
{
    if ( (name == NULL) || (size < 1) )
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

const char *rtcm3TypeDesc(const int msgType, const int subType)
{
    switch (msgType)
    {
        case 1001: return "L1-only GPS RTK observables";
        case 1002: return "Extended L1-only GPS RTK observables";
        case 1003: return "L1/L2 GPS RTK observables";
        case 1004: return "Extended L1/L2 GPS RTK observables";
        case 1005: return "Stationary RTK reference station ARP";
        case 1006: return "Stationary RTK reference station ARP with antenna height";
        case 1007: return "Antenna descriptor";
        case 1008: return "Antenna descriptor and serial number";
        case 1009: return "L1-only GLONASS RTK observables";
        case 1032: return "Physical reference station position message";
        case 1033: return "Receiver and antenna descriptors";

        case 1010: return "Extended L1-only GLONASS RTK observables";
        case 1011: return "L1/L2 GLONASS RTK observables";
        case 1012: return "Extended L1/L2 GLONASS RTK observables";
        case 1030: return "GPS network RTK residual message";
        case 1031: return "GLONASS network RTK residual message";
        case 1230: return "GLONASS code-phase biases";

        case 1071: return "GPS MSM1 (C)";
        case 1072: return "GPS MSM2 (L)";
        case 1073: return "GPS MSM3 (C, L)";
        case 1074: return "GPS MSM4 (full C, full L, S)";
        case 1075: return "GPS MSM5 (full C, full L, S, D)";
        case 1076: return "GPS MSM6 (ext full C, ext full L, S)";
        case 1077: return "GPS MSM7 (ext full C, ext full L, S, D)";
        case 1081: return "GLONASS MSM1 (C)";
        case 1082: return "GLONASS MSM2 (L)";
        case 1083: return "GLONASS MSM3 (C, L)";
        case 1084: return "GLONASS MSM4 (full C, full L, S)";
        case 1085: return "GLONASS MSM5 (full C, full L, S, D)";
        case 1086: return "GLONASS MSM6 (ext full C, ext full L, S)";
        case 1087: return "GLONASS MSM7 (ext full C, ext full L, S, D)";
        case 1091: return "Galileo MSM1 (C)";
        case 1092: return "Galileo MSM2 (L)";
        case 1093: return "Galileo MSM3 (C, L)";
        case 1094: return "Galileo MSM4 (full C, full L, S)";
        case 1095: return "Galileo MSM5 (full C, full L, S, D)";
        case 1096: return "Galileo MSM6 (full C, full L, S)";
        case 1097: return "Galileo MSM7 (ext full C, ext full L, S, D)";
        case 1121: return "BeiDou MSM1 (C)";
        case 1122: return "BeiDou MSM2 (L)";
        case 1123: return "BeiDou MSM3 (C, L)";
        case 1124: return "BeiDou MSM4 (full C, full L, S)";
        case 1125: return "BeiDou MSM5 (full C, full L, S, D)";
        case 1126: return "BeiDou MSM6 (full C, full L, S)";
        case 1127: return "BeiDou MSM7 (ext full C, ext full L, S, D)";
        case 1101: return "SBAS MSM1 (C)";
        case 1102: return "SBAS MSM2 (L)";
        case 1103: return "SBAS MSM3 (C, L)";
        case 1104: return "SBAS MSM4 (full C, full L, S)";
        case 1105: return "SBAS MSM5 (full C, full L, S, D)";
        case 1106: return "SBAS MSM6 (full C, full L, S)";
        case 1107: return "SBAS MSM7 (ext full C, ext full L, S, D)";
        case 1111: return "QZSS MSM1 (C)";
        case 1112: return "QZSS MSM2 (L)";
        case 1113: return "QZSS MSM3 (C, L)";
        case 1114: return "QZSS MSM4 (full C, full L, S)";
        case 1115: return "QZSS MSM5 (full C, full L, S, D)";
        case 1116: return "QZSS MSM6 (full C, full L, S)";
        case 1117: return "QZSS MSM7 (ext full C, ext full L, S, D)";

        case 1019: return "GPS ephemerides";
        case 1020: return "GLONASS ephemerides";
        case 1042: return "BeiDou satellite ephemeris data";
        case 1044: return "QZSS ephemerides";
        case 1045: return "Galileo F/NAV satellite ephemeris data";
        case 1046: return "Galileo I/NAV satellite ephemeris data";

        case 4072: switch (subType)
                   {
                       case 0: return "u-blox proprietary: Reference station PVT";
                       case 1: return "u-blox proprietary: Additional reference station information";
                   }
                   break;
    }

    return NULL;
}

bool rtcm3MessageInfo(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    if ( (info == NULL) || (size < 1) )
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

    if ( (len == 0) && ((type == 1005) || (type == 1006) || (type == 1032)) )
    {
        RTCM3_ARP_t arp;
        if (rtcm3GetArp(msg, &arp))
        {
            len = snprintf(info, size, "(#%d) %.2f %.2f %.2f - ", arp.refStaId, arp.X, arp.Y, arp.Z);
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
    }

    if (len < size)
    {
        const char *typeDescStr = NULL;
        if (type == 4072)
        {
            const uint32_t subtype = (msgSize > (RTCM3_HEAD_SIZE + 2 + 1)) ? RTCM3_4072_SUBTYPE(msg) : -1;
            typeDescStr = rtcm3TypeDesc(type, subtype);
        }
        else
        {
            typeDescStr = rtcm3TypeDesc(type, 0);
        }
        if (typeDescStr != NULL)
        {
            len += snprintf(&info[len], size - len, "%s", typeDescStr);
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
        if (msm != NULL)
        {
            *msm  = (RTCM3_MSM_TYPE_t)msmVal;
        }
        if (gnss != NULL)
        {
            *gnss = (RTCM3_MSM_GNSS_t)gnssVal;
        }
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

static int64_t sbits(const uint8_t *data, const int offs, const int size)
{
    uint64_t val = 0;
    for (int bo = 0; bo < size; bo++)
    {
        val <<= 1;
        const int bit = offs + bo;
        val |= ( data[bit / 8] >> (7 - (bit % 8)) ) & 0x01;
    }
    // Sign-extend to full 64 bits
    if ((val & ((uint64_t)1 << (size - 1))) != 0)
    {
        for (int bo = size; bo < 64; bo++)
        {
            val |= (uint64_t)1 << bo;
        }
    }
    return (int64_t)val;
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
            arp->refStaId =          bits(data,  12, 12);          // DF003
            arp->X        = (double)sbits(data,  34, 38) * 0.0001; // DF025
            arp->Y        = (double)sbits(data,  74, 38) * 0.0001; // DF026
            arp->Z        = (double)sbits(data, 114, 38) * 0.0001; // DF027
            break;
        case 1032:
            arp->refStaId =          bits(data,  12, 12);          // DF003
            arp->X        = (double)sbits(data,  42, 38) * 0.0001; // DF025
            arp->Y        = (double)sbits(data,  80, 38) * 0.0001; // DF026
            arp->Z        = (double)sbits(data, 118, 38) * 0.0001; // DF027
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

// ---------------------------------------------------------------------------------------------------------------------


bool rtcm3MessageClsId(const char *name, uint8_t *clsId, uint8_t *msgId)
{
    if ( (name == NULL) || (name[0] == '\0') )
    {
        return false;
    }

    const struct { const char *name; uint8_t clsId; uint8_t msgId; } kMsgInfo[] =
    {
        { .name = "RTCM-3X-TYPE1001",   .clsId = 0xf5, .msgId = 0x01 },
        { .name = "RTCM-3X-TYPE1002",   .clsId = 0xf5, .msgId = 0x02 },
        { .name = "RTCM-3X-TYPE1003",   .clsId = 0xf5, .msgId = 0x03 },
        { .name = "RTCM-3X-TYPE1004",   .clsId = 0xf5, .msgId = 0x04 },
        { .name = "RTCM-3X-TYPE1005",   .clsId = 0xf5, .msgId = 0x05 },
        { .name = "RTCM-3X-TYPE1006",   .clsId = 0xf5, .msgId = 0x06 },
        { .name = "RTCM-3X-TYPE1007",   .clsId = 0xf5, .msgId = 0x07 },
        { .name = "RTCM-3X-TYPE1009",   .clsId = 0xf5, .msgId = 0x09 },
        { .name = "RTCM-3X-TYPE1010",   .clsId = 0xf5, .msgId = 0x0a },
        { .name = "RTCM-3X-TYPE1011",   .clsId = 0xf5, .msgId = 0xa1 },
        { .name = "RTCM-3X-TYPE1012",   .clsId = 0xf5, .msgId = 0xa2 },
        { .name = "RTCM-3X-TYPE1033",   .clsId = 0xf5, .msgId = 0x21 },
        { .name = "RTCM-3X-TYPE1074",   .clsId = 0xf5, .msgId = 0x4a },
        { .name = "RTCM-3X-TYPE1075",   .clsId = 0xf5, .msgId = 0x4b },
        { .name = "RTCM-3X-TYPE1077",   .clsId = 0xf5, .msgId = 0x4d },
        { .name = "RTCM-3X-TYPE1084",   .clsId = 0xf5, .msgId = 0x54 },
        { .name = "RTCM-3X-TYPE1085",   .clsId = 0xf5, .msgId = 0x55 },
        { .name = "RTCM-3X-TYPE1087",   .clsId = 0xf5, .msgId = 0x57 },
        { .name = "RTCM-3X-TYPE1094",   .clsId = 0xf5, .msgId = 0x5e },
        { .name = "RTCM-3X-TYPE1095",   .clsId = 0xf5, .msgId = 0x5f },
        { .name = "RTCM-3X-TYPE1097",   .clsId = 0xf5, .msgId = 0x61 },
        { .name = "RTCM-3X-TYPE1124",   .clsId = 0xf5, .msgId = 0x7c },
        { .name = "RTCM-3X-TYPE1125",   .clsId = 0xf5, .msgId = 0x7d },
        { .name = "RTCM-3X-TYPE1127",   .clsId = 0xf5, .msgId = 0x7f },
        { .name = "RTCM-3X-TYPE1230",   .clsId = 0xf5, .msgId = 0xe6 },
        { .name = "RTCM-3X-TYPE4072_0", .clsId = 0xf5, .msgId = 0xfe },
        { .name = "RTCM-3X-TYPE4072_1", .clsId = 0xf5, .msgId = 0xfd },
    };
    for (int ix = 0; ix < NUMOF(kMsgInfo); ix++)
    {
        if (strcmp(kMsgInfo[ix].name, name) == 0)
        {
            if (clsId != NULL)
            {
                *clsId = kMsgInfo[ix].clsId;
            }
            if (msgId != NULL)
            {
                *msgId = kMsgInfo[ix].msgId;
            }
            return true;
        }
    }

    return false;
}

/* ****************************************************************************************************************** */
// eof
