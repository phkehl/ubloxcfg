// flipflip's c++ stuff: c++ wrappers for flipflip's c stuff
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

#ifndef __FF_CPP_H__
#define __FF_CPP_H__

#include <string>
#include <vector>
#include <cstdint>

#include "ff_parser.h"
#include "ff_epoch.h"
#include "ff_rx.h"
#include "ff_stuff.h"

/* ****************************************************************************************************************** */

namespace Ff
{
    // PARSER_MSG_t
    struct ParserMsg
    {
        ParserMsg(const PARSER_MSG_t *_msg);

        enum Type_e { UBX, NMEA, RTCM3, SPARTN, NOVATEL, GARBAGE };
        enum Type_e type;
        std::string typeStr;
        uint8_t     data[PARSER_MAX_ANY_SIZE];
        int         size;
        uint32_t    seq;
        uint32_t    ts;
        enum Src_e { UNKN, FROM_RX, TO_RX, VIRTUAL, USER, LOG };
        Src_e       src;
        std::string srcStr;
        std::string name;
        std::string info;
    };

    // EPOCH_t
    struct Epoch
    {
        Epoch(const EPOCH_t *_epoch);
        uint32_t    seq;
        std::string str;
        EPOCH_t     epoch;
    };

    // RX_t
    struct Rx
    {
        Rx(const std::string &port, const RX_OPTS_t *opts);
       ~Rx();
        void Abort();
        RX_t *rx;
    };

    // UBLOXCFG_KEYVAL_t
    struct KeyVal
    {
        KeyVal(const UBLOXCFG_LAYER_t _layer, const UBLOXCFG_KEYVAL_t *_kv, const int numKv);
        UBLOXCFG_LAYER_t               layer;
        std::vector<UBLOXCFG_KEYVAL_t> kv;
    };

    // UBX message
    struct UbxMessage
    {
        UbxMessage(const uint8_t clsId, const uint8_t msgId, const std::string &payload, const bool addNul = true);
        UbxMessage(const uint8_t clsId, const uint8_t msgId, const std::vector<uint8_t> &payload);
        UbxMessage(const uint8_t clsId, const uint8_t msgId, const uint8_t *payload, const int payloadSize);
        std::vector<uint8_t> raw;
    };
};

/* ****************************************************************************************************************** */

#endif // __FF_CPP_H__
