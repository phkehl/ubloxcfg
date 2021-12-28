/* ************************************************************************************************/ // clang-format off
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

#ifndef __INPUT_DATA_HPP__
#define __INPUT_DATA_HPP__

#include <memory>
#include <string>

#include "ubloxcfg.h"
#include "ff_parser.h"
#include "ff_epoch.h"
#include "ff_cpp.hpp"
#include "ff_rx.h"

/* ****************************************************************************************************************** */

struct InputData
{
    enum Type { EVENT_START, EVENT_STOP, DATA_MSG, DATA_EPOCH, RXVERSTR, INFO_NOTICE, INFO_WARNING, INFO_ERROR };
    InputData(enum Type _type) :
        type{_type}
    {}
    InputData(Ff::ParserMsg _msg) :
        type{Type::DATA_MSG}, msg{std::make_shared<Ff::ParserMsg>(_msg)}
    {}
    InputData(Ff::Epoch _epoch) :
        type{Type::DATA_EPOCH}, epoch{std::make_shared<Ff::Epoch>(_epoch)}
    {}
    InputData(enum Type _type, const std::string _info) :
        type{_type}, info{ _info }
    {}
   ~InputData()
    { }
    enum Type                      type;   // all
    std::shared_ptr<Ff::ParserMsg> msg;    // only DATA_MSG
    std::shared_ptr<Ff::Epoch>     epoch;  // only DATA_EPOCH
    std::string                    info;   // only INFO_*, RXVERSTR
};

/* ****************************************************************************************************************** */
#endif // __INPUT_DATA_HPP__
