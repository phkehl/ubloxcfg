// flipflip's c++ stuff: thread
//
// Copyright (c) 2021 Philippe Kehl (flipflip at oinkzwurgl dot org),
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

#include <cstdarg>
#include <algorithm>
#include <cinttypes>
#include <cstring>
#include <unordered_map>

#include <sys/prctl.h>

#include "ff_debug.h"

#include "ff_thread.hpp"

/* ****************************************************************************************************************** */

Ff::Thread::Thread(const std::string &name, Ff::Thread::ThreadFunc_t func, void *arg,
        Ff::Thread::PrepFunc_t prep, Ff::Thread::CleanFunc_t clean) :
    _name  { name },
    _func  { func },
    _arg   { arg },
    _prep  { prep },
    _clean { clean }
{
    DEBUG("Ff::Thread(%s)", _name.c_str());
}

Ff::Thread::~Thread()
{
    DEBUG("Ff::~Thread(%s)", _name.c_str());
    Stop();
}

// ---------------------------------------------------------------------------------------------------------------------

bool Ff::Thread::Start()
{
    bool res = true;
    Stop();
    try
    {
        _abort = false;
        _thread = std::make_unique<std::thread>(&Ff::Thread::_Thread, this);
        res = true;
    }
    catch (std::exception &e)
    {
        WARNING("%s thread fail: %s", _name.c_str(), e.what());
    }
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

void Ff::Thread::Stop()
{
    if ( _thread && (std::this_thread::get_id() != _thread->get_id()) )
    {
        _abort = true;
        Wakeup();
        _thread->join();
        _thread = nullptr;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool Ff::Thread::IsRunning()
{
    return _thread ? true : false;
}

// ---------------------------------------------------------------------------------------------------------------------

void Ff::Thread::Wakeup()
{
    _cond.notify_all();
}

// ---------------------------------------------------------------------------------------------------------------------

bool Ff::Thread::Sleep(const uint32_t millis)
{
    // Sleep, but wake up early in case we're notified
    // FIXME: study this stuff further, this may not be fully right
    std::unique_lock<std::mutex> lock(_mutex);
    return _cond.wait_for(lock, std::chrono::milliseconds(millis)) != std::cv_status::timeout;
}

// ---------------------------------------------------------------------------------------------------------------------

bool Ff::Thread::ShouldAbort()
{
    return _abort;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::string &Ff::Thread::GetName()
{
    return _name;
}

// ---------------------------------------------------------------------------------------------------------------------

void Ff::Thread::_Thread()
{
    {
        char currName[16];
        char threadName[32]; // max 16 cf. prctl(2)
        currName[0] = '\0';
        if (prctl(PR_GET_NAME, currName, 0, 0, 0) == 0)
        {
            currName[7] = '\0';
            std::snprintf(threadName, sizeof(threadName), "%s:%s", currName, _name.c_str());
            prctl(PR_SET_NAME, threadName, 0, 0, 0);
        }
    }

    DEBUG("%s thread start", _name.c_str());

    if (_prep)
    {
        _prep();
    }

    try
    {
        _func(this, _arg);
    }
    catch (const std::exception &e)
    {
        ERROR("%s thread crash: %s", _name.c_str(), e.what());
    }

    if (_clean)
    {
        _clean();
    }

    _abort = true;

    DEBUG("%s thread stop", _name.c_str());
}

/* ****************************************************************************************************************** */
