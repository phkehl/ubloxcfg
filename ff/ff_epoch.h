/*!
    \file
    \brief Navigation epoch abstraction

    - Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org),
      https://oinkzwurgl.org/hacking/ubloxcfg
    - Copyright (c) 2021 Charles Parent (charles.parent@orolia2s.com)

    This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with this program.
    If not, see <https://www.gnu.org/licenses/>.

    \defgroup FF_EPOCH Navigation epoch abstraction

    \b Concept

    - Collect individual messages from GNSS receivers in different formats into a "navigation epoch" structure
    - This (mostly) receiver-independent structure is a normalisation of the navigation solution, which is output in
      different ways for different receivers (or receiver configurations)
    - Different data can be calculated or derived from other data. For example, position in ECEF can be calculated from
      position in lat/lon/height and vice versa.
    - All data is normalised to SI units, receiver-specific identifiers are mapped to common enums, etc.
    - Data from multiple messages are collected into a single structure
    - Only navigation solution data is part of the epoch. Any auxillary data, such as raw measurements or system status
      messages, are not part of the navigation solution and therefore not part of the epoch. If such correlation is
      needed, it must be done on application level (i.e. not in the epoch code here).
    - The epoch collection works for data received in real-time from a receiver as well as when reading it from a
      logfile.
    - The epoch collection works for multiple, possibly overlapping or redundant, input formats. Currently it supports
      UBX and NMEA input, which can also be mixed.
    - ...

    \b Example

    \code{.c}
    #include "ff_parser.h"
    #include "ff_epoch.h"

    uint8_t      buf[250];  // Some buffer
    PARSER_t     parser;    // Memory for the parser
    PARSER_MSG_t msg;       // Memory for one message
    EPOCH_t      coll;      // Memory for the epoch collector
    EPOCH_t      epoch;     // Memory for collected epochs

    parserInit(&parser);
    epochInit(&coll);

    while (true)
    {
        // Get more data (from a logfile, a serial port, ...)
        const int numRead = readMoreDataFromSomewherIntoBuffer(buf, sizeof(buf));
        if (numRead <= 0)
        {
            break; // or sleep and retry later
        }

        // Feed parser
        parserAdd(&parser, buf, numRead);

        // Process messages
        while (true)
        {
            // Do we have another message?
            const bool haveMessage = parserProcess(&parser, &msg, false);
            if (haveMessage)
            {
                // Feed it to the epoch collector
                const bool haveEpoch = epochCollect(&coll, &msg, &epoch);

                // Have a complete epoch?
                if (haveEpoch)
                {
                    printf("epoch: %s\n", epoch.str);
                }
                // else: we have no epoch yet
            }

            // No message (not enough data in parser)
            else
            {
                break;
            }
        }
    }
    \endcode

    \b Notes

    - The epochCollect() function returns true once it has been fed with enough messages to have complete epoch
    - The completeness can vary, depending on what messages are provided. For example, if no UBX-NAV-SAT is fed into
      the collector, no satellite information will be available in the resulting epoch.
    - A epoch is considered complete by observing the supplied messages (called "epoch detection" in the code). It works
      in one of these two ways:
      - The receiver ends a sequence of navigation data messages with a specific "end of epoch" message. For example,
        u-blox receivers have UBX-NAV-EOE for this.
      - A new sequence of navigation data messages is detected by a changed time (iTOW field in UBX-NAV messages, or
        the time field in NMEA messages)
    - The consequence of the two detection methods are:
      - If a end-of-epoch marker is available, epochDetect() returns true and provides the epoch data right away as
        soon as this marker message is received. There is no delay besides the time it takes the receiver to
        calculate and output the navigation solution.
      - If no end-of-epoch marker is available, the detection relies on observing changes in the time fields. As the
        change can only be observed in a subsequent navigation solution output, epochDetect() returns true only once the
        receiver starts to output a next navigation solution. That is, if the navigation output rate is 1Hz, the
        epochDetect() repots the epoch with 1s delay (or 0.5s at 2Hz, etc.)

    @{
*/

#ifndef __FF_EPOCH_H__
#define __FF_EPOCH_H__

#include <stdint.h>
#include <stdbool.h>

#include "ff_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */

//! Fix types
typedef enum EPOCH_FIX_e
{
    EPOCH_FIX_UNKNOWN = 0,    //!< Unknown or unspecified fix
    EPOCH_FIX_NOFIX,          //!< No fix (no navigation solution)
    EPOCH_FIX_DRONLY,         //!< Dead-reckoning only (usually extrapolated position from previous position and velocity)
    EPOCH_FIX_TIME,           //!< Time-only fix
    EPOCH_FIX_S2D,            //!< Single 2d GNSS fix
    EPOCH_FIX_S3D,            //!< Single 3d GNSS fix
    EPOCH_FIX_S3D_DR,         //!< Single 3d GNSS + dead-reckoning (sensor fusion) fix
    EPOCH_FIX_RTK_FLOAT,      //!< RTK float fix
    EPOCH_FIX_RTK_FIXED,      //!< RTK fixed fix
    EPOCH_FIX_RTK_FLOAT_DR,   //!< RTK float + dead-reckoning fix
    EPOCH_FIX_RTK_FIXED_DR,   //!< RTK fixed + dead-reckoning fix
} EPOCH_FIX_t;

//! GNSS identifier
typedef enum EPOCH_GNSS_e
{
    EPOCH_GNSS_UNKNOWN = 0,   //!< Unknown or unspecified GNSS
    EPOCH_GNSS_GPS,           //!< GPS
    EPOCH_GNSS_GLO,           //!< GLONASS
    EPOCH_GNSS_BDS,           //!< BeiDou
    EPOCH_GNSS_GAL,           //!< Galileo
    EPOCH_GNSS_SBAS,          //!< SNAS
    EPOCH_GNSS_QZSS,          //!< QZSS
    // Keep in sync with kEpochGnssStrs!
} EPOCH_GNSS_t;

//! Signal identifiers
typedef enum EPOCH_SIGNAL_e
{
    EPOCH_SIGNAL_UNKNOWN = 0,               //!< Unknown or unspecified signal
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

//! Signal use
typedef enum EPOCH_SIGUSE_e
{
    EPOCH_SIGUSE_UNKNOWN = 0,    //!< Unknown or unspecified use of the signal
    EPOCH_SIGUSE_NONE,           //!< Signal not used
    EPOCH_SIGUSE_SEARCH,         //!< Signal is being searched
    EPOCH_SIGUSE_ACQUIRED,       //!< Signal was acquired
    EPOCH_SIGUSE_UNUSABLE,       //!< Signal tracked but unused
    EPOCH_SIGUSE_CODELOCK,       //!< Signal tracked and code locked
    EPOCH_SIGUSE_CARRLOCK,       //!< Signal tracked and carrier locked
    // Keep in sync with kEpochSiqUseStrs!
} EPOCH_SIGUSE_t;

//! Signal correction data availability
typedef enum EPOCH_SIGCORR_e
{
    EPOCH_SIGCORR_UNKNOWN = 0,   //!< Unknown or unspecified corrections
    EPOCH_SIGCORR_NONE,          //!< No corrections available
    EPOCH_SIGCORR_SBAS,          //!< SBAS (DGNSS) corrections available
    EPOCH_SIGCORR_BDS,           //!< BeiDou (DGNSS) corrections available
    EPOCH_SIGCORR_RTCM2,         //!< RTCM v2 (DGNSS) corrections available
    EPOCH_SIGCORR_RTCM3_OSR,     //!< RTCM v3.x OSR type RTK corrections available
    EPOCH_SIGCORR_RTCM3_SSR,     //!< RTCM v3.x SSR type RTK corrections available
    EPOCH_SIGCORR_QZSS_SLAS,     //!< QZSS SLAS corrections available
    EPOCH_SIGCORR_SPARTN,        //!< SPARTN corrections available
    // Keep in sync with kEpochSigCorrStrs!
} EPOCH_SIGCORR_t;

//! Ionosphere corrections
typedef enum EPOCH_SIGIONO_e
{
    EPOCH_SIGIONO_UNKNOWN = 0,   //!< Unknown or unspecified corrections
    EPOCH_SIGIONO_NONE,          //!< No corrections
    EPOCH_SIGIONO_KLOB_GPS,      //!< GPS style Klobuchar corrections
    EPOCH_SIGIONO_KLOB_BDS,      //!< BeiDou style Klobuchar corrections
    EPOCH_SIGIONO_SBAS,          //!< SBAS corrections
    EPOCH_SIGIONO_DUAL_FREQ,     //!< Dual frequency iono-free combination
    // Keep in sync with kEpochSigIonoStrs!
} EPOCH_SIGIONO_t;

//! Signal health
typedef enum EPOCH_SIGHEALTH_e
{
    EPOCH_SIGHEALTH_UNKNOWN = 0,  //!< Unknown or unspecified health
    EPOCH_SIGHEALTH_HEALTHY,      //!< Signal is healthy
    EPOCH_SIGHEALTH_UNHEALTHY,    //!< Signal is unhealthy
    // Keep in sync with kEpochSigHealthStrs!
} EPOCH_SIGHEALTH_t;

//! Frequency band (for any GNSS, in "GPS speak")
typedef enum EPOCH_BAND_e
{
    EPOCH_BAND_UNKNOWN = 0,     //!< Unknown or unspecified frequency band
    EPOCH_BAND_L1,              //!< L1 band (~1.5GHz)
    EPOCH_BAND_L2,              //!< L2 band (~1.2GHz)
    EPOCH_BAND_L5,              //!< L5 band (~1.1GHz)
    // Keep in sync with kEpochBandStrs!
} EPOCH_BAND_t;

//! Signal information
typedef struct EPOCH_SIGINFO_s
{
    bool              valid;
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
    bool              anyUsed;
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
    int               satIx;

    // Private
    uint32_t          _order;
} EPOCH_SIGINFO_t;

//! Satellite orbit source
typedef enum EPOCH_SATORB_e
{
    EPOCH_SATORB_NONE = 0,  //!< No orbit available
    EPOCH_SATORB_EPH,       //!< Ephemeris
    EPOCH_SATORB_ALM,       //!< Almanac
    EPOCH_SATORB_PRED,      //!< Predicted orbit
    EPOCH_SATORB_OTHER,     //!< Other, unspecified orbit source
    // Keep in sync with kEpochOrbStrs!
} EPOCH_SATORB_t;

//! Satellite information
typedef struct EPOCH_SATINFO_s
{
    bool              valid;
    EPOCH_GNSS_t      gnss;
    uint8_t           sv;
    EPOCH_SATORB_t    orbUsed;
    int               orbAvail; //!< Bits of EPOCH_SATORB_e
    int8_t            elev;     //!< Elevation [deg] (-90..+90), only valid if orbUsed > EPOCH_SATORB_NONE
    int16_t           azim;     //!< Azimuth [deg] (0..359), only valid if orbUsed > EPOCH_SATORB_NONE
    const char       *gnssStr;
    const char       *svStr;
    const char       *orbUsedStr;

    // Private
    uint32_t          _order;
} EPOCH_SATINFO_t;


// CNo histogram
// ix    0    1     2      3      4      5      6      7      8      9     10      11
// cno  0-4  5-9  10-14  15-19  20-24  25-29  30-34  35-39  40-44  45-49  50-44  55-...
#define EPOCH_SIGCNOHIST_NUM 12
#define EPOCH_SIGCNOHIST_CNO2IX(cno)   ( (cno) > 55 ? (EPOCH_SIGCNOHIST_NUM - 1) : (cno > 0 ? ((cno) / 5)  : 0) );
#define EPOCH_SIGCNOHIST_IX2CNO_L(ix)  ((ix) * 5)
#define EPOCH_SIGCNOHIST_IX2CNO_H(ix)  (EPOCH_SIGCNOHIST_IX2CNO_L((ix) + 1) - 1)

//! Navigation epoch data
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

    bool                haveVel;
    double              velNed[3];
    double              vel2d;
    double              vel3d;
    double              velAcc;

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

    bool                haveLeapSeconds;
    int                 leapSeconds;

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

    bool                havePosixTime;
    double              posixTime;

    bool                haveClock;
    double              clockBias;
    double              clockDrift;

    bool                haveLatency;
    float               latency;

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

    bool                haveDiffAge;
    double              diffAge;

    bool                haveUptime;
    double              uptime;
    char                uptimeStr[20];

    // Private states for epoch detection and collection
    uint64_t            _detect[3];
    uint64_t            _collect[8];

} EPOCH_t;

#define EPOCH_NUM_GPS        32
#define EPOCH_NUM_SBAS       39
#define EPOCH_NUM_GAL        36
#define EPOCH_NUM_BDS        63
#define EPOCH_NUM_QZSS       10
#define EPOCH_NUM_GLO        32
#define EPOCH_FIRST_GPS       1
#define EPOCH_FIRST_SBAS    120
#define EPOCH_FIRST_GAL       1
#define EPOCH_FIRST_BDS       1
#define EPOCH_FIRST_QZSS      1
#define EPOCH_FIRST_GLO       1
#define EPOCH_NUM_SV (EPOCH_NUM_GPS + EPOCH_NUM_SBAS + EPOCH_NUM_GAL + EPOCH_NUM_BDS + EPOCH_NUM_QZSS + EPOCH_NUM_GLO)
#define EPOCH_NO_SV (EPOCH_NUM_SV + 1)

// ---------------------------------------------------------------------------------------------------------------------

//! Initialise epoch collector
/*!
    \param[out]  coll  collector structure
*/
void epochInit(EPOCH_t *coll);

//! Collect message, determine if a complete epoch is available
/*!
    \param[in,out]  coll   collector structure
    \param[in]      msg    a message
    \param[out]     epoch  output epoch data, can be NULL, data only valid if the functions returns true

    \returns true if an epoch was detected and \c epoch contains valid data, false otherwise

    \note While \c coll has the same type as \c epoch, it must not be used by the user. Only the data returned in
          \c epoch is valid, consistent and complete.
*/
bool epochCollect(EPOCH_t *coll, const PARSER_MSG_t *msg, EPOCH_t *epoch);

// ---------------------------------------------------------------------------------------------------------------------

//! Epoch stringification header
/*!
    \returns a header string that corresponds to the data in the string in EPOCH_t.str
*/
const char *epochStrHeader(void);

//! Stringify gnss identifier
/*!
    \param[in] gnss  GNSS identifier
    \returns a concise unique string for the identifier ("GPS", "GLO", "GAL", "BDS", etc.)
*/
const char *epochGnssStr(const EPOCH_GNSS_t gnss);

//! Stringify signal identifier
/*!
    \param[in]  signal  signal identifier
    \returns a concise (and, within one GNSS, unique) string for the identifier ("L1CA", "E5B", etc.)
*/
const char *epochSignalStr(const EPOCH_SIGNAL_t signal);

//! Get gnss identifier from signal identifier
/*!
    \param[in]  signal  signal identifier
    \returns the gnss identifier for the given signal identifier
*/
EPOCH_GNSS_t epochSignalGnss(const EPOCH_SIGNAL_t signal);


//! GNSS + SV to index
/*!
    \param[in]  gnss  GNSS identifier
    \param[in]  sv    SV number
    \returns a index into an array of size #EPOCH_NUM_SV, or EPOCH_NO_SV if invalid params given
*/
int epochSvToIx(const EPOCH_GNSS_t gnss, const int sv);

/* ****************************************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_EPOCH_H__
///@}
