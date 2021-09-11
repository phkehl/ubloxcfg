// flipflip's NMEA protocol stuff
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

#ifndef __FF_NMEA_H__
#define __FF_NMEA_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */

#define NMEA_PREAMBLE  '$'

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
} NMEA_GNSS_t;

typedef enum NMEA_SIGNAL_e
{
    NMEA_SIGNAL_UNKNOWN = 0,               //!< Unspecified signal
    NMEA_SIGNAL_GPS_L1CA,                  //!< GPS L1 C/A signal
    NMEA_SIGNAL_GPS_L2C,                   //!< GPS L2 C signal
    NMEA_SIGNAL_SBAS_L1CA,                 //!< SBAS L1 C/A signal
    NMEA_SIGNAL_GAL_E1,                    //!< Galileo E1 signal
    NMEA_SIGNAL_GAL_E5B,                   //!< Galileo E5b signal
    NMEA_SIGNAL_BDS_B1I,                   //!< BeiDou B1I signal
    NMEA_SIGNAL_BDS_B2I,                   //!< BeiDou B2I signal
    NMEA_SIGNAL_QZSS_L1CA,                 //!< QZSS L1 C/A signal
    NMEA_SIGNAL_QZSS_L1S,                  //!< QZSS L1 S signal
    NMEA_SIGNAL_QZSS_L2C,                  //!< QZSS L2 CM signal
    NMEA_SIGNAL_GLO_L1OF,                  //!< GLONASS L1 OF signal
    NMEA_SIGNAL_GLO_L2OF,                  //!< GLONASS L2 OF signal
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
    char formatter[8];   //!< "GGA", "RMC", but also "UBX_nn"
    char info[200];
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

/* ****************************************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_NMEA_H__
