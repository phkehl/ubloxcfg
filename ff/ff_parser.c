// flipflip's UBX/NMEA/RTCM3 message parser
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

#include "crc24q.h"
#include "ff_debug.h"
#include "ff_stuff.h"
#include "ff_ubx.h"
#include "ff_rtcm3.h"
#include "ff_nmea.h"

#include "ff_parser.h"

/* ********************************************************************************************** */

#define PARSER_XTRA_TRACE_ENABLE 0 // Set to 1 to enable more trace output

#define _P_ADDR(p) ( (uint32_t)((uint64_t)(p)) & 0xffff )

#if PARSER_XTRA_TRACE_ENABLE
#  define _TRACE_FMT "[offs=%d, size=%d, tot=%u, cnt=%u]"
#  define _TRACE_ARG parser->offs, parser->size, parser->tot, parser->msg
#  define PARSER_XTRA_TRACE(fmt, args...) TRACE(  "parser(%04x) " fmt " " _TRACE_FMT, _P_ADDR(parser), ## args, _TRACE_ARG)
#else
#  define PARSER_XTRA_TRACE(fmt, ...) /* nothing */
#endif

#define PARSER_WARNING(fmt, args...) WARNING("parser(%04x) " fmt, _P_ADDR(parser), ## args)
#define PARSER_DEBUG(fmt, args...)   DEBUG(  "parser(%04x) " fmt, _P_ADDR(parser), ## args)
#define PARSER_TRACE(fmt, args...)   TRACE(  "parser(%04x) " fmt, _P_ADDR(parser), ## args)

// -------------------------------------------------------------------------------------------------

void parserInit(PARSER_t *parser)
{
    memset(parser, 0, sizeof(*parser));
    PARSER_XTRA_TRACE("init");
}

// -------------------------------------------------------------------------------------------------

void parserAdd(PARSER_t *parser, const uint8_t *data, const int size)
{
    // Overflow, discard all
    if ((parser->offs + parser->size + size) > (int)sizeof(parser->buf))
    {
        PARSER_WARNING("buffer overflow!");
        parserInit(parser);
    }
    // Add to buffer
    memcpy(&parser->buf[parser->offs + parser->size], data, size);
    parser->size += size;
    PARSER_XTRA_TRACE("add: size=%d ", size);
}

// -------------------------------------------------------------------------------------------------

const char *parserMsgtypeName(const PARSER_MSGTYPE_t type)
{
    switch (type)
    {
        case PARSER_MSGTYPE_UBX:     return "UBX";
        case PARSER_MSGTYPE_NMEA:    return "NMEA";
        case PARSER_MSGTYPE_RTCM3:   return "RTCM3";
        case PARSER_MSGTYPE_GARBAGE: return "GARBAGE";
    }
    return "UNKNOWN";
}

// -------------------------------------------------------------------------------------------------

static int _isUbxMessage(const uint8_t *buf, const int size);
static int _isNmeaMessage(const uint8_t *buf, const int size);
static int _isRtcm3Message(const uint8_t *buf, const int size);
static void _emitGarbage(PARSER_t *parser, PARSER_MSG_t *msg);
static void _emitMessage(PARSER_t *parser, PARSER_MSG_t *msg, const int msgSize, const PARSER_MSGTYPE_t msgType);

typedef struct PARSER_FUNC_s
{
    int            (*func)(const uint8_t *, const int);
    PARSER_MSGTYPE_t type;
    const char      *name;
} PARSER_FUNC_t;

static const PARSER_FUNC_t kParserFuncs[] =
{
    { .func = _isUbxMessage,   .type = PARSER_MSGTYPE_UBX,   .name = "UBX"   },
    { .func = _isNmeaMessage,  .type = PARSER_MSGTYPE_NMEA,  .name = "NMEA"  },
    { .func = _isRtcm3Message, .type = PARSER_MSGTYPE_RTCM3, .name = "RTCM3" },
};

bool parserProcess(PARSER_t *parser, PARSER_MSG_t *msg)
{
    while (parser->size > 0)
    {
        // Run parser functions
        int msgSize = 0;
        PARSER_MSGTYPE_t msgType;
        for (int ix = 0; ix < NUMOF(kParserFuncs); ix++)
        {
            msgSize = kParserFuncs[ix].func(&parser->buf[parser->offs], parser->size);
            PARSER_XTRA_TRACE("process: try %s, msgSize=%d ", kParserFuncs[ix].name, msgSize);
            
            // Parser said: Wait, need more data
            if (msgSize < 0)
            {
                break;
            }
            // Parser said: I have a message
            else if (msgSize > 0)
            {
                msgType = kParserFuncs[ix].type;
                break;
            }
            //else (msgSize == 0) // Parser said: No my message
        }

        // Waiting for more data...
        if (msgSize < 0)
        {
            PARSER_XTRA_TRACE("process: need more data");
            return false;
        }

        // No known message in buffer, move first byte to garbage
        else if (msgSize == 0)
        {
            //     buf: GGGG?????????????................ (p->offs >= 0, p->size > 0)
            // --> buf: GGGGG????????????................ (p->offs > 0, p->size >= 0)
            parser->offs++;
            parser->size--;
            PARSER_XTRA_TRACE("process: collect garbage");

            // Garbage bin full
            if (parser->offs >= PARSER_MAX_GARB_SIZE)
            {
                _emitGarbage(parser, msg);
                return true;
            }
        }

        // We have a message (and come back to the same message in the next iteration)
        else // msgLen > 0
        {
            // Return garbage first
            if (parser->offs > 0)
            {
                _emitGarbage(parser, msg);
                return true;
            }
            // else parser->offs == 0: Return message
            {
                _emitMessage(parser, msg, msgSize, msgType);
                return true;
            }
        }
    }

    // All data consumed, return garbage immediately if there is any
    if (parser->offs > 0)
    {
        _emitGarbage(parser, msg);
        return true;
    }

    return false;
}

/* ********************************************************************************************** */

static void _emitGarbage(PARSER_t *parser, PARSER_MSG_t *msg)
{
    uint32_t now = TIME();
    // Copy garbage to msg buf and move data in parser buf
    //     buf: GGGGGGGGGGGGG???????????????........ (p->offs > 0, p->size >= 0)
    //          ---p->offs--><-- p->size -->
    // --> tmp: GGGGGGGGGGGG......
    // --> buf  ???????????????..................... (p->offs = 0, p->size >= 0)
    const int size = parser->offs;
    //PARSER_XTRA_TRACE("garb copy tmp %d ", size);
    memcpy(parser->tmp, parser->buf, size);
    //PARSER_XTRA_TRACE("garb move 0 <- %d (%d) ", size, parser->size);
    memmove(&parser->buf[0], &parser->buf[size], parser->size);
    parser->offs = 0;
    parser->msg++;
    parser->tot += size;

    // Make message
    msg->type = PARSER_MSGTYPE_GARBAGE;
    msg->size = size;
    msg->data = parser->tmp;
    msg->seq  = parser->msg;
    msg->ts   = now;
    msg->src  = PARSER_MSGSRC_UNKN;
    msg->name = "GARBAGE";
    msg->info = NULL;

    PARSER_XTRA_TRACE("process: emit %s, size %d ", msg->name, size);
}

static void _emitMessage(PARSER_t *parser, PARSER_MSG_t *msg, const int msgSize, const PARSER_MSGTYPE_t msgType)
{
    uint32_t now = TIME();

    // Copy message to tmp, move remaining data to beginning of buf
    //     buf: MMMMMMMMMMMMMMM????????............. (p->offs = 0)
    //          <-- msgSize -->
    //          <----- p->size ------->
    // --> tmp: MMMMMMMMMMMMMMM.....
    // --> buf: ????????............................ (p->offs = 0, p->size >= 0)

    //PARSER_XTRA_TRACE("msg copy tmp %d "_TRACE_FMT, msgSize, _TRACE_ARG);
    memcpy(parser->tmp, parser->buf, msgSize);
    //PARSER_XTRA_TRACE("msg move 0 <- %d (%d) "_TRACE_FMT, parser->size - msgSize, parser->size, _TRACE_ARG);
    parser->size -= msgSize;
    if (parser->size > 0)
    {
        memmove(&parser->buf[0], &parser->buf[msgSize], parser->size);
    }
    parser->tot += msgSize;
    parser->msg++;
    // Make message
    msg->type = msgType;
    msg->size = msgSize;
    msg->data = parser->tmp;
    msg->seq  = parser->msg;
    msg->ts   = now;
    msg->src  = PARSER_MSGSRC_UNKN;
    parser->name[0] = '\0';
    parser->info[0] = '\0';
    switch (msgType)
    {
        case PARSER_MSGTYPE_UBX:
            msg->name = ubxMessageName(parser->name, sizeof(parser->name), parser->tmp, msgSize) ?
                parser->name : "UBX-?-?";
            msg->info = ubxMessageInfo(parser->info, sizeof(parser->info), parser->tmp, msgSize) ?
                parser->info : NULL;
            break;
        case PARSER_MSGTYPE_NMEA:
            msg->name = nmeaMessageName(parser->name, sizeof(parser->name), (const char *)parser->tmp) ?
                parser->name : "NMEA-?-?";
            msg->info = nmeaMessageInfo(parser->info, sizeof(parser->info), (const char *)parser->tmp) ?
                parser->info : NULL;
            break;
        case PARSER_MSGTYPE_RTCM3:
            msg->name = rtcm3MessageName(parser->name, sizeof(parser->name), parser->tmp, msgSize) ?
                parser->name : "RTCM3-?";
            msg->info = rtcm3MessageInfo(parser->info, sizeof(parser->info), parser->tmp, msgSize) ?
                parser->info : NULL;
            break;
        default:
            msg->name = "?";
            msg->info = NULL;
            break;
    }
    PARSER_XTRA_TRACE("process: emit %s, size %d, type %d ", msg->name, msgSize, msgType);
}

// -------------------------------------------------------------------------------------------------

// Parser functions work like this:
// Input: buffer to check, size >= 1
// Output: = 0 : definitively not a message at start of buffer
//         < 0 : can't say yet, need more data to make decision
//         > 0 : a message of this size detected at start of buffer

static int _isUbxMessage(const uint8_t *buf, const int size)
{
    if (buf[0] != UBX_SYNC_1)
    {
        return 0;
    }
    
    if (size < 2)
    {
        return -1;
    }

    if (buf[1] != UBX_SYNC_2)
    {
        return 0;
    }

    if (size < UBX_HEAD_SIZE)
    {
        return -1;
    }

    //const uint8_t  class = buf[2];
    //const uint8_t  msg   = buf[3];
    const int payloadSize = (int)( (uint16_t)buf[4] | ((uint16_t)buf[5] << 8) );

    if (payloadSize > (PARSER_MAX_UBX_SIZE - UBX_FRAME_SIZE))
    {
        return 0;
    }

    if (size < (payloadSize + UBX_FRAME_SIZE))
    {
        return -1;
    }

    uint8_t a = 0;
    uint8_t b = 0;
    const uint8_t *pCk = &buf[2];
    int cnt = payloadSize + (UBX_FRAME_SIZE - 2 - 2);
    while (cnt > 0)
    {
        a += *pCk;
        b += a;
        pCk++;
        cnt--;
    }

    if ( (pCk[0] != a) || (pCk[1] != b) )
    {
        return 0;
    }

    return payloadSize + UBX_FRAME_SIZE;
}

// -------------------------------------------------------------------------------------------------

static int _isNmeaMessage(const uint8_t *buf, const int size)
{
    // Start of sentence
    if (buf[0] != NMEA_PREAMBLE)
    {
        return 0;
    }

    // Find end of sentence, calculate checksum along the way
    int len = 1; // Length of sentence excl. "$"
    uint8_t ck = 0;
    while (true)
    {
        if (len > PARSER_MAX_NMEA_SIZE)
        {
            return 0;
        }
        if (len >= size) // len doesn't include '$'
        {
            return -1;
        }
        if ( (buf[len] == '\r') || (buf[len] == '\n') || (buf[len] == '*') )
        {
            break;
        }
        if ( // ((buf[len] & 0x80) != 0) || // 7-bit only
             (buf[len] < 0x20) || (buf[len] > 0x7e) || // valid range
             (buf[len] == '$') || (buf[len] == '\\') || (buf[len] == '!') || (buf[len] == '~') ) // reserved
        {
            return 0;
        }
        ck ^= buf[len];
        len++;
    }

    // Not nough data for sentence end (star + checksum + \r\n)?
    if (size < (len + 1 + 2 + 2))
    {
        return -1;
    }

    // Properly terminated sentence?
    if ( (buf[len] == '*') && (buf[len + 3] == '\r') && (buf[len + 4] == '\n') )
    {
        uint8_t n1 = buf[len + 1];
        uint8_t n2 = buf[len + 2];
        uint8_t c1 = '0' + ((ck >> 4) & 0x0f);
        uint8_t c2 = '0' + (ck & 0x0f);
        if (c2 > '9')
        {
            c2 += 'A' - '9' - 1;
        }
        // Checksum valid?
        if ( (n1 == c1) && (n2 == c2) )
        {
            return len + 5;
        }
    }

    return 0;
}

// -------------------------------------------------------------------------------------------------

static int _isRtcm3Message(const uint8_t *buf, const int size)
{
    // Not RTCM3 preamble?
    if (buf[0] != RTCM3_PREAMBLE)
    {
        return 0;
    }

    // Wait for enough data to look at header
    if (size < RTCM3_HEAD_SIZE)
    {
        return -1;
    }

    // Check header
    const uint16_t head = (uint16_t)buf[2] | ((uint16_t)buf[1] << 8);
    const uint16_t payloadSize = head & 0x03ff; // 10 bits
    //const uint16_t empty       = head & 0xfc00; // 6 bits
    
    // Too large?
    if ( (payloadSize > PARSER_MAX_RTCM3_SIZE) /*|| (empty != 0x0000)*/ )
    {
        return 0;
    }

    // Wait for full message
    const int msgSize = payloadSize + RTCM3_FRAME_SIZE;
    if (size < msgSize)
    {
        return -1;
    }

    // CRC okay?
    if (crc24q_check((const unsigned char *)buf, msgSize))
    {
        return msgSize;
    }

    return 0;
}

/* ********************************************************************************************** */
// eof
