// flipflip's u-blox positioning receiver control library
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

/* ****************************************************************************************************************** */

#define RX_XTRA_TRACE_ENABLE 0 // Set to 1 to enable more trace output

#if RX_XTRA_TRACE_ENABLE
#  define RX_XTRA_TRACE(fmt, args...) TRACE("%s: " fmt, rx->name, ## args)
#else
#  define RX_XTRA_TRACE(fmt, ...) /* nothing */
#endif

#define RX_PRINT(fmt, ...)   if (rx->verbose) { PRINT("%s: " fmt, rx->name, ## __VA_ARGS__); }
#define RX_WARNING(fmt, ...) WARNING("%s: " fmt, rx->name, ## __VA_ARGS__)
#define RX_DEBUG(fmt, ...)   DEBUG(  "%s: " fmt, rx->name, ## __VA_ARGS__)
#define RX_TRACE(fmt, ...)   TRACE(  "%s: " fmt, rx->name, ## __VA_ARGS__)
#define RX_TRACE_HD(data, size, fmt, ...) TRACE_HEXDUMP(data, size, "%s: " fmt, rx->name, ## __VA_ARGS__)

// ---------------------------------------------------------------------------------------------------------------------

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
    void       (*msgcb)(PARSER_MSG_t *, void *arg);
    void        *cbarg;
    bool         abort;
} RX_t;

RX_t *rxInit(const char *port, const RX_ARGS_t *args)
{
    if (port == NULL)
    {
        return NULL;
    }

    // Allocate handle
    RX_t *rx = (RX_t *)malloc(sizeof(RX_t));
    if (rx == NULL)
    {
        WARNING("rxInit() malloc fail!");
        return NULL;
    }
    memset(rx, 0, sizeof(*rx));

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
        rx->msgcb = rxArgs->msgcb;
        rx->cbarg = rxArgs->cbarg;
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

    return rx;
}

const PARSER_t *rxGetParser(RX_t *rx)
{
    return rx != NULL ? &rx->parser : NULL;
}

static bool _rxOpenDetect(RX_t *rx);

bool rxOpen(RX_t *rx)
{
    if (rx == NULL)
    {
        return false;
    }

    if (!portOpen(&rx->port))
    {
        return false;
    }

    // Autobaud
    rx->autobaud = rx->autobaud && portCanBaudrate(&rx->port);
    if (rx->detect && !_rxOpenDetect(rx))
    {
        portClose(&rx->port);
        return false;
    }

    return true;
}

static bool _rxOpenDetect(RX_t *rx)
{
    // Quick first try, which may just work..
    char verStr[100];
    if (rxGetVerStr(rx, verStr, sizeof(verStr)))
    {
        RX_PRINT("Receiver detected: %s", verStr);
        return true;
    }

    // Try different baudrates
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

    // And check again
    if (rx->detect)
    {
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

// ---------------------------------------------------------------------------------------------------------------------

static void _rxCallbackData(RX_t *rx, const PARSER_MSGSRC_t src, const uint8_t *buf, const int size)
{
    if (rx->msgcb != NULL)
    {
        PARSER_t p;
        parserInit(&p);
        if (!parserAdd(&p, buf, size))
        {
            RX_WARNING("Parser overflow!");
            parserInit(&p);
        }
        PARSER_MSG_t msg;
        if (parserProcess(&p, &msg, true))
        {
            msg.src = src;
            rx->msgcb(&msg, rx->cbarg);
        }
        //else // should not happen, parserProcess() should always return at least GARBAGE
    }
}

static void _rxCallbackMsg(RX_t *rx, PARSER_MSG_t *msg)
{
    if (rx->msgcb != NULL)
    {
        rx->msgcb(msg, rx->cbarg);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void rxClose(RX_t *rx)
{
    if (rx != NULL)
    {
        rx->abort = false;
        portClose(&rx->port);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void rxAbort(RX_t *rx)
{
    RX_WARNING("Abort!");
    rx->abort = true;
}

// ---------------------------------------------------------------------------------------------------------------------

bool rxSend(RX_t *rx, const uint8_t *data, const int size)
{
    if ( (rx != NULL) && !rx->abort )
    {
        _rxCallbackData(rx, PARSER_MSGSRC_TO_RX, data, size);
        return portWrite(&rx->port, data, size);
    }
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

int rxGetBaudrate(RX_t *rx)
{
    if (rx != NULL)
    {
        int res = portGetBaudrate(&rx->port);
        return res;
    }
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------------

bool rxSetBaudrate(RX_t *rx, const int baudrate)
{
    if (rx != NULL)
    {
        return portSetBaudrate(&rx->port, baudrate);
    }
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

PARSER_MSG_t *rxGetNextMessage(RX_t *rx)
{
    PARSER_MSG_t *msg = NULL;
    if (rx != NULL)
    {
        int readSize;
        while ( !rx->abort && portRead(&rx->port, rx->readBuf, sizeof(rx->readBuf), &readSize) && (readSize > 0) )
        {
            parserAdd(&rx->parser, rx->readBuf, readSize);
        }

        if (parserProcess(&rx->parser, &rx->msg, true))
        {
            msg = &rx->msg;
            msg->src = PARSER_MSGSRC_FROM_RX;
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
            if (rx->abort)
            {
                break;
            }
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

/* ****************************************************************************************************************** */

PARSER_MSG_t *rxPollUbx(RX_t *rx, const RX_POLL_UBX_t *param, bool *pollNak)
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
    const bool isUbxCfg   = param->clsId == UBX_CFG_CLSID;

    // Create poll request message, use parser's tmp message buffer
    const int pollSize = ubxMakeMessage(param->clsId, param->msgId, param->payload, param->payloadSize, rx->pollBuf);
    char pollName[PARSER_MAX_NAME_SIZE];
    ubxMessageName(pollName, sizeof(pollName), rx->pollBuf, pollSize);

    // Poll...
    PARSER_MSG_t *res = NULL;
    bool _pollNak = false;
    for (int attempt = 1; attempt <= retries; attempt++)
    {
        RX_DEBUG("poll %s, size %d, timeout=%d, isUbxCfg=%d, attempt %d/%d.",
            pollName, pollSize, timeout, attempt, isUbxCfg, retries);

        // Send request
        if (!rxSend(rx, rx->pollBuf, pollSize))
        {
            return NULL;
        }

        // Get response
        const uint32_t t0 = TIME();
        const uint32_t t1 = t0 + timeout;
        while ( (res == NULL) && (TIME() < t1) )
        {
            if (rx->abort)
            {
                break;
            }
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
            else
            {
                _rxCallbackMsg(rx, msg);

                // UBX-CFG polls can return NAK in case the message is not pollable
                if (isUbxCfg && (UBX_CLSID(msg->data) == UBX_ACK_CLSID) && (UBX_MSGID(msg->data) == UBX_ACK_NAK_MSGID))
                {
                    const UBX_ACK_ACK_V0_GROUP0_t *ack = (const UBX_ACK_ACK_V0_GROUP0_t *)&msg->data[UBX_HEAD_SIZE];
                    if ( (ack->clsId == param->clsId) && (ack->msgId == param->msgId) )
                    {
                        _pollNak = true;
                        attempt = retries; // No need to try again
                        break;
                    }
                }
            }
        }

        if (res != NULL)
        {
            break;
        }
        if (_pollNak)
        {
            RX_DEBUG("UBX-ACK-NAK: %s", pollName);
        }
        else
        {
            RX_DEBUG("poll %s timeout", pollName);
        }
        if (pollNak != NULL)
        {
            *pollNak = _pollNak;
        }
    }
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

bool rxSendUbxCfg(RX_t *rx, const uint8_t *msg, const int size, const uint32_t timeout)
{
    if ( (rx == NULL) ||(msg == NULL) || (size < 1) )
    {
        return false;
    }
    char sendName[PARSER_MAX_NAME_SIZE];
    ubxMessageName(sendName, sizeof(sendName), msg, size);
    RX_DEBUG("Sending %s, size %d, timeout %u", sendName, size, timeout);
    RX_TRACE_HD(msg, size, "%s", sendName);

    const uint8_t clsId = UBX_CLSID(msg);
    const uint8_t msgId = UBX_MSGID(msg);

    if (!rxSend(rx, msg, size))
    {
        return false;
    }

    const uint32_t t0 = TIME();
    const uint32_t t1 = t0 + (timeout > 0 ? timeout : 1000);
    bool res = true;
    bool resp = false;
    while ( !resp && (TIME() < t1) )
    {
        if (rx->abort)
        {
            break;
        }
        PARSER_MSG_t *pmsg = rxGetNextMessage(rx);
        if (pmsg == NULL)
        {
            SLEEP(10);
            continue;
        }
        _rxCallbackMsg(rx, pmsg);
        if ( (pmsg->type == PARSER_MSGTYPE_UBX) && (UBX_CLSID(pmsg->data) == UBX_ACK_CLSID) )
        {
            const uint8_t respMsgId = UBX_MSGID(pmsg->data);
            if (respMsgId == UBX_ACK_ACK_MSGID)
            {
                const UBX_ACK_ACK_V0_GROUP0_t *ack = (const UBX_ACK_ACK_V0_GROUP0_t *)&pmsg->data[UBX_HEAD_SIZE];
                if ( (ack->clsId == clsId) && (ack->msgId == msgId) )
                {
                    RX_DEBUG("UBX-ACK-ACK: %s", sendName);
                    res = true;
                    resp = true;
                }
            }
            else if (respMsgId == UBX_ACK_NAK_MSGID)
            {
                const UBX_ACK_NAK_V0_GROUP0_t *nak = (const UBX_ACK_NAK_V0_GROUP0_t *)&pmsg->data[UBX_HEAD_SIZE];
                if ( (nak->clsId == clsId) && (nak->msgId == msgId) )
                {
                    RX_DEBUG("UBX-ACK-NAK: %s", sendName);
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

// ---------------------------------------------------------------------------------------------------------------------

bool rxGetVerStr(RX_t *rx, char *str, const int size)
{
    if ( (rx == NULL) || (str == NULL) )
    {
        return false;
    }

    RX_POLL_UBX_t pollParam = { .clsId = UBX_MON_CLSID, .msgId = UBX_MON_VER_MSGID };
    PARSER_MSG_t *ubxMonVer = rxPollUbx(rx, &pollParam, NULL);
    if (ubxMonVer != NULL)
    {
        _rxCallbackMsg(rx, ubxMonVer);
        return ubxMonVerToVerStr(str, size, ubxMonVer->data, ubxMonVer->size);
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
    while ( !rx->abort && (maxRead > 0) && portRead(&rx->port, rx->readBuf, sizeof(rx->readBuf), &readSize) && (readSize > 0) )
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
    int baudrates[] = { currentBaudrate, 9600, 38400, 115200, 230400, 460800, 921600 };
    PARSER_MSG_t *ubxMonVer = NULL;

    // First, try quickly..
    {
        RX_POLL_UBX_t pollParam = { .clsId = UBX_MON_CLSID, .msgId = UBX_MON_VER_MSGID, .retries = 1, .timeout = 1000 };
        for (int ix = 0; !rx->abort && (ix < NUMOF(baudrates)); ix++)
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
            ubxMonVer = rxPollUbx(rx, &pollParam, NULL);
            if (ubxMonVer != NULL)
            {
                _rxCallbackMsg(rx, ubxMonVer);
                baudrate = baudrates[ix];
                break;
            }
        }
    }
    // Try harder
    if (baudrate == 0)
    {
        RX_POLL_UBX_t pollParam = { .clsId = UBX_MON_CLSID, .msgId = UBX_MON_VER_MSGID, .retries = 2, .timeout = 2500 };
        for (int ix = 0; !rx->abort && (ix < NUMOF(baudrates)); ix++)
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
            ubxMonVer = rxPollUbx(rx, &pollParam, NULL);
            if (ubxMonVer != NULL)
            {
                _rxCallbackMsg(rx, ubxMonVer);
                baudrate = baudrates[ix];
                break;
            }
        }
    }

    if ( !rx->abort && (baudrate != 0) )
    {
        char verStr[100];
        if (!ubxMonVerToVerStr(verStr, sizeof(verStr), ubxMonVer->data, ubxMonVer->size))
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


/* ****************************************************************************************************************** */

bool rxReset(RX_t *rx, const RX_RESET_t reset)
{
    if ( (rx == NULL) || (reset == RX_RESET_NONE) )
    {
        return false;
    }
    RX_PRINT("Doing receiver reset: %s", rxResetStr(reset));

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
    UBX_CFG_RST_V0_GROUP0_t payload = { 0 };
    payload.reserved = UBX_CFG_RST_V0_RESERVED;
    bool reenumerate = true;
    switch (reset)
    {
        case RX_RESET_NONE:
            return false;
        case RX_RESET_SOFT:
            payload.navBbrMask = UBX_CFG_RST_V0_NAVBBR_NONE;
            payload.resetMode  = UBX_CFG_RST_V0_RESETMODE_SW;
            break;
        case RX_RESET_HARD:
            payload.navBbrMask = UBX_CFG_RST_V0_NAVBBR_NONE;
            payload.resetMode  = UBX_CFG_RST_V0_RESETMODE_HW_CONTROLLED;
            break;
        case RX_RESET_HOT:
            payload.navBbrMask = UBX_CFG_RST_V0_NAVBBR_HOTSTART;
            payload.resetMode  = UBX_CFG_RST_V0_RESETMODE_GNSS;
            reenumerate = false;
            break;
        case RX_RESET_WARM:
            payload.navBbrMask = UBX_CFG_RST_V0_NAVBBR_WARMSTART;
            payload.resetMode  = UBX_CFG_RST_V0_RESETMODE_GNSS;
            reenumerate = false;
            break;
        case RX_RESET_COLD:
            payload.navBbrMask = UBX_CFG_RST_V0_NAVBBR_COLDSTART;
            payload.resetMode  = UBX_CFG_RST_V0_RESETMODE_GNSS;
            reenumerate = false;
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
        case RX_RESET_SAFEBOOT:
            break;
    }
    uint8_t msg[UBX_CFG_RST_V0_SIZE];
    int msgSize = 0;
    if (reset == RX_RESET_SAFEBOOT)
    {
        msgSize = ubxMakeMessage(UBX_UPD_CLSID, UBX_UPD_SAFEBOOT_MSGID, NULL, 0, msg);
        RX_DEBUG("Sending UBX-UPD-SAFEBOOT, size %d", msgSize);
    }
    else
    {
        memcpy(&msg[0], &payload, sizeof(payload));
        const int payloadSize = sizeof(payload);
        msgSize = ubxMakeMessage(UBX_CFG_CLSID, UBX_CFG_RST_MSGID, msg, payloadSize, msg);
        RX_DEBUG("Sending UBX-CFG-RST, size %d, bbrMask=0x%04x, resetMode=0x%02x",
            msgSize, payload.navBbrMask, payload.resetMode);
    }
    // Send the reset command...
    if (!rxSend(rx, msg, msgSize))
    {
        RX_WARNING("Failed sending reset command!");
        return false;
    }

    if (reenumerate)
    {
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
                if (rx->abort)
                {
                    break;
                }
                if (access(rx->port.file, F_OK) == 0)
                {
                    RX_DEBUG("%s available (dt=%u).", rx->port.file, TIME() - t0);
                    break;
                }
                SLEEP(101);
            }
            // portOpen() will complain in a useful way in case the device isn't present
        }
        if (rx->abort)
        {
            return false;
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

    }

    // Let's see if we can still talk to the receiver
    if (!_rxOpenDetect(rx))
    {
        RX_WARNING("Failed reconnecting receiver after reset!");
        return false;
    }

    return true;
}

const char *rxResetStr(const RX_RESET_t reset)
{
    switch (reset)
    {
        case RX_RESET_NONE:         return "None";
        case RX_RESET_SOFT:         return "Software reset";
        case RX_RESET_HARD:         return "Hardware reset";
        case RX_RESET_HOT:          return "Hotstart";
        case RX_RESET_WARM:         return "Warmstart";
        case RX_RESET_COLD:         return "Coldstart";
        case RX_RESET_DEFAULT:      return "Default";
        case RX_RESET_FACTORY:      return "Factory";
        case RX_RESET_GNSS_STOP:    return "Stop GNSS";
        case RX_RESET_GNSS_START:   return "Start GNSS";
        case RX_RESET_GNSS_RESTART: return "Restart GNSS";
        case RX_RESET_SAFEBOOT:     return "Safeboot";
    }
    return "?";
}

/* ****************************************************************************************************************** */

int rxGetConfig(RX_t *rx, const UBLOXCFG_LAYER_t layer, const uint32_t *keys, const int numKeys, UBLOXCFG_KEYVAL_t *kv, const int maxKv)
{
    if ( (rx == NULL) || (keys == NULL) || (numKeys < 1) || (kv == NULL) || (maxKv < 1) )
    {
        return -1;
    }

    const char *layerName = ubloxcfg_layerName(layer);
    RX_DEBUG("Polling receiver configuration for layer %s", layerName);

    uint8_t pollLayer = UBLOXCFG_LAYER_DEFAULT;
    switch (layer)
    {
        case UBLOXCFG_LAYER_RAM:
            pollLayer = UBX_CFG_VALGET_V0_LAYER_RAM;
            break;
        case UBLOXCFG_LAYER_BBR:
            pollLayer = UBX_CFG_VALGET_V0_LAYER_BBR;
            break;
        case UBLOXCFG_LAYER_FLASH:
            pollLayer = UBX_CFG_VALGET_V0_LAYER_FLASH;
            break;
        case UBLOXCFG_LAYER_DEFAULT:
            pollLayer = UBX_CFG_VALGET_V0_LAYER_DEFAULT;
            break;
    }

    uint32_t t0 = TIME();
    bool done = false;
    bool res = true;
    int totNumKv = 0;
    uint16_t position = 0;
    while (!done)
    {
        if (rx->abort)
        {
            break;
        }

        // UBX-CFG-VALGET poll request payload
        UBX_CFG_VALGET_V0_GROUP0_t pollHead =
        {
            .version  = UBX_CFG_VALGET_V0_VERSION,
            .layer    = pollLayer,
            .position = position
        };
        uint8_t pollPayload[UBX_CFG_VALGET_V0_MAX_SIZE];
        memcpy(&pollPayload[0], &pollHead, sizeof(pollHead));
        const int keysSize = numKeys * sizeof(uint32_t);
        memcpy(&pollPayload[sizeof(pollHead)], keys, keysSize);

        // Poll UBX-CFG-VALGET
        RX_POLL_UBX_t pollParam =
        {
            .clsId = UBX_CFG_CLSID, .msgId = UBX_CFG_VALGET_MSGID,
            .payload = pollPayload, .payloadSize = (sizeof(UBX_CFG_VALGET_V0_GROUP0_t) + keysSize),
            .retries = 2, .timeout = 2000,
        };
        bool pollNak = false;
        PARSER_MSG_t *msg = rxPollUbx(rx, &pollParam, &pollNak);
        if (msg == NULL)
        {
            if (pollNak)
            {
                RX_DEBUG("No data in layer %s!", layerName);
            }
            else
            {
                RX_WARNING("No response polling UBX-CFG-VALGET (position=%u, layer=%s)!", position, layerName);
                res = false;
            }
            break;
        }

        _rxCallbackMsg(rx, msg);

        // No key-val pairs in data
        if (msg->size < (int)(UBX_FRAME_SIZE + sizeof(UBX_CFG_VALGET_V0_GROUP0_t) + 4 + 1))
        {
            // No data in this layer
            if ( (position == 0) && ((layer == UBLOXCFG_LAYER_BBR) || (layer == UBLOXCFG_LAYER_FLASH)) )
            {
                break;
            }
            // No more data for this poll
            else if (position > 0)
            {
                break;
            }
            // Unexpectedly no data for layer that must have data
            else
            {
                RX_WARNING("Bad response polling UBX-CFG-VALGET (position=%u, layer=%s)!", position, layerName);
                res = false;
                break;
            }

            res = false;
            break;
        }

        // Check result
        UBX_CFG_VALGET_V1_GROUP0_t respHead;
        memcpy(&respHead, &msg->data[UBX_HEAD_SIZE], sizeof(respHead));
        if ( (respHead.version != UBX_CFG_VALGET_V1_VERSION) || (respHead.position != position) ||
            (respHead.layer != pollLayer) )
        {
            RX_WARNING("Unexpected response polling UBX-CFG-VALGET (position=%u, layer=%s)!", position, layerName);
            DEBUG_HEXDUMP(msg->data, msg->size, "version: %u %u, position: %u %u, layer: %u %u",
                respHead.version, UBX_CFG_VALGET_V1_VERSION,
                respHead.position, position, respHead.layer, layer);
            res = false;
            break;
        }

        // Add received data to list
        int numKv = 0;
        const int cfgDataSize = msg->size - UBX_FRAME_SIZE - sizeof(UBX_CFG_VALGET_V1_GROUP0_t);
        if (cfgDataSize > 0)
        {
            if (!ubloxcfg_parseData(&msg->data[UBX_HEAD_SIZE + sizeof(UBX_CFG_VALGET_V1_GROUP0_t)],
                cfgDataSize, &kv[totNumKv], UBX_CFG_VALGET_V1_MAX_KV, &numKv))
            {
                RX_WARNING("Bad config data in UBX-CFG-VALGET response (position=%u, layer=%s)!", position, layerName);
                DEBUG_HEXDUMP(msg->data, msg->size, NULL);
                res = false;
                break;
            }
            totNumKv += numKv;
        }

        // Are we done?
        if (numKv < UBX_CFG_VALGET_V1_MAX_KV)
        {
            done = true;
        }
        else
        {
            position += UBX_CFG_VALGET_V1_MAX_KV;
        }

        // Debug
        RX_DEBUG("Received %d items from (position=%u, layer=%s, done=%s)",
            numKv, position, layerName, done ? "yes" : "no");
        if (isTRACE())
        {
            for (int ix = totNumKv - numKv; ix < totNumKv; ix++)
            {
                char str[UBLOXCFG_MAX_KEYVAL_STR_SIZE];
                if (ubloxcfg_stringifyKeyVal(str, sizeof(str), &kv[ix]))
                {
                    RX_TRACE("kv[%d]: %s", ix, str);
                }
            }
        }

        // Enough space left in list?
        if ( (totNumKv + UBX_CFG_VALGET_V1_MAX_KV) > maxKv )
        {
            RX_WARNING("Too many config items (position=%u, layer=%s)!", position, layerName);
            res = false;
            break;
        }

    }
    // while 64 left, not complete, ...
    RX_DEBUG("Total %d items for layer %s (poll duration %ums), res=%d", totNumKv, layerName, TIME() - t0, res);

    return res ? totNumKv : -1;
}

bool rxSetConfig(RX_t *rx, const UBLOXCFG_KEYVAL_t *kv, const int nKv, const bool ram, const bool bbr, const bool flash)
{
    if ( (rx == NULL) || (kv == NULL) || !(ram || bbr || flash) )
    {
        return false;
    }

    int nMsgs = 0;
    UBX_CFG_VALSET_MSG_t *msgs = ubxKeyValToUbxCfgValset(kv, nKv, ram, bbr, flash, &nMsgs);
    if (msgs == NULL)
    {
        return false;
    }

    RX_PRINT("Sending %d key-value pairs in %d UBX-CFG-VALSET messages", nKv, nMsgs);
    bool res = true;
    for (int ix = 0; ix < nMsgs; ix++)
    {
        RX_PRINT("Sending UBX-CFG-VALSET %d/%d (%s)", ix + 1, nMsgs, msgs[ix].info);
        if (!rxSendUbxCfg(rx, msgs[ix].msg, msgs[ix].size, 2500))
        {
            RX_WARNING("Failed configuring receiver!");
            res = false;
            break;
        }
    }

    free(msgs);
    return res;
}


/* ****************************************************************************************************************** */
// eof
