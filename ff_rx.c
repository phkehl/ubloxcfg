// flipflip's u-blox positioning receiver control library
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
#include <ctype.h>
#include <unistd.h>

#include "ff_debug.h"
#include "ff_stuff.h"
#include "ff_ubx.h"
#include "ff_parser.h"
#include "ff_port.h"
#include "ff_rx.h"

/* ********************************************************************************************** */

#define RX_XTRA_TRACE_ENABLE 0 // Set to 1 to enable more trace output


#if RX_XTRA_TRACE_ENABLE
#  define RX_XTRA_TRACE(fmt, args...) TRACE("%s: " fmt, rx->name, ## args)
#else
#  define RX_XTRA_TRACE(fmt, ...) /* nothing */
#endif

#define RX_PRINT(fmt, args...)   if (rx->verbose) { PRINT("%s: " fmt, rx->name, ## args); }
#define RX_WARNING(fmt, args...) WARNING("%s: " fmt, rx->name, ## args)
#define RX_DEBUG(fmt, args...)   DEBUG(  "%s: " fmt, rx->name, ## args)
#define RX_TRACE(fmt, args...)   TRACE(  "%s: " fmt, rx->name, ## args)

// -------------------------------------------------------------------------------------------------

typedef struct RX_s
{
    PORT_t       port;
    PARSER_t     parser;
    uint8_t      readBuf[1024];
    uint8_t      pollBuf[PARSER_MAX_ANY_SIZE];
    PARSER_MSG_t msg;
    char         name[100];
    bool         verbose;
    bool         autobaud;
    bool         detect;
} RX_t;

static bool _rxOpenDetect(RX_t *rx);

RX_t *rxOpen(const char *port, const RX_ARGS_t *args)
{
    if (port == NULL)
    {
        return NULL;
    }

    // Allocate handle
    RX_t *rx = (RX_t *)malloc(sizeof(RX_t));
    if (rx == NULL)
    {
        WARNING("rxOpen() malloc fail!");
        return NULL;
    }

    {
        const RX_ARGS_t rxArgsDefault = RX_ARGS_DEFAULT();
        const RX_ARGS_t *rxArgs = args == NULL ? &rxArgsDefault : args;
        rx->verbose  = rxArgs->verbose;
        rx->detect   = rxArgs->detect;
        rx->autobaud = rxArgs->autobaud; // but see below
        static int instCnt;
        if ( (rxArgs->name != NULL) && (rxArgs->name[0] != '\0') )
        {
            snprintf(rx->name, sizeof(rx->name), "%s", rxArgs->name);
        }
        else
        {
            snprintf(rx->name, sizeof(rx->name), "rx%d", instCnt);
        }
        instCnt++;
    }
    RX_PRINT("Connecting to receiver at port %s", port);

    // Initialise parser
    parserInit(&rx->parser);

    // Initialise port
    if (!portInit(&rx->port, port))
    {
        free(rx);
        return NULL;
    }

    if (!portOpen(&rx->port))
    {
        free(rx);
        return NULL;
    }

    // Autobaud
    rx->autobaud = rx->autobaud && portCanBaudrate(&rx->port);
    if (!_rxOpenDetect(rx))
    {
        portClose(&rx->port);
        free(rx);
        return NULL;
    }

    return rx;
}

static bool _rxOpenDetect(RX_t *rx)
{
    if (rx->autobaud)
    {
        if (!rxAutobaud(rx))
        {
            RX_WARNING("Failed autobauding!");
            return false;
        }
        if (!rx->detect) // unless we print below
        {
            RX_PRINT("Receiver detected at baudrate %d", rxGetBaudrate(rx));
        }
    }

    if (rx->detect)
    {
        char verStr[100];
        if (!rxGetVerStr(rx, verStr, sizeof(verStr)))
        {
            RX_WARNING("Could not detect receiver!");
            return false;
        }
        RX_DEBUG("Receiver detected: %s", verStr);
        if (rx->autobaud)
        {
            RX_PRINT("Receiver detected at baudrate %d: %s", rxGetBaudrate(rx), verStr);
        }
        else
        {
            RX_PRINT("Receiver detected: %s", verStr);
        }
    }
    return true;
}

// -------------------------------------------------------------------------------------------------

void rxClose(RX_t *rx)
{
    if (rx != NULL)
    {
        portClose(&rx->port);
    }
}

// -------------------------------------------------------------------------------------------------

bool rxSend(RX_t *rx, const uint8_t *data, const int size)
{
    if (rx != NULL)
    {
        return portWrite(&rx->port, data, size);
    }
    return false;
}

// -------------------------------------------------------------------------------------------------

int rxGetBaudrate(RX_t *rx)
{
    if (rx != NULL)
    {
        int res = portGetBaudrate(&rx->port);
        return res;
    }
    return 0;
}

// -------------------------------------------------------------------------------------------------

bool rxSetBaudrate(RX_t *rx, const int baudrate)
{
    if (rx != NULL)
    {
        return portSetBaudrate(&rx->port, baudrate);
    }
    return false;
}

// -------------------------------------------------------------------------------------------------

PARSER_MSG_t *rxGetNextMessage(RX_t *rx)
{
    PARSER_MSG_t *msg = NULL;
    if (rx != NULL)
    {
        int readSize;
        while ( portRead(&rx->port, rx->readBuf, sizeof(rx->readBuf), &readSize) && (readSize > 0) )
        {
            parserAdd(&rx->parser, rx->readBuf, readSize);
        }

        if (parserProcess(&rx->parser, &rx->msg))
        {
            msg = &rx->msg;
        }
    }
    return msg;
}

PARSER_MSG_t *rxGetNextMessageTimeout(RX_t *rx, const uint32_t timeout)
{
    PARSER_MSG_t *msg = NULL;
    if (rx != NULL)
    {
        const uint32_t t0 = TIME();
        const uint32_t t1 = t0 + timeout;
        while (TIME() < t1)
        {
            msg = rxGetNextMessage(rx);
            if (msg != NULL)
            {
                break;
            }
            SLEEP(10);
        }
    }
    return msg;
}



/* ********************************************************************************************** */

PARSER_MSG_t *rxPollUbx(RX_t *rx, const RX_POLL_UBX_t *param)
{
    if ( (rx == NULL) || (param == NULL) ||
         ((param->payload != NULL) && (param->payloadSize > (int)(sizeof(rx->pollBuf) - UBX_FRAME_SIZE))) )
    {
        return NULL;
    }

    // Parameters
    const int timeout     = param->timeout     > 0 ? param->timeout     : 1500;
    const int respSizeMin = param->respSizeMin > 0 ? param->respSizeMin : (UBX_FRAME_SIZE + 1);
    const int retries     = param->retries     > 0 ? param->retries     : 2;

    // Create poll request message, use parser's tmp message buffer
    const int pollSize = ubxMakeMessage(param->clsId, param->msgId, param->payload, param->payloadSize, rx->pollBuf);
    char pollName[PARSER_MAX_NAME_SIZE];
    ubxMessageName(pollName, sizeof(pollName), rx->pollBuf, pollSize);

    // Poll...
    PARSER_MSG_t *res = NULL;
    for (int attempt = 1; attempt <= retries; attempt++)
    {
        RX_DEBUG("poll %s, size %d, timeout=%d, attempt %d/%d.", pollName, pollSize, timeout, attempt, retries);

        // Send request
        if (!portWrite(&rx->port, rx->pollBuf, pollSize))
        {
            return NULL;
        }

        // Get response
        const uint32_t t0 = TIME();
        const uint32_t t1 = t0 + timeout;
        while ( (res == NULL) && (TIME() < t1) )
        {
            PARSER_MSG_t *msg = rxGetNextMessage(rx);
            if (msg == NULL)
            {
                SLEEP(10);
                continue;
            }
            if ( (msg->type == PARSER_MSGTYPE_UBX) &&
                 (msg->size >= respSizeMin) &&
                 (UBX_CLSID(msg->data) == param->clsId) &&
                 (UBX_MSGID(msg->data) == param->msgId) )
            {
                RX_DEBUG("poll answer %s, size=%d, dt=%u", msg->name, msg->size, TIME() - t0);
                res = msg;
                break;
            }
        }

        if (res != NULL)
        {
            break;
        }
        RX_DEBUG("poll %s timeout", pollName);
    }
    return res;
}

// -------------------------------------------------------------------------------------------------

bool rxSendUbxCfg(RX_t *rx, const uint8_t *msg, const int size, const uint32_t timeout)
{
    if ( (rx == NULL) ||(msg == NULL) || (size < 1) )
    {
        return false;
    }
    char sendName[PARSER_MAX_NAME_SIZE];
    ubxMessageName(sendName, sizeof(sendName), msg, size);
    RX_DEBUG("Sending %s, size %d, timeout %u", sendName, size, timeout);

    const uint8_t clsId = UBX_CLSID(msg);
    const uint8_t msgId = UBX_MSGID(msg);

    if (!portWrite(&rx->port, msg, size))
    {
        return NULL;
    }

    const uint32_t t0 = TIME();
    const uint32_t t1 = t0 + (timeout > 0 ? timeout : 1000);
    bool res = true;
    bool resp = false;
    while ( !resp && (TIME() < t1) )
    {
        PARSER_MSG_t *msg = rxGetNextMessage(rx);
        if (msg == NULL)
        {
            SLEEP(10);
            continue;
        }
        if ( (msg->type == PARSER_MSGTYPE_UBX) && (UBX_CLSID(msg->data) == UBX_ACK_CLSID) )
        {
            const uint8_t respMsgId = UBX_MSGID(msg->data);
            if (respMsgId == UBX_ACK_ACK_MSGID)
            {
                const UBX_ACK_ACK_V0_GROUP0_t *ack = (const UBX_ACK_ACK_V0_GROUP0_t *)&msg->data[UBX_HEAD_SIZE];
                if ( (ack->clsId == clsId) && (ack->msgId == msgId) )
                {
                    RX_DEBUG("UBX-CFG-ACK: %s", sendName);
                    res = true;
                    resp = true;
                }
            }
            else if (respMsgId == UBX_ACK_NAK_MSGID)
            {
                const UBX_ACK_NAK_V0_GROUP0_t *nak = (const UBX_ACK_NAK_V0_GROUP0_t *)&msg->data[UBX_HEAD_SIZE];
                if ( (nak->clsId == clsId) && (nak->msgId == msgId) )
                {
                    RX_DEBUG("UBX-CFG-NAK: %s", sendName);
                    res = false;
                    resp = true;
                }
            }
        }
    }

    if (!resp)
    {
        RX_DEBUG("ack/nak %s timeout", sendName);
        res = false;
    }
    return res;
}

// -------------------------------------------------------------------------------------------------

static bool ubxMonVerToVerStr(const PARSER_MSG_t *msg, char *str, const int size)
{
    if ( (msg == NULL) || (str == NULL) || (size < 2) ||
         (UBX_CLSID(msg->data) != UBX_MON_CLSID) ||
         (UBX_MSGID(msg->data) != UBX_MON_VER_MSGID) ||
         (msg->size < (int)(UBX_FRAME_SIZE + sizeof(UBX_MON_VER_V0_GROUP0_t))) )
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
    memcpy(&gr0, &msg->data[offs], sizeof(gr0));
    gr0.swVersion[sizeof(gr0.swVersion) - 1] = '\0';

    char verStr[sizeof(gr0.swVersion)];
    verStr[0] = '\0';
    char modStr[sizeof(gr0.swVersion)];
    modStr[0] = '\0';

    offs += sizeof(gr0);
    UBX_MON_VER_V0_GROUP1_t gr1;
    while (offs <= (msg->size - 2 - (int)sizeof(gr1)))
    {
        memcpy(&gr1, &msg->data[offs], sizeof(gr1));
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

    int res = 0;
    if (modStr[0] != '\0')
    {
        res = snprintf(str, size, "%s (%s)", verStr, modStr);
    }
    else
    {
        res = snprintf(str, size, "%s", verStr);
    }
    return (res > 0) && (res < size);
    return false;
}

// -------------------------------------------------------------------------------------------------

bool rxGetVerStr(RX_t *rx, char *str, const int size)
{
    if ( (rx == NULL) || (str == NULL) )
    {
        return false;
    }

    RX_POLL_UBX_t pollParam = { .clsId = UBX_MON_CLSID, .msgId = UBX_MON_VER_MSGID };
    PARSER_MSG_t *ubxMonVer = rxPollUbx(rx, &pollParam);
    if (ubxMonVer != NULL)
    {
        return ubxMonVerToVerStr(ubxMonVer, str, size);
    }
    return false;
}

static bool _rxFlushRx(RX_t *rx)
{
    if (rx == NULL)
    {
        return false;
    }
    int maxRead = 1000;
    int readSize = 0;
    while ( (maxRead > 0) && portRead(&rx->port, rx->readBuf, sizeof(rx->readBuf), &readSize) && (readSize > 0) )
    {
        maxRead--;
    }
    return maxRead > 0 ? true : false;
}

static bool _rxFlushTx(RX_t *rx)
{
    if (rx == NULL)
    {
        return false;
    }
    const uint8_t flushSeq[1024] = { [0 ... 1023] = 0x55 };
    return rxSend(rx, flushSeq, sizeof(flushSeq));
}

bool rxAutobaud(RX_t *rx)
{
    if (rx == NULL)
    {
        return false;
    }

    int baudrate = 0;
    const int currentBaudrate = rxGetBaudrate(rx);
    int baudrates[] = { currentBaudrate, 9600, 38400, 115200, 460800, 921600 };
    PARSER_MSG_t *ubxMonVer = NULL;

    // First, try quickly..
    {
        RX_POLL_UBX_t pollParam = { .clsId = UBX_MON_CLSID, .msgId = UBX_MON_VER_MSGID, .retries = 1, .timeout = 1000 };
        for (int ix = 0; ix < NUMOF(baudrates); ix++)
        {
            if ( (ix > 0) && (baudrates[ix] == currentBaudrate) )
            {
                continue;
            }
            if (!rxSetBaudrate(rx, baudrates[ix]))
            {
                continue;
            }
            RX_DEBUG("autobaud %d (quick)", baudrates[ix]);
            ubxMonVer = rxPollUbx(rx, &pollParam);
            if (ubxMonVer != NULL)
            {
                baudrate = baudrates[ix];
                break;
            }
        }
    }
    // Try harder
    if (baudrate == 0)
    {
        RX_POLL_UBX_t pollParam = { .clsId = UBX_MON_CLSID, .msgId = UBX_MON_VER_MSGID, .retries = 2, .timeout = 2500 };
        for (int ix = 0; ix < NUMOF(baudrates); ix++)
        {
            if ( (ix > 0) && (baudrates[ix] == currentBaudrate) )
            {
                continue;
            }
            if (!rxSetBaudrate(rx, baudrates[ix]))
            {
                continue;
            }
            RX_DEBUG("autobaud %d (flush rx/tx)", baudrates[ix]);
            _rxFlushRx(rx);
            _rxFlushTx(rx);
            ubxMonVer = rxPollUbx(rx, &pollParam);
            if (ubxMonVer != NULL)
            {
                baudrate = baudrates[ix];
                break;
            }
        }
    }

    if (baudrate != 0)
    {
        char verStr[100];
        if (!ubxMonVerToVerStr(ubxMonVer, verStr, sizeof(verStr)))
        {
            verStr[0] = '?';
            verStr[1] = '\0';
        }
        RX_DEBUG("autobaud %d success: %s", baudrate, verStr);
        return true;
    }
    else
    {
        RX_DEBUG("autobaud fail");
        return false;
    }
}


/* ********************************************************************************************** */

bool rxReset(RX_t *rx, const RX_RESET_t reset)
{
    if (rx == NULL)
    {
        return false;
    }
    switch (reset)
    {
        case RX_RESET_HOT:
            RX_PRINT("Hotstarting receiver");
            break;
        case RX_RESET_WARM:
            RX_PRINT("Warmstarting receiver");
            break;
        case RX_RESET_COLD:
            RX_PRINT("Coldstarting receiver");
            break;
        case RX_RESET_DEFAULT:
            RX_PRINT("Defaulting receiver configuration");
            break;
        case RX_RESET_FACTORY:
            RX_PRINT("Factory-resetting receiver");
            break;
        case RX_RESET_GNSS_STOP:
            RX_PRINT("Stopping GNSS");
            break;
        case RX_RESET_GNSS_START:
            RX_PRINT("Starting GNSS");
            break;
        case RX_RESET_GNSS_RESTART:
            RX_PRINT("Restarting GNSS");
            break;
        default:
            return false;
    }

    // Delete config?
    if ( (reset == RX_RESET_DEFAULT) || (reset == RX_RESET_FACTORY) )
    {
        RX_DEBUG("Deleting configuration from BBR and Flash layers");
#if 0
        // Unfortunately UBX-CFG-VALDEL doesn't support wildcards...
        const UBX_CFG_VALDEL_V1_GROUP0_t payloadHead =
        {
            .version     = UBX_CFG_VALDEL_V1_VERSION,
            .layers      = (UBX_CFG_VALDEL_V1_LAYER_BBR | UBX_CFG_VALDEL_V1_LAYER_FLASH),
            .transaction = UBX_CFG_VALDEL_V1_TRANSACTION_NONE,
            .reserved    = UBX_CFG_VALDEL_V1_RESERVED
        };
        uint8_t msg[UBX_CFG_VALDEL_V1_MAX_SIZE];
        memcpy(&msg[0], &payloadHead, sizeof(payloadHead));
        const uint32_t keys[] = { UBX_CFG_VALDEL_V0_ALL_WILDCARD };
        memcpy(&msg[sizeof(payloadHead)], keys, sizeof(keys));
        const int payloadSize = sizeof(payloadHead) + sizeof(keys);
        const int msgSize = ubxMakeMessage(UBX_CFG_CLSID, UBX_CFG_VALDEL_MSGID, msg, payloadSize, msg);
#else
        // ...so we'll use the deprecated UBX-CFG-CFG interface instead
        const UBX_CFG_CFG_V0_GROUP0_t payload0 =
        {
            .clearMask = UBX_CFG_CFG_V0_CLEAR_ALL,
            .saveMask  = UBX_CFG_CFG_V0_SAVE_NONE,
            .loadMask  = UBX_CFG_CFG_V0_LOAD_NONE
        };
        const UBX_CFG_CFG_V0_GROUP1_t payload1 =
        {
            .deviceMask = (UBX_CFG_CFG_V0_DEVICE_BBR | UBX_CFG_CFG_V0_DEVICE_FLASH)
        };
        uint8_t msg[UBX_CFG_CFG_V0_MAX_SIZE];
        memcpy(&msg[0], &payload0, sizeof(payload0));
        memcpy(&msg[sizeof(payload0)], &payload1, sizeof(payload1));
        const int payloadSize = sizeof(payload0) + sizeof(payload1);
        const int msgSize = ubxMakeMessage(UBX_CFG_CLSID, UBX_CFG_CFG_MSGID, msg, payloadSize, msg);
#endif
        //DEBUG_HEXDUMP(msg, msgSize, "msgSize=%d", msgSize);
        if (!rxSendUbxCfg(rx, msg, msgSize, 2000))
        {
            RX_WARNING("Failed deleting configuration from BBR and Flash layers!");
            return false;
        }
    }

    // Reset
    UBX_CFG_RST_V0_GROUP0_t payload;
    payload.reserved = UBX_CFG_RST_V0_RESERVED;
    switch (reset)
    {
        case RX_RESET_HOT:
            payload.navBbrMask = UBX_CFG_RST_V0_NAVBBR_HOTSTART;
            payload.resetMode  = UBX_CFG_RST_V0_RESETMODE_HW_FORCED;
            break;
        case RX_RESET_WARM:
            payload.navBbrMask = UBX_CFG_RST_V0_NAVBBR_WARMSTART;
            payload.resetMode  = UBX_CFG_RST_V0_RESETMODE_HW_FORCED;
            break;
        case RX_RESET_COLD:
            payload.navBbrMask = UBX_CFG_RST_V0_NAVBBR_COLDSTART;
            payload.resetMode  = UBX_CFG_RST_V0_RESETMODE_HW_FORCED;
            break;
        case RX_RESET_DEFAULT:
            payload.navBbrMask = UBX_CFG_RST_V0_NAVBBR_NONE;
            payload.resetMode  = UBX_CFG_RST_V0_RESETMODE_HW_FORCED;
            break;
        case RX_RESET_FACTORY:
            payload.navBbrMask = UBX_CFG_RST_V0_NAVBBR_COLDSTART;
            payload.resetMode  = UBX_CFG_RST_V0_RESETMODE_HW_CONTROLLED;
            break;
        case RX_RESET_GNSS_STOP:
            payload.navBbrMask = UBX_CFG_RST_V0_NAVBBR_NONE;
            payload.resetMode  = UBX_CFG_RST_V0_RESETMODE_GNSS_STOP;
            break;
        case RX_RESET_GNSS_START:
            payload.navBbrMask = UBX_CFG_RST_V0_NAVBBR_NONE;
            payload.resetMode  = UBX_CFG_RST_V0_RESETMODE_GNSS_START;
            break;
        case RX_RESET_GNSS_RESTART:
            payload.navBbrMask = UBX_CFG_RST_V0_NAVBBR_NONE;
            payload.resetMode  = UBX_CFG_RST_V0_RESETMODE_GNSS;
            break;
    }
    uint8_t msg[UBX_CFG_RST_V0_SIZE];
    memcpy(&msg[0], &payload, sizeof(payload));
    const int payloadSize = sizeof(payload);
    const int msgSize = ubxMakeMessage(UBX_CFG_CLSID, UBX_CFG_RST_MSGID, msg, payloadSize, msg);
    RX_DEBUG("Sending UBX-CFG-RST, size %d, bbrMask=0x%04x, resetMode=0x%02x",
        msgSize, payload.navBbrMask, payload.resetMode);

    // Send the reset command...
    if (!portWrite(&rx->port, msg, msgSize))
    {
        RX_WARNING("Failed sending reset command!");
        return false;
    }
    // ...and disconnect immediately (free device) as the reset may cause a USB re-enumeration
    portClose(&rx->port);

    // ..and wait a bit for the reset to complete (and the USB device to disappear)
    RX_DEBUG("Waiting for reset to complete...");
    SLEEP(1009);

    // Wait for device to show up (mainly for USB re-enumeration)
    if (rx->port.type == PORT_TYPE_SER)
    {
        const uint32_t timeout = 5000;
        const uint32_t t0 = TIME();
        const uint32_t t1 = t0 + timeout;
        RX_DEBUG("Wait for %s, timeout %u", rx->port.file, timeout);
        while (TIME() < t1)
        {
            if (access(rx->port.file, F_OK) == 0)
            {
                RX_DEBUG("%s available (dt=%u).", rx->port.file, TIME() - t0);
                break;
            }
            SLEEP(101);
        }
        // portOpen() will complain in a useful way in case the device isn't present
    }

    // The device should now be available again
    int retries = 2;
    bool portOk = false;
    while (retries > 0)
    {
        if (portOpen(&rx->port))
        {
            portOk = true;
            break;
        }
        SLEEP(1009);
        retries--;
    }
    if (!portOk)
    {
        return false;
    }

    // Let's see if we can still talk to the receiver
    if (!_rxOpenDetect(rx))
    {
        RX_WARNING("Failed reconnecting receiver after reset!");
        return false;
    }

    return true;
}

/* ********************************************************************************************** */
// eof
