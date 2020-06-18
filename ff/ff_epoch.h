// flipflip's navigation epoch abstraction
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

// This is a UBX, NMEA and RTCM3 message parser. It only parses the frames, not the content
// of the message (it does not decode any message fields).
// The parser will pass-through all data that is input. Unknown parts (other protocols,
// spurious data, incorrect messages, etc.) are output as GARBAGE type messages. GARBAGE messages
// are not guaranteed to be combined and can be split arbitrarily (into several GARBAGE messages).

#ifndef __FF_EPOCH_H__
#define __FF_EPOCH_H__

#include <stdint.h>
#include <stdbool.h>

#include "ff_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ********************************************************************************************** */

typedef enum EPOCH_FIX_e
{
    EPOCH_FIX_UNKNOWN = 0,
    EPOCH_FIX_NOFIX,
    EPOCH_FIX_DRONLY,
    EPOCH_FIX_2D,
    EPOCH_FIX_3D,
    EPOCH_FIX_3D_DR,
    EPOCH_FIX_TIME
} EPOCH_FIX_t;

typedef enum EPOCH_QUAL_e
{
    EPOCH_QUAL_UNKNOWN = 0,
    EPOCH_QUAL_MASKED,
    EPOCH_QUAL_OK
} EPOCH_QUAL_t;

typedef enum EPOCH_RTK_e
{
    EPOCH_RTK_UNKNOWN = 0,
    EPOCH_RTK_NONE,
    EPOCH_RTK_FLOAT,
    EPOCH_RTK_FIXED
} EPOCH_RTK_t;

typedef enum EPOCH_VALID_e
{
    EPOCH_VALID_UNKNOWN = 0,
    EPOCH_VALID_FALSE,
    EPOCH_VALID_TRUE,
    EPOCH_VALID_CONFIRMED

} EPOCH_VALID_t;

typedef struct EPOCH_s
{
    // Public

    uint32_t      seq;
    char          str[256];

    EPOCH_FIX_t   fix;
    EPOCH_QUAL_t  fixQual;
    const char   *fixStr;
    const char   *fixQualStr;
    EPOCH_VALID_t validFix;

    int           numSv;
    EPOCH_VALID_t validNumSv;

    EPOCH_RTK_t   rtk;
    const char   *rtkStr;
    EPOCH_VALID_t validRtk;

    float         pDOP;
    EPOCH_VALID_t validPdop;

    double        lat;
    double        lon;
    double        height;
    EPOCH_VALID_t validLlh;

    double        horizAcc;
    double        vertAcc;
    EPOCH_VALID_t validPosAcc;

    double        x;
    double        y;
    double        z;
    EPOCH_VALID_t validXyz;

    double        heightMsl;
    EPOCH_VALID_t validMsl;

    int           hour;
    int           minute;
    double        second;
    EPOCH_VALID_t validTime;

    double        timeAcc;
    EPOCH_VALID_t validTimeAcc;

    int           day;
    int           month;
    int           year;
    EPOCH_VALID_t validDate;
    
    int           gpsWeek;
    EPOCH_VALID_t validGpsWeek;

    double        gpsTow;
    double        gpsTowAcc;
    EPOCH_VALID_t validGpsTow;

    // Private
    bool         _haveUbxNavPvt;
    bool         _haveUbxNavHpposLlh;
    uint32_t     _ubxItow;
    bool         _haveUbxItow;

} EPOCH_t;

void epochInit(EPOCH_t *coll);

bool epochCollect(EPOCH_t *coll, PARSER_MSG_t *msg, EPOCH_t *epoch);



/* ********************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_EPOCH_H__
