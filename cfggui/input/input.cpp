/* ************************************************************************************************/ // clang-format off
// flipflip's cfggui
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

#include <chrono>

#include "ff_debug.h"
#include "ff_ubx.h"

#include "platform.hpp"

#include "input.hpp"

/* ****************************************************************************************************************** */

Input::Input(const std::string &name, std::shared_ptr<Database> database) :
    _inputName       { name },
    _inputDatabase   { database },
    _inputThread     { _inputName, std::bind(&Input::_Thread, this, std::placeholders::_1), nullptr,
                        std::bind(&Input::_ThreadPrep, this), std::bind(&Input::_ThreadClean, this) }
{
    DEBUG("Input(%s)", _inputName.c_str());
}

// ---------------------------------------------------------------------------------------------------------------------

Input::~Input()
{
    DEBUG("~Input(%s)", _inputName.c_str());
    _inputThread.Stop();
    _inputDataCb = nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::string &Input::GetRxVer()
{
    return _inputRxVer;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::string &Input::GetName()
{
    return _inputName;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::shared_ptr<Database> &Input::GetDatabase()
{
    return _inputDatabase;
}

// ---------------------------------------------------------------------------------------------------------------------

void Input::SetDataCb(std::function<void(const InputData &)> cb)
{
    _inputDataCb = cb;
}

// ---------------------------------------------------------------------------------------------------------------------

void Input::_CallDataCb(const InputData &data)
{
    if (_inputDataCb)
    {
        _inputDataCb(data);
    }

    if (data.type == InputData::DATA_MSG)
    {
        if (data.msg->name == "UBX-MON-VER")
        {
            char str[100];
            if (ubxMonVerToVerStr(str, sizeof(str), data.msg->data, data.msg->size))
            {
                _inputRxVer = str;
                if (_inputDataCb)
                {
                    _inputDataCb(InputData(InputData::RXVERSTR, _inputRxVer));
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool Input::_HaveDataCb()
{
    return _inputDataCb ? true : false;
}

// ---------------------------------------------------------------------------------------------------------------------

bool Input::_ThreadStart()
{
    return _inputThread.Start();
}

// ---------------------------------------------------------------------------------------------------------------------

void Input::_ThreadStop()
{
    _inputThread.Stop();
}

// ---------------------------------------------------------------------------------------------------------------------

void Input::_ThreadWakeup()
{
    _inputThread.Wakeup();
}

// ---------------------------------------------------------------------------------------------------------------------

void Input::_ThreadPrep()
{
    _inputRxVer.clear();
    _ThreadPrepare();
    _CallDataCb( InputData(InputData::EVENT_START) );
}

// ---------------------------------------------------------------------------------------------------------------------

void Input::_ThreadClean()
{
    _CallDataCb( InputData(InputData::EVENT_STOP) );
    _ThreadCleanup();
    _inputRxVer.clear();
}

/* ****************************************************************************************************************** */
