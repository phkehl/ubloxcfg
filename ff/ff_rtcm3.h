// flipflip's RTCM3 protocol stuff
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

#ifndef __FF_RTCM3_H__
#define __FF_RTCM3_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */

#define RTCM3_PREAMBLE 0xd3
#define RTCM3_HEAD_SIZE 3
#define RTCM3_FRAME_SIZE (RTCM3_HEAD_SIZE + 3)

//! Get message type from message
#define RTCM3_TYPE(msg) ( (((uint8_t *)(msg))[RTCM3_HEAD_SIZE + 0] << 4) | ((((uint8_t *)(msg))[RTCM3_HEAD_SIZE + 1] >> 4) & 0x0f) )

//! Get sub-type for RTCM-4072 message
#define RTCM3_4072_SUBTYPE(msg) ( (((uint8_t *)(msg))[RTCM3_HEAD_SIZE + 1] & 0x0f) | (((uint8_t *)(msg))[RTCM3_HEAD_SIZE + 2]) )

bool rtcm3MessageName(char *name, const int size, const uint8_t *msg, const int msgSize);

bool rtcm3MessageInfo(char *info, const int size, const uint8_t *msg, const int msgSize);

const char *rtcm3TypeDesc(const int msgType, const int subType);

/* ****************************************************************************************************************** */

typedef enum RTCM3_MSM_GNSS_e
{
    RTCM3_MSM_GNSS_GPS  =  70,
    RTCM3_MSM_GNSS_GLO  =  80,
    RTCM3_MSM_GNSS_GAL  =  90,
    RTCM3_MSM_GNSS_SBAS = 100,
    RTCM3_MSM_GNSS_QZSS = 110,
    RTCM3_MSM_GNSS_BDS  = 120,
} RTCM3_MSM_GNSS_t;

typedef enum RTCM3_MSM_TYPE_e
{
    RTCM3_MSM_TYPE_1 = 1,
    RTCM3_MSM_TYPE_2 = 2,
    RTCM3_MSM_TYPE_3 = 3,
    RTCM3_MSM_TYPE_4 = 4,
    RTCM3_MSM_TYPE_5 = 5,
    RTCM3_MSM_TYPE_6 = 6,
    RTCM3_MSM_TYPE_7 = 7,
} RTCM3_MSM_TYPE_t;

//! RTMC3 message type to MSM GNSS and type
/*!
    \param[in]   msgType  The RTMC3 message type
    \param[out]  gnss     The GNSS
    \param[out]  msm      The MSM type (number)
    \returns true if msgType was a valid and known RTCM3 MSM message
*/
bool rtcm3typeToMsm(int msgType, RTCM3_MSM_GNSS_t *gnss, RTCM3_MSM_TYPE_t *msm);

//! RTCM3 MSM messages common header
typedef struct RTCM3_MSM_HEADER_s
{
    RTCM3_MSM_GNSS_t gnss; //!< GNSS
    RTCM3_MSM_TYPE_t msm;  //!< MSM

    uint16_t msgType;      //!< Message number (DF002, uint12)
    uint16_t refStaId;     //!< Reference station ID (DF003, uint12)
    union
    {
        double anyTow;     //!< Any time of week [s]
        double gpsTow;     //!< GPS time of week [s] (DF004 uint30 [ms])
        double sbasTow;    //!< SBAS time of week [s] (DF004 uint30 [ms])
        double gloTow;     //!< GLONASS time of week [s] (DF416 uint3 [d], DF034 uint27 [ms])
        double galTow;     //!< Galileo time of week [s] (DF248 uint30 [ms])
        double qzssTow;    //!< QZSS time of week [s] (DF428 uint30 [ms])
        double bdsTow;     //!< BeiDou time of week [s] (DF427 uint30 [ms])
    };
    bool     multiMsgBit;  //!< Multiple message bit (DF393, bit(1)), 1 = more to follow, 0 = last message
    uint8_t  iods;         //!< IODS, issue of data station (DF409, uint3)
    uint8_t  clkSteering;  //!< Clock steering indicator (DF411, uint2)
    uint8_t  extClock;     //!< External clock indicator (DF412, uint2)
    bool     smooth;       //!< GNSS divergence-free smoothing indicator (DF417, bit(1))
    uint8_t  smoothInt;    //!< GNSS smoothing interval (DF418, bit(3))
    uint64_t satMask;      //!< GNSS satellite mask (DF394, bit(64))
    uint64_t sigMask;      //!< GNSS signal mask (DF395, bit(64))
    uint64_t cellMask;     //!< GNSS cell mask (DF396, bit(64))

    int      numSat;       //!< Number of satellites (in satMask)
    int      numSig;       //!< Number of signals (in sigMask)
    int      numCell;      //!< Number of cells (in cellMask)

} RTCM3_MSM_HEADER_t;

bool rtcm3GetMsmHeader(const uint8_t *msg, RTCM3_MSM_HEADER_t *header);

//! Antenna reference point
typedef struct RTCM3_ARP_s
{
    int    refStaId;
    double X;
    double Y;
    double Z;
} RTCM3_ARP_t;

//! Get ARP from message types 1005, 1006 or 1032
bool rtcm3GetArp(const uint8_t *msg, RTCM3_ARP_t *arp);

//! Antenna info
typedef struct RTCM3_ANT_s
{
    int     refStaId;
    char    antDesc[32];
    char    antSerial[32];
    uint8_t antSetupId;
    char    rxType[32];
    char    rxFw[32];
    char    rxSerial[32];
} RTCM3_ANT_t;

//! Get (some) antenna info from  message type 1007, 1008 or 1033
bool rtcm3GetAnt(const uint8_t *msg, RTCM3_ANT_t *ant);

//! Get RTCM3 message IDs ("fake" UBX class and message IDs)
/*!
    \param[in]   name   Message name (e.g. "NMEA-STANDARD-GGA", "NMEA-PUBX-POSITION")
    \param[out]  clsId  Class ID, or NULL
    \param[out]  msgId  Message ID, or NULL

    \returns true if \c name was found and \c clsId and \c msgId are valid
*/
bool rtcm3MessageClsId(const char *name, uint8_t *clsId, uint8_t *msgId);

/* ****************************************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_RTCM3_H__
