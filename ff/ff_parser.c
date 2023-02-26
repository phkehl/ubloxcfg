// flipflip's UBX/NMEA/RTCM3 message parser
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

#include "ff_debug.h"
#include "ff_stuff.h"
#include "ff_ubx.h"
#include "ff_rtcm3.h"
#include "ff_spartn.h"
#include "ff_nmea.h"
#include "ff_novatel.h"
#include "ff_crc.h"

#include "ff_parser.h"

/* ****************************************************************************************************************** */

#define PARSER_XTRA_TRACE_ENABLE 0 // Set to 1 to enable more trace output

#define _P_ADDR(p) ( (uint32_t)((uint64_t)(p)) & 0xffff )

#if PARSER_XTRA_TRACE_ENABLE
#  define _TRACE_FMT "[offs=%d, size=%d, tot=%u, cnt=%u]"
#  define _TRACE_ARG parser->offs, parser->size, parser->tot, parser->msg
#  define PARSER_XTRA_TRACE(fmt, args...) TRACE(  "parser(%04x) " fmt " " _TRACE_FMT, _P_ADDR(parser), ## args, _TRACE_ARG)
#else
#  define PARSER_XTRA_TRACE(fmt, ...) /* nothing */
#endif

// ---------------------------------------------------------------------------------------------------------------------

void parserInit(PARSER_t *parser)
{
    memset(parser, 0, sizeof(*parser));
    PARSER_XTRA_TRACE("init");
}

// ---------------------------------------------------------------------------------------------------------------------

bool parserAdd(PARSER_t *parser, const uint8_t *data, const int size)
{
    // Overflow, discard all
    if ((parser->offs + parser->size + size) > (int)sizeof(parser->buf))
    {
        return false;
    }
    // Add to buffer
    memcpy(&parser->buf[parser->offs + parser->size], data, size);
    parser->size += size;
    PARSER_XTRA_TRACE("add: size=%d ", size);
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

const char *parserMsgtypeName(const PARSER_MSGTYPE_t type)
{
    switch (type)
    {
        case PARSER_MSGTYPE_UBX:     return "UBX";
        case PARSER_MSGTYPE_NMEA:    return "NMEA";
        case PARSER_MSGTYPE_RTCM3:   return "RTCM3";
        case PARSER_MSGTYPE_SPARTN:  return "SPARTN";
        case PARSER_MSGTYPE_NOVATEL: return "NOVATEL";
        case PARSER_MSGTYPE_GARBAGE: return "GARBAGE";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------------------------------------------------

static int _isUbxMessage(const uint8_t *buf, const int size);
static int _isNmeaMessage(const uint8_t *buf, const int size);
static int _isRtcm3Message(const uint8_t *buf, const int size);
static int _isSpartnMessage(const uint8_t *buf, const int size);
static int _isNovatelMessage(const uint8_t *buf, const int size);
static void _emitGarbage(PARSER_t *parser, PARSER_MSG_t *msg);
static void _emitMessage(PARSER_t *parser, PARSER_MSG_t *msg, const int msgSize, const PARSER_MSGTYPE_t msgType, const bool info);

typedef struct PARSER_FUNC_s
{
    int            (*func)(const uint8_t *, const int);
    PARSER_MSGTYPE_t type;
    const char      *name;
} PARSER_FUNC_t;

static const PARSER_FUNC_t kParserFuncs[] =
{
    { .func = _isUbxMessage,     .type = PARSER_MSGTYPE_UBX,     .name = "UBX"     },
    { .func = _isNmeaMessage,    .type = PARSER_MSGTYPE_NMEA,    .name = "NMEA"    },
    { .func = _isRtcm3Message,   .type = PARSER_MSGTYPE_RTCM3,   .name = "RTCM3"   },
    { .func = _isSpartnMessage,  .type = PARSER_MSGTYPE_SPARTN,  .name = "SPARTN"  },
    { .func = _isNovatelMessage, .type = PARSER_MSGTYPE_NOVATEL, .name = "NOVATEL" },
};

bool parserProcess(PARSER_t *parser, PARSER_MSG_t *msg, const bool info)
{
    while (parser->size > 0)
    {
        // Run parser functions
        int msgSize = 0;
        PARSER_MSGTYPE_t msgType = PARSER_MSGTYPE_GARBAGE;
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
                _emitMessage(parser, msg, msgSize, msgType, info);
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

bool parserFlush(PARSER_t *parser, PARSER_MSG_t *msg)
{
    const int rem = parser->offs + parser->size;
    if (rem > 0)
    {
        parser->offs += parser->size;
        _emitGarbage(parser, msg);
        parser->size = 0;
        return true;
    }
    else
    {
        return false;
    }
}

/* ****************************************************************************************************************** */

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
    parser->nMsgs++;
    parser->sMsgs += size;
    parser->nGarbage++;
    parser->sGarbage += size;

    // Make message
    msg->type = PARSER_MSGTYPE_GARBAGE;
    msg->size = size;
    msg->data = parser->tmp;
    msg->seq  = parser->nMsgs;
    msg->ts   = now;
    msg->src  = PARSER_MSGSRC_UNKN;
    msg->name = "GARBAGE";
    msg->info = NULL;

    PARSER_XTRA_TRACE("process: emit %s, size %d ", msg->name, size);
}

static void _emitMessage(PARSER_t *parser, PARSER_MSG_t *msg, const int msgSize, const PARSER_MSGTYPE_t msgType, const bool info)
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
    parser->sMsgs += msgSize;
    parser->nMsgs++;
    // Make message
    msg->type = msgType;
    msg->size = msgSize;
    msg->data = parser->tmp;
    msg->seq  = parser->nMsgs;
    msg->ts   = now;
    msg->src  = PARSER_MSGSRC_UNKN;
    parser->name[0] = '\0';
    parser->info[0] = '\0';
    switch (msgType)
    {
        case PARSER_MSGTYPE_UBX:
            parser->nUbx++;
            parser->sUbx += msgSize;
            msg->name = (ubxMessageName(parser->name, sizeof(parser->name), parser->tmp, msgSize) ?
                parser->name : "UBX-?-?");
            if (info)
            {
                msg->info = (ubxMessageInfo(parser->info, sizeof(parser->info), parser->tmp, msgSize) ?
                    parser->info : NULL);
            }
            break;
        case PARSER_MSGTYPE_NMEA:
            parser->nNmea++;
            parser->sNmea += msgSize;
            msg->name = (nmeaMessageName(parser->name, sizeof(parser->name), parser->tmp, msgSize) ?
                parser->name : "NMEA-?-?");
            if (info)
            {
                msg->info = (nmeaMessageInfo(parser->info, sizeof(parser->info), parser->tmp, msgSize) ?
                    parser->info : NULL);
            }
            break;
        case PARSER_MSGTYPE_RTCM3:
            parser->nRtcm3++;
            parser->sRtcm3 += msgSize;
            msg->name = (rtcm3MessageName(parser->name, sizeof(parser->name), parser->tmp, msgSize) ?
                parser->name : "RTCM3-?");
            if (info)
            {
                msg->info = (rtcm3MessageInfo(parser->info, sizeof(parser->info), parser->tmp, msgSize) ?
                    parser->info : NULL);
            }
            break;
        case PARSER_MSGTYPE_SPARTN:
            parser->nSpartn++;
            parser->sSpartn += msgSize;
            msg->name = (spartnMessageName(parser->name, sizeof(parser->name), parser->tmp, msgSize) ?
                parser->name : "SPARTN-?");
            if (info)
            {
                msg->info = (spartnMessageInfo(parser->info, sizeof(parser->info), parser->tmp, msgSize) ?
                    parser->info : NULL);
            }
            break;
        case PARSER_MSGTYPE_NOVATEL:
            parser->nNovatel++;
            parser->sNovatel += msgSize;
            msg->name = (novatelMessageName(parser->name, sizeof(parser->name), parser->tmp, msgSize) ?
                parser->name : "NOVATEL-?");
            if (info)
            {
                msg->info = (novatelMessageInfo(parser->info, sizeof(parser->info), parser->tmp, msgSize) ?
                    parser->info : NULL);
            }
            break;
        default:
            msg->name = "?";
            msg->info = NULL;
            break;
    }
    PARSER_XTRA_TRACE("process: emit %s, size %d, type %d ", msg->name, msgSize, msgType);
}

// ---------------------------------------------------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------------------------------------------------

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
    const uint32_t crc = ((uint32_t)buf[msgSize - 3] << 16) | ((uint32_t)buf[msgSize - 2] << 8) | ((uint32_t)buf[msgSize - 1]);
    if (crc == crcRtcm3(&buf[0], msgSize - 3))
    {
        return msgSize;
    }

    return 0;
}

// ---------------------------------------------------------------------------------------------------------------------

static int _isSpartnMessage(const uint8_t *buf, const int size)
{
    // Not RTCM3 preamble?
    if (buf[0] != SPARTN_PREAMBLE)
    {
        return 0;
    }

    // Wait for enough data to look at header
    if (size < SPARTN_MIN_HEAD_SIZE)
    {
        return -1;
    }

    // byte 0    0b11111111   8 bits  preamble
    // byte 1    0b1111111.   7 bits  message type
    //const int msgType = (buf[1] & 0xfe) >> 1;
    //           0b.......1 .
    // byte 2    0b11111111  10 bits  payload length
    // byte 3    0b1....... .
    const int payloadSize = ((uint16_t)(buf[1] & 0x01) << 9) | ((uint16_t)buf[2] << 1) | (((uint16_t)buf[3] & 0x80) >> 7);
    //           0b.1......   1 bit   encryption and authentication flag
    const int encAuthFlag = (buf[3] & 0x40) >> 6;
    //           0b..11....   2 bits  crc type
    const int crcType = (buf[3] & 0x30) >> 4;
    //           0b....1111   4 bits  frame crc
    const uint32_t frameCrc = (buf[3] & 0x0f);
    uint8_t tmp[3] = { buf[1], buf[2], buf[3] & 0xf0 };
    if (frameCrc != crcSpartn4(tmp, sizeof(tmp)))
    {
        //WARNING("SPARTN frame crc fail");
        return 0;
    }
    // byte 4    0b1111....   4 bits  message subtype
    //const int msgSubType = (buf[4] & 0xf0) >> 4;
    //           0b....1...   1 bit   time tag type
    const int timeTagType = (buf[4] & 0x08) >> 3;
    //           0b.....111 .
    // byte 5    0b11111111  16 bits  gnss time tag
    //         6 0b11111111  or 32
    //         7 0b11111111
    // byte 6  8 0b11111... .
    //           0b.....111 .
    // byte 7  9 0b1111.... . 7 bits  solution id
    //           0b....1111   4 bits  slution processor id
    int msgSize = (timeTagType ? 10 : 8);
    if (size < msgSize)
    {
        return -1;
    }
    // if ea flag is set
    // byte s    0b1111....   4 bits  encryption id
    // byte s+1  0b....1111 .
    //           0b11...... - 6 bits  encryption sequence number
    //           0b..111... - 3 bits  authentication indicator
    const int authInd = (buf[msgSize + 1] & 38) >> 3;
    //           0b.....111 - 3 bits  embedded authentication length
    int eaLength = -1;
    if (encAuthFlag)
    {
        if (authInd > 1)
        {
            /*const int*/ eaLength = (buf[msgSize + 1] & 0x07);
            const int eaLengths[8] = { 8, 12, 16, 32, 64, 0, 0, 0};
            msgSize += eaLengths[eaLength];
        }
        msgSize += 2;
    }
    // byte s+2...  payload
    msgSize += payloadSize;
    // message crc
    const int crcSizes[4] = { 1, 2, 3, 4 };
    msgSize += crcSizes[crcType];

    if (size < msgSize)
    {
        return -1;
    }

    //DEBUG("msgType=%d payloadSize=%d encAuthFlag=%d authInd=%d eaLength=%d crcType=%d frameCrc=%u (%u) msgSubType=%d timeTagType=%d msgSize=%d",
    //    msgType, payloadSize, encAuthFlag, authInd, eaLength, crcType, frameCrc, crcSpartn4(tmp, sizeof(tmp)), msgSubType, timeTagType, msgSize);

    uint32_t msgCrc = 0;
    uint32_t chkCrc = 0;
    switch (crcType)
    {
        case 0: // untested
            msgCrc = (uint32_t)buf[msgSize - 1];
            chkCrc = crcSpartn8(&buf[1], msgSize - 1 - crcSizes[crcType]);
            break;
        case 1: // untested
            msgCrc = ((uint32_t)buf[msgSize - 2] << 8) | ((uint32_t)buf[msgSize - 1]);
            chkCrc = crcSpartn16(&buf[1], msgSize - 1 - crcSizes[crcType]);
            break;
        case 2: // OK
            msgCrc = ((uint32_t)buf[msgSize - 3] << 16) | ((uint32_t)buf[msgSize - 2] << 8) | ((uint32_t)buf[msgSize - 1]);
            chkCrc = crcSpartn24(&buf[1], msgSize - 1 - crcSizes[crcType]);
            break;
        case 3: // untested
            msgCrc = ((uint32_t)buf[msgSize - 4] << 24) | ((uint32_t)buf[msgSize - 3] << 16) | ((uint32_t)buf[msgSize - 2] << 8) | ((uint32_t)buf[msgSize - 1]);
            chkCrc = crcSpartn32(&buf[1], msgSize - 1 - crcSizes[crcType]);
            break;
    }

    if (msgCrc != chkCrc)
    {
        //WARNING("SPARTN crc fail");
        return 0;
    }

    //DEBUG("crcType=%d size=%d 0x%08x 0x%08x %s", crcType, crcSizes[crcType], msgCrc, chkCrc, msgCrc == chkCrc ? "OK" : ":-(");

    return msgSize;
}

// ---------------------------------------------------------------------------------------------------------------------

static int _isNovatelMessage(const uint8_t *buf, const int size)
{
    if (buf[0] != NOVATEL_SYNC_1)
    {
        return 0;
    }

    if (size < 3)
    {
        return -1;
    }

    if ( (buf[1] != NOVATEL_SYNC_2) || ( (buf[2] != NOVATEL_SYNC_3_LONG) && (buf[2] != NOVATEL_SYNC_3_SHORT) ) )
    {
        return 0;
    }

    // https://docs.novatel.com/OEM7/Content/Messages/Binary.htm
    // https://docs.novatel.com/OEM7/Content/Messages/Description_of_Short_Headers.htm
    // Offset  Type      Long header   Short header
    // 0       uint8_t   0xaa         uint8_t   0xaa
    // 1       uint8_t   0x44         uint8_t   0x44
    // 2       uint8_t   0x12         uint8_t   0x13
    // 3       uint8_t   header len   uint8_t   msg len
    // 4       uint16_t  msg ID       uint16_t  msg ID
    // 6       uint8_t   msg type     uint16_t  wno
    // 7       uint8_t   port addr
    // 8       uint16_t  msg len      int32_t   tow
    // ...
    // 3+hlen  payload             offs 13 payload

    if (size < 12)
    {
        return -1;
    }

    int len = 0;

    // Long header
    if (buf[2] == NOVATEL_SYNC_3_LONG)
    {
        const uint8_t headerLen = buf[3];
        const uint16_t msgLen = ((uint16_t)buf[9] << 8)| (uint16_t)buf[8];
        len = headerLen + msgLen + sizeof(uint32_t);
    }
    // Short header
    else
    {
        const uint8_t headerLen = 12;
        const uint8_t msgLen = buf[3];
        len = headerLen + msgLen + sizeof(uint32_t);
    }

    if (len > PARSER_MAX_NOVATEL_SIZE)
    {
        return 0;
    }

    if (size < len)
    {
        return -1;
    }

    const uint32_t crc = (buf[len - 1] << 24) | (buf[len - 2] << 16) | (buf[len - 3] << 8) | (buf[len - 4]);
    if (crc == crcNovatel32(buf, len - sizeof(uint32_t)))
    {
        return len;
    }
    else
    {
        return 0;
    }
}

/* ****************************************************************************************************************** */
// eof
