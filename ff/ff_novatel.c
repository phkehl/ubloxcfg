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

bool novatelMessageInfo(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    UNUSED(info);
    UNUSED(size);
    UNUSED(msg);
    UNUSED(msgSize);
    return false;
}

/* ****************************************************************************************************************** */
// eof
