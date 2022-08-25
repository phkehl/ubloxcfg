// flipflip's NOVATEL protocol stuff
//
// Copyright (c) 2022 Philippe Kehl (flipflip at oinkzwurgl dot org),
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
    }

    return (len > 0) && (len < size);
}

#define _CHKSIZE(_type_) \
    ( ((msg[2] == NOVATEL_SYNC_3_LONG)  && (msgSize == (sizeof(NOVATEL_HEADER_LONG_t)  + sizeof(_type_) + 4))) || \
      ((msg[2] == NOVATEL_SYNC_3_SHORT) && (msgSize == (sizeof(NOVATEL_HEADER_SHORT_t) + sizeof(_type_) + 4))) )

#define _PAYLOAD(_type_, _dst_) \
    _type_ _dst_; \
    memcpy(&_dst_, &msg[ msg[2] == NOVATEL_SYNC_3_LONG ? sizeof(NOVATEL_HEADER_LONG_t) : sizeof(NOVATEL_HEADER_SHORT_t) ], sizeof(_dst_))

static int _strNovatelRawdmi(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    int len = 0;
    if (_CHKSIZE(NOVATEL_RAWDMI_PAYLOAD_t))
    {
        _PAYLOAD(NOVATEL_RAWDMI_PAYLOAD_t, dmi);
        len = snprintf(info, size, "[%c]=%d [%c]=%d [%c]=%d [%c]=%d",
            CHKBITS(dmi.mask, BIT(0)) ? '1' : '.', dmi.dmi1,
            CHKBITS(dmi.mask, BIT(1)) ? '2' : '.', dmi.dmi2,
            CHKBITS(dmi.mask, BIT(2)) ? '3' : '.', dmi.dmi3,
            CHKBITS(dmi.mask, BIT(3)) ? '4' : '.', dmi.dmi3);
    }

    return len;
}

/* ****************************************************************************************************************** */
// eof
