// flipflip's UBX protocol stuff
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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "ff_stuff.h"
#include "ff_ubx.h"
#include "ff_debug.h"

/* ********************************************************************************************** */

int ubxMakeMessage(const uint8_t clsId, const uint8_t msgId, const uint8_t *payload, const uint16_t payloadSize, uint8_t *msg)
{
    if ( (payload != NULL) && (payloadSize > 0) )
    {
        memmove(&msg[6], payload, payloadSize);
    }
    const int msgSize = payloadSize + UBX_FRAME_SIZE;
    msg[0] = UBX_SYNC_1;
    msg[1] = UBX_SYNC_2;
    msg[2] = clsId;
    msg[3] = msgId;
    msg[4] = (payloadSize & 0xff);
    msg[5] = (payloadSize >> 8);
    uint8_t a = 0;
    uint8_t b = 0;
    uint8_t *pMsg = &msg[2];
    int cnt = msgSize - 4;
    while (cnt > 0)
    {
        a += *pMsg;
        b += a;
        pMsg++;
        cnt--;
    }
    *pMsg = a; pMsg++;
    *pMsg = b;
    return payloadSize + UBX_FRAME_SIZE;
}

/* ********************************************************************************************** */

// It seems that when using transactions, the last message (with UBX-CFG-VALSET.transaction = 3, i.e. end/commit)
// must not contain any key-value pairs or those key-value pairs in the last message are ignored and not applied.
#define NEED_EMPTY_TRANSACTION_END

#define UBX_CFG_VALSET_MSG_MAX 20

UBX_CFG_VALSET_MSG_t *ubxKeyValToUbxCfgValset(const UBLOXCFG_KEYVAL_t *kv, const int nKv, const bool ram, const bool bbr, const bool flash, int *nValset)
{
    if (nValset == NULL)
    {
        return NULL;
    }

    const int nMsgs = (nKv / (UBX_CFG_VALSET_V1_MAX_KV)) + ((nKv % UBX_CFG_VALSET_V1_MAX_KV) != 0 ? 1 : 0) ;
#ifdef NEED_EMPTY_TRANSACTION_END
    if (nMsgs > (UBX_CFG_VALSET_MSG_MAX - 1))
#else
    if (nMsgs > UBX_CFG_VALSET_MSG_MAX)
#endif
    {
        return NULL;
    }

    const uint8_t layers = (ram   ? UBX_CFG_VALSET_V1_LAYER_RAM   : 0x00) |
                           (bbr   ? UBX_CFG_VALSET_V1_LAYER_BBR   : 0x00) |
                           (flash ? UBX_CFG_VALSET_V1_LAYER_FLASH : 0x00);
    if (layers == 0x00)
    {
        return NULL;
    }
    char layersStr[100];
    layersStr[0] = '\0';
    if (ram)
    {
        strcat(&layersStr[strlen(layersStr)], ",RAM");
    }
    if (bbr)
    {
        strcat(&layersStr[strlen(layersStr)], ",BBR");
    }
    if (flash)
    {
        strcat(&layersStr[strlen(layersStr)], ",Flash");
    }

    const int msgsSize = (UBX_CFG_VALSET_MSG_MAX + 1) * sizeof(UBX_CFG_VALSET_MSG_t);
    UBX_CFG_VALSET_MSG_t *msgs = malloc(msgsSize);
    if (msgs == NULL)
    {
        WARNING("ubxKeyValToUbxCfgValset() malloc fail");
        return NULL;
    }
    memset(msgs, 0, msgsSize);

    // Create as many UBX-CFG-VALSET as required
    bool res = true;
    for (int msgIx = 0; msgIx < nMsgs; msgIx++)
    {
        const int kvOffs = msgIx * UBX_CFG_VALSET_V1_MAX_KV;
        const int remKv = nKv - kvOffs;
        const int nKvThisMsg = remKv > UBX_CFG_VALSET_V1_MAX_KV ? UBX_CFG_VALSET_V1_MAX_KV : remKv;
        //DEBUG("msgIx=%d nMsgs=%d nKv=%d kvOffs=%d remKv=%d nKvThisMsg=%d", msgIx, nMsgs, nKv, kvOffs, remKv, nKvThisMsg);

        // Message header
        uint8_t transaction = UBX_CFG_VALSET_V1_TRANSACTION_NONE;
        const char *transactionStr = "no transaction";
        if (nMsgs > 1)
        {
            if (msgIx == 0)
            {
                transaction = UBX_CFG_VALSET_V1_TRANSACTION_BEGIN;
                transactionStr = "transaction begin";
            }
            else
#ifndef NEED_EMPTY_TRANSACTION_END
                  if (msgIx < (nMsgs - 1))
#endif
            {
                transaction = UBX_CFG_VALSET_V1_TRANSACTION_CONTINUE;
                transactionStr = "transaction continue";
            }
#ifndef NEED_EMPTY_TRANSACTION_END
            else
            {
                transaction = UBX_CFG_VALSET_V1_TRANSACTION_END;
                transactionStr = "transaction end";
            }
#endif
        }
        DEBUG("Creating UBX-CFG-VALSET %d items (%d..%d/%d, %s)",
            nKvThisMsg, kvOffs + 1, kvOffs + nKvThisMsg, nKv, transactionStr);

        uint8_t *pUbxData = msgs[msgIx].msg;

        // Add payload head
        const UBX_CFG_VALSET_V1_GROUP0_t payloadHead =
        {
            .version     = UBX_CFG_VALSET_V1_VERSION,
            .layers      = layers,
            .transaction = transaction,
            .reserved    = UBX_CFG_VALSET_V1_RESERVED
        };
        const int payloadHeadSize = sizeof(payloadHead);
        memcpy(pUbxData, &payloadHead, payloadHeadSize);

        // Add config data
        int cfgDataSize = 0;
        if (!ubloxcfg_makeData(&pUbxData[payloadHeadSize], UBX_CFG_VALGET_V1_MAX_SIZE,
                &kv[kvOffs], nKvThisMsg, &cfgDataSize))
        {
            WARNING("UBX-CFG-VALSET.cfgData encode fail!");
            res = false;
            break;
        }

        // Make UBX message
        const int msgSize = ubxMakeMessage(UBX_CFG_CLSID, UBX_CFG_VALSET_MSGID,
            pUbxData, payloadHeadSize + cfgDataSize, pUbxData);

        msgs[msgIx].size = msgSize;

        snprintf(msgs[msgIx].info, sizeof(msgs[msgIx].info), "%d items: %d..%d/%d, %d bytes, %s, %s",
            nKvThisMsg, kvOffs + 1, kvOffs + nKvThisMsg, nKv, msgSize, &layersStr[1], transactionStr);
    }

    if (!res)
    {
        free(msgs);
        return NULL;
    }

#ifdef NEED_EMPTY_TRANSACTION_END
    // Add empty transaction-complete message
    if (nMsgs > 1)
    {
        const UBX_CFG_VALSET_V1_GROUP0_t payload =
        {
            .version     = UBX_CFG_VALSET_V1_VERSION,
            .layers      = layers,
            .transaction = UBX_CFG_VALSET_V1_TRANSACTION_END,
            .reserved    = UBX_CFG_VALSET_V1_RESERVED
        };
        msgs[nMsgs].size = ubxMakeMessage(UBX_CFG_CLSID, UBX_CFG_VALSET_MSGID,
            (const uint8_t *)&payload, sizeof(payload), msgs[nMsgs].msg);
        snprintf(msgs[nMsgs].info, sizeof(msgs[nMsgs].info), "no items, %d bytes, %s, transaction end",
            msgs[nMsgs].size, &layersStr[1]);
        *nValset = nMsgs + 1;
    }
    else
#endif
    {
        *nValset = nMsgs;
    }
    return msgs;
}

/* ********************************************************************************************** */

static bool _ubxMessageName(char *name, const int size, const uint8_t clsId, const uint8_t msgId);

bool ubxMessageName(char *name, const int size, const uint8_t *msg, const int msgSize)
{
    if ( (name == NULL) || (size < 1) )
    {
        return false;
    }
    if ( (msgSize < (UBX_HEAD_SIZE)) || (msg == NULL) )
    {
        name[0] = '\0';
        return false;
    }
    const uint8_t clsId = UBX_CLSID(msg);
    const uint8_t msgId = UBX_MSGID(msg);
    return _ubxMessageName(name, size, clsId, msgId);
}

bool ubxMessageNameIds(char *name, const int size, const uint8_t clsId, const uint8_t msgId)
{
    if ( (name == NULL) || (size < 1) )
    {
        return false;
    }
    return _ubxMessageName(name, size, clsId, msgId);
}

typedef struct MSGINFO_s
{
    uint8_t     clsId;
    uint8_t     msgId;
    const char *msgName;
} MSGINFO_t;

#define _P_MSGINFO(_clsId_, _msgId_, _msgName_) { .clsId = (_clsId_), .msgId = (_msgId_), .msgName = (_msgName_) },
#define _P_MSGINFO_UNKN(_clsId_, _clsName_) { .clsId = (_clsId_), .msgName = (_clsName_) },

static bool _ubxMessageName(char *name, const int size, const uint8_t clsId, const uint8_t msgId)
{

    const MSGINFO_t msgInfo[] =
    {
        UBX_MESSAGES(_P_MSGINFO)
    };
    const MSGINFO_t unknInfo[] =
    {
        UBX_CLASSES(_P_MSGINFO_UNKN)
    };
    int res = 0;
    for (int ix = 0; ix < NUMOF(msgInfo); ix++)
    {
        if ( (msgInfo[ix].clsId == clsId) && (msgInfo[ix].msgId == msgId) )
        {
            res = snprintf(name, size, "%s", msgInfo[ix].msgName);
            break;
        }
    }
    if (res == 0)
    {
        for (int ix = 0; ix < NUMOF(unknInfo); ix++)
        {
            if (unknInfo[ix].clsId == clsId)
            {
                res = snprintf(name, size, "%s-%02"PRIX8, unknInfo[ix].msgName, msgId);
                break;
            }
        }
    }
    if (res == 0)
    {
        res = snprintf(name, size, "UBX-%02"PRIX8"-%02"PRIX8, clsId, msgId);
    }

    return res < size;
}

/* ********************************************************************************************** */

const char *ubxGnssStr(const uint8_t gnssId)
{
    switch (gnssId)
    {
        case UBX_GNSSID_GPS:
            return "GPS";
        case UBX_GNSSID_SBAS:
            return "SBAS";
        case UBX_GNSSID_GAL:
            return "GAL";
        case UBX_GNSSID_BDS:
            return "BDS";
        case UBX_GNSSID_QZSS:
            return "QZSS";
        case UBX_GNSSID_GLO:
            return "GLO";
    }
    return "?";    
}

const char *ubxSvStr(const uint8_t gnssId, const uint8_t svId)
{
    switch (gnssId)
    {
        case UBX_GNSSID_GPS:
        {
            const char * const strs[] =
            {
                "G01", "G02", "G03", "G04", "G05", "G06", "G07", "G08", "G09", "G10",
                "G11", "G12", "G13", "G14", "G15", "G16", "G17", "G18", "G19", "G20",
                "G21", "G22", "G23", "G24", "G25", "G26", "G27", "G28", "G29", "G30",
                "G31", "G32"
            };
            const int ix = svId - UBX_FIRST_GPS;
            if ( (ix >= 0) && (ix < NUMOF(strs)) )
            {
                return strs[ix];
            }
            else
            {
                return "G?";
            }
        }
        case UBX_GNSSID_SBAS:
        {
            const char * const strs[] =
            {
                "S120", "S121", "S122", "S123", "S123", "S124", "S126", "S127", "S128", "S129"
                "S130", "S131", "S132", "S133", "S133", "S134", "S136", "S137", "S138", "S139"
                "S140", "S141", "S142", "S143", "S143", "S144", "S146", "S147", "S148", "S149"
                "S150", "S151", "S152", "S153", "S153", "S154", "S156", "S157", "S158"
            };
            const int ix = svId - UBX_FIRST_SBAS;
            if ( (ix >= 0) && (ix < NUMOF(strs)) )
            {
                return strs[ix];
            }
            else
            {
                return "G?";
            }
        }
        case UBX_GNSSID_GAL:
        {
            const char * const strs[] =
            {
                "E01", "E02", "E03", "E04", "E05", "E06", "E07", "E08", "E09", "E10",
                "E11", "E12", "E13", "E14", "E15", "E16", "E17", "E18", "E19", "E20",
                "E21", "E22", "E23", "E24", "E25", "E26", "E27", "E28", "E29", "E30",
                "E31", "E32", "E33", "E34", "E35", "E36"
            };
            const int ix = svId - UBX_FIRST_GAL;
            if ( (ix >= 0) && (ix < NUMOF(strs)) )
            {
                return strs[ix];
            }
            else
            {
                return "E?";
            }
        }
        case UBX_GNSSID_BDS:
        {
            const char * const strs[] =
            {
                "B01", "B02", "B03", "B04", "B05", "B06", "B07", "B08", "B09", "B10",
                "B11", "B12", "B13", "B14", "B15", "B16", "B17", "B18", "B19", "B20",
                "B21", "B22", "B23", "B24", "B25", "B26", "B27", "B28", "B29", "B30",
                "B31", "B32"
            };
            const int ix = svId - UBX_FIRST_BDS;
            if ( (ix >= 0) && (ix < NUMOF(strs)) )
            {
                return strs[ix];
            }
            else
            {
                return "B?";
            }
        }
        case UBX_GNSSID_QZSS:
        {
            const char * const strs[] =
            {
                "Q01", "Q02", "Q03", "Q04", "Q05", "Q06", "Q07", "Q08", "Q09", "Q10",
                "Q11", "Q12", "Q13", "Q14", "Q15", "Q16", "Q17", "Q18", "Q19", "Q20",
                "Q21", "Q22", "Q23", "Q24", "Q25", "Q26", "Q27", "Q28", "Q29", "Q30",
                "Q31", "Q32"
            };
            const int ix = svId - UBX_FIRST_QZSS;
            if ( (ix >= 0) && (ix < NUMOF(strs)) )
            {
                return strs[ix];
            }
            else
            {
                return "Q?";
            }
        }
        case UBX_GNSSID_GLO:
        {
            const char * const strs[] =
            {
                "R01", "R02", "R03", "R04", "R05", "R06", "R07", "R08", "R09", "R10",
                "R11", "R12", "R13", "R14", "R15", "R16", "R17", "R18", "R19", "R20",
                "R21", "R22", "R23", "R24", "R25", "R26", "R27", "R28", "R29", "R30",
                "R31", "R32"
            };
            const int ix = svId - UBX_FIRST_GLO;
            if ( (ix >= 0) && (ix < NUMOF(strs)) )
            {
                return strs[ix];
            }
            else
            {
                return "R?";
            }
        }
    }
    return "?";
}

const char *ubxSigStr(const uint8_t gnssId, const uint8_t sigId)
{
    switch (gnssId)
    {
        case UBX_GNSSID_GPS:
            switch (sigId)
            {
                case UBX_SIGID_GPS_L1CA:
                    return "L1CA";
                case UBX_SIGID_GPS_L2CL:
                    return "L2CL";
                case UBX_SIGID_GPS_L2CM:
                    return "L2CM";
            }
            break;
        case UBX_GNSSID_SBAS:
            switch (sigId)
            {
                case UBX_SIGID_SBAS_L1CA:
                    return "L1CA";
            }
            break;
        case UBX_GNSSID_GAL:
            switch (sigId)
            {
                case UBX_SIGID_GAL_E1C:
                    return "E1C";
                case UBX_SIGID_GAL_E1B:
                    return "E1B";
                case UBX_SIGID_GAL_E5BI:
                    return "E5bI";
                case UBX_SIGID_GAL_E5BQ:
                    return "E5bQ";
            }
            break;
        case UBX_GNSSID_BDS:
            switch (sigId)
            {
                case UBX_SIGID_BDS_B1ID1:
                    return "B1ID1";
                case UBX_SIGID_BDS_B1ID2:
                    return "B1ID2";
                case UBX_SIGID_BDS_B2ID1:
                    return "B2ID1";
                case UBX_SIGID_BDS_B2ID2:
                    return "B2ID2";
            }
            break;
        case UBX_GNSSID_QZSS:
            switch (sigId)
            {
                case UBX_SIGID_QZSS_L1CA:
                    return "L1CA";
                case UBX_SIGID_QZSS_L1S:
                    return "L1S";
                case UBX_SIGID_QZSS_L2CM:
                    return "L2CM";
                case UBX_SIGID_QZSS_L2CL:
                    return "L2CL";
            }
            break;
        case UBX_GNSSID_GLO:
            switch (sigId)
            {
                case UBX_SIGID_GLO_L1OF:
                    return "L1OF";
                case UBX_SIGID_GLO_L2OF:
                    return "L2OF";
            }
            break;
    }
    return "?";    
}

/* ********************************************************************************************** */

static int _ubxMonVerStr(char *str, const int size, const uint8_t *msg, const int msgSize)
{
    if ( (msg == NULL) || (str == NULL) || (size < 2) ||
         (UBX_CLSID(msg) != UBX_MON_CLSID) ||
         (UBX_MSGID(msg) != UBX_MON_VER_MSGID) ||
         (msgSize < (int)(UBX_FRAME_SIZE + sizeof(UBX_MON_VER_V0_GROUP0_t))) )
    {
        return false;
    }

    // swVersion: EXT CORE 1.00 (61ce84)
    // hwVersion: 00190000
    // extension: ROM BASE 0xDD3FE36C
    // extension: FWVER=HPG 1.00
    // extension: PROTVER=27.00
    // extension: MOD=ZED-F9P
    // extension: GPS;GLO;GAL;BDS
    // extension: QZSS

    UBX_MON_VER_V0_GROUP0_t gr0;
    int offs = UBX_HEAD_SIZE;
    memcpy(&gr0, &msg[offs], sizeof(gr0));
    gr0.swVersion[sizeof(gr0.swVersion) - 1] = '\0';

    char verStr[sizeof(gr0.swVersion)];
    verStr[0] = '\0';
    char modStr[sizeof(gr0.swVersion)];
    modStr[0] = '\0';

    offs += sizeof(gr0);
    UBX_MON_VER_V0_GROUP1_t gr1;
    while (offs <= (msgSize - 2 - (int)sizeof(gr1)))
    {
        memcpy(&gr1, &msg[offs], sizeof(gr1));
        if ( (verStr[0] == '\0') && (strncmp("FWVER=", gr1.extension, 6) == 0) )
        {
            gr1.extension[sizeof(gr1.extension) - 1] = '\0';
            strcat(verStr, &gr1.extension[6]);
        }
        else if ( (modStr[0] == '\0') && (strncmp("MOD=", gr1.extension, 4) == 0) )
        {
            gr1.extension[sizeof(gr1.extension) - 1] = '\0';
            strcat(modStr, &gr1.extension[4]);
        }
        offs += sizeof(gr1);
        if ( (verStr[0] != '\0') && (modStr[0] != '\0') )
        {
            break;
        }
    }
    if (verStr[0] == '\0')
    {
        strcat(verStr, gr0.swVersion);
    }

    int len = 0;
    if (modStr[0] != '\0')
    {
        len = snprintf(str, size, "%s (%s)", verStr, modStr);
    }
    else
    {
        len = snprintf(str, size, "%s", verStr);
    }
    return len;
}

bool ubxMonVerToVerStr(char *str, const int size, const uint8_t *msg, const int msgSize)
{
    const int len = _ubxMonVerStr(str, size, msg, msgSize);
    return (len > 0) && (len < size);
}

/* ********************************************************************************************** */

static int _strUbxNav(char *info, const int size, const uint8_t *msg, const int msgSize, const int iTowOffs);
static int _strUbxNavPvt(char *info, const int size, const uint8_t *msg, const int msgSize);
static int _strUbxInf(char *info, const int size, const uint8_t *msg, const int msgSize);
static int _strUbxRxmRawx(char *info, const int size, const uint8_t *msg, const int msgSize);
static int _strUbxMonVer(char *info, const int size, const uint8_t *msg, const int msgSize);
static int _strUbxCfgValget(char *info, const int size, const uint8_t *msg, const int msgSize);
static int _strUbxAckAck(char *info, const int size, const uint8_t *msg, const int msgSize, const bool ack);

bool ubxMessageInfo(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    if (info == NULL)
    {
        return false;
    }
    if ( (msg == NULL) || (msgSize < UBX_FRAME_SIZE) )
    {
        info[0] = '\0';
        return false;
    }

    if (msgSize == UBX_FRAME_SIZE)
    {
        const int len = snprintf(info, size, "empty message / poll request");
        return (len > 0) && (len < size);
    }

    const uint8_t clsId = UBX_CLSID(msg);
    const uint8_t msgId = UBX_MSGID(msg);
    int len = 0;
    switch (clsId)
    {
        case UBX_NAV_CLSID:
            switch (msgId)
            {
                case UBX_NAV_PVT_MSGID:
                    len = _strUbxNavPvt(info, size, msg, msgSize);
                    break;
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
                case UBX_NAV_EOE_MSGID:
                case UBX_NAV_GEOFENCE_MSGID:
                case UBX_NAV_TIMEUTC_MSGID:
                case UBX_NAV_TIMELS_MSGID:
                case UBX_NAV_TIMEGPS_MSGID:
                case UBX_NAV_TIMEGLO_MSGID:
                case UBX_NAV_TIMEBDS_MSGID:
                case UBX_NAV_TIMEGAL_MSGID:
                    len = _strUbxNav(info, size, msg, msgSize, 0);
                    break;
                case UBX_NAV_SVIN_MSGID:
                case UBX_NAV_ODO_MSGID:
                case UBX_NAV_HPPOSLLH_MSGID:
                case UBX_NAV_HPPOSECEF_MSGID:
                case UBX_NAV_RELPOSNED_MSGID:
                    len = _strUbxNav(info, size, msg, msgSize, 4);
                    break;
                default:
                    break;
            }
            break;
        case UBX_INF_CLSID:
            switch (msgId)
            {
                case UBX_INF_NOTICE_MSGID:
                case UBX_INF_WARNING_MSGID:
                case UBX_INF_ERROR_MSGID:
                case UBX_INF_TEST_MSGID:
                case UBX_INF_DEBUG_MSGID:
                    len = _strUbxInf(info, size, msg, msgSize);
                    break;
            }
            break;
        case UBX_RXM_CLSID:
            switch (msgId)
            {
                case UBX_RXM_RAWX_MSGID:
                    len = _strUbxRxmRawx(info, size, msg, msgSize);
                    break;
            }
            break;
        case UBX_MON_CLSID:
            switch (msgId)
            {
                case UBX_MON_VER_MSGID:
                    len = _strUbxMonVer(info, size, msg, msgSize);
                    break;
            }
            break;
        case UBX_CFG_CLSID:
            switch (msgId)
            {
                case UBX_CFG_VALGET_MSGID:
                    len = _strUbxCfgValget(info, size, msg, msgSize);
                    break;
            }
            break;
        case UBX_ACK_CLSID:
            switch (msgId)
            {
                case UBX_ACK_ACK_MSGID:
                    len = _strUbxAckAck(info, size, msg, msgSize, true);
                    break;
                case UBX_ACK_NAK_MSGID:
                    len = _strUbxAckAck(info, size, msg, msgSize, false);
                    break;
            }
            break;
        default:
            break;
    }
    // TODO: insert ellipsis instead of failing in case len > size
    return (len > 0) && (len < size);
}

#define F(field, flag) ( ((field) & (flag)) == (flag) )

static int _strUbxNav(char *info, const int size, const uint8_t *msg, const int msgSize, const int iTowOffs)
{
    if (msgSize < (UBX_FRAME_SIZE + iTowOffs + (int)sizeof(uint32_t)))
    {
        return 0;
    }
    uint32_t iTOW;
    memcpy(&iTOW, &msg[UBX_HEAD_SIZE + iTowOffs], sizeof(iTOW));
    const int n = snprintf(info, size, "%010.3f", (double)iTOW * 1e-3);
    return n;
}

static int _strUbxNavPvt(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    if (msgSize != UBX_NAV_PVT_V1_SIZE) 
    {
        return 0;
    }
    UBX_NAV_PVT_V1_GROUP0_t pvt;
    memcpy(&pvt, &msg[UBX_HEAD_SIZE], sizeof(pvt));
    int sec = pvt.sec;
    int msec = (pvt.nano / 1000 + 500) / 1000;
    if (msec < 0)
    {
        sec -= 1;
        msec = 1000 + msec;
    }
    const int carrSoln = UBX_NAV_PVT_V1_FLAGS_CARRSOLN_GET(pvt.flags);
    const char * const fixTypes[] = { "nofix", "dr", "2D", "3D", "3D+DR", "time" };
    const int n = snprintf(info, size,
        "%010.3f"
        " %04u-%02u-%02u (%c)"
        " %02u:%02u:%02d.%03d (%c)"
        " %s (%s, %s)"
        " %2d %4.2f"
        " %+11.7f %+12.7f (%5.1f) %+6.0f (%5.1f)",
        (double)pvt.iTOW * 1e-3,
        pvt.year, pvt.month, pvt.day,
        F(pvt.valid, UBX_NAV_PVT_V1_VALID_VALIDDATE) ? (F(pvt.flags2, UBX_NAV_PVT_V1_FLAGS2_CONFDATE) ? 'Y' : 'y') : 'n',
        pvt.hour, pvt.min, sec, msec,
        F(pvt.valid, UBX_NAV_PVT_V1_VALID_VALIDTIME) ? (F(pvt.flags2, UBX_NAV_PVT_V1_FLAGS2_CONFTIME) ? 'Y' : 'y') : 'n',
        pvt.fixType < NUMOF(fixTypes) ? fixTypes[pvt.fixType] : "?",
            F(pvt.flags, UBX_NAV_PVT_V1_FLAGS_GNSSFIXOK) ? "OK" : "masked",
        carrSoln == 0 ? "none" : (carrSoln == 1 ? "float" : (carrSoln == 2 ? "fixed" : "?")),
        pvt.numSV, (double)pvt.pDOP * UBX_NAV_PVT_V1_PDOP_SCALE,
        (double)pvt.lat    * UBX_NAV_PVT_V1_LAT_SCALE,
        (double)pvt.lon    * UBX_NAV_PVT_V1_LON_SCALE,
        (double)pvt.hAcc   * UBX_NAV_PVT_V1_HACC_SCALE,
        (double)pvt.height * UBX_NAV_PVT_V1_HEIGHT_SCALE,
        (double)pvt.vAcc   * UBX_NAV_PVT_V1_VACC_SCALE
        );
    return n;
}

static int _strUbxInf(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    if (msgSize < UBX_FRAME_SIZE)
    {
        return 0;
    }
    const int infSize = msgSize - UBX_FRAME_SIZE + 1;
    const int n = snprintf(info, infSize > size ? size : infSize,
        "%s", (const char *)&msg[UBX_HEAD_SIZE]);
    return n;
}

typedef struct SV_LIST_s
{
    uint8_t gnssId;
    uint8_t svId;
    uint8_t sigId;
    uint8_t res;
} SV_LIST_t;

static int _svListSort(const void *a, const void *b);

static int _strUbxRxmRawx(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    if ( (msgSize < (int)UBX_RXM_RAWX_MIN_SIZE) || (UBX_RXM_RAWX_V1_VERSION_GET(msg) != UBX_RXM_RAWX_V1_VERSION) )
    {
        return 0;
    }
    int iLen = 0;
    int iRem = size;

    {
        UBX_RXM_RAWX_V1_GROUP0_t rawx;
        memcpy(&rawx, &msg[UBX_HEAD_SIZE], sizeof(rawx));
        const int n = snprintf(info, size, "%010.3f %04u %u",
            rawx.rcvTow, rawx.week, rawx.numMeas);
        iLen += n;
        iRem -= n;
    }

    int offs = UBX_HEAD_SIZE + sizeof(UBX_RXM_RAWX_V1_GROUP0_t);
    int rem = msgSize - UBX_FRAME_SIZE - sizeof(UBX_RXM_RAWX_V1_GROUP0_t);
    SV_LIST_t list[100];
    int nList = 0;
    while ( (rem >= (int)sizeof(UBX_RXM_RAWX_V1_GROUP1_t)) && (nList < NUMOF(list)) )
    {
        UBX_RXM_RAWX_V1_GROUP1_t sv;
        memcpy(&sv, &msg[offs], sizeof(sv));

        list[nList].gnssId = sv.gnssId;
        list[nList].svId   = sv.svId;
        list[nList].sigId  = sv.sigId;
        nList++;

        offs += sizeof(sv);
        rem  -= sizeof(sv);
    }

    qsort(list, nList, sizeof(SV_LIST_t), _svListSort);

    for (int ix = 0; (ix < nList) && (iRem > 0); ix++)
    {
        const char *svStr = ubxSvStr(list[ix].gnssId, list[ix].svId);
        const char *sigStr = ubxSigStr(list[ix].gnssId, list[ix].sigId);
        const int n = snprintf(&info[iLen], iRem, " %s(%s)", svStr, sigStr);
        iLen += n;
        iRem -= n;
    }

    return iLen;
}

static int _svListSort(const void *a, const void *b)
{
    const SV_LIST_t *svA = (const SV_LIST_t *)a;
    const SV_LIST_t *svB = (const SV_LIST_t *)b;
    if (svA->gnssId == svB->gnssId)
    {
        if (svA->svId == svB->svId)
        {
            if (svA->sigId == svB->sigId)
            {
                return 0;
            }
            else
            {
                return svA->sigId < svB->sigId ? -1 : 1;
            }
        }
        else
        {
            return svA->svId < svB->svId ? -1 : 1;
        }
    }
    else
    {
        return svA->gnssId < svB->gnssId ? -1 : 1;
    }
}

static int _strUbxMonVer(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    return ubxMonVerToVerStr(info, size, msg, msgSize);
}

static const char *_valgetLayerName(const uint8_t layer)
{
    switch (layer)
    {
        case UBX_CFG_VALGET_V0_LAYER_RAM:     return "RAM";
        case UBX_CFG_VALGET_V0_LAYER_BBR:     return "BBR";
        case UBX_CFG_VALGET_V0_LAYER_FLASH:   return "Flash";
        case UBX_CFG_VALGET_V0_LAYER_DEFAULT: return "Default";
    }
    return "?";
}

static int _strUbxCfgValget(char *info, const int size, const uint8_t *msg, const int msgSize)
{
    if (msgSize < (int)sizeof(UBX_CFG_VALGET_V0_GROUP0_t))
    {
        return 0;
    }
    UBX_CFG_VALGET_V0_GROUP0_t /* (= UBX_CFG_VALGET_V0_GROUP0_t) */ head;
    memcpy(&head, &msg[UBX_HEAD_SIZE], sizeof(head));
    switch (head.version)
    {
        case UBX_CFG_VALGET_V0_VERSION:
        {
            const int numKeys = (msgSize - UBX_FRAME_SIZE - sizeof(head)) / sizeof(uint32_t);
            return snprintf(info, size, "poll %d items from layer %s, position %u",
                numKeys, _valgetLayerName(head.layer), head.position);
            break;
        }
        case UBX_CFG_VALGET_V1_VERSION:
        {
            const int dataSize = size - UBX_FRAME_SIZE - sizeof(head);
            return snprintf(info, size, "response %d bytes from layer %s, position %u",
                dataSize, _valgetLayerName(head.layer), head.position);
            break; 
        }
    }
    return 0;
}

static int _strUbxAckAck(char *info, const int size, const uint8_t *msg, const int msgSize, const bool ack)
{
    if (msgSize < (int)sizeof(UBX_ACK_ACK_V0_GROUP0_t))
    {
        return 0;
    }
    UBX_ACK_ACK_V0_GROUP0_t /* (= UBX_ACK_NAK_V0_GROUP0_t) */ acknak;
    memcpy(&acknak, &msg[UBX_HEAD_SIZE], sizeof(acknak));
    char tmp[100];
    ubxMessageNameIds(tmp, sizeof(tmp), acknak.clsId, acknak.msgId);
    return snprintf(info, size, "%s %s", ack ? "acknowledgement" : "negative-acknowledgement", tmp);
}


/* ********************************************************************************************** */
// eof
