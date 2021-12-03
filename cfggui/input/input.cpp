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

#include <chrono>

#include "ff_debug.h"

#include "platform.hpp"

#include "input.hpp"

/* ****************************************************************************************************************** */

Input::Input(const std::string &name, std::shared_ptr<Database> database) :
    _inputName       { name },
    _inputDatabase   { database }
{
    DEBUG("Input(%s)", _inputName.c_str());
}

// ---------------------------------------------------------------------------------------------------------------------

Input::~Input()
{
    DEBUG("~Input(%s)", _inputName.c_str());
    _ThreadStop();
    _inputDataCb = nullptr;
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
}

// ---------------------------------------------------------------------------------------------------------------------

bool Input::_HaveDataCb()
{
    return _inputDataCb ? true : false;
}

// ---------------------------------------------------------------------------------------------------------------------

bool Input::_ThreadStart()
{
    bool res = true;
    _ThreadStop();
    try
    {
        _inputThreadAbort = false;
        _inputThread = std::make_unique<std::thread>(&Input::_ThreadWrapper, this);
        res = true;
    }
    catch (std::exception &e)
    {
        WARNING("Input(%s) thread fail: %s", _inputName.c_str(), e.what());
    }
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

void Input::_ThreadStop()
{
    if ( _inputThread && (std::this_thread::get_id() != _inputThread->get_id()) )
    {
        _inputThreadAbort = true;
        _ThreadWakeup();
        _inputThread->join();
        _inputThread = nullptr;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool Input::_ThreadSleep(const uint32_t millis)
{
    // Sleep, but wake up early in case we're notified
    // FIXME: study this stuff further, this may not be fully right
    std::unique_lock<std::mutex> lock(_inputThreadMutex);
    const bool intr = _inputThreadCondVar.wait_for(lock, std::chrono::milliseconds(millis)) != std::cv_status::timeout;

    return intr;
}

// ---------------------------------------------------------------------------------------------------------------------

void Input::_ThreadWakeup()
{
    _inputThreadCondVar.notify_all();
}

// ---------------------------------------------------------------------------------------------------------------------

bool Input::_ThreadAbort()
{
    return _inputThreadAbort;
}

// ---------------------------------------------------------------------------------------------------------------------

void Input::_ThreadWrapper()
{
    Platform::SetThreadName(_inputName);
    DEBUG("Input(%s) thread start", _inputName.c_str());

    _ThreadPrepare();
    _CallDataCb( InputData(InputData::EVENT_START) );

    try
    {
        _Thread();
    }
    catch (const std::exception &e)
    {
        ERROR("Input(%s) thread crash: %s", _inputName.c_str(), e.what());
    }

    _inputThreadAbort = true;

    _CallDataCb( InputData(InputData::EVENT_STOP) );
    _ThreadCleanup();

    DEBUG("Input(%s) thread stop", _inputName.c_str());
}

/* ****************************************************************************************************************** */
