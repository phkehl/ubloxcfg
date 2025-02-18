// flipflip's NMEA protocol stuff
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

#ifndef __FF_NMEA_H__
#define __FF_NMEA_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */

#define NMEA_FRAME_SIZE  6   // "$*cc\r\n"
#define NMEA_PREAMBLE    '$'

typedef struct NMEA_TIME_s
{
    bool    valid;
    int     hour;
    int     minute;
    double  second;
} NMEA_TIME_t;

typedef struct NMEA_DATE_s
{
    bool    valid;
    int     day;
    int     month;
    int     year;
} NMEA_DATE_t;

typedef enum NMEA_FIX_e
{
    NMEA_FIX_UNKNOWN = 0,
    NMEA_FIX_NOFIX,
    NMEA_FIX_DRONLY,
    NMEA_FIX_S2D,
    NMEA_FIX_S3D,
    NMEA_FIX_S3D_DR,
    NMEA_FIX_RTK_FLOAT,
    NMEA_FIX_RTK_FIXED,
    // keep in sync with kNmeaFixStrs
} NMEA_FIX_t;

typedef enum NMEA_GNSS_e
{
    NMEA_GNSS_UNKNOWN = 0,
    NMEA_GNSS_GPS,
    NMEA_GNSS_GLO,
    NMEA_GNSS_BDS,
    NMEA_GNSS_GAL,
    NMEA_GNSS_SBAS,
    NMEA_GNSS_QZSS,
    NMEA_GNSS_NAVIC,
} NMEA_GNSS_t;

typedef enum NMEA_SIGNAL_e
{
    NMEA_SIGNAL_UNKNOWN = 0,
    NMEA_SIGNAL_GPS_L1CA,
    NMEA_SIGNAL_GPS_L2CL,
    NMEA_SIGNAL_GPS_L2CM,
    NMEA_SIGNAL_GPS_L5I,
    NMEA_SIGNAL_GPS_L5Q,
    NMEA_SIGNAL_GLO_L1OF,
    NMEA_SIGNAL_GLO_L2OF,
    NMEA_SIGNAL_GAL_E1,
    NMEA_SIGNAL_GAL_E5A,
    NMEA_SIGNAL_GAL_E5B,
    NMEA_SIGNAL_BDS_B1ID,
    NMEA_SIGNAL_BDS_B2ID,
    NMEA_SIGNAL_BDS_B1C,
    NMEA_SIGNAL_BDS_B2A,
    NMEA_SIGNAL_QZSS_L1CA,
    NMEA_SIGNAL_QZSS_L1S,
    NMEA_SIGNAL_QZSS_L2CM,
    NMEA_SIGNAL_QZSS_L2CL,
    NMEA_SIGNAL_QZSS_L5I,
    NMEA_SIGNAL_QZSS_L5Q,
    NMEA_SIGNAL_NAVIC_L5A,
} NMEA_SIGNAL_t;

typedef struct NMEA_GGA_s
{
    NMEA_TIME_t time;
    double      lat;
    double      lon;
    NMEA_FIX_t  fix;
    int         numSv;
    double      hDOP;
    double      height;
    double      heightMsl;
    double      diffAge;     // < 0 = no DGPS
    int         diffStation; // < 0 = no DGPS
} NMEA_GGA_t;

typedef struct NMEA_RMC_s
{
    NMEA_DATE_t date;
    NMEA_TIME_t time;
    NMEA_FIX_t  fix;
    bool        valid;
    double      lat;
    double      lon;
    double      spd;
    double      cog;
    double      mv;
} NMEA_RMC_t;

typedef struct NMEA_GLL_s
{
    NMEA_TIME_t time;
    NMEA_FIX_t  fix;
    bool        valid;
    double      lat;
    double      lon;
} NMEA_GLL_t;

typedef struct NMEA_GSV_s
{
    int         numMsg;
    int         msgNum;
    int         numSat;
    struct
    {
        int     svId;
        int     elev;
        int     azim;
        int     cno;
        NMEA_SIGNAL_t sig;
        NMEA_GNSS_t   gnss;
    } svs[4];
    int nSvs;
} NMEA_GSV_t;

typedef struct NMEA_TXT_s
{
    int         numMsg;
    int         msgNum;
    int         msgType;
    char        text[200];
} NMEA_TXT_t;

typedef enum NMEA_TYPE_e
{
    NMEA_TYPE_NONE = 0,
    NMEA_TYPE_TXT,
    NMEA_TYPE_GGA,
    NMEA_TYPE_RMC,
    NMEA_TYPE_GLL,
    NMEA_TYPE_GSV,
} NMEA_TYPE_t;

typedef struct NMEA_MSG_s
{
    char talker[3];      //!< Talker ID ("GP", "GN", "P", ...)
    char formatter[8];   //!< Formatter ("GGA", "RMC", "UBX", ...)
    char info[200];
    int  payloadIx0;
    int  payloadIx1;
    NMEA_TYPE_t type;
    union
    {
        NMEA_TXT_t txt;
        NMEA_GGA_t gga;
        NMEA_RMC_t rmc;
        NMEA_GLL_t gll;
        NMEA_GSV_t gsv;
    };

} NMEA_MSG_t;

// ---------------------------------------------------------------------------------------------------------------------

bool nmeaMessageName(char *name, const int size, const uint8_t *msg, const int msgSize);

bool nmeaMessageInfo(char *info, const int size, const uint8_t *msg, const int msgSize);

bool nmeaDecode(NMEA_MSG_t *nmea, const uint8_t *msg, const int msgSize);

//! Get NMEA message IDs ("fake" UBX class and message IDs)
/*!
    \param[in]   name   Message name (e.g. "NMEA-STANDARD-GGA", "NMEA-PUBX-POSITION")
    \param[out]  clsId  Class ID, or NULL
    \param[out]  msgId  Message ID, or NULL

    \returns true if \c name was found and \c clsId and \c msgId are valid
*/
bool nmeaMessageClsId(const char *name, uint8_t *clsId, uint8_t *msgId);

//! Make a NMEA message (sentence)
/*!
    \param[in]   talker       Talker ID (can be "")
    \param[in]   formatter    Formatter (can be "")
    \param[in]   payload      The message payload (can be NULL)
    \param[out]  msg          Message buffer (must be at least \c strlen(talker) + \c strlen(formatter) +
                              \c strlen(payload) + #NMEA_FRAME_SIZE + 3)

    \note \c payload and \c msg may be the same buffer (or may overlap).

    \returns the message size
*/
int nmeaMakeMessage(const char *talker, const char *formatter, const char *payload, char *msg);

/* ****************************************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_NMEA_H__
