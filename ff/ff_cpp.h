// flipflip's c++ stuff for flipflip's c stuff
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

#ifndef __FF_CPP_H__
#define __FF_CPP_H__

#include <string>
#include <cstring>

#include "ff_parser.h"
#include "ff_epoch.h"

namespace Ff
{
    // PARSER_MSG_t
    struct ParserMsg
    {
        ParserMsg(const PARSER_MSG_t *_msg) :
            type{}, data{}, size{_msg->size}, seq{_msg->seq}, ts{_msg->ts}, name{}, info{}
        {
            switch (_msg->type)
            {
                case PARSER_MSGTYPE_UBX:     type = UBX;     break;
                case PARSER_MSGTYPE_NMEA:    type = NMEA;    break;
                case PARSER_MSGTYPE_RTCM3:   type = RTCM3;   break;
                case PARSER_MSGTYPE_GARBAGE: type = GARBAGE; break;
            }
            data = new uint8_t [size];
            memcpy(data, _msg->data, size);
            name = _msg->name;
            if (_msg->info != NULL)
            {
                info = _msg->info;
            }
        }
        ~ParserMsg()
        {
            delete data;
        }

        enum Type_e { UBX, NMEA, RTCM3, GARBAGE };
        enum Type_e type;
        uint8_t    *data;
        int         size;
        uint32_t    seq;
        uint32_t    ts;
        std::string name;
        std::string info;
    };

    // EPOCH_t
    struct Epoch
    {
        Epoch(const EPOCH_t *_epoch) : seq{_epoch->seq}, str{_epoch->str}, epoch{*_epoch} { }
        uint32_t    seq;
        std::string str;
        EPOCH_t     epoch;
    };

};

#endif

