// flipflip's UBX protocol stuff
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
#include <stdio.h>
#include <inttypes.h>

#include "ff_stuff.h"
#include "ff_ubx.h"

/* ********************************************************************************************** */

int ubxMakeMessage(const uint8_t clsId, const uint8_t msgId, const uint8_t *payload, const uint16_t payloadSize, uint8_t *msg)
{
    if ( (payload != NULL) && (payloadSize > 0) )
    {
        memmove(&msg[6], payload, payloadSize);
    }
    const int msgSize = payloadSize + UBX_FRAME_SIZE;
    msg[0] = UBX_SYNC_1;
    msg[1] = UBX_SYNC_2;
    msg[2] = clsId;
    msg[3] = msgId;
    msg[4] = (payloadSize & 0xff);
    msg[5] = (payloadSize >> 8);
    uint8_t a = 0;
    uint8_t b = 0;
    uint8_t *pMsg = &msg[2];
    int cnt = msgSize - 4;
    while (cnt > 0)
    {
        a += *pMsg;
        b += a;
        pMsg++;
        cnt--;
    }
    *pMsg = a; pMsg++;
    *pMsg = b;
    return payloadSize + UBX_FRAME_SIZE;
}

typedef struct MSGINFO_s
{
    uint8_t     clsId;
    uint8_t     msgId;
    const char *msgName;
} MSGINFO_t;

#define _P_MSGINFO(_clsId_, _msgId_, _msgName_) { .clsId = (_clsId_), .msgId = (_msgId_), .msgName = (_msgName_) },
#define _P_MSGINFO_UNKN(_clsId_, _clsName_) { .clsId = (_clsId_), .msgName = (_clsName_) },

bool ubxMessageName(char *name, const int size, const uint8_t *msg, const int msgSize)
{
    if (name == NULL)
    {
        return false;
    }
    if ( (msgSize < (UBX_HEAD_SIZE)) || (msg == NULL) )
    {
        name[0] = '\0';
        return false;
    }

    const uint8_t clsId = UBX_CLSID(msg);
    const uint8_t msgId = UBX_MSGID(msg);
    const MSGINFO_t msgInfo[] =
    {
        UBX_MESSAGES(_P_MSGINFO)
    };
    const MSGINFO_t unknInfo[] =
    {
        UBX_CLASSES(_P_MSGINFO_UNKN)
    };
    int res = 0;
    for (int ix = 0; ix < NUMOF(msgInfo); ix++)
    {
        if ( (msgInfo[ix].clsId == clsId) && (msgInfo[ix].msgId == msgId) )
        {
            res = snprintf(name, size, "%s", msgInfo[ix].msgName);
            break;
        }
    }
    if (res == 0)
    {
        for (int ix = 0; ix < NUMOF(unknInfo); ix++)
        {
            if (unknInfo[ix].clsId == clsId)
            {
                res = snprintf(name, size, "%s-%02"PRIX8, unknInfo[ix].msgName, msgId);
                break;
            }
        }
    }
    if (res == 0)
    {
        res = snprintf(name, size, "UBX-%02"PRIX8"-%02"PRIX8, clsId, msgId);
    }

    return res < size;
}

bool ubxMessageInfo(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    if (info == NULL)
    {
        return false;
    }
    if ( (msg == NULL) || (msgSize < UBX_FRAME_SIZE) )
    {
        info[0] = '\0';
        return false;
    }

    const uint8_t clsId = UBX_CLSID(msg);
    const uint8_t msgId = UBX_MSGID(msg);
    int len = 0;
    int remSize = size;
    switch (clsId)
    {
        case UBX_NAV_CLSID:
            switch (msgId)
            {
                case UBX_NAV_PVT_MSGID:
                case UBX_NAV_SAT_MSGID:
                case UBX_NAV_ORB_MSGID:
                case UBX_NAV_STATUS_MSGID:
                case UBX_NAV_SIG_MSGID:
                case UBX_NAV_CLOCK_MSGID:
                case UBX_NAV_DOP_MSGID:
                case UBX_NAV_POSECEF_MSGID:
                case UBX_NAV_HPPOSECEF_MSGID:
                case UBX_NAV_POSLLH_MSGID:
                case UBX_NAV_HPPOSLLH_MSGID:
                case UBX_NAV_RELPOSNED_MSGID:
                case UBX_NAV_VELECEF_MSGID:
                case UBX_NAV_VELNED_MSGID:
                case UBX_NAV_SVIN_MSGID:
                case UBX_NAV_EOE_MSGID:
                case UBX_NAV_GEOFENCE_MSGID:
                case UBX_NAV_ODO_MSGID:
                case UBX_NAV_TIMEUTC_MSGID:
                case UBX_NAV_TIMELS_MSGID:
                case UBX_NAV_TIMEGPS_MSGID:
                case UBX_NAV_TIMEGLO_MSGID:
                case UBX_NAV_TIMEBDS_MSGID:
                case UBX_NAV_TIMEGAL_MSGID:
                    if (msgSize > (UBX_FRAME_SIZE + (int)sizeof(uint32_t)))
                    {
                        uint32_t iTOW;
                        memcpy(&iTOW, &msg[UBX_HEAD_SIZE], sizeof(iTOW));
                        const int n = snprintf(&info[len], remSize, "%09.3f", (double)iTOW * 1e-3);
                        len += n;
                        remSize -= n;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return (len > 0) && (len < size);
}


/* ********************************************************************************************** */
// eof
