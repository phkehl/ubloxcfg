// flipflip's c++ stuff: thread
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

#ifndef __FF_THREAD_HPP__
#define __FF_THREAD_HPP__

#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <functional>

/* ****************************************************************************************************************** */

namespace Ff
{

    class Thread
    {
        public:

            using ThreadFunc_t = std::function<void(Thread *, void *)>;
            using PrepFunc_t   = std::function<void()>;
            using CleanFunc_t  = std::function<void()>;

            Thread(const std::string &name, ThreadFunc_t func, void *arg = nullptr,
                PrepFunc_t prep = nullptr, CleanFunc_t clean = nullptr);
           ~Thread();

            // Main thread methods
            bool Start();                           // Start thread
            void Stop();                            // Stop thread
            void Wakeup();                          // Wake up sleeping thread
            bool IsRunning();                       // Check if thread is running

            // Thread methods
            bool Sleep(const uint32_t millis);      // Sleep, return true if interrupted (woken up)
            bool ShouldAbort();                     // Check if should abort (return)
            const std::string &GetName();           // Thread name

        private:
            std::string                  _name;     // Thread name
            std::unique_ptr<std::thread> _thread;   // Thread handle
            ThreadFunc_t                 _func;     // Thread function
            void                        *_arg;      // Thread function argument
            PrepFunc_t                   _prep;     // Thread prepare function
            CleanFunc_t                  _clean;    // Thread cleanup function
            std::atomic<bool>            _abort;    // Abort signal
            std::mutex                   _mutex;    // Fake a semaphore to wake up...
            std::condition_variable      _cond;     // ... a sleeping thread
            void _Thread();
    };

};

/* ****************************************************************************************************************** */

#endif // __FF_THREAD_HPP__
