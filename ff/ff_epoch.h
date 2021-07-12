// flipflip's navigation epoch abstraction
//
// Copyright (c) 2020 Philippe Kehl (flipflip at oinkzwurgl dot org),
// https://oinkzwurgl.org/hacking/ubloxcfg
//
// Copyright (c) 2021 Charles Parent (charles.parent@orolia2s.com)
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

/* ****************************************************************************************************************** */

typedef enum EPOCH_FIX_e
{
    EPOCH_FIX_UNKNOWN = 0,
    EPOCH_FIX_NOFIX,
    EPOCH_FIX_DRONLY,
    EPOCH_FIX_TIME,
    EPOCH_FIX_S2D,
    EPOCH_FIX_S3D,
    EPOCH_FIX_S3D_DR,
    EPOCH_FIX_RTK_FLOAT,
    EPOCH_FIX_RTK_FIXED,
    EPOCH_FIX_RTK_FLOAT_DR,
    EPOCH_FIX_RTK_FIXED_DR,
} EPOCH_FIX_t;

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
    EPOCH_SIGNAL_UNKNOWN = 0,               //!< Unspecified signal
    EPOCH_SIGNAL_GPS_L1CA,                  //!< GPS L1 C/A signal
    EPOCH_SIGNAL_GPS_L2C,                   //!< GPS L2 C signal
    EPOCH_SIGNAL_SBAS_L1CA,                 //!< SBAS L1 C/A signal
    EPOCH_SIGNAL_GAL_E1,                    //!< Galileo E1 signal
    EPOCH_SIGNAL_GAL_E5B,                   //!< Galileo E5b signal
    EPOCH_SIGNAL_BDS_B1I,                   //!< BeiDou B1I signal
    EPOCH_SIGNAL_BDS_B2I,                   //!< BeiDou B2I signal
    EPOCH_SIGNAL_QZSS_L1CA,                 //!< QZSS L1 C/A signal
    EPOCH_SIGNAL_QZSS_L1S,                  //!< QZSS L1 S signal
    EPOCH_SIGNAL_QZSS_L2C,                  //!< QZSS L2 CM signal
    EPOCH_SIGNAL_GLO_L1OF,                  //!< GLONASS L1 OF signal
    EPOCH_SIGNAL_GLO_L2OF,                  //!< GLONASS L2 OF signal
    // Keep in sync with kEpochSignalStrs!
} EPOCH_SIGNAL_t;

typedef enum EPOCH_SIGUSE_e
{
    EPOCH_SIGUSE_UNKNOWN = 0,
    EPOCH_SIGUSE_NONE,
    EPOCH_SIGUSE_SEARCH,
    EPOCH_SIGUSE_ACQUIRED,
    EPOCH_SIGUSE_UNUSED,
    EPOCH_SIGUSE_CODELOCK,
    EPOCH_SIGUSE_CARRLOCK,
    // Keep in sync with kEpochSiqUseStrs!
} EPOCH_SIGUSE_t;

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

typedef enum EPOCH_BAND_e
{
    EPOCH_BAND_UNKNOWN = 0,
    EPOCH_BAND_L1,    //!< L1 band (~1.5GHz)
    EPOCH_BAND_L2,    //!< L2 band (~1.2GHz)
    EPOCH_BAND_L5,    //!< L5 band (~1.1GHz)
    // Keep in sync with kEpochBandStrs!
} EPOCH_BAND_t;

typedef struct EPOCH_SIGINFO_s
{
    EPOCH_GNSS_t      gnss;
    uint8_t           sv;
    EPOCH_SIGNAL_t    signal;
    EPOCH_BAND_t      band;
    int8_t            gloFcn;
    float             prRes;
    float             cno;
    EPOCH_SIGUSE_t    use;
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
    const char       *bandStr;
    const char       *useStr;
    const char       *corrStr;
    const char       *ionoStr;
    const char       *healthStr;

    // Private
    uint32_t          _order;
} EPOCH_SIGINFO_t;

typedef enum EPOCH_SATORB_e
{
    EPOCH_SATORB_NONE = 0,
    EPOCH_SATORB_EPH,
    EPOCH_SATORB_ALM,
    EPOCH_SATORB_PRED,
    EPOCH_SATORB_OTHER,
    // Keep in sync with kEpochOrbStrs!
} EPOCH_SATORB_t;

typedef struct EPOCH_SATINFO_s
{
    EPOCH_GNSS_t      gnss;
    uint8_t           sv;
    EPOCH_SATORB_t    orbUsed;
    int               orbAvail;
    int8_t            elev;  // only valid if orbit > NONE
    int16_t           azim;  // only valid if orbit > NONE
    const char       *gnssStr;
    const char       *svStr;
    const char       *orbUsedStr;

    // Private
    uint32_t          _order;
} EPOCH_SATINFO_t;

#define EPOCH_SIGCNOHIST_NUM 12

typedef struct EPOCH_s
{
    // Public
    bool                valid;
    uint32_t            seq;
    uint32_t            ts;
    char                str[256];

    bool                haveFix;
    EPOCH_FIX_t         fix;
    bool                fixOk;
    const char         *fixStr;

    bool                haveNumSv;
    int                 numSv;

    bool                havePdop;
    float               pDOP;

    bool                havePos;
    double              llh[3];
    double              xyz[3];
    double              horizAcc;
    double              vertAcc;
    double              posAcc;

    bool                haveRelPos;
    double              relLen;
    double              relNed[3];
    double              relAcc[3];

    bool                haveMsl;
    double              heightMsl;

    bool                haveTime;
    bool                confTime;
    bool                leapSecKnown;
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

    EPOCH_SATINFO_t     satellites[100];
    int                 numSatellites;

    bool                haveNumSig;
    int                 numSigUsed;
    int                 numSigUsedGps;
    int                 numSigUsedGlo;
    int                 numSigUsedGal;
    int                 numSigUsedBds;
    int                 numSigUsedSbas;
    int                 numSigUsedQzss;

    bool                haveNumSat;
    int                 numSatUsed;
    int                 numSatUsedGps;
    int                 numSatUsedGlo;
    int                 numSatUsedGal;
    int                 numSatUsedBds;
    int                 numSatUsedSbas;
    int                 numSatUsedQzss;

    bool                haveSigCnoHist;
    int                 sigCnoHistTrk[EPOCH_SIGCNOHIST_NUM];
    int                 sigCnoHistNav[EPOCH_SIGCNOHIST_NUM];

    bool                haveLeapSeconds;
    int                 leapSeconds;

    // Information as per UBX-NAV-TIMELS
    // Warning: This is subject to changed (simplified) in future versions of this library. Users are advised
    //          to directly use the UBX-NAV-TIMELS message if they have to rely on u-blox specific behaviour.
    bool                haveLeapSecondEvent;
    int                 srcOfCurrLs;
    int                 srcOfLsChange;
    int                 lsChange;
    int                 timeToLsEvent;
    int                 dateOfLsGpsWn;
    int                 dateOfLsGpsDn;

    // Private
    int          _haveFix;
    int          _haveTime;
    int          _haveDate;
    int          _haveLlh;
    int          _haveHacc;
    int          _haveVacc;
    int          _havePacc;
    int          _haveXyz;
    int          _haveSig;
    int          _haveSat;
    int          _haveGpsTow;
    int          _haveGpsWeek;
    int          _haveRelPos;
    bool         _relPosValid;

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

/* ****************************************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_EPOCH_H__
