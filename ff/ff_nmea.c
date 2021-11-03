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

#include <string.h>
#include <stdio.h>

#include "ff_stuff.h"
#include "ff_debug.h"
#include "ff_nmea.h"

/* ****************************************************************************************************************** */

#if 0
#  define NMEA_DEBUG(...) DEBUG(__VA_ARGS__)
#else
#  define NMEA_DEBUG(...) /* nothing */
#endif

typedef struct MSG_INFO_s
{
    char talker[3];     // = NMEA_MSG_t.talker
    char formatter[8];  // = NMEA_MSG_t.formatter
    int  payloadIx0;
    int  payloadIx1;

} MSG_INFO_t;

static bool sNmeaMessageInfo(MSG_INFO_t *info, const uint8_t *msg, const int msgSize)
{
    if ( (info == NULL) || (msg == NULL) || (msgSize < 11) )
    {
        return false;
    }

    // 012345678901234567890123456789
    // $GNGGA,...,...,...*xx\r\n
    // $PUBX,nn*xx,...,...,...\r\n

    // Talker ID
    int offs;
    if (msg[1] == 'P') // Proprietary
    {
        info->talker[0] = 'P';
        info->talker[1] = '\0';
        offs = 2;
    }
    else
    {
        info->talker[0] = msg[1];
        info->talker[1] = msg[2];
        info->talker[2] = '\0';
        offs = 3;
    }

    // Sentence formatter
    int commaIx = 0;
    for (int ix = offs; (ix < (msgSize - 5)) && (ix < ((int)sizeof(info->formatter) - 1 + offs)); ix++)
    {
        if (msg[ix] == ',')
        {
            commaIx = ix;
            break;
        }
    }
    if (commaIx <= 0)
    {
        info->formatter[0] = '?';
        info->formatter[1] = '?';
        return false;
    }
    info->payloadIx0 = commaIx + 1;

    int iIx;
    int oIx;
    for (iIx = offs, oIx = 0; iIx < commaIx; iIx++, oIx++)
    {
        info->formatter[oIx] = msg[iIx];
    }
    info->formatter[oIx] = '\0';

    // u-blox proprietary formatter, add "_XX"
    if ( (info->formatter[0] == 'U') && (info->formatter[1] == 'B') &&
         (info->formatter[2] == 'X') && (info->formatter[3] == '\0') )
    {
        iIx++;
        info->formatter[oIx++] = '_';
        info->formatter[oIx++] = msg[iIx++];
        info->formatter[oIx++] = msg[iIx++];
        info->formatter[oIx] = '\0';
        info->payloadIx0 += 3;
    }

    info->payloadIx1 = msgSize - 5 - 1;

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

bool nmeaMessageName(char *name, const int size, const uint8_t *msg, const int msgSize)
{
    MSG_INFO_t info;
    if (sNmeaMessageInfo(&info, msg, msgSize))
    {
        if ( (info.talker[0] == 'P') && (info.formatter[0] == 'U') && (info.formatter[1] == 'B') && (info.formatter[2] == 'X') )
        {
            const char *pubx = NULL;
            if ( (info.formatter[4] == '4') && (info.formatter[5] == '1') )
            {
                pubx = "CONFIG";
            }
            else if ( (info.formatter[4] == '4') && (info.formatter[5] == '1') )
            {
                pubx = "CONFIG";
            }
            else if ( (info.formatter[4] == '0') && (info.formatter[5] == '0') )
            {
                pubx = "POSITION";
            }
            else if ( (info.formatter[4] == '4') && (info.formatter[5] == '0') )
            {
                pubx = "RATE";
            }
            else if ( (info.formatter[4] == '0') && (info.formatter[5] == '3') )
            {
                pubx = "SVSTATUS";
            }
            else if ( (info.formatter[4] == '0') && (info.formatter[5] == '4') )
            {
                pubx = "TIME";
            }
            else
            {
                return snprintf(name, size, "NMEA-PUBX-%c%c", info.formatter[4], info.formatter[5]) < size;
            }
            return snprintf(name, size, "NMEA-PUBX-%s", pubx) < size;
        }
        else
        {
            return snprintf(name, size, "NMEA-%s-%s", info.talker, info.formatter) < size;
        }
    }
    else
    {
        return false;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool nmeaMessageInfo(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    if (info == NULL)
    {
        return false;
    }
    // "$GNGGA*xx\r\n", "$PUBX,nn*xx\r\n"
    if ( (msg == NULL) || (msgSize < 11) )
    {
        info[0] = '\0';
        return false;
    }

    // Find offset of start of payload (first comma after formatter)
    // 0123456789
    // $GNGGA,...,...,...*xx\r\n
    //  o     ^here
    // $PUBX,nn,...,...,...*xx\r\n
    //       o  ^here
    int startIx = (msg[1] == 'P') && (msg[2] == 'U') && (msg[3] == 'B') && (msg[4] == 'X') ? 6 : 1;
    int commaIx = 0;
    for (int ix = startIx; ix < (msgSize - 5); ix++)
    {
        if (msg[ix] == ',')
        {
            commaIx = ix;
            break;
        }
    }
    if (commaIx <= 0)
    {
        info[0] = '\0';
        return false;
    }

    // For TXT the info is the text, with type stringified
    if ( (msg[3] == 'T') && (msg[4] == 'X') && (msg[5] == 'T') )
    {
        char other[5];
        const char *prefix = "";
        // 01234567890123456789
        // $GPTXT,01,01,02,u-blox ag - www.u-blox.com*50\r\n
        switch (msg[14])
        {
            case '0': prefix = "ERROR: ";   break;
            case '1': prefix = "WARNING: "; break;
            case '2': prefix = "NOTICE: ";  break;
            default:
                other[0] = msg[13];
                other[1] = msg[14];
                other[2] = ':';
                other[3] = ' ';
                other[4] = '\0';
                prefix = other;
                break;
        }
        char fmt[20];
        snprintf(fmt, sizeof(fmt), "%%s%%.%ds", msgSize - 5 - 16);
        snprintf(info, size, fmt, prefix, (const char *)&msg[16]);
    }
    // Otherwise the info is the payload
    else
    {
        // $GNGGA,...,...,...*xx\r\n
        //        ^^^^^^^^^^^this
        // $PUBX,nn,...,...,...*xx\r\n
        //          ^^^^^^^^^^^this
        char fmt[20];
        snprintf(fmt, sizeof(fmt), "%%.%ds", msgSize - commaIx - 6);
        snprintf(info, size, fmt, (const char *)&msg[commaIx + 1]);
    }

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool sNmeaDecodeTxt(NMEA_TXT_t *txt, char *payload, const char *talker);
static bool sNmeaDecodeGga(NMEA_GGA_t *gga, char *payload, const char *talker);
static bool sNmeaDecodeRmc(NMEA_RMC_t *gga, char *payload, const char *talker);
static bool sNmeaDecodeGll(NMEA_GLL_t *gll, char *payload, const char *talker);
static bool sNmeaDecodeGsv(NMEA_GSV_t *gsv, char *payload, const char *talker);
static const char *sNmeaFixStr(const NMEA_FIX_t fix);

bool nmeaDecode(NMEA_MSG_t *nmea, const uint8_t *msg, const int msgSize)
{
    if ( (nmea == NULL)  || (msg == 0) )
    {
        return false;
    }
    memset(nmea, 0, sizeof(*nmea));

    MSG_INFO_t info;
    if (!sNmeaMessageInfo(&info, msg, msgSize))
    {
        return false;
    }
    memcpy(nmea->talker,    info.talker,    sizeof(nmea->talker));
    memcpy(nmea->formatter, info.formatter, sizeof(nmea->formatter));

    // 012345678901234567890
    // $GNGGA,.......*xx\r\n
    //        ^=7   ^=13  --> 13 - 7 + 1 = 7
    char payload[1000];
    const int payloadLen = info.payloadIx1 - info.payloadIx0 + 1;
    if (payloadLen > ((int)sizeof(payload) - 1))
    {
        return false;
    }
    memcpy(payload, &msg[info.payloadIx0], payloadLen);
    payload[payloadLen] = '\0';

    bool res = false;
    if (strcmp(info.formatter, "GGA") == 0)
    {
        nmea->type = NMEA_TYPE_GGA;
        res = sNmeaDecodeGga(&nmea->gga, payload, info.talker);
        snprintf(nmea->info, sizeof(nmea->info), "%02d:%02d:%06.3f (%d) %s %+11.7f %+12.7f %+5.0f",
            nmea->gga.time.hour, nmea->gga.time.minute, nmea->gga.time.second, nmea->gga.time.valid,
            sNmeaFixStr(nmea->gga.fix), nmea->gga.lat, nmea->gga.lon, nmea->gga.height);
    }
    else if (strcmp(info.formatter, "RMC") == 0)
    {
        nmea->type = NMEA_TYPE_RMC;
        res = sNmeaDecodeRmc(&nmea->rmc, payload, info.talker);
        snprintf(nmea->info, sizeof(nmea->info), "%04d-%02d-%02d (%d) %02d:%02d:%06.3f (%d) %s (%d) %+11.7f %+12.7f",
            nmea->rmc.date.year, nmea->rmc.date.month, nmea->rmc.date.day, nmea->rmc.date.valid,
            nmea->rmc.time.hour, nmea->rmc.time.minute, nmea->rmc.time.second, nmea->rmc.time.valid,
            sNmeaFixStr(nmea->rmc.fix), nmea->rmc.valid, nmea->rmc.lat, nmea->rmc.lon);
    }
    else if (strcmp(info.formatter, "GLL") == 0)
    {
        nmea->type = NMEA_TYPE_GLL;
        res = sNmeaDecodeGll(&nmea->gll, payload, info.talker);
        snprintf(nmea->info, sizeof(nmea->info), "%02d:%02d:%06.3f (%d) %s (%d) %+11.7f %+12.7f",
            nmea->gll.time.hour, nmea->gll.time.minute, nmea->gll.time.second, nmea->gll.time.valid,
            sNmeaFixStr(nmea->gll.fix), nmea->gll.valid, nmea->gll.lat, nmea->gll.lon);
    }
    else if (strcmp(info.formatter, "GSV") == 0)
    {
        nmea->type = NMEA_TYPE_GSV;
        res = sNmeaDecodeGsv(&nmea->gsv, payload, info.talker);
        snprintf(nmea->info, sizeof(nmea->info), "%d/%d %d",
            nmea->gsv.msgNum, nmea->gsv.numMsg, nmea->gsv.numSat);
    }
    else if (strcmp(info.formatter, "TXT") == 0)
    {
        nmea->type = NMEA_TYPE_TXT;
        res = sNmeaDecodeTxt(&nmea->txt, payload, info.talker);
        snprintf(nmea->info, sizeof(nmea->info), "%s", nmea->txt.text);
    }

    NMEA_DEBUG("decodeNmea: %d [%s] [%s] [%s]", res, nmea->talker, nmea->formatter, nmea->info);
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

static const char * const kNmeaFixStrs[] =
{
    [NMEA_FIX_UNKNOWN]   = "UNKNOWN",
    [NMEA_FIX_NOFIX]     = "NOFIX",
    [NMEA_FIX_DRONLY]    = "DRONLY",
    [NMEA_FIX_S2D]       = "S2D",
    [NMEA_FIX_S3D]       = "S3D",
    [NMEA_FIX_S3D_DR]    = "S3D_DR",
    [NMEA_FIX_RTK_FLOAT] = "RTK_FLOAT",
    [NMEA_FIX_RTK_FIXED] = "RTK_FIXED",
};

static const char *sNmeaFixStr(const NMEA_FIX_t fix)
{
    return (fix >= 0) && (fix < NUMOF(kNmeaFixStrs)) ? kNmeaFixStrs[fix] : kNmeaFixStrs[NMEA_FIX_UNKNOWN];
}

static int sGetFields(const char **fields, const int maxFields, char *payload)
{
    const char *sep = ",";
    int nTok = 0;
    int nFields = 0;
    for (char *tok = strsep(&payload, sep); tok != NULL; tok = strsep(&payload, sep))
    {
        nTok++;
        //NMEA_DEBUG("tok %d [%s]", nTok, tok);
        if (nFields < maxFields)
        {
            fields[nFields] = tok;
            nFields++;
        }
    }
    return nFields <= maxFields ? nFields : 0;
}

static bool sStrToTime(NMEA_TIME_t *time, const char *str)
{
    int len = 0;
    time->valid =
        (sscanf(str, "%2d%2d%lf%n", &time->hour, &time->minute, &time->second, &len) == 3) && (len == (int)strlen(str));
    // FIXME: validate data?
    NMEA_DEBUG("sStrToTime [%s] -> %d %d %.3f (%d)", str, time->hour, time->minute, time->second, time->valid);
    return time->valid;
}

static bool sStrToDate(NMEA_DATE_t *date, const char *str)
{
    int len = 0;
    date->valid =
        (sscanf(str, "%2d%2d%2d%n", &date->day, &date->month, &date->year, &len) == 3) && (len == (int)strlen(str));
    date->year += 2000; // probably... :-/
    // FIXME: validate data?
    NMEA_DEBUG("sStrToDate [%s] -> %d %d %d (%d)", str, date->day, date->month, date->year, date->valid);
    return date->valid;
}

static bool sStrToLatOrLon(double *latOrLon, const char *str)
{
    int len = 0;
    int deg = 0;
    double min = 0.0;
    if ( (sscanf(str, "%2d%lf%n", &deg, &min, &len) == 2) && (len == (int)strlen(str)) )
    {
        *latOrLon = (double)deg + (min * (1.0/60.0));
        NMEA_DEBUG("sStrToLatOrLon [%s] -> %d %g -> %g", str, deg, min, *latOrLon);
        return true;
    }
    else
    {
        return false;
    }
}

static bool sStrToFix(NMEA_FIX_t *fix, const char *str, const NMEA_TYPE_t type)
{
    // FIXME: very confusing and the interface description isn't terribly helpful... to check actual receiver behaviour
    bool res = true;
    switch (type)
    {
        case NMEA_TYPE_GGA:
            switch (str[0])
            {
                case '0': *fix = NMEA_FIX_NOFIX; break;
                case '1':
                case '2': *fix = NMEA_FIX_S3D; break;
                case '4': *fix = NMEA_FIX_RTK_FIXED; break;
                case '5': *fix = NMEA_FIX_RTK_FLOAT; break;
                case '6': *fix = NMEA_FIX_DRONLY; break;
                default:  *fix = NMEA_FIX_UNKNOWN; break;
            }
            break;
        case NMEA_TYPE_RMC:
        case NMEA_TYPE_GLL:
            switch (str[0])
            {
                case 'N': *fix = NMEA_FIX_NOFIX; break;
                case 'A':
                case 'D': *fix = NMEA_FIX_S3D; break;
                case 'R': *fix = NMEA_FIX_RTK_FIXED; break;
                case 'F': *fix = NMEA_FIX_RTK_FLOAT; break;
                case 'E': *fix = NMEA_FIX_DRONLY; break;
                default:  *fix = NMEA_FIX_UNKNOWN; break;
            }
            break;
        case NMEA_TYPE_TXT:
        case NMEA_TYPE_GSV:
        case NMEA_TYPE_NONE:
            res = false;
            break;
    }
    NMEA_DEBUG("sStrToFix [%s] -> %d", str, *fix);
    return res;
}

static bool sStrToInt(int *val, const char *str, const bool checkLo, const int lo, const bool checkHi, const int hi)
{
    int len = 0;
    bool res = (sscanf(str, "%d%n", val, &len) == 1) && (len == (int)strlen(str)) &&
         (!checkLo || (*val >= lo)) && (!checkHi || (*val <= hi));
    NMEA_DEBUG("sStrToInt [%s] -> %d (%d, %d:%d - %d:%d)", str, *val, res, checkLo, lo, checkHi, hi);
    return res;
}

static bool sStrToDbl(double *val, const char *str, const bool checkLo, const double lo, const bool checkHi, const double hi)
{
    int len = 0;
    bool res = (sscanf(str, "%lf%n", val, &len) == 1) && (len == (int)strlen(str)) &&
         (!checkLo || (*val >= lo)) && (!checkHi || (*val <= hi));
    NMEA_DEBUG("sStrToDbl [%s] -> %g (%d, %d:%g - %d:%g)", str, *val, res, checkLo, lo, checkHi, hi);
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool sNmeaDecodeGga(NMEA_GGA_t *gga, char *payload, const char *talker)
{
    NMEA_DEBUG("sNmeaDecodeGga [%s] [%s]", talker, payload);
    UNUSED(talker);

    const char *fields[30];
    const int nFields = sGetFields(fields, NUMOF(fields), payload);
    if (nFields != 14)
    {
        return false;
    }

    bool res = true;

    if (fields[0][0] != '\0')
    {
        res = sStrToTime(&gga->time, fields[0]);
    }

    if ( (fields[1][0] != '\0') && (fields[3][0] != '\0') && (fields[5][0] != '\0') )
    {
        if (!sStrToLatOrLon( &gga->lat, fields[1]) ||
            !sStrToLatOrLon( &gga->lon, fields[3]) ||
            !sStrToFix(      &gga->fix, fields[5], NMEA_TYPE_GGA))
        {
            res = false;
        }
        if (fields[2][0] == 'S')
        {
            gga->lat *= -1.0;
        }
        if (fields[4][0] == 'W')
        {
            gga->lon *= -1.0;
        }
    }
    else
    {
        gga->fix = NMEA_FIX_NOFIX;
    }

    if ( (fields[6][0] != '\0') && !sStrToInt(&gga->numSv, fields[6], true, 0, false, 0) )
    {
        res = false;
    }

    if ( (fields[7][0] != '\0') && !sStrToDbl(&gga->hDOP, fields[7], true, 0.0, false, 0.0) )
    {
        res = false;
    }

    if ( (fields[8][0] != '\0') && !sStrToDbl(&gga->height, fields[8], false, 0.0, false, 0.0) )
    {
        res = false;
    }

    if (fields[10][0] != '\0')
    {
        double sep = 0.0;
        if (!sStrToDbl(&sep, fields[10], false, 0.0, false, 0.0))
        {
            res = false;
        }
        gga->heightMsl = gga->height - sep;
    }

    if (fields[12][0] != '\0')
    {
        if (!sStrToDbl(&gga->diffAge, fields[12], true, 0, false, 0))
        {
            res = false;
        }
    }
    else
    {
        gga->diffAge = -1.0;
    }

    if (fields[13][0] != '\0')
    {
        if (!sStrToInt(&gga->diffStation, fields[13], true, 0, false, 0))
        {
            WARNING("ouch2");
            res = false;
        }
    }
    else
    {
        gga->diffStation = -1;
    }

    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool sNmeaDecodeRmc(NMEA_RMC_t *rmc, char *payload, const char *talker)
{
    NMEA_DEBUG("sNmeaDecodeRmc [%s] [%s]", talker, payload);
    UNUSED(talker);

    const char *fields[30];
    const int nFields = sGetFields(fields, NUMOF(fields), payload);
    if (nFields < 13)
    {
        return false;
    }

    bool res = true;

    if (fields[0][0] != '\0')
    {
        res = sStrToTime(&rmc->time, fields[0]);
    }
    if (fields[8][0] != '\0')
    {
        res = sStrToDate(&rmc->date, fields[8]);
    }

    if ( (fields[2][0] != '\0') && (fields[4][0] != '\0') && (fields[11][0] != '\0') )
    {
        if (!sStrToLatOrLon( &rmc->lat, fields[2]) ||
            !sStrToLatOrLon( &rmc->lon, fields[4]) ||
            !sStrToFix(      &rmc->fix, fields[11], NMEA_TYPE_RMC))
        {
            res = false;
        }
        if (fields[3][0] != 'N')
        {
            rmc->lat *= -1.0;
        }
        if (fields[5][0] != 'E')
        {
            rmc->lon *= -1.0;
        }
        if ( (nFields > 13) && (fields[12][0] != 'V') )
        {
            rmc->fix = NMEA_FIX_NOFIX; // FIXME: or what does != 'V' mean?
        }
    }
    else
    {
        rmc->fix = NMEA_FIX_NOFIX;
    }

    rmc->valid = (fields[1][0] == 'A');

    if (fields[6][0] != '\0')
    {
        if (!sStrToDbl(&rmc->spd, fields[6], false, 0.0, false, 0.0))
        {
            res = false;
        }
    }

    if (fields[7][0] != '\0')
    {
        if (!sStrToDbl(&rmc->cog, fields[7], true, 0.0, true, 360.0))
        {
            res = false;
        }
    }

    if (fields[9][0] != '\0')
    {
        if (!sStrToDbl(&rmc->mv, fields[9], true, -180.0, true, 180.0))
        {
            res = false;
        }
    }
    if (fields[10][0] == 'W')
    {
        rmc->mv *= -1.0;
    }

    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool sNmeaDecodeTxt(NMEA_TXT_t *txt, char *payload, const char *talker)
{
    UNUSED(talker);
    NMEA_DEBUG("sNmeaDecodeTxt [%s] [%s]", talker, payload);

    const char *fields[5];
    const int nFields = sGetFields(fields, NUMOF(fields), payload);
    if ((nFields != 4) ||
        !sStrToInt(&txt->numMsg,  fields[0], true, 0, false, 0) ||
        !sStrToInt(&txt->msgNum,  fields[1], true, 0, false, 0) ||
        !sStrToInt(&txt->msgType, fields[2], true, 0, false, 0))
    {
        return false;
    }
    snprintf(txt->text, sizeof(txt->text), "%s", fields[3]);
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool sNmeaDecodeGll(NMEA_GLL_t *gll, char *payload, const char *talker)
{
    UNUSED(talker);
    NMEA_DEBUG("sNmeaDecodeGll [%s] [%s]", talker, payload);

    const char *fields[15];
    const int nFields = sGetFields(fields, NUMOF(fields), payload);
    if (nFields != 7)
    {
        return false;
    }

    bool res = true;

    if (fields[4][0] != '\0')
    {
        res = sStrToTime(&gll->time, fields[4]);
    }

    if ( (fields[0][0] != '\0') && (fields[2][0] != '\0') && (fields[6][0] != '\0') )
    {
        if (!sStrToLatOrLon( &gll->lat, fields[0]) ||
            !sStrToLatOrLon( &gll->lon, fields[2]) ||
            !sStrToFix(      &gll->fix, fields[6], NMEA_TYPE_RMC))
        {
            res = false;
        }
        if (fields[1][0] != 'N')
        {
            gll->lat *= -1.0;
        }
        if (fields[3][0] != 'E')
        {
            gll->lon *= -1.0;
        }
    }
    else
    {
        gll->fix = NMEA_FIX_NOFIX;
    }

    gll->valid = (fields[5][0] == 'A');

    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool sNmeaDecodeGsv(NMEA_GSV_t *gsv, char *payload, const char *talker)
{
    NMEA_DEBUG("sNmeaDecodeGsv [%s] [%s]", talker, payload);

    const char *fields[30];
    const int nFields = sGetFields(fields, NUMOF(fields), payload);
    if (nFields < 3)
    {
        return false;
    }
    if (!sStrToInt(&gsv->numMsg, fields[0], true, 1, false, 0) ||
        !sStrToInt(&gsv->msgNum, fields[1], true, 1, true, gsv->numMsg) ||
        !sStrToInt(&gsv->numSat, fields[2], true, 0, false, 0) )
    {
        return false;
    }
    const int nSat = (nFields - 3) / 4;
    const int remFields = nFields - (nSat * 4) - 3;
    int nmeaSig = 0;
    if (remFields > 0)
    {
        if (!sStrToInt(&nmeaSig, fields[3 + (nSat * 4)], false, 0, false, 0))
        {
            return false;
        }
    }
    NMEA_DEBUG("nFields=%d/%d nSat=%d remFields=%d nmeaSig=%d", nFields, NUMOF(fields), nSat, remFields, nmeaSig);

    NMEA_GNSS_t   gnss = NMEA_GNSS_UNKNOWN;
    NMEA_SIGNAL_t sig  = NMEA_SIGNAL_UNKNOWN;
    if (talker[0] == 'G')
    {
        switch (talker[1])
        {
            case 'P':
                gnss = NMEA_GNSS_GPS;  // or SBAS
                switch (nmeaSig)
                {
                    case 1: sig = NMEA_SIGNAL_GPS_L1CA; break;
                    case 5:
                    case 6: sig = NMEA_SIGNAL_GPS_L2C; break;
                }
                break;
            case 'L':
                gnss = NMEA_GNSS_GLO;
                switch (nmeaSig)
                {
                    case 1: sig = NMEA_SIGNAL_GLO_L1OF; break;
                    case 3: sig = NMEA_SIGNAL_GLO_L2OF; break;
                }
                break;
            case 'A':
                gnss = NMEA_GNSS_GAL;
                switch (nmeaSig)
                {
                    case 7: sig = NMEA_SIGNAL_GAL_E1; break;
                    case 2: sig = NMEA_SIGNAL_GAL_E5B; break;
                }
                break;
            case 'B':
                gnss = NMEA_GNSS_BDS;
                switch (nmeaSig)
                {
                    case  1: sig = NMEA_SIGNAL_BDS_B1I; break;
                    case 11: sig = NMEA_SIGNAL_BDS_B2I; break;
                }
                break;
            case 'Q':
                gnss = NMEA_GNSS_QZSS;
                switch (nmeaSig)
                {
                    case 1: sig = NMEA_SIGNAL_QZSS_L1CA; break;
                    case 4: sig = NMEA_SIGNAL_QZSS_L1S; break;
                    case 5:
                    case 6: sig = NMEA_SIGNAL_QZSS_L2C; break;
                }
                break;
        }
    }

    for (int satIx = 0; (satIx < nSat) && (satIx < (int)NUMOF(gsv->svs)); satIx++)
    {
        const int offs = 3 + (satIx * 4);
        if (!sStrToInt(&gsv->svs[satIx].svId, fields[offs    ], true, 1, false, 0) ||
            !sStrToInt(&gsv->svs[satIx].elev, fields[offs + 1], true, -90, true, 90) ||
            !sStrToInt(&gsv->svs[satIx].azim, fields[offs + 2], true, 0, false, 360) ||
            !sStrToInt(&gsv->svs[satIx].cno,  fields[offs + 3], true, 0, false, 0) )
        {
            return false;
        }
        gsv->svs[satIx].gnss = gnss;
        gsv->svs[satIx].sig = sig;
        if ( (gnss == NMEA_GNSS_GPS) && (gsv->svs[satIx].svId > 32) )
        {
            gsv->svs[satIx].gnss = NMEA_GNSS_SBAS;
            gsv->svs[satIx].sig = (sig == NMEA_SIGNAL_GPS_L1CA ? NMEA_SIGNAL_SBAS_L1CA : NMEA_SIGNAL_UNKNOWN);
            // S120-S158 (39) = 33-64 (32, S120-S151) and 152-158 (7, S152-158)
            if  ((gsv->svs[satIx].svId >= 33) && (gsv->svs[satIx].svId <= 64) )
            {
                gsv->svs[satIx].svId += 87;
            }
        }
        NMEA_DEBUG("satIx=%d svId=%d elev=%d azim=%d cno=%d gnss=%d sig=%d",
            satIx, gsv->svs[satIx].svId, gsv->svs[satIx].elev, gsv->svs[satIx].azim, gsv->svs[satIx].cno, gsv->svs[satIx].gnss, gsv->svs[satIx].sig);
        gsv->nSvs++;
    }

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

bool nmeaMessageClsId(const char *name, uint8_t *clsId, uint8_t *msgId)
{
    if ( (name == NULL) || (name[0] == '\0') )
    {
        return false;
    }

    const struct { const char *name; uint8_t clsId; uint8_t msgId; } kMsgInfo[] =
    {
        { .name = "NMEA-STANDARD-DTM",  .clsId = 0xf0, .msgId = 0x0a },
        { .name = "NMEA-STANDARD-GAQ",  .clsId = 0xf0, .msgId = 0x45 },
        { .name = "NMEA-STANDARD-GBQ",  .clsId = 0xf0, .msgId = 0x44 },
        { .name = "NMEA-STANDARD-GBS",  .clsId = 0xf0, .msgId = 0x09 },
        { .name = "NMEA-STANDARD-GGA",  .clsId = 0xf0, .msgId = 0x00 },
        { .name = "NMEA-STANDARD-GLL",  .clsId = 0xf0, .msgId = 0x01 },
        { .name = "NMEA-STANDARD-GLQ",  .clsId = 0xf0, .msgId = 0x43 },
        { .name = "NMEA-STANDARD-GNQ",  .clsId = 0xf0, .msgId = 0x42 },
        { .name = "NMEA-STANDARD-GNS",  .clsId = 0xf0, .msgId = 0x0d },
        { .name = "NMEA-STANDARD-GPQ",  .clsId = 0xf0, .msgId = 0x40 },
        { .name = "NMEA-STANDARD-GQQ",  .clsId = 0xf0, .msgId = 0x47 },
        { .name = "NMEA-STANDARD-GRS",  .clsId = 0xf0, .msgId = 0x06 },
        { .name = "NMEA-STANDARD-GSA",  .clsId = 0xf0, .msgId = 0x02 },
        { .name = "NMEA-STANDARD-GST",  .clsId = 0xf0, .msgId = 0x07 },
        { .name = "NMEA-STANDARD-GSV",  .clsId = 0xf0, .msgId = 0x03 },
        { .name = "NMEA-STANDARD-RLM",  .clsId = 0xf0, .msgId = 0x0b },
        { .name = "NMEA-STANDARD-RMC",  .clsId = 0xf0, .msgId = 0x04 },
        { .name = "NMEA-STANDARD-TXT",  .clsId = 0xf0, .msgId = 0x41 },
        { .name = "NMEA-STANDARD-VLW",  .clsId = 0xf0, .msgId = 0x0f },
        { .name = "NMEA-STANDARD-VTG",  .clsId = 0xf0, .msgId = 0x05 },
        { .name = "NMEA-STANDARD-ZDA",  .clsId = 0xf0, .msgId = 0x08 },
        { .name = "NMEA-PUBX-CONFIG",   .clsId = 0xf1, .msgId = 0x41 },
        { .name = "NMEA-PUBX-POSITION", .clsId = 0xf1, .msgId = 0x00 },
        { .name = "NMEA-PUBX-RATE",     .clsId = 0xf1, .msgId = 0x40 },
        { .name = "NMEA-PUBX-SVSTATUS", .clsId = 0xf1, .msgId = 0x03 },
        { .name = "NMEA-PUBX-TIME",     .clsId = 0xf1, .msgId = 0x04 },
    };
    for (int ix = 0; ix < NUMOF(kMsgInfo); ix++)
    {
        if (strcmp(kMsgInfo[ix].name, name) == 0)
        {
            if (clsId != NULL)
            {
                *clsId = kMsgInfo[ix].clsId;
            }
            if (msgId != NULL)
            {
                *msgId = kMsgInfo[ix].msgId;
            }
            return true;
        }
    }

    return false;
}

/* ****************************************************************************************************************** */
// eof
