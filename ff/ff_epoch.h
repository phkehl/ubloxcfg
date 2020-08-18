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
    EPOCH_FIX_NOFIX = 0,
    EPOCH_FIX_DRONLY,
    EPOCH_FIX_2D,
    EPOCH_FIX_3D,
    EPOCH_FIX_3D_DR,
    EPOCH_FIX_TIME
} EPOCH_FIX_t;

typedef enum EPOCH_RTK_e
{
    EPOCH_RTK_NONE = 0,
    EPOCH_RTK_FLOAT,
    EPOCH_RTK_FIXED
} EPOCH_RTK_t;

typedef enum EPOCH_GNSS_e
{
    EPOCH_GNSS_UNKNOWN = 0,
    EPOCH_GNSS_GPS,
    EPOCH_GNSS_GLO,
    EPOCH_GNSS_BDS,
    EPOCH_GNSS_GAL,
    EPOCH_GNSS_SBAS,
    EPOCH_GNSS_QZSS,
    // Keep in sync with kEpochGnssStrs!
} EPOCH_GNSS_t;

typedef enum EPOCH_SIGNAL_e
{
    EPOCH_SIGNAL_UNKNOWN = 0,
    EPOCH_SIGNAL_GPS_L1CA,
    EPOCH_SIGNAL_GPS_L2CL,
    EPOCH_SIGNAL_GPS_L2CM,
    EPOCH_SIGNAL_GLO_L1OF,
    EPOCH_SIGNAL_GLO_L2OF,
    EPOCH_SIGNAL_GAL_E1C,
    EPOCH_SIGNAL_GAL_E1B,
    EPOCH_SIGNAL_GAL_E5BI,
    EPOCH_SIGNAL_GAL_E5BQ,
    EPOCH_SIGNAL_BDS_B1ID1,
    EPOCH_SIGNAL_BDS_B1ID2,
    EPOCH_SIGNAL_BDS_B2ID1,
    EPOCH_SIGNAL_BDS_B2ID2,
    EPOCH_SIGNAL_SBAS_L1CA,
    EPOCH_SIGNAL_QZSS_L1CA,
    EPOCH_SIGNAL_QZSS_L1S,
    EPOCH_SIGNAL_QZSS_L2CM,
    EPOCH_SIGNAL_QZSS_L2CL,
    // Keep in sync with kEpochSignalStrs!
} EPOCH_SIGNAL_t;

typedef enum EPOCH_SIGQUAL_e
{
    EPOCH_SIGQUAL_UNKNOWN = 0,
    EPOCH_SIGQUAL_NONE,
    EPOCH_SIGQUAL_SEARCH,
    EPOCH_SIGQUAL_ACQUIRED,
    EPOCH_SIGQUAL_UNUSED,
    EPOCH_SIGQUAL_CODELOCK,
    EPOCH_SIGQUAL_CARRLOCK,
    // Keep in sync with kEpochSiqQualStrs!
} EPOCH_SIGQUAL_t;

typedef enum EPOCH_SIGCORR_e
{
    EPOCH_SIGCORR_UNKNOWN = 0,
    EPOCH_SIGCORR_NONE,
    EPOCH_SIGCORR_SBAS,
    EPOCH_SIGCORR_BDS,
    EPOCH_SIGCORR_RTCM2,
    EPOCH_SIGCORR_RTCM3_OSR,
    EPOCH_SIGCORR_RTCM3_SSR,
    EPOCH_SIGCORR_QZSS_SLAS,
    // Keep in sync with kEpochSigCorrStrs!
} EPOCH_SIGCORR_t;

typedef enum EPOCH_SIGIONO_e
{
    EPOCH_SIGIONO_UNKNOWN = 0,
    EPOCH_SIGIONO_NONE,
    EPOCH_SIGIONO_KLOB_GPS,
    EPOCH_SIGIONO_KLOB_BDS,
    EPOCH_SIGIONO_SBAS,
    EPOCH_SIGIONO_DUAL_FREQ,
    // Keep in sync with kEpochSigIonoStrs!
} EPOCH_SIGIONO_t;

typedef enum EPOCH_SIGHEALTH_e
{
    EPOCH_SIGHEALTH_UNKNOWN = 0,
    EPOCH_SIGHEALTH_HEALTHY,
    EPOCH_SIGHEALTH_UNHEALTHY,
    // Keep in sync with kEpochSigHealthStrs!
} EPOCH_SIGHEALTH_t;

typedef struct EPOCH_SIGINFO_s
{
    EPOCH_GNSS_t      gnss;
    uint8_t           sv;
    EPOCH_SIGNAL_t    signal;
    int8_t            gloFcn;
    float             prRes;
    float             cno;
    EPOCH_SIGQUAL_t   qual;
    EPOCH_SIGCORR_t   corr;
    EPOCH_SIGIONO_t   iono;
    EPOCH_SIGHEALTH_t health;
    bool              prUsed;
    bool              crUsed;
    bool              doUsed;
    bool              prCorrUsed;
    bool              crCorrUsed;
    bool              doCorrUsed;
    const char       *gnssStr;
    const char       *svStr;
    const char       *signalStr;
    const char       *qualStr;
    const char       *corrStr;
    const char       *ionoStr;
    const char       *healthStr;

    // Private
    uint32_t          _order;
} EPOCH_SIGINFO_t;

typedef struct EPOCH_s
{
    // Public
    bool                valid;
    uint32_t            seq;
    char                str[256];

    bool                haveFix;
    EPOCH_FIX_t         fix;
    bool                fixOk;
    const char         *fixStr;
    EPOCH_RTK_t         rtk;
    const char         *rtkStr;

    bool                haveNumSv;
    int                 numSv;

    bool                havePdop;
    float               pDOP;

    bool                havePos;
    double              llh[3];
    double              xyz[3];
    double              horizAcc;
    double              vertAcc;

    bool                haveMsl;
    double              heightMsl;

    bool                haveTime;
    bool                confTime;
    int                 hour;
    int                 minute;
    double              second;
    double              timeAcc;

    bool                haveDate;
    bool                confDate;
    int                 day;
    int                 month;
    int                 year;

    bool                haveGpsWeek;    
    int                 gpsWeek;

    bool                haveGpsTow;
    double              gpsTow;
    double              gpsTowAcc;

    EPOCH_SIGINFO_t     signals[100];
    int                 numSignals;

    // Private   
    bool         _haveUbxNavPvt;
    bool         _haveUbxNavSig;
    bool         _haveUbxNavHpposLlh;
    bool         _haveLlh;
    bool         _haveXyz;

    uint32_t     _detectTow;
    bool         _detectHaveTow;
} EPOCH_t;

#define EPOCH_NUM_GPS        32
#define EPOCH_NUM_SBAS       39
#define EPOCH_NUM_GAL        36
#define EPOCH_NUM_BDS        37
#define EPOCH_NUM_QZSS       10
#define EPOCH_NUM_GLO        32
#define EPOCH_FIRST_GPS       1
#define EPOCH_FIRST_SBAS    120
#define EPOCH_FIRST_GAL       1
#define EPOCH_FIRST_BDS       1
#define EPOCH_FIRST_QZSS      1
#define EPOCH_FIRST_GLO       1


void epochInit(EPOCH_t *coll);

bool epochCollect(EPOCH_t *coll, PARSER_MSG_t *msg, EPOCH_t *epoch);

const char *epochStrHeader(void);

/* ********************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_EPOCH_H__
