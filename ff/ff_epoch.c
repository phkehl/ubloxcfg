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

#include "ff_ubx.h"
#include "ff_nmea.h"

#include "ff_epoch.h"

/* ********************************************************************************************** */

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
        memset(coll, 0, sizeof(*coll));
        coll->seq = seq;
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
                if (!coll->_haveUbxItow)
                {
                    coll->_haveUbxItow = true;
                    coll->_ubxItow = iTow;
                }
                if (coll->_ubxItow != iTow)
                {
                    detect = true;
                }
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
                if (!coll->_haveUbxItow)
                {
                    coll->_haveUbxItow = true;
                    coll->_ubxItow = iTow;
                }
                if (coll->_ubxItow != iTow)
                {
                    detect = true;
                }
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
            if (!coll->_haveUbxNavPvt && (msg->size == UBX_NAV_PVT_V1_SIZE)) 
            {
                UBX_NAV_PVT_V1_GROUP0_t pvt;
                memcpy(&pvt, &msg->data[UBX_HEAD_SIZE], sizeof(pvt));
                switch (pvt.fixType)
                {
                    case UBX_NAV_PVT_V1_FIXTYPE_NOFIX:  coll->fix = EPOCH_FIX_NOFIX;  break;
                    case UBX_NAV_PVT_V1_FIXTYPE_DRONLY: coll->fix = EPOCH_FIX_DRONLY; break;
                    case UBX_NAV_PVT_V1_FIXTYPE_2D:     coll->fix = EPOCH_FIX_2D;     break;
                    case UBX_NAV_PVT_V1_FIXTYPE_3D:     coll->fix = EPOCH_FIX_3D;     break;
                    case UBX_NAV_PVT_V1_FIXTYPE_3D_DR:  coll->fix = EPOCH_FIX_3D_DR;  break;
                    case UBX_NAV_PVT_V1_FIXTYPE_TIME:   coll->fix = EPOCH_FIX_TIME;   break;
                }

                coll->fixQual = FLAG(pvt.flags, UBX_NAV_PVT_V1_FLAGS_GNSSFIXOK) ? EPOCH_QUAL_OK : EPOCH_QUAL_MASKED;
                coll->validFix = EPOCH_VALID_TRUE;

                switch (UBX_NAV_PVT_V1_FLAGS_CARRSOLN_GET(pvt.flags))
                {
                    case UBX_NAV_PVT_V1_FLAGS_CARRSOLN_NO:    coll->rtk = EPOCH_RTK_NONE;  break;
                    case UBX_NAV_PVT_V1_FLAGS_CARRSOLN_FLOAT: coll->rtk = EPOCH_RTK_FLOAT; break;
                    case UBX_NAV_PVT_V1_FLAGS_CARRSOLN_FIXED: coll->rtk = EPOCH_RTK_FIXED; break;
                }

                coll->year        = pvt.year;
                coll->month       = pvt.month;
                coll->day         = pvt.day;
                coll->validDate   = FLAG(pvt.valid, UBX_NAV_PVT_V1_VALID_VALIDDATE) ? (FLAG(pvt.flags2, UBX_NAV_PVT_V1_FLAGS2_CONFDATE)
                    ? EPOCH_VALID_CONFIRMED : EPOCH_VALID_TRUE) : EPOCH_VALID_FALSE;

                coll->hour        = pvt.hour;
                coll->minute      = pvt.min;
                coll->second      = (double)pvt.sec + ((double)pvt.nano * 1e-9);
                coll->validTime   = FLAG(pvt.valid, UBX_NAV_PVT_V1_VALID_VALIDTIME) ? (FLAG(pvt.flags2, UBX_NAV_PVT_V1_FLAGS2_CONFTIME)
                    ? EPOCH_VALID_CONFIRMED : EPOCH_VALID_TRUE) : EPOCH_VALID_FALSE;

                coll->timeAcc     = (double)pvt.tAcc * UBX_NAV_PVT_V1_TACC_SCALE;
                coll->validTimeAcc= EPOCH_VALID_TRUE;

                coll->lat         = (double)pvt.lat    * UBX_NAV_PVT_V1_LAT_SCALE;
                coll->lon         = (double)pvt.lon    * UBX_NAV_PVT_V1_LON_SCALE;
                coll->height      = (double)pvt.height * UBX_NAV_PVT_V1_HEIGHT_SCALE;
                coll->heightMsl   = (double)pvt.hMSL * UBX_NAV_PVT_V1_HEIGHT_SCALE;
                coll->validLlh    = FLAG(pvt.flags3, UBX_NAV_PVT_V1_FLAGS3_INVALIDLLH) ? EPOCH_VALID_FALSE :  EPOCH_VALID_TRUE;

                coll->horizAcc    = (double)pvt.hAcc * UBX_NAV_PVT_V1_HACC_SCALE;
                coll->vertAcc     = (double)pvt.vAcc * UBX_NAV_PVT_V1_VACC_SCALE;
                coll->validPosAcc = EPOCH_VALID_TRUE;

                coll->pDOP        = (float)pvt.pDOP * UBX_NAV_PVT_V1_PDOP_SCALE;
                coll->validPdop   = EPOCH_VALID_TRUE;

                coll->numSv       = pvt.numSV;
                coll->validNumSv  = EPOCH_VALID_TRUE;

                coll->gpsTow      = pvt.iTOW;
                coll->gpsTowAcc   = coll->timeAcc > 1e-3 ? coll->timeAcc : 1e-3;
                coll->validGpsTow = EPOCH_VALID_TRUE;

                coll->_haveUbxNavPvt = true;
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

static const char kValidChar[] =
{
    [EPOCH_VALID_UNKNOWN]   = '?',
    [EPOCH_VALID_FALSE]     = 'n',
    [EPOCH_VALID_TRUE]      = 'y',
    [EPOCH_VALID_CONFIRMED] = 'Y'
};

static void _epochComplete(EPOCH_t *coll, EPOCH_t *epoch)
{
    memcpy(epoch, coll, sizeof(*epoch));

    epoch->fixStr = "UNKN";
    switch (epoch->fix)
    {
        case EPOCH_FIX_UNKNOWN:                          break;
        case EPOCH_FIX_NOFIX:   epoch->fixStr = "NOFIX"; break;
        case EPOCH_FIX_DRONLY:  epoch->fixStr = "DR";    break;
        case EPOCH_FIX_2D:      epoch->fixStr = "2D";    break;
        case EPOCH_FIX_3D:      epoch->fixStr = "3D";    break;
        case EPOCH_FIX_3D_DR:   epoch->fixStr = "3D+DR"; break;
        case EPOCH_FIX_TIME:    epoch->fixStr = "TIME";  break;
    }

    epoch->fixQualStr = "UNKN";
    switch (epoch->fixQual)
    {
        case EPOCH_QUAL_UNKNOWN:                               break;
        case EPOCH_QUAL_MASKED:  epoch->fixQualStr = "MASKED"; break;
        case EPOCH_QUAL_OK:      epoch->fixQualStr = "OK";     break;
    }

    epoch->rtkStr = "UNKN";
    switch (epoch->rtk)
    {
        case EPOCH_RTK_UNKNOWN:                          break;
        case EPOCH_RTK_NONE:    epoch->rtkStr = "NONE";  break;
        case EPOCH_RTK_FLOAT:   epoch->rtkStr = "FLOAT"; break;
        case EPOCH_RTK_FIXED:   epoch->rtkStr = "FIXED"; break;
    }

    snprintf(epoch->str, sizeof(epoch->str),
        "%s %s %d %s %04d-%02d-%02d (%c) %02d:%02d:%06.3f (%c) %+.7f %+.7f (%.1f) %+.0f (%.1f)",
        epoch->fixStr, epoch->fixQualStr, epoch->numSv, epoch->rtkStr,
        epoch->year, epoch->month, epoch->day, kValidChar[epoch->validDate],
        epoch->hour, epoch->minute, epoch->second, kValidChar[epoch->validTime],
        epoch->lat, epoch->lon, epoch->horizAcc, epoch->height, epoch->vertAcc);
}

/* ********************************************************************************************** */
// eof
