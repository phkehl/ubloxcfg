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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "ff_stuff.h"
#include "ff_ubx.h"
#include "ff_nmea.h"
#include "ff_trafo.h"
#include "ff_debug.h"
#include "ff_trafo.h"

#include "ff_epoch.h"

/* ****************************************************************************************************************** */

#define EPOCH_XTRA_DEBUG 0
#if EPOCH_XTRA_DEBUG
#  define EPOCH_DEBUG(fmt, args...) DEBUG("epoch: " fmt, ## args)
#else
#  define EPOCH_DEBUG(...) /* nothing */
#endif

void epochInit(EPOCH_t *coll)
{
    memset(coll, 0, sizeof(*coll));
}

static bool _detectUbx(EPOCH_t *coll, PARSER_MSG_t *msg);
static bool _detectNmea(EPOCH_t *coll, PARSER_MSG_t *msg);

static void _collectUbx(EPOCH_t *coll, PARSER_MSG_t *msg);
static void _collectNmea(EPOCH_t *coll, PARSER_MSG_t *msg);

static void _epochComplete(EPOCH_t *coll, EPOCH_t *epoch);

bool epochCollect(EPOCH_t *coll, PARSER_MSG_t *msg, EPOCH_t *epoch)
{
    if ( (coll == NULL) || (msg == NULL) )
    {
        return false;
    }

    // Detect end of epoch / start of next epoch
    bool detect = false;
    switch (msg->type)
    {
        case PARSER_MSGTYPE_UBX:
            detect = _detectUbx(coll, msg);
            break;
        case PARSER_MSGTYPE_NMEA:
            detect = _detectNmea(coll, msg);
            break;
        default:
            break;
    }

    // Output epoch
    if (detect)
    {
        coll->seq++;
        if (epoch != NULL)
        {
            _epochComplete(coll, epoch);
        }
        const uint32_t seq = coll->seq;
        const uint32_t tow = coll->_detectTow;
        const bool detectHaveTow = coll->_detectHaveTow;
        memset(coll, 0, sizeof(*coll));
        coll->seq = seq;
        coll->_detectTow = tow;
        coll->_detectHaveTow = detectHaveTow;
    }

    // Collect data
    switch (msg->type)
    {
        case PARSER_MSGTYPE_UBX:
            _collectUbx(coll, msg);
            break;
        case PARSER_MSGTYPE_NMEA:
            _collectNmea(coll, msg);
            break;
        default:
            break;
    }

    return detect;
}

static bool _detectUbx(EPOCH_t *coll, PARSER_MSG_t *msg)
{
    const uint8_t clsId = UBX_CLSID(msg->data);
    if (clsId != UBX_NAV_CLSID)
    {
        return false;
    }
    const uint8_t msgId = UBX_MSGID(msg->data);
    bool detect = false;
    switch (msgId)
    {
        case UBX_NAV_EOE_MSGID:
            coll->_detectHaveTow = false;
            detect = true;
            break;
        case UBX_NAV_PVT_MSGID:
        case UBX_NAV_SAT_MSGID:
        case UBX_NAV_ORB_MSGID:
        case UBX_NAV_STATUS_MSGID:
        case UBX_NAV_SIG_MSGID:
        case UBX_NAV_CLOCK_MSGID:
        case UBX_NAV_DOP_MSGID:
        case UBX_NAV_POSECEF_MSGID:
        case UBX_NAV_POSLLH_MSGID:
        case UBX_NAV_VELECEF_MSGID:
        case UBX_NAV_VELNED_MSGID:
        case UBX_NAV_GEOFENCE_MSGID:
        case UBX_NAV_TIMEUTC_MSGID:
        case UBX_NAV_TIMELS_MSGID:
        case UBX_NAV_TIMEGPS_MSGID:
        case UBX_NAV_TIMEGLO_MSGID:
        case UBX_NAV_TIMEBDS_MSGID:
        case UBX_NAV_TIMEGAL_MSGID:
            if (msg->size > (UBX_FRAME_SIZE + 4))
            {
                uint32_t iTow;
                memcpy(&iTow, &msg->data[UBX_HEAD_SIZE], sizeof(iTow));
                if (coll->_detectHaveTow && (coll->_detectTow != iTow))
                {
                    detect = true;
                }
                coll->_detectTow = iTow;
                coll->_detectHaveTow = true;
            }
            break;
        case UBX_NAV_SVIN_MSGID:
        case UBX_NAV_ODO_MSGID:
        case UBX_NAV_HPPOSLLH_MSGID:
        case UBX_NAV_HPPOSECEF_MSGID:
        case UBX_NAV_RELPOSNED_MSGID:
            if (msg->size > (UBX_FRAME_SIZE + 4 + 4))
            {
                uint32_t iTow;
                memcpy(&iTow, &msg->data[UBX_HEAD_SIZE + 4], sizeof(iTow));
                if (coll->_detectHaveTow && (coll->_detectTow != iTow))
                {
                    detect = true;
                }
                coll->_detectTow = iTow;
                coll->_detectHaveTow = true;
            }
            break;
    }
    return detect;
}

static bool _detectNmea(EPOCH_t *coll, PARSER_MSG_t *msg)
{
    (void)coll;
    (void)msg;
    return false;
}

#define FLAG(field, flag) ( ((field) & (flag)) == (flag) )

static EPOCH_GNSS_t _ubxGnssIdToGnss(const uint8_t gnssId)
{
    switch (gnssId)
    {
        case UBX_GNSSID_GPS:   return EPOCH_GNSS_GPS;
        case UBX_GNSSID_SBAS:  return EPOCH_GNSS_SBAS;
        case UBX_GNSSID_GAL:   return EPOCH_GNSS_GAL;
        case UBX_GNSSID_BDS:   return EPOCH_GNSS_BDS;
        case UBX_GNSSID_QZSS:  return EPOCH_GNSS_QZSS;
        case UBX_GNSSID_GLO:   return EPOCH_GNSS_GLO;
        default:               return EPOCH_GNSS_UNKNOWN;
    }
}

static const char * const kEpochGnssStrs[] =
{
    [EPOCH_GNSS_UNKNOWN] = "?",
    [EPOCH_GNSS_GPS]  = "GPS",
    [EPOCH_GNSS_GLO]  = "GLO",
    [EPOCH_GNSS_GAL]  = "GAL",
    [EPOCH_GNSS_BDS]  = "BDS",
    [EPOCH_GNSS_SBAS] = "SBAS",
    [EPOCH_GNSS_QZSS] = "QZSS",
};

static EPOCH_SIGNAL_t _ubxSigIdToSignal(const uint8_t gnssId, const uint8_t sigId)
{
    switch (gnssId)
    {
        case UBX_GNSSID_GPS:
            switch (sigId)
            {
                case UBX_SIGID_GPS_L1CA:  return EPOCH_SIGNAL_GPS_L1CA;
                case UBX_SIGID_GPS_L2CL:  return EPOCH_SIGNAL_GPS_L2CL;
                case UBX_SIGID_GPS_L2CM:  return EPOCH_SIGNAL_GPS_L2CM;
            }
            break;
        case UBX_GNSSID_SBAS:
            switch (sigId)
            {
                case UBX_SIGID_SBAS_L1CA: return EPOCH_SIGNAL_SBAS_L1CA;
            }
            break;
        case UBX_GNSSID_GAL:
            switch (sigId)
            {
                case UBX_SIGID_GAL_E1C:   return EPOCH_SIGNAL_GAL_E1C;
                case UBX_SIGID_GAL_E1B:   return EPOCH_SIGNAL_GAL_E1B;
                case UBX_SIGID_GAL_E5BI:  return EPOCH_SIGNAL_GAL_E5BI;
                case UBX_SIGID_GAL_E5BQ:  return EPOCH_SIGNAL_GAL_E5BQ;
            }
            break;
        case UBX_GNSSID_BDS:
            switch (sigId)
            {
                case UBX_SIGID_BDS_B1ID1: return EPOCH_SIGNAL_BDS_B1ID1;
                case UBX_SIGID_BDS_B1ID2: return EPOCH_SIGNAL_BDS_B1ID2;
                case UBX_SIGID_BDS_B2ID1: return EPOCH_SIGNAL_BDS_B2ID1;
                case UBX_SIGID_BDS_B2ID2: return EPOCH_SIGNAL_BDS_B2ID2;
            }
            break;
        case UBX_GNSSID_QZSS:
            switch (sigId)
            {
                case UBX_SIGID_QZSS_L1CA: return EPOCH_SIGNAL_QZSS_L1CA;
                case UBX_SIGID_QZSS_L1S:  return EPOCH_SIGNAL_QZSS_L1S;
                case UBX_SIGID_QZSS_L2CM: return EPOCH_SIGNAL_QZSS_L2CM;
                case UBX_SIGID_QZSS_L2CL: return EPOCH_SIGNAL_QZSS_L2CL;
            }
            break;
        case UBX_GNSSID_GLO:
            switch (sigId)
            {
                case UBX_SIGID_GLO_L1OF:  return EPOCH_SIGNAL_GLO_L1OF;
                case UBX_SIGID_GLO_L2OF:  return EPOCH_SIGNAL_GLO_L2OF;
            }
            break;
    }
    return EPOCH_SIGNAL_UNKNOWN;
}

const char * const kEpochSignalStrs[] =
{
    [EPOCH_SIGNAL_UNKNOWN]   = "?",
    [EPOCH_SIGNAL_GPS_L1CA]  = "L1CA",
    [EPOCH_SIGNAL_GPS_L2CL]  = "L2CL",
    [EPOCH_SIGNAL_GPS_L2CM]  = "L2CL",
    [EPOCH_SIGNAL_GLO_L1OF]  = "L1OF",
    [EPOCH_SIGNAL_GLO_L2OF]  = "L2OF",
    [EPOCH_SIGNAL_GAL_E1C]   = "E1C",
    [EPOCH_SIGNAL_GAL_E1B]   = "E1B",
    [EPOCH_SIGNAL_GAL_E5BI]  = "E5BI",
    [EPOCH_SIGNAL_GAL_E5BQ]  = "E5BQ",
    [EPOCH_SIGNAL_BDS_B1ID1] = "B1ID1",
    [EPOCH_SIGNAL_BDS_B1ID2] = "B1ID2",
    [EPOCH_SIGNAL_BDS_B2ID1] = "B2ID1",
    [EPOCH_SIGNAL_BDS_B2ID2] = "B2ID2",
    [EPOCH_SIGNAL_SBAS_L1CA] = "L1CA",
    [EPOCH_SIGNAL_QZSS_L1CA] = "L1CA",
    [EPOCH_SIGNAL_QZSS_L1S]  = "L1S",
    [EPOCH_SIGNAL_QZSS_L2CM] = "L2CM",
    [EPOCH_SIGNAL_QZSS_L2CL] = "L2CL",
};

static const char *_epochSvStr(const EPOCH_GNSS_t gnss, const uint8_t sv)
{
    switch (gnss)
    {
        case EPOCH_GNSS_GPS:
        {
            const char * const strs[] =
            {
                "G01", "G02", "G03", "G04", "G05", "G06", "G07", "G08", "G09", "G10",
                "G11", "G12", "G13", "G14", "G15", "G16", "G17", "G18", "G19", "G20",
                "G21", "G22", "G23", "G24", "G25", "G26", "G27", "G28", "G29", "G30",
                "G31", "G32"
            };
            const int ix = sv - 1;
            if ( (ix >= 0) && (ix < NUMOF(strs)) )
            {
                return strs[ix];
            }
            else
            {
                return "G?";
            }
        }
        case EPOCH_GNSS_GLO:
        {
            const char * const strs[] =
            {
                "R01", "R02", "R03", "R04", "R05", "R06", "R07", "R08", "R09", "R10",
                "R11", "R12", "R13", "R14", "R15", "R16", "R17", "R18", "R19", "R20",
                "R21", "R22", "R23", "R24", "R25", "R26", "R27", "R28", "R29", "R30",
                "R31", "R32"
            };
            const int ix = sv - 1;
            if ( (ix >= 0) && (ix < NUMOF(strs)) )
            {
                return strs[ix];
            }
            else
            {
                return "R?";
            }
        }
        case EPOCH_GNSS_GAL:
        {
            const char * const strs[] =
            {
                "E01", "E02", "E03", "E04", "E05", "E06", "E07", "E08", "E09", "E10",
                "E11", "E12", "E13", "E14", "E15", "E16", "E17", "E18", "E19", "E20",
                "E21", "E22", "E23", "E24", "E25", "E26", "E27", "E28", "E29", "E30",
                "E31", "E32", "E33", "E34", "E35", "E36"
            };
            const int ix = sv - 1;
            if ( (ix >= 0) && (ix < NUMOF(strs)) )
            {
                return strs[ix];
            }
            else
            {
                return "E?";
            }
        }
        case EPOCH_GNSS_BDS:
        {
            const char * const strs[] =
            {
                "B01", "B02", "B03", "B04", "B05", "B06", "B07", "B08", "B09", "B10",
                "B11", "B12", "B13", "B14", "B15", "B16", "B17", "B18", "B19", "B20",
                "B21", "B22", "B23", "B24", "B25", "B26", "B27", "B28", "B29", "B30",
                "B31", "B32"
            };
            const int ix = sv - 1;
            if ( (ix >= 0) && (ix < NUMOF(strs)) )
            {
                return strs[ix];
            }
            else
            {
                return "B?";
            }
        }
        case EPOCH_GNSS_SBAS:
        {
            const char * const strs[] =
            {
                "S120", "S121", "S122", "S123", "S123", "S124", "S126", "S127", "S128", "S129"
                "S130", "S131", "S132", "S133", "S133", "S134", "S136", "S137", "S138", "S139"
                "S140", "S141", "S142", "S143", "S143", "S144", "S146", "S147", "S148", "S149"
                "S150", "S151", "S152", "S153", "S153", "S154", "S156", "S157", "S158"
            };
            const int ix = sv - 120;
            if ( (ix >= 0) && (ix < NUMOF(strs)) )
            {
                return strs[ix];
            }
            else
            {
                return "G?";
            }
        }
        case EPOCH_GNSS_QZSS:
        {
            const char * const strs[] =
            {
                "Q01", "Q02", "Q03", "Q04", "Q05", "Q06", "Q07", "Q08", "Q09", "Q10",
                "Q11", "Q12", "Q13", "Q14", "Q15", "Q16", "Q17", "Q18", "Q19", "Q20",
                "Q21", "Q22", "Q23", "Q24", "Q25", "Q26", "Q27", "Q28", "Q29", "Q30",
                "Q31", "Q32"
            };
            const int ix = sv - 1;
            if ( (ix >= 0) && (ix < NUMOF(strs)) )
            {
                return strs[ix];
            }
            else
            {
                return "Q?";
            }
        }
        default:
            break;
    }
    return "?";
}

static EPOCH_SIGQUAL_t _ubxSigQual(const uint8_t qualityInd)
{
    switch (qualityInd)
    {
        case UBX_NAV_SIG_V0_QUALITYIND_SEARCH:    return EPOCH_SIGQUAL_SEARCH;
        case UBX_NAV_SIG_V0_QUALITYIND_ACQUIRED:  return EPOCH_SIGQUAL_ACQUIRED;
        case UBX_NAV_SIG_V0_QUALITYIND_UNUSED:    return EPOCH_SIGQUAL_UNUSED;
        case UBX_NAV_SIG_V0_QUALITYIND_CODELOCK:  return EPOCH_SIGQUAL_CODELOCK;
        case UBX_NAV_SIG_V0_QUALITYIND_CARRLOCK1:
        case UBX_NAV_SIG_V0_QUALITYIND_CARRLOCK2:
        case UBX_NAV_SIG_V0_QUALITYIND_CARRLOCK3: return EPOCH_SIGQUAL_CARRLOCK;
        case UBX_NAV_SIG_V0_QUALITYIND_NOSIG:     return EPOCH_SIGQUAL_NONE;
        default:                                  return EPOCH_SIGQUAL_UNKNOWN;
    }
}

static const char * const kEpochSiqQualStrs[] =
{
    [EPOCH_SIGQUAL_UNKNOWN]  = "UNKNOWN",
    [EPOCH_SIGQUAL_NONE]     = "NONE",
    [EPOCH_SIGQUAL_SEARCH]   = "SEARCH",
    [EPOCH_SIGQUAL_ACQUIRED] = "ACQUIRED",
    [EPOCH_SIGQUAL_UNUSED]   = "UNUSED",
    [EPOCH_SIGQUAL_CODELOCK] = "CODELOCK",
    [EPOCH_SIGQUAL_CARRLOCK] = "CARRLOCK",
};

static EPOCH_SIGCORR_t _ubxSigCorrSource(const uint8_t corrSource)
{
    switch (corrSource)
    {
        case UBX_NAV_SIG_V0_CORRUSED_NONE:      return EPOCH_SIGCORR_NONE;
        case UBX_NAV_SIG_V0_CORRUSED_SBAS:      return EPOCH_SIGCORR_SBAS;
        case UBX_NAV_SIG_V0_CORRUSED_BDS:       return EPOCH_SIGCORR_BDS;
        case UBX_NAV_SIG_V0_CORRUSED_RTCM2:     return EPOCH_SIGCORR_RTCM2;
        case UBX_NAV_SIG_V0_CORRUSED_RTCM3_OSR: return EPOCH_SIGCORR_RTCM3_OSR;
        case UBX_NAV_SIG_V0_CORRUSED_RTCM3_SSR: return EPOCH_SIGCORR_RTCM3_SSR;
        case UBX_NAV_SIG_V0_CORRUSED_QZSS_SLAS: return EPOCH_SIGCORR_QZSS_SLAS;
        default:                                return EPOCH_SIGCORR_UNKNOWN;
    }
}

static const char * const kEpochSigCorrStrs[] =
{
    [EPOCH_SIGCORR_UNKNOWN]   = "UNKNOWN",
    [EPOCH_SIGCORR_NONE]      = "NONE",
    [EPOCH_SIGCORR_SBAS]      = "SBAS",
    [EPOCH_SIGCORR_BDS]       = "BDS",
    [EPOCH_SIGCORR_RTCM2]     = "RTCM2",
    [EPOCH_SIGCORR_RTCM3_OSR] = "RTCM3-OSR",
    [EPOCH_SIGCORR_RTCM3_SSR] = "RTCM3-SSR",
    [EPOCH_SIGCORR_QZSS_SLAS] = "QZSS-SLAS",
};

static EPOCH_SIGIONO_t _ubxIonoModel(const uint8_t ionoModel)
{
    switch (ionoModel)
    {
        case UBX_NAV_SIG_V0_IONOMODEL_NONE:     return EPOCH_SIGIONO_NONE;
        case UBX_NAV_SIG_V0_IONOMODEL_KLOB_GPS: return EPOCH_SIGIONO_KLOB_GPS;
        case UBX_NAV_SIG_V0_IONOMODEL_KLOB_BDS: return EPOCH_SIGIONO_KLOB_BDS; 
        case UBX_NAV_SIG_V0_IONOMODEL_SBAS:     return EPOCH_SIGIONO_SBAS; 
        case UBX_NAV_SIG_V0_IONOMODEL_DUALFREQ: return EPOCH_SIGIONO_DUAL_FREQ;
        default:                                return EPOCH_SIGIONO_UNKNOWN;
    }
}

static const char * const kEpochSigIonoStrs[] =
{
    [EPOCH_SIGIONO_UNKNOWN]   = "UNKNOWN",
    [EPOCH_SIGIONO_NONE]      = "NONE",
    [EPOCH_SIGIONO_KLOB_GPS]  = "KLOB-GPS",
    [EPOCH_SIGIONO_KLOB_BDS]  = "KLOB-BDS",
    [EPOCH_SIGIONO_SBAS]      = "SBAS",
    [EPOCH_SIGIONO_DUAL_FREQ] = "DUAL-FREQ",
};

static EPOCH_SIGHEALTH_t _ubxSigHealth(const uint8_t health)
{
    switch (health)
    {
        case UBX_NAV_SIG_V0_SIGFLAGS_HEALTH_HEALTHY:   return EPOCH_SIGHEALTH_HEALTHY;
        case UBX_NAV_SIG_V0_SIGFLAGS_HEALTH_UNHEALTHY: return EPOCH_SIGHEALTH_UNHEALTHY;
        case UBX_NAV_SIG_V0_SIGFLAGS_HEALTH_UNKNO:
        default:                                       return EPOCH_SIGHEALTH_UNKNOWN;
    }
}

static const char * const kEpochSigHealthStrs[] =
{
    [EPOCH_SIGHEALTH_UNKNOWN]   = "UNKNOWN",
    [EPOCH_SIGHEALTH_HEALTHY]   = "HEALTHY",
    [EPOCH_SIGHEALTH_UNHEALTHY] = "UNHEALTHY",
};

static int _epochSigInfoSort(const void *a, const void *b)
{
    const EPOCH_SIGINFO_t *sA = (const EPOCH_SIGINFO_t *)a;
    const EPOCH_SIGINFO_t *sB = (const EPOCH_SIGINFO_t *)b;
    return (int32_t)(sA->_order - sB->_order);
}

// "Quality" (precision) if information: UBX better than NMEA, UBX high-precision messages better than normal UBX
enum { HAVE_NOTHING = 0, HAVE_NMEA = 1, HAVE_UBX = 2, HAVE_UBX_HP = 3 };

static void _collectUbx(EPOCH_t *coll, PARSER_MSG_t *msg)
{
    const uint8_t clsId = UBX_CLSID(msg->data);
    if (clsId != UBX_NAV_CLSID)
    {
        return;
    }
    const uint8_t msgId = UBX_MSGID(msg->data);
    switch (msgId)
    {
        case UBX_NAV_PVT_MSGID:
            if (msg->size == UBX_NAV_PVT_V1_SIZE)
            {
                EPOCH_DEBUG("collect %s", msg->name);
                UBX_NAV_PVT_V1_GROUP0_t pvt;
                memcpy(&pvt, &msg->data[UBX_HEAD_SIZE], sizeof(pvt));

                // Fix info
                if (coll->_haveFix < HAVE_UBX)
                {
                    coll->_haveFix = HAVE_UBX;
                    switch (pvt.fixType)
                    {
                        case UBX_NAV_PVT_V1_FIXTYPE_NOFIX:  coll->fix = EPOCH_FIX_NOFIX;  break;
                        case UBX_NAV_PVT_V1_FIXTYPE_DRONLY: coll->fix = EPOCH_FIX_DRONLY; break;
                        case UBX_NAV_PVT_V1_FIXTYPE_2D:     coll->fix = EPOCH_FIX_2D;     break;
                        case UBX_NAV_PVT_V1_FIXTYPE_3D:     coll->fix = EPOCH_FIX_3D;     break;
                        case UBX_NAV_PVT_V1_FIXTYPE_3D_DR:  coll->fix = EPOCH_FIX_3D_DR;  break;
                        case UBX_NAV_PVT_V1_FIXTYPE_TIME:   coll->fix = EPOCH_FIX_TIME;   break;
                    }
                    if (coll->fix > EPOCH_FIX_NOFIX)
                    {
                        coll->fixOk = FLAG(pvt.flags, UBX_NAV_PVT_V1_FLAGS_GNSSFIXOK);
                    }
                    switch (UBX_NAV_PVT_V1_FLAGS_CARRSOLN_GET(pvt.flags))
                    {
                        case UBX_NAV_PVT_V1_FLAGS_CARRSOLN_NO:    coll->rtk = EPOCH_RTK_NONE;  break;
                        case UBX_NAV_PVT_V1_FLAGS_CARRSOLN_FLOAT: coll->rtk = EPOCH_RTK_FLOAT; break;
                        case UBX_NAV_PVT_V1_FLAGS_CARRSOLN_FIXED: coll->rtk = EPOCH_RTK_FIXED; break;
                    }
                    coll->haveFix = true;
                }

                // Time
                if (coll->_haveTime < HAVE_UBX)
                {
                    coll->_haveTime = HAVE_UBX;
                    coll->hour        = pvt.hour;
                    coll->minute      = pvt.min;
                    coll->second      = (double)pvt.sec + ((double)pvt.nano * 1e-9);
                    if (coll->second < 0.0)
                    {
                        coll->second += 60.0;
                    }
                    coll->haveTime    = FLAG(pvt.valid, UBX_NAV_PVT_V1_VALID_VALIDTIME);
                    coll->confTime    = FLAG(pvt.flags2, UBX_NAV_PVT_V1_FLAGS2_CONFTIME);
                    coll->timeAcc     = (double)pvt.tAcc * UBX_NAV_PVT_V1_TACC_SCALE;
                }

                // Date
                if (coll->_haveDate < HAVE_UBX)
                {
                    coll->_haveDate = HAVE_UBX;
                    coll->year        = pvt.year;
                    coll->month       = pvt.month;
                    coll->day         = pvt.day;
                    coll->haveDate    = FLAG(pvt.valid, UBX_NAV_PVT_V1_VALID_VALIDDATE);
                    coll->confDate    = FLAG(pvt.flags2, UBX_NAV_PVT_V1_FLAGS2_CONFDATE);
                }

                // Geodetic coordinates
                if (coll->_haveLlh < HAVE_UBX)
                {
                    coll->_haveLlh = HAVE_UBX;
                    coll->llh[0]      = deg2rad((double)pvt.lat * UBX_NAV_PVT_V1_LAT_SCALE);
                    coll->llh[1]      = deg2rad((double)pvt.lon * UBX_NAV_PVT_V1_LON_SCALE);
                    coll->llh[2]      = (double)pvt.height * UBX_NAV_PVT_V1_HEIGHT_SCALE;
                    coll->heightMsl   = (double)pvt.hMSL * UBX_NAV_PVT_V1_HEIGHT_SCALE;
                    coll->haveMsl     = FLAG(pvt.flags3, UBX_NAV_PVT_V1_FLAGS3_INVALIDLLH);
                }

                // Position accuracy estimate
                if (pvt.fixType > UBX_NAV_PVT_V1_FIXTYPE_NOFIX)
                {
                    if (coll->_haveHacc < HAVE_UBX)
                    {
                        coll->_haveHacc = HAVE_UBX;
                        coll->horizAcc = (double)pvt.hAcc * UBX_NAV_PVT_V1_HACC_SCALE;
                    }
                    if (coll->_haveVacc < HAVE_UBX)
                    {
                        coll->_haveVacc = HAVE_UBX;
                        coll->vertAcc     = (double)pvt.vAcc * UBX_NAV_PVT_V1_VACC_SCALE;
                    }
                }

                coll->pDOP        = (float)pvt.pDOP * UBX_NAV_PVT_V1_PDOP_SCALE;
                coll->havePdop    = true;

                coll->numSv       = pvt.numSV;
                coll->haveNumSv   = true;

                coll->gpsTow      = pvt.iTOW;
                coll->gpsTowAcc   = coll->timeAcc > 1e-3 ? coll->timeAcc : 1e-3;
                coll->haveGpsTow  = true;
            }
            break;
        case UBX_NAV_POSECEF_MSGID:
            if (msg->size == UBX_NAV_POSECEF_V0_SIZE)
            {
                EPOCH_DEBUG("collect %s", msg->name);
                UBX_NAV_POSECEF_V0_GROUP0_t pos;
                memcpy(&pos, &msg->data[UBX_HEAD_SIZE], sizeof(pos));
                if (coll->_haveXyz < HAVE_UBX)
                {
                    coll->_haveXyz = HAVE_UBX;
                    coll->xyz[0] = (double)pos.ecefX * UBX_NAV_POSECEF_V0_ECEF_XYZ_SCALE;
                    coll->xyz[1] = (double)pos.ecefY * UBX_NAV_POSECEF_V0_ECEF_XYZ_SCALE;
                    coll->xyz[2] = (double)pos.ecefZ * UBX_NAV_POSECEF_V0_ECEF_XYZ_SCALE;
                }
                if (coll->_havePacc < HAVE_UBX)
                {
                    coll->_havePacc = HAVE_UBX;
                    coll->posAcc = (double)pos.pAcc  * UBX_NAV_POSECEF_V0_PACC_SCALE;
                }
            }
            break;
        case UBX_NAV_HPPOSECEF_MSGID:
            if ( (msg->size == UBX_NAV_HPPOSECEF_V0_SIZE) && (UBX_NAV_HPPOSECEF_V0_VERSION_GET(msg->data) == UBX_NAV_HPPOSECEF_V0_VERSION) )
            {
                EPOCH_DEBUG("collect %s", msg->name);
                UBX_NAV_HPPOSECEF_V0_GROUP0_t pos;
                memcpy(&pos, &msg->data[UBX_HEAD_SIZE], sizeof(pos));
                if (!FLAG(pos.flags, UBX_NAV_HPPOSECEF_V0_FLAGS_INVALIDECEF))
                {
                    if (coll->_haveXyz < HAVE_UBX_HP)
                    {
                        coll->_haveXyz = HAVE_UBX_HP;
                        coll->xyz[0] = ((double)pos.ecefX * UBX_NAV_HPPOSECEF_V0_ECEF_XYZ_SCALE) + ((double)pos.ecefXHp * UBX_NAV_HPPOSECEF_V0_ECEF_XYZ_HP_SCALE);
                        coll->xyz[1] = ((double)pos.ecefY * UBX_NAV_HPPOSECEF_V0_ECEF_XYZ_SCALE) + ((double)pos.ecefYHp * UBX_NAV_HPPOSECEF_V0_ECEF_XYZ_HP_SCALE);
                        coll->xyz[2] = ((double)pos.ecefZ * UBX_NAV_HPPOSECEF_V0_ECEF_XYZ_SCALE) + ((double)pos.ecefZHp * UBX_NAV_HPPOSECEF_V0_ECEF_XYZ_HP_SCALE);
                    }
                    if (coll->_havePacc < HAVE_UBX_HP)
                    {
                        coll->_havePacc = HAVE_UBX_HP;
                        coll->posAcc = (double)pos.pAcc * UBX_NAV_HPPOSECEF_V0_PACC_SCALE;
                    }
                }
            }
            break;
        case UBX_NAV_SIG_MSGID:
            if ( (msg->size >= (int)UBX_NAV_SIG_V0_MIN_SIZE) && (UBX_NAV_SIG_VERSION_GET(msg->data) == UBX_NAV_SIG_V0_VERSION) )
            {
                EPOCH_DEBUG("collect %s", msg->name);
                if (coll->_haveSig < HAVE_UBX)
                {
                    coll->_haveSig = HAVE_UBX;
                    UBX_NAV_SIG_V0_GROUP0_t head;
                    memcpy(&head, &msg->data[UBX_HEAD_SIZE], sizeof(head));
                    int ix;
                    for (coll->numSignals = 0, ix = 0; (coll->numSignals < (int)head.numSigs) && (coll->numSignals < NUMOF(coll->signals)); coll->numSignals++, ix++)
                    {
                        UBX_NAV_SIG_V0_GROUP1_t uInfo;
                        memcpy(&uInfo, &msg->data[UBX_HEAD_SIZE + sizeof(UBX_NAV_SIG_V0_GROUP0_t) + (ix * sizeof(uInfo))], sizeof(uInfo));
                        EPOCH_SIGINFO_t *eInfo = &coll->signals[ix];
                        eInfo->gnss        = _ubxGnssIdToGnss(uInfo.gnssId);
                        eInfo->sv          = uInfo.svId;
                        eInfo->signal      = _ubxSigIdToSignal(uInfo.gnssId, uInfo.sigId);
                        eInfo->gloFcn      = (int)uInfo.freqId - 7;
                        eInfo->prRes       = (float)uInfo.prRes * (float)UBX_NAV_SIG_V0_PRRES_SCALE;
                        eInfo->cno         = uInfo.cno;
                        eInfo->prUsed      = FLAG(uInfo.sigFlags, UBX_NAV_SIG_V0_SIGFLAGS_PR_USED);
                        eInfo->crUsed      = FLAG(uInfo.sigFlags, UBX_NAV_SIG_V0_SIGFLAGS_CR_USED);
                        eInfo->doUsed      = FLAG(uInfo.sigFlags, UBX_NAV_SIG_V0_SIGFLAGS_DO_USED);
                        eInfo->prCorrUsed  = FLAG(uInfo.sigFlags, UBX_NAV_SIG_V0_SIGFLAGS_PR_CORR_USED);
                        eInfo->crCorrUsed  = FLAG(uInfo.sigFlags, UBX_NAV_SIG_V0_SIGFLAGS_CR_CORR_USED);
                        eInfo->doCorrUsed  = FLAG(uInfo.sigFlags, UBX_NAV_SIG_V0_SIGFLAGS_DO_CORR_USED);
                        eInfo->qual        = _ubxSigQual(uInfo.qualityInd);
                        eInfo->corr        = _ubxSigCorrSource(uInfo.corrSource);
                        eInfo->iono        = _ubxIonoModel(uInfo.ionoModel);
                        eInfo->health      = _ubxSigHealth(UBX_NAV_SIG_V0_SIGFLAGS_HEALTH_GET(uInfo.sigFlags));
                        eInfo->gnssStr     = eInfo->gnss   < NUMOF(kEpochGnssStrs)      ? kEpochGnssStrs[eInfo->gnss]        : kEpochGnssStrs[EPOCH_GNSS_UNKNOWN];
                        eInfo->svStr       = _epochSvStr(eInfo->gnss, eInfo->sv);
                        eInfo->signalStr   = eInfo->signal < NUMOF(kEpochSignalStrs)    ? kEpochSignalStrs[eInfo->signal]    : kEpochSignalStrs[EPOCH_SIGNAL_UNKNOWN];
                        eInfo->qualStr     = eInfo->qual   < NUMOF(kEpochSiqQualStrs)   ? kEpochSiqQualStrs[eInfo->qual]     : kEpochSiqQualStrs[EPOCH_SIGQUAL_UNKNOWN];
                        eInfo->corrStr     = eInfo->corr   < NUMOF(kEpochSigCorrStrs)   ? kEpochSigCorrStrs[eInfo->corr]     : kEpochSigCorrStrs[EPOCH_SIGCORR_UNKNOWN];
                        eInfo->ionoStr     = eInfo->iono   < NUMOF(kEpochSigIonoStrs)   ? kEpochSigIonoStrs[eInfo->iono]     : kEpochSigIonoStrs[EPOCH_SIGIONO_UNKNOWN];
                        eInfo->healthStr   = eInfo->health < NUMOF(kEpochSigHealthStrs) ? kEpochSigHealthStrs[eInfo->health] : kEpochSigHealthStrs[EPOCH_SIGHEALTH_UNKNOWN];
                        eInfo->_order = ((eInfo->gnss & 0xff) << 24) | ((eInfo->sv & 0xff) << 16) | ((eInfo->signal & 0xff) << 8);
                    }
                    qsort(coll->signals, coll->numSignals, sizeof(*coll->signals), _epochSigInfoSort);
                }
            }
            break;
    }
}

static void _collectNmea(EPOCH_t *coll, PARSER_MSG_t *msg)
{
    // TODO...
    (void)coll;
    (void)msg;
}

static void _epochComplete(EPOCH_t *coll, EPOCH_t *epoch)
{
    memcpy(epoch, coll, sizeof(*epoch));
    epoch->valid = true;
    epoch->ts = TIME();

    // Convert stuff, prefer better quality

    if (epoch->_haveLlh > epoch->_haveXyz)
    {
        EPOCH_DEBUG("complete: llh (%d) -> xyz (%d)", epoch->_haveLlh, epoch->_haveXyz);
        llh2xyz_vec(epoch->llh, epoch->xyz);
        epoch->havePos = true;
    }
    else if (epoch->_haveXyz > epoch->_haveLlh)
    {
        EPOCH_DEBUG("complete: xyz (%d) -> llh (%d)", epoch->_haveXyz, epoch->_haveLlh);
        xyz2llh_vec(epoch->xyz, epoch->llh);
        epoch->havePos = true;
    }
    else
    {
        EPOCH_DEBUG("complete: llh (%d) = xyz (%d)", epoch->_haveLlh, epoch->_haveXyz);
    }

    if ( (epoch->_haveHacc > HAVE_NOTHING) && (epoch->_haveVacc > HAVE_NOTHING) && (epoch->_havePacc < HAVE_UBX) )
    {
        epoch->posAcc = sqrt( (epoch->horizAcc * epoch->horizAcc) + (epoch->vertAcc * epoch->vertAcc) );
        epoch->_havePacc = MIN(epoch->_haveHacc, epoch->_haveVacc);
    }

    // Stringify
    epoch->fixStr = epoch->fixOk ? "UNKN" : "(UNKN)";
    switch (epoch->fix)
    {
        case EPOCH_FIX_NOFIX:   epoch->fixStr = epoch->fixOk ? "NOFIX" : "(NOFIX)"; break;
        case EPOCH_FIX_DRONLY:  epoch->fixStr = epoch->fixOk ? "DR"    : "(DR)";    break;
        case EPOCH_FIX_2D:      epoch->fixStr = epoch->fixOk ? "2D"    : "(2D)";    break;
        case EPOCH_FIX_3D:      epoch->fixStr = epoch->fixOk ? "3D"    : "(3D)";    break;
        case EPOCH_FIX_3D_DR:   epoch->fixStr = epoch->fixOk ? "3D+DR" : "(3D+DR)"; break;
        case EPOCH_FIX_TIME:    epoch->fixStr = epoch->fixOk ? "TIME"  : "(TIME)";  break;
    }

    epoch->rtkStr = "UNKN";
    switch (epoch->rtk)
    {
        case EPOCH_RTK_NONE:    epoch->rtkStr = "NORTK";  break;
        case EPOCH_RTK_FLOAT:   epoch->rtkStr = "FLOAT"; break;
        case EPOCH_RTK_FIXED:   epoch->rtkStr = "FIXED"; break;
    }

    snprintf(epoch->str, sizeof(epoch->str),
        "%-7s %-5s %2d %04d-%02d-%02d (%c) %02d:%02d:%06.3f (%c) %+11.7f %+12.7f (%5.1f) %+5.0f (%5.1f) %4.1f",
        epoch->fixStr, epoch->rtkStr, epoch->numSv,
        epoch->year, epoch->month, epoch->day, epoch->haveDate ? (epoch->confDate ? 'Y' : 'y') : 'N',
        epoch->hour, epoch->minute, epoch->second, epoch->haveTime ? (epoch->confTime ? 'Y' : 'y') : 'N',
        rad2deg(epoch->llh[0]), rad2deg(epoch->llh[1]), epoch->horizAcc, epoch->llh[2], epoch->vertAcc,
        epoch->pDOP);
}

const char *epochStrHeader(void)
{
    return "Fix     RTK  #SV YYYY-MM-DD     HH:MM:SS         Latitude      Longitude           Height       PDOP";
}

/* ****************************************************************************************************************** */
// eof
