// flipflip's cfggui
//
// Copyright (c) 2021 Philippe Kehl (flipflip at oinkzwurgl dot org),
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

#ifndef __DATA_H__
#define __DATA_H__

#include <memory>
#include <string>

#include "ubloxcfg.h"
#include "ff_parser.h"
#include "ff_epoch.h"
#include "ff_cpp.h"
#include "ff_rx.h"

#include "data.hpp"

/* ****************************************************************************************************************** */

struct Data
{
    enum class Type { DATA_MSG, DATA_EPOCH, DATA_KEYVAL, INFO_NOTICE, INFO_WARN, INFO_ERROR, EVENT_START, EVENT_STOP, ACK };
    Data(Type _type) :
        type{_type}, uid{0}, msg{nullptr}, epoch{nullptr}, keyval{nullptr}, info{nullptr}, ack{false}
    {}
    Data(std::shared_ptr<Ff::ParserMsg> _msg, const uint64_t _uid) :
        type{Type::DATA_MSG}, uid{_uid}, msg{_msg}, epoch{nullptr}, keyval{nullptr}, info{nullptr}, ack{false}
    {}
    Data(std::shared_ptr<Ff::Epoch> _epoch, const uint64_t _uid) :
        type{Type::DATA_EPOCH}, uid{_uid}, msg{nullptr}, epoch{_epoch}, keyval{nullptr}, info{nullptr}, ack{false}
    {}
    Data(std::shared_ptr<Ff::KeyVal> _keyval, const uint64_t _uid) :
        type{Type::DATA_KEYVAL}, uid{_uid}, msg{nullptr}, epoch{nullptr}, keyval{_keyval}, info{nullptr}, ack{false}
    {}
    Data(const bool _ack, const uint64_t _uid) :
        type{Type::ACK}, uid{_uid}, msg{nullptr}, epoch{nullptr}, keyval{nullptr}, info{nullptr}, ack{_ack}
    {}
    Data(Type _type, const std::string &_info, const uint64_t _uid) :
        type{_type}, uid{_uid}, msg{nullptr}, epoch{nullptr}, keyval{nullptr}, info{ std::make_shared<std::string>(_info) }, ack{false}
    {}
   ~Data()
    {}
    Type                           type;
    uint64_t                       uid;
    std::shared_ptr<Ff::ParserMsg> msg;    // DATA_MSG
    std::shared_ptr<Ff::Epoch>     epoch;  // DATA_EPOCH
    std::shared_ptr<Ff::KeyVal>    keyval; // DATA_KEYVAL
    std::shared_ptr<std::string>   info;   // INFO_*
    bool                           ack;    // ACK
};

/* ****************************************************************************************************************** */
#endif // __DATA_H__
