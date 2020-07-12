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

// This is a UBX, NMEA and RTCM3 message parser. It only parses the frames, not the content
// of the message (it does not decode any message fields).
// The parser will pass-through all data that is input. Unknown parts (other protocols,
// spurious data, incorrect messages, etc.) are output as GARBAGE type messages. GARBAGE messages
// are not guaranteed to be combined and can be split arbitrarily (into several GARBAGE messages).

#ifndef __FF_PARSER_H__
#define __FF_PARSER_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ********************************************************************************************** */

#define PARSER_BUF_SIZE       8192 // must be >= 2*PARSER_MAX_ANY_SIZE (FIXME: does it?)
#define PARSER_MAX_UBX_SIZE   4096 // messages larger than this will be GARBAGE
#define PARSER_MAX_NMEA_SIZE   200 // messages larger than this will be GARBAGE
#define PARSER_MAX_RTCM3_SIZE 4096 // messages larger than this will be GARBAGE
#define PARSER_MAX_GARB_SIZE   256
#define PARSER_MAX_ANY_SIZE   4096 // the largest of the above
#define PARSER_MAX_NAME_SIZE   100
#define PARSER_MAX_INFO_SIZE  1000

typedef struct PARSER_s
{
    // Parser state, don't mess with this
    uint8_t   buf[PARSER_BUF_SIZE];
    int       size;
    int       offs;
    uint8_t   tmp[PARSER_MAX_ANY_SIZE];
    char      name[PARSER_MAX_NAME_SIZE];
    char      info[PARSER_MAX_INFO_SIZE];
    // Statistics
    uint32_t  msg;
    uint32_t  tot;
} PARSER_t;

typedef enum PARSER_MSGTYPE_e
{
    PARSER_MSGTYPE_GARBAGE,
    PARSER_MSGTYPE_UBX,
    PARSER_MSGTYPE_NMEA,
    PARSER_MSGTYPE_RTCM3
} PARSER_MSGTYPE_t;

typedef enum PARSER_MSGSRC_e
{
    PARSER_MSGSRC_UNKN = 0,
    PARSER_MSGSRC_FROM_RX,
    PARSER_MSGSRC_TO_RX,
    PARSER_MSGSRC_VIRTUAL,
    PARSER_MSGSRC_USER,
    PARSER_MSGSRC_LOG
} PARSER_MSGSRC_t;

typedef struct PARSER_MSG_s
{
    PARSER_MSGTYPE_t type;
    const uint8_t   *data;
    int              size;
    uint32_t         seq;
    uint32_t         ts;
    PARSER_MSGSRC_t  src;
    const char      *name;
    const char      *info; // may be NULL
} PARSER_MSG_t;

void parserInit(PARSER_t *parser);
void parserAdd(PARSER_t *parser, const uint8_t *data, const int size);
bool parserProcess(PARSER_t *parser, PARSER_MSG_t *msg);

const char *parserMsgtypeName(const PARSER_MSGTYPE_t type);

/* ********************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_PARSER_H__
