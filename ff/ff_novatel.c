// flipflip's NOVATEL protocol stuff
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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>

#include "ff_stuff.h"
#include "ff_debug.h"

#include "ff_novatel.h"

/* ****************************************************************************************************************** */

typedef struct MSGINFO_s
{
    uint16_t    msgId;
    const char *msgName;
} MSGINFO_t;

#define _P_MSGINFO(_msgId_, _msgName_) { .msgId = (_msgId_), .msgName = (_msgName_) },

static const MSGINFO_t kMsgInfo[] =
{
    NOVATEL_MESSAGES(_P_MSGINFO)
};

bool novatelMessageName(char *name, const int size, const uint8_t *msg, const int msgSize)
{
    if ( (name == NULL) || (size < 1) || (msgSize < 12) || (msg == NULL) )
    {
        return false;
    }

    const uint16_t msgId = NOVATEL_MSGID(msg);

    for (int ix = 0; ix < NUMOF(kMsgInfo); ix++)
    {
        if (kMsgInfo[ix].msgId == msgId)
        {
            return snprintf(name, size, "NOVATEL-%s", kMsgInfo[ix].msgName) < size;
        }
    }

    return snprintf(name, size, "NOVATEL-%" PRIu16, msgId) < size;
}

// ---------------------------------------------------------------------------------------------------------------------

static int _strNovatelRawdmi(char *info, const int size, const uint8_t *msg, const int msgSize);
static int _strNovatelHeader(char *info, const int size, const uint8_t *msg, const int msgSize);

bool novatelMessageInfo(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    if ( (info == NULL) || (size < 1) )
    {
        return false;
    }
    if ( (msg == NULL) || (msgSize < 6) )
    {
        info[0] = '\0';
        return false;
    }

    const uint16_t msgId = NOVATEL_MSGID(msg);
    int len = 0;
    switch (msgId)
    {
        case NOVATEL_RAWDMI_MSGID:
            len = _strNovatelRawdmi(info, size, msg, msgSize);
            break;
        case NOVATEL_BESTPOS_MSGID:
        case NOVATEL_BESTXYZ_MSGID:
        case NOVATEL_BESTUTM_MSGID:
        case NOVATEL_BESTVEL_MSGID:
        case NOVATEL_CORRIMUS_MSGID:
        case NOVATEL_HEADING2_MSGID:
        case NOVATEL_IMURATECORRIMUS_MSGID:
        case NOVATEL_INSCONFIG_MSGID:
        case NOVATEL_INSPVAS_MSGID:
        case NOVATEL_INSPVAX_MSGID:
        case NOVATEL_INSSTDEV_MSGID:
        case NOVATEL_PSRDOP2_MSGID:
        case NOVATEL_RXSTATUS_MSGID:
        case NOVATEL_TIME_MSGID:
        case NOVATEL_RAWIMUSX_MSGID:
        case NOVATEL_RAWIMU_MSGID:
        case NOVATEL_INSPVA_MSGID:
        case NOVATEL_BESTGNSSPOS_MSGID:
            len = _strNovatelHeader(info, size, msg, msgSize);
            break;
    }

    return (len > 0) && (len < size);
}

// ---------------------------------------------------------------------------------------------------------------------

static int _strNovatelHeader(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    if ((msg[2] == NOVATEL_SYNC_3_LONG) && (msgSize >= (int)sizeof(NOVATEL_HEADER_LONG_t)))
    {
        NOVATEL_HEADER_LONG_t hdr;
        memcpy(&hdr, msg, sizeof(hdr));
        return snprintf(info, size, "l %04u:%010.3f", hdr.gpsWeek, (double)hdr.gpsTowMs * 1e-3);
    }
    else if ((msg[2] == NOVATEL_SYNC_3_SHORT) && (msgSize >= (int)sizeof(NOVATEL_HEADER_SHORT_t)))
    {
        NOVATEL_HEADER_SHORT_t hdr;
        memcpy(&hdr, msg, sizeof(hdr));
        return snprintf(info, size, "s %04u:%010.3f", hdr.gpsWeek, (double)hdr.gpsTowMs * 1e-3);
    }
    else
    {
        return 0;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

#define _CHKSIZE(_type_) \
    ( ((msg[2] == NOVATEL_SYNC_3_LONG)  && (msgSize == (sizeof(NOVATEL_HEADER_LONG_t)  + sizeof(_type_) + 4))) || \
      ((msg[2] == NOVATEL_SYNC_3_SHORT) && (msgSize == (sizeof(NOVATEL_HEADER_SHORT_t) + sizeof(_type_) + 4))) )

#define _PAYLOAD(_type_, _dst_) \
    _type_ _dst_; \
    memcpy(&_dst_, &msg[ msg[2] == NOVATEL_SYNC_3_LONG ? sizeof(NOVATEL_HEADER_LONG_t) : sizeof(NOVATEL_HEADER_SHORT_t) ], sizeof(_dst_))


static int _strNovatelRawdmi(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    int len = _strNovatelHeader(info, size, msg, msgSize);
    if (_CHKSIZE(NOVATEL_RAWDMI_PAYLOAD_t))
    {
        _PAYLOAD(NOVATEL_RAWDMI_PAYLOAD_t, dmi);
        len += snprintf(&info[len], size - len, " [%c]=%d [%c]=%d [%c]=%d [%c]=%d",
            CHKBITS(dmi.mask, BIT(0)) ? '1' : '.', dmi.dmi1,
            CHKBITS(dmi.mask, BIT(1)) ? '2' : '.', dmi.dmi2,
            CHKBITS(dmi.mask, BIT(2)) ? '3' : '.', dmi.dmi3,
            CHKBITS(dmi.mask, BIT(3)) ? '4' : '.', dmi.dmi4);
    }

    return len;
}

/* ****************************************************************************************************************** */
// eof
