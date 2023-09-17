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

#ifndef __INPUT_HPP__
#define __INPUT_HPP__

#include <memory>
#include <string>

#include "ff_thread.hpp"

#include "input_data.hpp"
#include "database.hpp"

/* ****************************************************************************************************************** */

struct InputEvent;
struct InputCommand;

// Common helpers for inputs (receiver, logfile)
class Input
{
    public:
        Input(const std::string &name, std::shared_ptr<Database> database);
        virtual ~Input();

        virtual void Loop(const double &now) = 0;
        void SetDataCb(std::function<void(const InputData &)> cb);

        const std::string &GetName();
        const std::string &GetRxVer();
        const std::shared_ptr<Database> &GetDatabase();
        const Database::Row &LatestRow();

    protected:

        std::string                  _inputName;           // Name, for debugging
        std::shared_ptr<Database>    _inputDatabase;       // Database
        std::string                  _inputRxVer;          // Receiver version (if known)
        Database::Row                _latestRow;           // Latest database row

        bool _ThreadStart();                               // Start input thread
        void _ThreadStop();                                // Stop input thread
        virtual void _ThreadPrepare() = 0;                 // Called before Thread()
        virtual void _Thread(Ff::Thread *thread) = 0;      // Input thread, must abort if thread->ShouldAbort() says so
        virtual void _ThreadCleanup() = 0;                 // Called after Thread()
        void _ThreadWakeup();                              // Wake up sleeping thread

        std::function<void(const InputData &)> _inputDataCb;
        void _CallDataCb(const InputData &data);
        bool _HaveDataCb();

    private:

        Ff::Thread _inputThread;
        void _ThreadPrep();
        void _ThreadClean();
};

/* ****************************************************************************************************************** */
#endif // __INPUT_HPP__
