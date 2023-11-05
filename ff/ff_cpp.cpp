// flipflip's c++ stuff: c++ wrappers for flipflip's c stuff
//
// Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org),
// https://oinkzwurgl.org/hacking/ubloxcfg
//
// Parts copyright by others. See source code.
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

#include <stdexcept>
#include <cstring>

#include "ff_ubx.h"

#include "ff_cpp.hpp"

/* ****************************************************************************************************************** */

Ff::ParserMsg::ParserMsg(const PARSER_MSG_t *_msg) :
    type{}, data{}, size{_msg->size}, seq{_msg->seq}, ts{_msg->ts}, name{_msg->name}, info{}
{
    switch (_msg->type)
    {
        case PARSER_MSGTYPE_UBX:     type = UBX;     typeStr = "UBX";     break;
        case PARSER_MSGTYPE_NMEA:    type = NMEA;    typeStr = "NMEA";    break;
        case PARSER_MSGTYPE_RTCM3:   type = RTCM3;   typeStr = "RTCM3";   break;
        case PARSER_MSGTYPE_SPARTN:  type = SPARTN;  typeStr = "SPARTN";   break;
        case PARSER_MSGTYPE_NOVATEL: type = NOVATEL; typeStr = "NOVATEL"; break;
        case PARSER_MSGTYPE_GARBAGE: type = GARBAGE; typeStr = "GARBAGE"; break;
    }
    switch (_msg->src)
    {
        case PARSER_MSGSRC_UNKN:    src = UNKN;    srcStr = "UNKN";    break;
        case PARSER_MSGSRC_FROM_RX: src = FROM_RX; srcStr = "FROM_RX"; break;
        case PARSER_MSGSRC_TO_RX:   src = TO_RX;   srcStr = "TO_RX";   break;
        case PARSER_MSGSRC_VIRTUAL: src = VIRTUAL; srcStr = "VIRTUAL"; break;
        case PARSER_MSGSRC_USER:    src = USER;    srcStr = "USER";    break;
        case PARSER_MSGSRC_LOG:     src = LOG;     srcStr = "LOG";     break;
    }
    std::memcpy(data, _msg->data, size > PARSER_MAX_ANY_SIZE ? PARSER_MAX_ANY_SIZE : size);
    if (_msg->info != NULL)
    {
        info = _msg->info;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

Ff::Epoch::Epoch(const EPOCH_t *_epoch) :
    seq{_epoch->seq}, str{_epoch->str}, epoch{*_epoch}
{
}

// ---------------------------------------------------------------------------------------------------------------------

Ff::Rx::Rx(const std::string &port, const RX_OPTS_t *opts) :
    rx{NULL}
{
    rx = rxInit(port.c_str(), opts);
    if (rx == NULL)
    {
        throw std::runtime_error("rxInit() fail");
    }
}

Ff::Rx::~Rx()
{
    if (rx != NULL)
    {
        rxClose(rx);
        free(rx);
        rx = NULL;
    }
}

void Ff::Rx::Abort()
{
    if (rx != NULL)
    {
        rxAbort(rx);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

Ff::KeyVal::KeyVal(const UBLOXCFG_LAYER_t _layer, const UBLOXCFG_KEYVAL_t *_kv, const int numKv) :
    layer{_layer}, kv{}
{
    if (numKv > 0)
    {
        kv.resize(numKv);
        std::memcpy(kv.data(), _kv, sizeof(*_kv) * kv.size());
    }
}

// ---------------------------------------------------------------------------------------------------------------------

Ff::UbxMessage::UbxMessage(const uint8_t clsId, const uint8_t msgId, const std::vector<uint8_t> &payload) :
    UbxMessage(clsId, msgId, payload.data(), payload.size())
{
}

Ff::UbxMessage::UbxMessage(const uint8_t clsId, const uint8_t msgId, const std::string &payload, const bool addNul) :
    UbxMessage(clsId, msgId, (const uint8_t *)payload.data(), payload.size() + (addNul ? 1 : 0))
{
}

Ff::UbxMessage::UbxMessage(const uint8_t clsId, const uint8_t msgId, const uint8_t *payload, const int payloadSize)
{
    raw.resize(payloadSize + UBX_FRAME_SIZE);
    ubxMakeMessage(clsId, msgId, payload, payloadSize, raw.data());
}

/* ****************************************************************************************************************** */
