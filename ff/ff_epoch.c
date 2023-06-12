// flipflip's navigation epoch abstraction
//
// Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org),
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "ff_stuff.h"
#include "ff_ubx.h"
#include "ff_nmea.h"
#include "ff_trafo.h"
#include "ff_debug.h"
#include "ff_trafo.h"
#include "ff_time.h"

#include "ff_epoch.h"

/* ****************************************************************************************************************** */

//#define EPOCH_DEBUG(fmt, args...) DEBUG("epoch: " fmt, ## args)
#define EPOCH_DEBUG(...) /* nothing */

void epochInit(EPOCH_t *coll)
{
    memset(coll, 0, sizeof(*coll));
}

// "Quality" (precision) if information: UBX better than NMEA, UBX high-precision messages better than normal UBX
typedef enum COLL_QUAL_e
{
    HAVE_NOTHING = 0, HAVE_NMEA = 1, HAVE_BETTER_NMEA = 2, HAVE_UBX = 3, HAVE_UBX_HP = 4
} COLL_QUAL_t;

// Private epoch collector state
typedef struct EPOCH_COLLECT_s
{
    COLL_QUAL_t  haveFix;
    COLL_QUAL_t  haveTime;
    COLL_QUAL_t  haveDate;
    COLL_QUAL_t  haveLlh;
    COLL_QUAL_t  haveVel;
    COLL_QUAL_t  haveHacc;
    COLL_QUAL_t  haveVacc;
    COLL_QUAL_t  havePacc;
    COLL_QUAL_t  haveXyz;
    COLL_QUAL_t  haveSig;
    COLL_QUAL_t  haveSat;
    COLL_QUAL_t  haveGpsTow;
    COLL_QUAL_t  haveGpsWeek;
    COLL_QUAL_t  haveRelPos;
    bool         relPosValid;
    COLL_QUAL_t  haveDiffAge;
} EPOCH_COLLECT_t;

STATIC_ASSERT(SIZEOF_MEMBER(EPOCH_t, _collect) >= sizeof(EPOCH_COLLECT_t));

typedef struct EPOCH_DETECT_s
{
    uint32_t     seq;
    uint32_t     ubxItow;
    bool         haveUbxItow;
    int          nmeaMs;
    bool         haveNmeaMs;
} EPOCH_DETECT_t;

STATIC_ASSERT(SIZEOF_MEMBER(EPOCH_t, _detect) >= sizeof(EPOCH_DETECT_t));

static bool _detectUbx(EPOCH_DETECT_t *detect, const PARSER_MSG_t *msg);
static bool _detectNmea(EPOCH_DETECT_t *detect, const NMEA_MSG_t *nmea);

static void _collectUbx(EPOCH_t *coll, EPOCH_COLLECT_t *collect, const PARSER_MSG_t *msg);
static void _collectNmea(EPOCH_t *coll, EPOCH_COLLECT_t *collect, const NMEA_MSG_t *nmea);

static void _epochComplete(const EPOCH_COLLECT_t *collect, EPOCH_t *epoch);

bool epochCollect(EPOCH_t *coll, const PARSER_MSG_t *msg, EPOCH_t *epoch)
{
    if ( (coll == NULL) || (msg == NULL) )
    {
        return false;
    }
    EPOCH_COLLECT_t *collect = (EPOCH_COLLECT_t *)coll->_collect;
    EPOCH_DETECT_t  *detect  = (EPOCH_DETECT_t *)coll->_detect;

    // Decode NMEA here, as this is quite expensive
    NMEA_MSG_t nmea;
    bool haveNmea = false;
    if (msg->type == PARSER_MSGTYPE_NMEA)
    {
        haveNmea = nmeaDecode(&nmea, msg->data, msg->size);
    }

    // Detect end of epoch / start of next epoch
    bool complete = false;
    switch (msg->type)
    {
        case PARSER_MSGTYPE_UBX:
            complete = _detectUbx(detect, msg);
            if (complete)
            {
                detect->haveNmeaMs = false;
            }
            break;
        case PARSER_MSGTYPE_NMEA:
            if (haveNmea)
            {
                complete = _detectNmea(detect, &nmea);
                if (complete)
                {
                    detect->haveUbxItow = false;
                }
            }
            break;
        default:
            break;
    }
    // TODO: in case of UBX-NAV-EOE, output that first and complete epoch on next call (if it can be done nicely..)

    // Output epoch
    if (complete)
    {
        detect->seq++;
        if (epoch != NULL)
        {
            memcpy(epoch, coll, sizeof(*epoch));
            epoch->seq = detect->seq;
            _epochComplete(collect, epoch);
        }

        // Initialise collector
        const EPOCH_DETECT_t saveDetect = *detect;
        memset(coll, 0, sizeof(*coll));
        *detect = saveDetect;

        //DEBUG("epoch %u ubx %u %d nmea %d %d", seq, tow, detectHaveTow, ms, detectHaveMs);
    }

    // Collect data
    switch (msg->type)
    {
        case PARSER_MSGTYPE_UBX:
            _collectUbx(coll, collect, msg);
            break;
        case PARSER_MSGTYPE_NMEA:
            if (haveNmea)
            {
                _collectNmea(coll, collect, &nmea);
            }
            break;
        default:
            break;
    }

    return complete;
}

static bool _detectUbx(EPOCH_DETECT_t *detect, const PARSER_MSG_t *msg)
{
    const uint8_t clsId = UBX_CLSID(msg->data);
    if (clsId != UBX_NAV_CLSID)
    {
        return false;
    }
    const uint8_t msgId = UBX_MSGID(msg->data);
    bool complete = false;
    switch (msgId)
    {
        case UBX_NAV_EOE_MSGID:
            EPOCH_DEBUG("detect %s", msg->name);
            detect->haveUbxItow = false;
            complete = true;
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
                if (detect->haveUbxItow && (detect->ubxItow != iTow))
                {
                    EPOCH_DEBUG("detect %s %u != %u", msg->name, detect->ubxItow, iTow);
                    complete = true;
                }
                detect->ubxItow = iTow;
                detect->haveUbxItow = true;
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
                if (detect->haveUbxItow && (detect->ubxItow != iTow))
                {
                    EPOCH_DEBUG("detect %s %u != %u", msg->name, detect->ubxItow, iTow);
                    complete = true;
                }
                detect->ubxItow = iTow;
                detect->haveUbxItow = true;
            }
            break;
    }

    return complete;
}

static bool _detectNmea(EPOCH_DETECT_t *detect, const NMEA_MSG_t *nmea)
{
    bool complete = false;
    int ms = -1;
    switch (nmea->type)
    {
        case NMEA_TYPE_NONE:
        case NMEA_TYPE_TXT:
        case NMEA_TYPE_GSV:
            break;
        case NMEA_TYPE_GGA:
            ms = (int)floor(nmea->gga.time.second * 1e3);
            break;
        case NMEA_TYPE_RMC:
            ms = (int)floor(nmea->rmc.time.second * 1e3);
            break;
        case NMEA_TYPE_GLL:
            ms = (int)floor(nmea->gll.time.second * 1e3);
            break;
    }
    if (ms >= 0)
    {
        if ( detect->haveNmeaMs && (ms != detect->nmeaMs) )
        {
            EPOCH_DEBUG("detect %s %s %d != %d", nmea->talker, nmea->formatter, ms, detect->nmeaMs);
            complete = true;
        }
        detect->nmeaMs = ms;
        detect->haveNmeaMs = true;
    }

    return complete;
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

static EPOCH_GNSS_t _nmeaGnssToGnss(NMEA_GNSS_t gnss)
{
    switch (gnss)
    {
        case NMEA_GNSS_UNKNOWN: break;
        case NMEA_GNSS_GPS:     return EPOCH_GNSS_GPS;
        case NMEA_GNSS_GLO:     return EPOCH_GNSS_GLO;
        case NMEA_GNSS_BDS:     return EPOCH_GNSS_BDS;
        case NMEA_GNSS_GAL:     return EPOCH_GNSS_GAL;
        case NMEA_GNSS_SBAS:    return EPOCH_GNSS_SBAS;
        case NMEA_GNSS_QZSS:    return EPOCH_GNSS_QZSS;
    }
    return EPOCH_GNSS_UNKNOWN;
}

static const char * const kEpochGnssStrs[] =
{
    [EPOCH_GNSS_UNKNOWN] = "?",
    [EPOCH_GNSS_GPS]     = "GPS",
    [EPOCH_GNSS_GLO]     = "GLO",
    [EPOCH_GNSS_GAL]     = "GAL",
    [EPOCH_GNSS_BDS]     = "BDS",
    [EPOCH_GNSS_SBAS]    = "SBAS",
    [EPOCH_GNSS_QZSS]    = "QZSS",
};

const char *epochGnssStr(const EPOCH_GNSS_t gnss)
{
    return (gnss >= 0) && (gnss < NUMOF(kEpochGnssStrs)) ? kEpochGnssStrs[gnss] : kEpochGnssStrs[EPOCH_GNSS_UNKNOWN];
}

static EPOCH_SIGNAL_t _ubxSigIdToSignal(const uint8_t gnssId, const uint8_t sigId)
{
    switch (gnssId)
    {
        case UBX_GNSSID_GPS:
            switch (sigId)
            {
                case UBX_SIGID_GPS_L1CA: return EPOCH_SIGNAL_GPS_L1CA;
                case UBX_SIGID_GPS_L2CL:
                case UBX_SIGID_GPS_L2CM: return EPOCH_SIGNAL_GPS_L2C;
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
                case UBX_SIGID_GAL_E1C:
                case UBX_SIGID_GAL_E1B:  return EPOCH_SIGNAL_GAL_E1;
                case UBX_SIGID_GAL_E5BI:
                case UBX_SIGID_GAL_E5BQ: return EPOCH_SIGNAL_GAL_E5B;
            }
            break;
        case UBX_GNSSID_BDS:
            switch (sigId)
            {
                case UBX_SIGID_BDS_B1ID1:
                case UBX_SIGID_BDS_B1ID2: return EPOCH_SIGNAL_BDS_B1I;
                case UBX_SIGID_BDS_B2ID1:
                case UBX_SIGID_BDS_B2ID2: return EPOCH_SIGNAL_BDS_B2I;
            }
            break;
        case UBX_GNSSID_QZSS:
            switch (sigId)
            {
                case UBX_SIGID_QZSS_L1CA: return EPOCH_SIGNAL_QZSS_L1CA;
                case UBX_SIGID_QZSS_L1S:  return EPOCH_SIGNAL_QZSS_L1S;
                case UBX_SIGID_QZSS_L2CM:
                case UBX_SIGID_QZSS_L2CL: return EPOCH_SIGNAL_QZSS_L2C;
            }
            break;
        case UBX_GNSSID_GLO:
            switch (sigId)
            {
                case UBX_SIGID_GLO_L1OF: return EPOCH_SIGNAL_GLO_L1OF;
                case UBX_SIGID_GLO_L2OF: return EPOCH_SIGNAL_GLO_L2OF;
            }
            break;
    }
    return EPOCH_SIGNAL_UNKNOWN;
}

static EPOCH_SIGNAL_t _nmeaSignalToSignal(NMEA_SIGNAL_t signal)
{
    switch (signal)
    {
        case NMEA_SIGNAL_UNKNOWN:    break;
        case NMEA_SIGNAL_GPS_L1CA:   return EPOCH_SIGNAL_GPS_L1CA;
        case NMEA_SIGNAL_GPS_L2C:    return EPOCH_SIGNAL_GPS_L2C;
        case NMEA_SIGNAL_SBAS_L1CA:  return EPOCH_SIGNAL_SBAS_L1CA;
        case NMEA_SIGNAL_GAL_E1:     return EPOCH_SIGNAL_GAL_E1;
        case NMEA_SIGNAL_GAL_E5B:    return EPOCH_SIGNAL_GAL_E5B;
        case NMEA_SIGNAL_BDS_B1I:    return EPOCH_SIGNAL_BDS_B1I;
        case NMEA_SIGNAL_BDS_B2I:    return EPOCH_SIGNAL_BDS_B2I;
        case NMEA_SIGNAL_QZSS_L1CA:  return EPOCH_SIGNAL_QZSS_L1CA;
        case NMEA_SIGNAL_QZSS_L1S:   return EPOCH_SIGNAL_QZSS_L1S;
        case NMEA_SIGNAL_QZSS_L2C:   return EPOCH_SIGNAL_QZSS_L2C;
        case NMEA_SIGNAL_GLO_L1OF:   return EPOCH_SIGNAL_GLO_L1OF;
        case NMEA_SIGNAL_GLO_L2OF:   return EPOCH_SIGNAL_GLO_L2OF;
    }
    return EPOCH_SIGNAL_UNKNOWN;
}

const char * const kEpochSignalStrs[] =
{
    [EPOCH_SIGNAL_UNKNOWN]   = "?",
    [EPOCH_SIGNAL_GPS_L1CA]  = "L1CA",
    [EPOCH_SIGNAL_GPS_L2C]   = "L2C",
    [EPOCH_SIGNAL_SBAS_L1CA] = "L1CA",
    [EPOCH_SIGNAL_GAL_E1]    = "E1",
    [EPOCH_SIGNAL_GAL_E5B]   = "E5B",
    [EPOCH_SIGNAL_BDS_B1I]   = "B1I",
    [EPOCH_SIGNAL_BDS_B2I]   = "B2I",
    [EPOCH_SIGNAL_QZSS_L1CA] = "L1CA",
    [EPOCH_SIGNAL_QZSS_L1S]  = "L1S",
    [EPOCH_SIGNAL_QZSS_L2C]  = "L2C",
    [EPOCH_SIGNAL_GLO_L1OF]  = "L1OF",
    [EPOCH_SIGNAL_GLO_L2OF]  = "L2OF",
};

const char *epochSignalStr(const EPOCH_SIGNAL_t signal)
{
    return (signal >= 0) && (signal < NUMOF(kEpochSignalStrs)) ? kEpochSignalStrs[signal] : kEpochSignalStrs[EPOCH_SIGNAL_UNKNOWN];
}

EPOCH_GNSS_t epochSignalGnss(const EPOCH_SIGNAL_t signal)
{
    switch (signal)
    {
        case EPOCH_SIGNAL_UNKNOWN:
            return EPOCH_GNSS_UNKNOWN;
        case EPOCH_SIGNAL_GPS_L1CA:
        case EPOCH_SIGNAL_GPS_L2C:
            return EPOCH_GNSS_GPS;
        case EPOCH_SIGNAL_SBAS_L1CA:
            return EPOCH_GNSS_SBAS;
        case EPOCH_SIGNAL_GAL_E1:
        case EPOCH_SIGNAL_GAL_E5B:
            return EPOCH_GNSS_GAL;
        case EPOCH_SIGNAL_BDS_B1I:
        case EPOCH_SIGNAL_BDS_B2I:
            return EPOCH_GNSS_BDS;
        case EPOCH_SIGNAL_QZSS_L1CA:
        case EPOCH_SIGNAL_QZSS_L1S:
        case EPOCH_SIGNAL_QZSS_L2C:
            return EPOCH_GNSS_QZSS;
        case EPOCH_SIGNAL_GLO_L1OF:
        case EPOCH_SIGNAL_GLO_L2OF:
            return EPOCH_GNSS_GLO;
    }
    return EPOCH_GNSS_UNKNOWN;
}

int epochSvToIx(const EPOCH_GNSS_t gnss, const int sv)
{
    int ix = EPOCH_NO_SV;
    switch (gnss)
    {
        case EPOCH_GNSS_UNKNOWN:
            break;
        case EPOCH_GNSS_GPS:
            if ( (sv >= EPOCH_FIRST_GPS) && (sv <= EPOCH_NUM_GPS) )
            {
                ix = sv - EPOCH_FIRST_GPS;
            }
            break;
        case EPOCH_GNSS_GLO:
            if ( (sv >= EPOCH_FIRST_GLO) && (sv <= EPOCH_NUM_GLO) )
            {
                ix = EPOCH_NUM_GPS + sv - EPOCH_FIRST_GLO;
            }
            break;
        case EPOCH_GNSS_GAL:
            if ( (sv >= EPOCH_FIRST_GAL) && (sv <= EPOCH_NUM_GAL) )
            {
                ix = EPOCH_NUM_GPS + EPOCH_NUM_GLO + sv - EPOCH_FIRST_GAL;
            }
            break;
        case EPOCH_GNSS_BDS:
            if ( (sv >= EPOCH_FIRST_BDS) && (sv <= EPOCH_NUM_BDS) )
            {
                ix = EPOCH_NUM_GPS + EPOCH_NUM_GLO + EPOCH_NUM_GAL + sv - EPOCH_FIRST_BDS;
            }
            break;
        case EPOCH_GNSS_SBAS:
            if ( (sv >= EPOCH_FIRST_SBAS) && (sv <= EPOCH_NUM_SBAS) )
            {
                ix = EPOCH_NUM_GPS + EPOCH_NUM_GLO + EPOCH_NUM_GAL + EPOCH_NUM_BDS + sv - EPOCH_FIRST_SBAS;
            }
            break;
        case EPOCH_GNSS_QZSS:
            if ( (sv >= EPOCH_FIRST_QZSS) && (sv <= EPOCH_NUM_QZSS) )
            {
                ix = EPOCH_NUM_GPS + EPOCH_NUM_GLO + EPOCH_NUM_GAL + EPOCH_NUM_BDS + EPOCH_NUM_SBAS + sv - EPOCH_FIRST_QZSS;
            }
            break;
    }
    return ix;
}

const char * const kEpochBandStrs[] =
{
    [EPOCH_BAND_UNKNOWN] = "?",
    [EPOCH_BAND_L1]      = "L1",
    [EPOCH_BAND_L2]      = "L2",
    [EPOCH_BAND_L5]      = "L5",
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
                "B31", "B32", "B33", "B34", "B35", "B36", "B37", "B38", "B30", "B40",
                "B31", "B32", "B33", "B34", "B35", "B36", "B37", "B38", "B30", "B40",
                "B41", "B42", "B43", "B44", "B45", "B46", "B47", "B48", "B40", "B50",
                "B51", "B52", "B53", "B54", "B55", "B56", "B57", "B58", "B50", "B60",
                "B61", "B62", "B63"
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

static EPOCH_SIGUSE_t _ubxSigUse(const uint8_t qualityInd)
{
    switch (qualityInd)
    {
        case UBX_NAV_SIG_V0_QUALITYIND_SEARCH:    return EPOCH_SIGUSE_SEARCH;
        case UBX_NAV_SIG_V0_QUALITYIND_ACQUIRED:  return EPOCH_SIGUSE_ACQUIRED;
        case UBX_NAV_SIG_V0_QUALITYIND_UNUSED:    return EPOCH_SIGUSE_UNUSABLE;
        case UBX_NAV_SIG_V0_QUALITYIND_CODELOCK:  return EPOCH_SIGUSE_CODELOCK;
        case UBX_NAV_SIG_V0_QUALITYIND_CARRLOCK1:
        case UBX_NAV_SIG_V0_QUALITYIND_CARRLOCK2:
        case UBX_NAV_SIG_V0_QUALITYIND_CARRLOCK3: return EPOCH_SIGUSE_CARRLOCK;
        case UBX_NAV_SIG_V0_QUALITYIND_NOSIG:     return EPOCH_SIGUSE_NONE;
        default:                                  return EPOCH_SIGUSE_UNKNOWN;
    }
}

static const char * const kEpochSiqUseStrs[] =
{
    [EPOCH_SIGUSE_UNKNOWN]  = "UNKNOWN",
    [EPOCH_SIGUSE_NONE]     = "NONE",
    [EPOCH_SIGUSE_SEARCH]   = "SEARCH",
    [EPOCH_SIGUSE_ACQUIRED] = "ACQUIRED",
    [EPOCH_SIGUSE_UNUSABLE] = "UNUSABLE",
    [EPOCH_SIGUSE_CODELOCK] = "CODELOCK",
    [EPOCH_SIGUSE_CARRLOCK] = "CARRLOCK",
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
        case UBX_NAV_SIG_V0_CORRUSED_SPARTN:    return EPOCH_SIGCORR_SPARTN;
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
    [EPOCH_SIGCORR_SPARTN]    = "SPARTN",
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

static const char * const kEpochOrbStrs[] =
{
    [EPOCH_SATORB_NONE]  = "NONE",
    [EPOCH_SATORB_EPH]   = "EPH",
    [EPOCH_SATORB_ALM]   = "ALM",
    [EPOCH_SATORB_PRED]  = "PRED",
    [EPOCH_SATORB_OTHER] = "OTHER",
};

static int _epochSigInfoSort(const void *a, const void *b)
{
    const EPOCH_SIGINFO_t *sA = (const EPOCH_SIGINFO_t *)a;
    const EPOCH_SIGINFO_t *sB = (const EPOCH_SIGINFO_t *)b;
    return (int32_t)(sA->_order - sB->_order);
}

static int _epochSatInfoSort(const void *a, const void *b)
{
    const EPOCH_SATINFO_t *sA = (const EPOCH_SATINFO_t *)a;
    const EPOCH_SATINFO_t *sB = (const EPOCH_SATINFO_t *)b;
    return (int32_t)(sA->_order - sB->_order);
}

static void _collectUbx(EPOCH_t *coll, EPOCH_COLLECT_t *collect, const PARSER_MSG_t *msg)
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
                if (collect->haveFix < HAVE_UBX)
                {
                    collect->haveFix = HAVE_UBX;
                    switch (pvt.fixType)
                    {
                        case UBX_NAV_PVT_V1_FIXTYPE_NOFIX:  coll->fix = EPOCH_FIX_NOFIX;  break;
                        case UBX_NAV_PVT_V1_FIXTYPE_DRONLY: coll->fix = EPOCH_FIX_DRONLY; break;
                        case UBX_NAV_PVT_V1_FIXTYPE_2D:     coll->fix = EPOCH_FIX_S2D;    break;
                        case UBX_NAV_PVT_V1_FIXTYPE_3D:     coll->fix = EPOCH_FIX_S3D;    break;
                        case UBX_NAV_PVT_V1_FIXTYPE_3D_DR:  coll->fix = EPOCH_FIX_S3D_DR; break;
                        case UBX_NAV_PVT_V1_FIXTYPE_TIME:   coll->fix = EPOCH_FIX_TIME;   break;
                    }
                    if (coll->fix > EPOCH_FIX_NOFIX)
                    {
                        coll->fixOk = FLAG(pvt.flags, UBX_NAV_PVT_V1_FLAGS_GNSSFIXOK);
                    }
                    switch (UBX_NAV_PVT_V1_FLAGS_CARRSOLN_GET(pvt.flags))
                    {
                        case UBX_NAV_PVT_V1_FLAGS_CARRSOLN_FLOAT:
                            coll->fix = coll->fix == EPOCH_FIX_S3D_DR ? EPOCH_FIX_RTK_FLOAT_DR : EPOCH_FIX_RTK_FLOAT;
                            break;
                        case UBX_NAV_PVT_V1_FLAGS_CARRSOLN_FIXED:
                            coll->fix = coll->fix == EPOCH_FIX_S3D_DR ? EPOCH_FIX_RTK_FIXED_DR : EPOCH_FIX_RTK_FIXED;
                            break;
                    }
                    coll->haveFix = true;
                }

                // Time
                if (collect->haveTime < HAVE_UBX)
                {
                    collect->haveTime = HAVE_UBX;
                    coll->hour        = pvt.hour;
                    coll->minute      = pvt.min;
                    coll->second      = (double)pvt.sec + ((double)pvt.nano * 1e-9);
                    coll->haveTime    = FLAG(pvt.valid, UBX_NAV_PVT_V1_VALID_VALIDTIME);
                    coll->confTime    = FLAG(pvt.flags2, UBX_NAV_PVT_V1_FLAGS2_CONFTIME);
                    coll->timeAcc     = (double)pvt.tAcc * UBX_NAV_PVT_V1_TACC_SCALE;
                    coll->leapSecKnown = FLAG(pvt.valid, UBX_NAV_PVT_V1_VALID_FULLYRESOLVED);
                }

                // Date
                if (collect->haveDate < HAVE_UBX)
                {
                    collect->haveDate = HAVE_UBX;
                    coll->year        = pvt.year;
                    coll->month       = pvt.month;
                    coll->day         = pvt.day;
                    coll->haveDate    = FLAG(pvt.valid, UBX_NAV_PVT_V1_VALID_VALIDDATE);
                    coll->confDate    = FLAG(pvt.flags2, UBX_NAV_PVT_V1_FLAGS2_CONFDATE);
                }

                // Geodetic coordinates
                if (collect->haveLlh < HAVE_UBX)
                {
                    collect->haveLlh = HAVE_UBX;
                    coll->llh[0]      = deg2rad((double)pvt.lat * UBX_NAV_PVT_V1_LAT_SCALE);
                    coll->llh[1]      = deg2rad((double)pvt.lon * UBX_NAV_PVT_V1_LON_SCALE);
                    coll->llh[2]      = (double)pvt.height * UBX_NAV_PVT_V1_HEIGHT_SCALE;
                    coll->heightMsl   = (double)pvt.hMSL * UBX_NAV_PVT_V1_HEIGHT_SCALE;
                    coll->haveMsl     = !FLAG(pvt.flags3, UBX_NAV_PVT_V1_FLAGS3_INVALIDLLH);
                }

                // Position accuracy estimate
                if (pvt.fixType > UBX_NAV_PVT_V1_FIXTYPE_NOFIX)
                {
                    if (collect->haveHacc < HAVE_UBX)
                    {
                        collect->haveHacc = HAVE_UBX;
                        coll->horizAcc = (double)pvt.hAcc * UBX_NAV_PVT_V1_HACC_SCALE;
                    }
                    if (collect->haveVacc < HAVE_UBX)
                    {
                        collect->haveVacc = HAVE_UBX;
                        coll->vertAcc     = (double)pvt.vAcc * UBX_NAV_PVT_V1_VACC_SCALE;
                    }
                }

                // Velocity
                if (collect->haveVel < HAVE_UBX)
                {
                    collect->haveVel = HAVE_UBX;
                    coll->velNed[0] = pvt.velN * UBX_NAV_PVT_V1_VELNED_SCALE;
                    coll->velNed[1] = pvt.velE * UBX_NAV_PVT_V1_VELNED_SCALE;
                    coll->velNed[2] = pvt.velD * UBX_NAV_PVT_V1_VELNED_SCALE;
                    coll->velAcc    = pvt.sAcc * UBX_NAV_PVT_V1_SACC_SCALE;
                }

                coll->pDOP        = (float)pvt.pDOP * UBX_NAV_PVT_V1_PDOP_SCALE;
                coll->havePdop    = true;

                coll->numSv       = pvt.numSV;
                coll->haveNumSv   = true;

                if (collect->haveGpsTow < HAVE_UBX)
                {
                    collect->haveGpsTow = HAVE_UBX;
                    coll->gpsTow      = pvt.iTOW * UBX_NAV_PVT_V1_ITOW_SCALE;
                    coll->gpsTowAcc   = coll->timeAcc > 1e-3 ? coll->timeAcc : 1e-3; // 1ms at best
                    coll->haveGpsTow  = coll->haveTime;
                }
            }
            break;
        case UBX_NAV_POSECEF_MSGID:
            if (msg->size == UBX_NAV_POSECEF_V0_SIZE)
            {
                EPOCH_DEBUG("collect %s", msg->name);
                UBX_NAV_POSECEF_V0_GROUP0_t pos;
                memcpy(&pos, &msg->data[UBX_HEAD_SIZE], sizeof(pos));
                if (collect->haveXyz < HAVE_UBX)
                {
                    collect->haveXyz = HAVE_UBX;
                    coll->xyz[0] = (double)pos.ecefX * UBX_NAV_POSECEF_V0_ECEF_XYZ_SCALE;
                    coll->xyz[1] = (double)pos.ecefY * UBX_NAV_POSECEF_V0_ECEF_XYZ_SCALE;
                    coll->xyz[2] = (double)pos.ecefZ * UBX_NAV_POSECEF_V0_ECEF_XYZ_SCALE;
                }
                if (collect->havePacc < HAVE_UBX)
                {
                    collect->havePacc = HAVE_UBX;
                    coll->posAcc = (double)pos.pAcc  * UBX_NAV_POSECEF_V0_PACC_SCALE;
                }
            }
            break;
        case UBX_NAV_TIMEGPS_MSGID:
            if (msg->size == UBX_NAV_TIMEGPS_V0_SIZE)
            {
                EPOCH_DEBUG("collect %s", msg->name);
                UBX_NAV_TIMEGPS_V0_GROUP0_t time;
                memcpy(&time, &msg->data[UBX_HEAD_SIZE], sizeof(time));
                if (FLAG(time.valid, UBX_NAV_TIMEGPS_V0_VALID_WEEKVALID))
                {
                    coll->gpsWeek = time.week;
                    coll->haveGpsWeek = true;
                }

                if (collect->haveGpsTow < HAVE_UBX_HP)
                {
                    collect->haveGpsTow = HAVE_UBX_HP;
                    coll->gpsTow      = (time.iTow * UBX_NAV_TIMEGPS_V0_ITOW_SCALE) + (time.fTOW * UBX_NAV_TIMEGPS_V0_FTOW_SCALE);
                    coll->gpsTowAcc   = time.tAcc * UBX_NAV_TIMEGPS_V0_TACC_SCALE;
                    coll->haveGpsTow  = FLAG(time.valid, UBX_NAV_TIMEGPS_V0_VALID_TOWVALID);
                }

                if (collect->haveGpsWeek < HAVE_UBX)
                {
                    collect->haveGpsWeek = HAVE_UBX;
                    coll->gpsWeek      = time.week;
                    coll->haveGpsWeek  = FLAG(time.valid, UBX_NAV_TIMEGPS_V0_VALID_WEEKVALID);
                }

                if (!coll->haveLeapSeconds && FLAG(time.valid, UBX_NAV_TIMEGPS_V0_VALID_LEAPSVALID))
                {
                    coll->haveLeapSeconds = true;
                    coll->leapSeconds = time.leapS;
                }
            }
            break;
        case UBX_NAV_HPPOSECEF_MSGID:
            if ( (msg->size == UBX_NAV_HPPOSECEF_V0_SIZE) && (UBX_NAV_HPPOSECEF_VERSION_GET(msg->data) == UBX_NAV_HPPOSECEF_V0_VERSION) )
            {
                EPOCH_DEBUG("collect %s", msg->name);
                UBX_NAV_HPPOSECEF_V0_GROUP0_t pos;
                memcpy(&pos, &msg->data[UBX_HEAD_SIZE], sizeof(pos));
                if (!FLAG(pos.flags, UBX_NAV_HPPOSECEF_V0_FLAGS_INVALIDECEF))
                {
                    if (collect->haveXyz < HAVE_UBX_HP)
                    {
                        collect->haveXyz = HAVE_UBX_HP;
                        coll->xyz[0] = ((double)pos.ecefX * UBX_NAV_HPPOSECEF_V0_ECEF_XYZ_SCALE) + ((double)pos.ecefXHp * UBX_NAV_HPPOSECEF_V0_ECEF_XYZ_HP_SCALE);
                        coll->xyz[1] = ((double)pos.ecefY * UBX_NAV_HPPOSECEF_V0_ECEF_XYZ_SCALE) + ((double)pos.ecefYHp * UBX_NAV_HPPOSECEF_V0_ECEF_XYZ_HP_SCALE);
                        coll->xyz[2] = ((double)pos.ecefZ * UBX_NAV_HPPOSECEF_V0_ECEF_XYZ_SCALE) + ((double)pos.ecefZHp * UBX_NAV_HPPOSECEF_V0_ECEF_XYZ_HP_SCALE);
                    }
                    if (collect->havePacc < HAVE_UBX_HP)
                    {
                        collect->havePacc = HAVE_UBX_HP;
                        coll->posAcc = (double)pos.pAcc * UBX_NAV_HPPOSECEF_V0_PACC_SCALE;
                    }
                }
            }
            break;
        case UBX_NAV_RELPOSNED_MSGID:
            if ( (msg->size == UBX_NAV_RELPOSNED_V1_SIZE) && (UBX_NAV_RELPOSNED_VERSION_GET(msg->data) == UBX_NAV_RELPOSNED_V1_VERSION) )
            {
                EPOCH_DEBUG("collect %s", msg->name);
                UBX_NAV_RELPOSNED_V1_GROUP0_t rel;
                memcpy(&rel, &msg->data[UBX_HEAD_SIZE], sizeof(rel));
                if (FLAG(rel.flags, UBX_NAV_RELPOSNED_V1_FLAGS_RELPOSVALID))
                {
                    coll->relNed[0] = (rel.relPosN * UBX_NAV_RELPOSNED_V1_RELPOSN_E_D_SCALE) + (rel.relPosHPN * UBX_NAV_RELPOSNED_V1_RELPOSHPN_E_D_SCALE);
                    coll->relNed[1] = (rel.relPosE * UBX_NAV_RELPOSNED_V1_RELPOSN_E_D_SCALE) + (rel.relPosHPE * UBX_NAV_RELPOSNED_V1_RELPOSHPN_E_D_SCALE);
                    coll->relNed[2] = (rel.relPosD * UBX_NAV_RELPOSNED_V1_RELPOSN_E_D_SCALE) + (rel.relPosHPD * UBX_NAV_RELPOSNED_V1_RELPOSHPN_E_D_SCALE);
                    coll->relAcc[0] = rel.accN * UBX_NAV_RELPOSNED_V1_ACCN_E_D_SCALE;
                    coll->relAcc[1] = rel.accE * UBX_NAV_RELPOSNED_V1_ACCN_E_D_SCALE;
                    coll->relAcc[2] = rel.accD * UBX_NAV_RELPOSNED_V1_ACCN_E_D_SCALE;
                    coll->relLen    = (rel.relPosLength * UBX_NAV_RELPOSNED_V1_RELPOSLENGTH_SCALE) + (rel.relPosHPLength * UBX_NAV_RELPOSNED_V1_RELPOSHPLENGTH_SCALE);
                    collect->haveRelPos = HAVE_UBX_HP;
                    collect->relPosValid = FLAG(rel.flags, UBX_NAV_RELPOSNED_V1_FLAGS_RELPOSVALID);
                }
            }
            break;
        case UBX_NAV_SIG_MSGID:
            if ( (msg->size >= UBX_NAV_SIG_V0_MIN_SIZE) && (UBX_NAV_SIG_VERSION_GET(msg->data) == UBX_NAV_SIG_V0_VERSION) )
            {
                EPOCH_DEBUG("collect %s", msg->name);
                if (collect->haveSig < HAVE_UBX)
                {
                    collect->haveSig = HAVE_UBX;
                    UBX_NAV_SIG_V0_GROUP0_t head;
                    memcpy(&head, &msg->data[UBX_HEAD_SIZE], sizeof(head));
                    int ix;
                    for (coll->numSignals = 0, ix = 0; (coll->numSignals < (int)head.numSigs) && (coll->numSignals < NUMOF(coll->signals)); coll->numSignals++, ix++)
                    {
                        UBX_NAV_SIG_V0_GROUP1_t uInfo;
                        memcpy(&uInfo, &msg->data[UBX_HEAD_SIZE + sizeof(UBX_NAV_SIG_V0_GROUP0_t) + (ix * sizeof(uInfo))], sizeof(uInfo));
                        EPOCH_SIGINFO_t *eInfo = &coll->signals[ix];
                        eInfo->valid       = true;
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
                        eInfo->use         = _ubxSigUse(uInfo.qualityInd);
                        eInfo->corr        = _ubxSigCorrSource(uInfo.corrSource);
                        eInfo->iono        = _ubxIonoModel(uInfo.ionoModel);
                        eInfo->health      = _ubxSigHealth(UBX_NAV_SIG_V0_SIGFLAGS_HEALTH_GET(uInfo.sigFlags));
                    }
                }
            }
            break;
        case UBX_NAV_SAT_MSGID:
            if ( (msg->size >= UBX_NAV_SAT_V1_MIN_SIZE) && (UBX_NAV_SAT_VERSION_GET(msg->data) == UBX_NAV_SAT_V1_VERSION) )
            {
                EPOCH_DEBUG("collect %s", msg->name);
                if (collect->haveSat < HAVE_UBX)
                {
                    collect->haveSat = HAVE_UBX;
                    UBX_NAV_SAT_V1_GROUP0_t head;
                    memcpy(&head, &msg->data[UBX_HEAD_SIZE], sizeof(head));
                    int ix;
                    for (coll->numSatellites = 0, ix = 0; (coll->numSatellites < (int)head.numSvs) && (coll->numSatellites < NUMOF(coll->satellites)); coll->numSatellites++, ix++)
                    {
                        UBX_NAV_SAT_V1_GROUP1_t uInfo;
                        memcpy(&uInfo, &msg->data[UBX_HEAD_SIZE + sizeof(UBX_NAV_SAT_V1_GROUP0_t) + (ix * sizeof(uInfo))], sizeof(uInfo));
                        EPOCH_SATINFO_t *eInfo = &coll->satellites[ix];
                        eInfo->valid       = true;
                        eInfo->gnss        = _ubxGnssIdToGnss(uInfo.gnssId);
                        eInfo->sv          = uInfo.svId;
                        const int orbSrc = UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_GET(uInfo.flags);
                        eInfo->orbUsed = EPOCH_SATORB_NONE;
                        eInfo->azim = uInfo.azim;
                        eInfo->elev = uInfo.elev;
                        switch (orbSrc)
                        {
                            case UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_NONE: break;
                            case UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_EPH:    eInfo->orbUsed = EPOCH_SATORB_EPH; break;
                            case UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_ALM:    eInfo->orbUsed = EPOCH_SATORB_ALM; break;
                            case UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_ANO:    /* FALLTHROUGH */
                            case UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_ANA:    eInfo->orbUsed = EPOCH_SATORB_PRED; break;
                            case UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_OTHER1: /* FALLTHROUGH */
                            case UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_OTHER2: /* FALLTHROUGH */
                            case UBX_NAV_SAT_V1_FLAGS_ORBITSOURCE_OTHER3: eInfo->orbUsed = EPOCH_SATORB_OTHER; break;
                        }
                        if (FLAG(uInfo.flags, UBX_NAV_SAT_V1_FLAGS_EPHAVAIL))
                        {
                            eInfo->orbAvail |= BIT(EPOCH_SATORB_EPH);
                        }
                        if (FLAG(uInfo.flags, UBX_NAV_SAT_V1_FLAGS_ALMAVAIL))
                        {
                            eInfo->orbAvail |= BIT(EPOCH_SATORB_ALM);
                        }
                        if (FLAG(uInfo.flags, UBX_NAV_SAT_V1_FLAGS_ANOAVAIL) || FLAG(uInfo.flags, UBX_NAV_SAT_V1_FLAGS_AOPAVAIL))
                        {
                            eInfo->orbAvail |= BIT(EPOCH_SATORB_PRED);
                        }
                    }
                }
            }
            break;
        case UBX_NAV_TIMELS_MSGID:
            if (msg->size == UBX_NAV_TIMELS_V0_SIZE)
            {
                EPOCH_DEBUG("collect %s", msg->name);
                UBX_NAV_TIMELS_V0_GROUP0_t timels;
                memcpy(&timels, &msg->data[UBX_HEAD_SIZE], sizeof(timels));

                if (!coll->haveLeapSeconds && FLAG(timels.valid, UBX_NAV_TIMELS_V0_VALID_CURRLSVALID))
                {
                    coll->leapSeconds = timels.currLs;
                    coll->haveLeapSeconds = true;
                }
            }
            break;
        case UBX_NAV_STATUS_MSGID:
            if (msg->size == UBX_NAV_STATUS_V0_SIZE)
            {
                EPOCH_DEBUG("collect %s", msg->name);
                UBX_NAV_STATUS_V0_GROUP0_t status;
                memcpy(&status, &msg->data[UBX_HEAD_SIZE], sizeof(status));
                if (!coll->haveUptime)
                {
                    coll->haveUptime = true;
                    coll->uptime = (double)status.msss * UBX_NAV_STATUS_V0_MSSS_SCALE;
                }
            }
            break;
        case UBX_NAV_CLOCK_MSGID:
            if (msg->size == UBX_NAV_CLOCK_V0_SIZE)
            {
                EPOCH_DEBUG("collect %s", msg->name);
                UBX_NAV_CLOCK_V0_GROUP0_t clock;
                memcpy(&clock, &msg->data[UBX_HEAD_SIZE], sizeof(clock));
                coll->haveClock = true;
                coll->clockBias = (double)clock.clkB * UBX_NAV_CLOCK_V0_CLKB_SCALE;
                coll->clockDrift = (double)clock.clkD * UBX_NAV_CLOCK_V0_CLKD_SCALE;
            }
            break;
    }
}

static void _collectNmea(EPOCH_t *coll, EPOCH_COLLECT_t *collect, const NMEA_MSG_t *nmea)
{
    switch (nmea->type)
    {
        case NMEA_TYPE_GGA:
            EPOCH_DEBUG("collect %s %s", nmea->talker, nmea->formatter);
            if (collect->haveTime < HAVE_NMEA)
            {
                collect->haveTime = HAVE_NMEA;
                coll->hour      = nmea->gga.time.hour;
                coll->minute    = nmea->gga.time.minute;
                coll->second    = nmea->gga.time.second;
                coll->haveTime  = nmea->gga.time.valid;
            }
            if (collect->haveFix < HAVE_NMEA)
            {
                collect->haveFix = HAVE_NMEA;
                switch (nmea->gga.fix)
                {
                    case NMEA_FIX_UNKNOWN:   coll->fix = EPOCH_FIX_UNKNOWN;   break;
                    case NMEA_FIX_NOFIX:     coll->fix = EPOCH_FIX_NOFIX;     break;
                    case NMEA_FIX_DRONLY:    coll->fix = EPOCH_FIX_DRONLY;    break;
                    case NMEA_FIX_S2D:       coll->fix = EPOCH_FIX_S2D;       break;
                    case NMEA_FIX_S3D:       coll->fix = EPOCH_FIX_S3D;       break;
                    case NMEA_FIX_S3D_DR:    coll->fix = EPOCH_FIX_S3D_DR;    break;
                    case NMEA_FIX_RTK_FLOAT: coll->fix = EPOCH_FIX_RTK_FLOAT; break;
                    case NMEA_FIX_RTK_FIXED: coll->fix = EPOCH_FIX_RTK_FIXED; break;
                }
                coll->fixOk = true;
                coll->haveFix = true;
            }
            if ( (nmea->gga.fix > NMEA_FIX_NOFIX) && (collect->haveLlh < HAVE_BETTER_NMEA) )
            {
                collect->haveLlh = HAVE_BETTER_NMEA;
                coll->llh[0]      = deg2rad(nmea->gga.lat);
                coll->llh[1]      = deg2rad(nmea->gga.lon);
                coll->llh[2]      = nmea->gga.height;
                coll->heightMsl   = nmea->gga.heightMsl;
                coll->haveMsl     = true;
            }
            if ( (nmea->gga.diffAge > -DBL_EPSILON) && (collect->haveDiffAge < HAVE_NMEA) )
            {
                collect->haveDiffAge = HAVE_NMEA;
                coll->diffAge = nmea->gga.diffAge;
                coll->haveDiffAge = true;
            }
            if (!coll->haveNumSv)
            {
                coll->numSv       = nmea->gga.numSv;
                coll->haveNumSv   = true;
            }
            break;

        case NMEA_TYPE_RMC:
            EPOCH_DEBUG("collect %s %s", nmea->talker, nmea->formatter);
            if (collect->haveTime < HAVE_NMEA)
            {
                collect->haveTime = HAVE_NMEA;
                coll->hour      = nmea->rmc.time.hour;
                coll->minute    = nmea->rmc.time.minute;
                coll->second    = nmea->rmc.time.second;
                coll->haveTime  = nmea->rmc.time.valid;
            }
            if (collect->haveDate < HAVE_NMEA)
            {
                collect->haveDate = HAVE_NMEA;
                coll->day       = nmea->rmc.date.day;
                coll->month     = nmea->rmc.date.month;
                coll->year      = nmea->rmc.date.year;
                coll->haveDate  = nmea->rmc.date.valid;
            }
            if (collect->haveFix < HAVE_BETTER_NMEA)
            {
                collect->haveFix = HAVE_BETTER_NMEA;
                switch (nmea->rmc.fix)
                {
                    case NMEA_FIX_UNKNOWN:   coll->fix = EPOCH_FIX_UNKNOWN;   break;
                    case NMEA_FIX_NOFIX:     coll->fix = EPOCH_FIX_NOFIX;     break;
                    case NMEA_FIX_DRONLY:    coll->fix = EPOCH_FIX_DRONLY;    break;
                    case NMEA_FIX_S2D:       coll->fix = EPOCH_FIX_S2D;       break;
                    case NMEA_FIX_S3D:       coll->fix = EPOCH_FIX_S3D;       break;
                    case NMEA_FIX_S3D_DR:    coll->fix = EPOCH_FIX_S3D_DR;    break;
                    case NMEA_FIX_RTK_FLOAT: coll->fix = EPOCH_FIX_RTK_FLOAT; break;
                    case NMEA_FIX_RTK_FIXED: coll->fix = EPOCH_FIX_RTK_FIXED; break;
                }
                coll->fixOk = nmea->rmc.valid;
                coll->haveFix = true;
            }
            if ( (nmea->rmc.fix > NMEA_FIX_NOFIX) && (collect->haveLlh < HAVE_NMEA) )
            {
                collect->haveLlh = HAVE_BETTER_NMEA;
                coll->llh[0]      = deg2rad(nmea->rmc.lat);
                coll->llh[1]      = deg2rad(nmea->rmc.lon);
                coll->llh[2]      = 0.0;
                coll->heightMsl   = 0.0;
                coll->haveMsl     = false;
            }
            break;

        case NMEA_TYPE_GLL:
            EPOCH_DEBUG("collect %s %s", nmea->talker, nmea->formatter);
            if (collect->haveTime < HAVE_NMEA)
            {
                collect->haveTime = HAVE_NMEA;
                coll->hour      = nmea->gll.time.hour;
                coll->minute    = nmea->gll.time.minute;
                coll->second    = nmea->gll.time.second;
                coll->haveTime  = nmea->gll.time.valid;
            }
            if (collect->haveFix < HAVE_BETTER_NMEA)
            {
                collect->haveFix = HAVE_BETTER_NMEA;
                switch (nmea->gll.fix)
                {
                    case NMEA_FIX_UNKNOWN:   coll->fix = EPOCH_FIX_UNKNOWN;   break;
                    case NMEA_FIX_NOFIX:     coll->fix = EPOCH_FIX_NOFIX;     break;
                    case NMEA_FIX_DRONLY:    coll->fix = EPOCH_FIX_DRONLY;    break;
                    case NMEA_FIX_S2D:       coll->fix = EPOCH_FIX_S2D;       break;
                    case NMEA_FIX_S3D:       coll->fix = EPOCH_FIX_S3D;       break;
                    case NMEA_FIX_S3D_DR:    coll->fix = EPOCH_FIX_S3D_DR;    break;
                    case NMEA_FIX_RTK_FLOAT: coll->fix = EPOCH_FIX_RTK_FLOAT; break;
                    case NMEA_FIX_RTK_FIXED: coll->fix = EPOCH_FIX_RTK_FIXED; break;
                }
                coll->fixOk = nmea->gll.valid;
                coll->haveFix = true;
            }
            if ( (nmea->gll.fix > NMEA_FIX_NOFIX) && (collect->haveLlh < HAVE_NMEA) )
            {
                collect->haveLlh = HAVE_BETTER_NMEA;
                coll->llh[0]      = deg2rad(nmea->gll.lat);
                coll->llh[1]      = deg2rad(nmea->gll.lon);
                coll->llh[2]      = 0.0;
                coll->heightMsl   = 0.0;
                coll->haveMsl     = false;
            }
            break;

        case NMEA_TYPE_GSV:
            EPOCH_DEBUG("collect %s %s", nmea->talker, nmea->formatter);
            if (collect->haveSat <= HAVE_NMEA) // multiple NMEA-Gx-GSV messages!
            {
                collect->haveSat = HAVE_NMEA;
                for (int ix = 0; (ix < nmea->gsv.nSvs) && (coll->numSatellites < (int)NUMOF(coll->satellites)); ix++)
                {
                    EPOCH_SATINFO_t *sat = &coll->satellites[coll->numSatellites];
                    sat->gnss      = _nmeaGnssToGnss(nmea->gsv.svs[ix].gnss);
                    sat->sv        = nmea->gsv.svs[ix].svId;
                    sat->orbUsed   = EPOCH_SATORB_EPH;       // presumably..
                    sat->orbAvail |= BIT(EPOCH_SATORB_EPH);  // presumably..
                    sat->elev      = nmea->gsv.svs[ix].elev;
                    sat->azim      = nmea->gsv.svs[ix].azim;
                    coll->numSatellites++;
                }
            }
            if (collect->haveSig <= HAVE_NMEA) // multiple NMEA-Gx-GSV messages!
            {
                collect->haveSig = HAVE_NMEA;
                for (int ix = 0; (ix < nmea->gsv.nSvs) && (coll->numSignals < (int)NUMOF(coll->signals)); ix++)
                {
                    EPOCH_SIGINFO_t *sig = &coll->signals[coll->numSignals];
                    switch (nmea->gsv.svs[ix].gnss)
                    {
                        case NMEA_GNSS_UNKNOWN: sig->gnss = EPOCH_GNSS_UNKNOWN; break;
                        case NMEA_GNSS_GPS:     sig->gnss = EPOCH_GNSS_GPS;     break;
                        case NMEA_GNSS_GLO:     sig->gnss = EPOCH_GNSS_GLO;     break;
                        case NMEA_GNSS_BDS:     sig->gnss = EPOCH_GNSS_BDS;     break;
                        case NMEA_GNSS_GAL:     sig->gnss = EPOCH_GNSS_GAL;     break;
                        case NMEA_GNSS_SBAS:    sig->gnss = EPOCH_GNSS_SBAS;    break;
                        case NMEA_GNSS_QZSS:    sig->gnss = EPOCH_GNSS_QZSS;    break;
                    }
                    sig->gnss      = _nmeaGnssToGnss(nmea->gsv.svs[ix].gnss);
                    sig->sv        = nmea->gsv.svs[ix].svId;
                    sig->signal    = _nmeaSignalToSignal(nmea->gsv.svs[ix].sig);
                    sig->cno       = nmea->gsv.svs[ix].cno;
                    sig->use       = EPOCH_SIGUSE_CODELOCK;   // presumably..
                    sig->health    = EPOCH_SIGHEALTH_HEALTHY; // presumably..
                    sig->prUsed    = true; // presumably..
                    coll->numSignals++;
                }
            }
            break;

        case NMEA_TYPE_NONE:
        case NMEA_TYPE_TXT:
            break;
    }
}

static void _epochComplete(const EPOCH_COLLECT_t *collect, EPOCH_t *epoch)
{
    epoch->valid = true;
    epoch->ts = TIME();
    const double now = posixNow();

    // Convert stuff, prefer better quality, FIXME: this assumes WGS84...

    if (collect->haveLlh > collect->haveXyz)
    {
        EPOCH_DEBUG("complete: llh (%d) -> xyz (%d)", collect->haveLlh, collect->haveXyz);
        llh2xyz_vec(epoch->llh, epoch->xyz);
        epoch->havePos = epoch->fix > EPOCH_FIX_NOFIX;
    }
    else if (collect->haveXyz > collect->haveLlh)
    {
        EPOCH_DEBUG("complete: xyz (%d) -> llh (%d)", collect->haveXyz, collect->haveLlh);
        xyz2llh_vec(epoch->xyz, epoch->llh);
        epoch->havePos = epoch->fix > EPOCH_FIX_NOFIX;
    }
    else
    {
        EPOCH_DEBUG("complete: llh (%d) = xyz (%d)", collect->haveLlh, collect->haveXyz);
    }

    if ( (collect->haveHacc > HAVE_NOTHING) && (collect->haveVacc > HAVE_NOTHING) && (collect->havePacc < HAVE_UBX) )
    {
        epoch->posAcc = sqrt( (epoch->horizAcc * epoch->horizAcc) + (epoch->vertAcc * epoch->vertAcc) );
        //collect->havePacc = MIN(collect->haveHacc, collect->haveVacc);
    }

    if (collect->haveRelPos > HAVE_NOTHING)
    {
        epoch->haveRelPos = true;
        // Sometimes fix is still RTK_FIXED/FLOAT but UBX-NAV-RELPOSNED.relPosValid indicates that the
        // relative (RTK) position is no longer available. Don't trust that fix.
        if ( (epoch->fix >= EPOCH_FIX_RTK_FLOAT) && !collect->relPosValid )
        {
            epoch->fixOk = false;
        }
    }

    if (collect->haveVel > HAVE_NOTHING)
    {
        epoch->haveVel = epoch->fix > EPOCH_FIX_NOFIX;
        const double velNEsq = (epoch->velNed[0] * epoch->velNed[0]) + (epoch->velNed[1] * epoch->velNed[1]);
        epoch->vel2d = sqrt(velNEsq);
        epoch->vel3d = sqrt( velNEsq + (epoch->velNed[2] * epoch->velNed[2]) );
    }

    // Stringify and sort list of satellites
    for (int ix = 0; ix < epoch->numSatellites; ix++)
    {
        EPOCH_SATINFO_t *sat = &epoch->satellites[ix];
        sat->gnssStr    = sat->gnss    < NUMOF(kEpochGnssStrs) ? kEpochGnssStrs[sat->gnss]   : kEpochGnssStrs[EPOCH_GNSS_UNKNOWN];
        sat->orbUsedStr = sat->orbUsed < NUMOF(kEpochOrbStrs)  ? kEpochOrbStrs[sat->orbUsed] : kEpochOrbStrs[EPOCH_SATORB_NONE];
        sat->svStr      = _epochSvStr(sat->gnss, sat->sv);
        sat->_order     = ((sat->gnss & 0xff) << 24) | ((sat->sv & 0xff) << 16);
    }
    qsort(epoch->satellites, epoch->numSatellites, sizeof(*epoch->satellites), _epochSatInfoSort);

    // Create lookup table for signal to satellite
    uint8_t satIxs[EPOCH_NUM_SV] = { [0 ... (EPOCH_NUM_SV-1)] = EPOCH_NO_SV };
    for (int ix = 0; ix < epoch->numSatellites; ix++)
    {
        EPOCH_SATINFO_t *sat = &epoch->satellites[ix];
        satIxs[ epochSvToIx(sat->gnss, sat->sv) ] = ix;
    }

    // Process, stringify and sort list of signals
    for (int ix = 0; ix < epoch->numSignals; ix++)
    {
        EPOCH_SIGINFO_t *sig = &epoch->signals[ix];
        switch (sig->signal)
        {
            case EPOCH_SIGNAL_UNKNOWN:    sig->band = EPOCH_BAND_UNKNOWN; break;
            case EPOCH_SIGNAL_GPS_L1CA:   sig->band = EPOCH_BAND_L1; break;
            case EPOCH_SIGNAL_GPS_L2C:    sig->band = EPOCH_BAND_L2; break;
            case EPOCH_SIGNAL_SBAS_L1CA:  sig->band = EPOCH_BAND_L1; break;
            case EPOCH_SIGNAL_GAL_E1:     sig->band = EPOCH_BAND_L1; break;
            case EPOCH_SIGNAL_GAL_E5B:    sig->band = EPOCH_BAND_L2; break;
            case EPOCH_SIGNAL_BDS_B1I:    sig->band = EPOCH_BAND_L1; break;
            case EPOCH_SIGNAL_BDS_B2I:    sig->band = EPOCH_BAND_L2; break;
            case EPOCH_SIGNAL_QZSS_L1CA:  sig->band = EPOCH_BAND_L1; break;
            case EPOCH_SIGNAL_QZSS_L1S:   sig->band = EPOCH_BAND_L1; break;
            case EPOCH_SIGNAL_QZSS_L2C:   sig->band = EPOCH_BAND_L2; break;
            case EPOCH_SIGNAL_GLO_L1OF:   sig->band = EPOCH_BAND_L1; break;
            case EPOCH_SIGNAL_GLO_L2OF:   sig->band = EPOCH_BAND_L2; break;
        }
        sig->anyUsed     = sig->prUsed || sig->crUsed || sig->doUsed;
        sig->gnssStr     = sig->gnss   < NUMOF(kEpochGnssStrs)      ? kEpochGnssStrs[sig->gnss]        : kEpochGnssStrs[EPOCH_GNSS_UNKNOWN];
        sig->svStr       = _epochSvStr(sig->gnss, sig->sv);
        sig->signalStr   = sig->signal < NUMOF(kEpochSignalStrs)    ? kEpochSignalStrs[sig->signal]    : kEpochSignalStrs[EPOCH_SIGNAL_UNKNOWN];
        sig->bandStr     = sig->band   < NUMOF(kEpochBandStrs)      ? kEpochBandStrs[sig->band]        : kEpochBandStrs[EPOCH_BAND_UNKNOWN];
        sig->useStr      = sig->use    < NUMOF(kEpochSiqUseStrs)    ? kEpochSiqUseStrs[sig->use]       : kEpochSiqUseStrs[EPOCH_SIGUSE_UNKNOWN];
        sig->corrStr     = sig->corr   < NUMOF(kEpochSigCorrStrs)   ? kEpochSigCorrStrs[sig->corr]     : kEpochSigCorrStrs[EPOCH_SIGCORR_UNKNOWN];
        sig->ionoStr     = sig->iono   < NUMOF(kEpochSigIonoStrs)   ? kEpochSigIonoStrs[sig->iono]     : kEpochSigIonoStrs[EPOCH_SIGIONO_UNKNOWN];
        sig->healthStr   = sig->health < NUMOF(kEpochSigHealthStrs) ? kEpochSigHealthStrs[sig->health] : kEpochSigHealthStrs[EPOCH_SIGHEALTH_UNKNOWN];
        sig->satIx       = satIxs[epochSvToIx(sig->gnss, sig->sv)];

        sig->_order = ((sig->gnss & 0xff) << 24) | ((sig->sv & 0xffff) << 8) | ((sig->signal & 0xff) << 0);
    }
    qsort(&epoch->signals, epoch->numSignals, sizeof(*epoch->signals), _epochSigInfoSort);

    // TODO: time/date <--(leapSec)--> wno/tow

    // Stringify fix type
    epoch->fixStr = epoch->fixOk ? "UNKN" : "(UNKN)";
    switch (epoch->fix)
    {
        case EPOCH_FIX_UNKNOWN: break;
        case EPOCH_FIX_NOFIX:        epoch->fixStr = epoch->fixOk ? "NOFIX"        : "(NOFIX)";        break;
        case EPOCH_FIX_DRONLY:       epoch->fixStr = epoch->fixOk ? "DR"           : "(DR)";           break;
        case EPOCH_FIX_TIME:         epoch->fixStr = epoch->fixOk ? "TIME"         : "(TIME)";         break;
        case EPOCH_FIX_S2D:          epoch->fixStr = epoch->fixOk ? "S2D"          : "(2D)";           break;
        case EPOCH_FIX_S3D:          epoch->fixStr = epoch->fixOk ? "S3D"          : "(3D)";           break;
        case EPOCH_FIX_S3D_DR:       epoch->fixStr = epoch->fixOk ? "S3D+DR"       : "(3D+DR)";        break;
        case EPOCH_FIX_RTK_FLOAT:    epoch->fixStr = epoch->fixOk ? "RTK_FLOAT"    : "(RTK_FLOAT)";    break;
        case EPOCH_FIX_RTK_FIXED:    epoch->fixStr = epoch->fixOk ? "RTK_FIXED"    : "(RTK_FIXED)";    break;
        case EPOCH_FIX_RTK_FLOAT_DR: epoch->fixStr = epoch->fixOk ? "RTK_FLOAT_DR" : "(RTK_FLOAT_DR)"; break;
        case EPOCH_FIX_RTK_FIXED_DR: epoch->fixStr = epoch->fixOk ? "RTK_FIXED_DR" : "(RTK_FIXED_DR)"; break;
    }

    // Count number of signals (and satellites) used, calculate CN0 histogram
    if (epoch->numSignals > 0)
    {
        epoch->haveNumSig = true;
        epoch->haveNumSat = true;
        epoch->haveSigCnoHist = true;
        const char *prevSvStr = "";
        for (int ix = 0; ix < epoch->numSignals; ix++)
        {
            const EPOCH_SIGINFO_t *sig = &epoch->signals[ix];
            if (sig->use < EPOCH_SIGUSE_ACQUIRED)
            {
                continue;
            }

            const int histIx = EPOCH_SIGCNOHIST_CNO2IX(sig->cno);
            epoch->sigCnoHistTrk[histIx]++;

            if (sig->anyUsed)
            {
                epoch->sigCnoHistNav[histIx]++;

                epoch->numSigUsed++;
                switch (sig->gnss)
                {
                    case EPOCH_GNSS_GPS:  epoch->numSigUsedGps++;  break;
                    case EPOCH_GNSS_GLO:  epoch->numSigUsedGlo++;  break;
                    case EPOCH_GNSS_BDS:  epoch->numSigUsedGal++;  break;
                    case EPOCH_GNSS_GAL:  epoch->numSigUsedBds++;  break;
                    case EPOCH_GNSS_SBAS: epoch->numSigUsedSbas++; break;
                    case EPOCH_GNSS_QZSS: epoch->numSigUsedQzss++; break;
                    case EPOCH_GNSS_UNKNOWN: break;
                }

                if (/*strcmp(prevSvStr, sig->svStr) != 0*/ prevSvStr != sig->svStr)
                {
                    epoch->numSatUsed++;
                    switch (sig->gnss)
                    {
                        case EPOCH_GNSS_GPS:  epoch->numSatUsedGps++;  break;
                        case EPOCH_GNSS_GLO:  epoch->numSatUsedGlo++;  break;
                        case EPOCH_GNSS_BDS:  epoch->numSatUsedGal++;  break;
                        case EPOCH_GNSS_GAL:  epoch->numSatUsedBds++;  break;
                        case EPOCH_GNSS_SBAS: epoch->numSatUsedSbas++; break;
                        case EPOCH_GNSS_QZSS: epoch->numSatUsedQzss++; break;
                        case EPOCH_GNSS_UNKNOWN: break;
                    }
                }
                prevSvStr = sig->svStr;
            }
        }
    }

    // Uptime string
    if (epoch->haveUptime)
    {
        bool anyway = false;
        int offs = 0;
        double val = epoch->uptime;
        // Days
        if (val > 86400.0)
        {
            const int days = floor(val / 86400.0);
            offs += snprintf(&epoch->uptimeStr[offs], sizeof(epoch->uptimeStr) - offs, "%dd", days);
            val -= (double)days * 86400.0;
            anyway = true;
        }
        // Hours
        if (anyway || (val > 3600.0))
        {
            const int hours = floor(val / 3600.0);
            offs += snprintf(&epoch->uptimeStr[offs], sizeof(epoch->uptimeStr) - offs, " %dh", hours);
            val -= (double)hours * 3600.0;
            anyway = true;
        }
        // Minutes
        if (anyway || (val > 60.0))
        {
            const int minutes = floor(val / 60.0);
            offs += snprintf(&epoch->uptimeStr[offs], sizeof(epoch->uptimeStr) - offs, " %dm", minutes);
            val -= (double)minutes * 60.0;
            anyway = true;
        }
        // Seconds
        {
            offs += snprintf(&epoch->uptimeStr[offs], sizeof(epoch->uptimeStr) - offs, anyway ? " %.1fs" : "%.1fs", val);
        }
    }

    // Time and date
    if (epoch->haveGpsWeek && epoch->haveGpsTow)
    {
        epoch->posixTime = ts2posix(wnoTow2ts(epoch->gpsWeek, epoch->gpsTow), epoch->leapSeconds, epoch->haveLeapSeconds);
        epoch->havePosixTime = true;

    }
    // else if (epoch->haveDate && epoch->haveTime) // FIXME TODO
    // {
    //     ...
    // }

    // Latency
    if (epoch->havePosixTime)
    {
        epoch->latency = now - epoch->posixTime;
        epoch->haveLatency = ((epoch->latency > 0.0f) && (epoch->latency <= 2.0f)); // Only when reading a live receiver
    }

    // Epoch info stringification
    snprintf(epoch->str, sizeof(epoch->str),
        "%-12s %2d %04d-%02d-%02d (%c) %02d:%02d:%06.3f (%c) %+11.7f %+12.7f (%5.1f) %+5.0f (%5.1f) %4.1f",
        epoch->fixStr, epoch->numSv,
        epoch->year, epoch->month, epoch->day, epoch->haveDate ? (epoch->confDate ? 'Y' : 'y') : 'N',
        epoch->hour, epoch->minute, epoch->second < 0.001 ? 0.0 : epoch->second, epoch->haveTime ? (epoch->confTime ? 'Y' : 'y') : 'N',
        rad2deg(epoch->llh[0]), rad2deg(epoch->llh[1]), epoch->horizAcc, epoch->llh[2], epoch->vertAcc,
        epoch->pDOP);
}

const char *epochStrHeader(void)
{
    return "Fix         #SV YYYY-MM-DD     HH:MM:SS         Latitude      Longitude           Height       PDOP";
}

/* ****************************************************************************************************************** */
// eof
