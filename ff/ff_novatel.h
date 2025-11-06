// clang-format off
// flipflip's NOVATEL protocol stuff
//
// Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org) and contributors
// https://oinkzwurgl.org/projaeggd/ubloxcfg/
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

#ifndef __FF_NOVATEL_H__
#define __FF_NOVATEL_H__

#include <stdint.h>
#include <stdbool.h>

#include "ubloxcfg/ubloxcfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */

#define NOVATEL_SYNC_1       0xaa
#define NOVATEL_SYNC_2       0x44
#define NOVATEL_SYNC_3_LONG  0x12
#define NOVATEL_SYNC_3_SHORT 0x13

#define NOVATEL_MSGID(msg) ( ((uint16_t)((uint8_t *)msg)[5] << 8) | (uint16_t)((uint8_t *)msg)[4] )

#define NOVATEL_BESTPOS_MSGID             42
#define NOVATEL_BESTXYZ_MSGID            241
#define NOVATEL_BESTUTM_MSGID            726
#define NOVATEL_BESTVEL_MSGID             99
#define NOVATEL_CORRIMUS_MSGID          2264
#define NOVATEL_HEADING2_MSGID          1335
#define NOVATEL_IMURATECORRIMUS_MSGID   1362
#define NOVATEL_INSCONFIG_MSGID         1945
#define NOVATEL_INSPVAS_MSGID            508
#define NOVATEL_INSPVAX_MSGID           1465
#define NOVATEL_INSSTDEV_MSGID          2051
#define NOVATEL_PSRDOP2_MSGID           1163
#define NOVATEL_RXSTATUS_MSGID            93
#define NOVATEL_TIME_MSGID               101
#define NOVATEL_RAWIMUSX_MSGID          1462
#define NOVATEL_RAWDMI_MSGID            2269
#define NOVATEL_RAWIMU_MSGID             268
#define NOVATEL_INSPVA_MSGID             507
#define NOVATEL_BESTGNSSPOS_MSGID       1429

#define NOVATEL_MESSAGES(_P_) \
    _P_(NOVATEL_BESTPOS_MSGID,          "BESTPOS") \
    _P_(NOVATEL_BESTXYZ_MSGID,          "BESTXYZ") \
    _P_(NOVATEL_BESTUTM_MSGID,          "BESTUTM") \
    _P_(NOVATEL_BESTVEL_MSGID,          "BESTVEL") \
    _P_(NOVATEL_CORRIMUS_MSGID,         "CORRIMUS") \
    _P_(NOVATEL_HEADING2_MSGID,         "HEADING2") \
    _P_(NOVATEL_IMURATECORRIMUS_MSGID,  "IMURATECORRIMUS") \
    _P_(NOVATEL_INSCONFIG_MSGID,        "INSCONFIG") \
    _P_(NOVATEL_INSPVAS_MSGID,          "INSPVAS") \
    _P_(NOVATEL_INSPVAX_MSGID,          "INSPVAX") \
    _P_(NOVATEL_INSSTDEV_MSGID,         "INSSTDEV") \
    _P_(NOVATEL_PSRDOP2_MSGID,          "PSRDOP2") \
    _P_(NOVATEL_RXSTATUS_MSGID,         "RXSTATUS") \
    _P_(NOVATEL_TIME_MSGID,             "TIME") \
    _P_(NOVATEL_RAWIMUSX_MSGID,         "RAWIMUSX") \
    _P_(NOVATEL_RAWDMI_MSGID,           "RAWDMI") \
    _P_(NOVATEL_RAWIMU_MSGID,           "RAWIMU") \
    _P_(NOVATEL_INSPVA_MSGID,           "INSPVA") \
    _P_(NOVATEL_BESTGNSSPOS_MSGID,      "BESTGNSSPOS")

typedef struct NOVATEL_HEADER_LONG_s
{
    uint8_t  sync1;
    uint8_t  sync2;
    uint8_t  sync3;
    uint8_t  headerLen;
    uint16_t msgId;
    uint8_t  msgType;
    uint8_t  portAddr;
    uint16_t msgLen;
    uint16_t seq;
    uint8_t  idleTime;
    uint8_t  timeStatus;
    uint16_t gpsWeek;
    uint32_t gpsTowMs;
    uint32_t rxStatus;
    uint16_t reserved;
    uint16_t swVersion;
} NOVATEL_HEADER_LONG_t;

typedef struct NOVATEL_HEADER_SHORT_s
{
    uint8_t  sync1;
    uint8_t  sync2;
    uint8_t  sync3;
    uint8_t  msgLen;
    uint16_t msgId;
    uint16_t gpsWeek;
    uint32_t gpsTowMs;
} NOVATEL_HEADER_SHORT_t;

typedef struct NOVATEL_RAWDMI_PAYLOAD_s
{
    int32_t dmi1;
    int32_t dmi2;
    int32_t dmi3;
    int32_t dmi4;
    int32_t mask;
} NOVATEL_RAWDMI_PAYLOAD_t;

bool novatelMessageName(char *name, const int size, const uint8_t *msg, const int msgSize);
bool novatelMessageInfo(char *info, const int size, const uint8_t *msg, const int msgSize);

/* ****************************************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_UBX_H__
