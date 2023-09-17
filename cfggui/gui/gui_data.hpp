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

#ifndef __GUI_DATA_HPP__
#define __GUI_DATA_HPP__

#include <memory>
#include <vector>
#include <mutex>
#include <memory>
#include <cstdint>

#include "database.hpp"
#include "input_receiver.hpp"
#include "input_logfile.hpp"

/* ****************************************************************************************************************** */

class GuiData
{
    public:

        // Control API

        static void AddReceiver(std::shared_ptr<InputReceiver> receiver);
        static void RemoveReceiver(std::shared_ptr<InputReceiver> receiver);
        static void AddLogfile(std::shared_ptr<InputLogfile> logfile);
        static void RemoveLogfile(std::shared_ptr<InputLogfile> logfile);

        // User API

        using ReceiverList = std::vector< std::shared_ptr<InputReceiver> >;
        static ReceiverList Receivers();

        using LogfileList = std::vector< std::shared_ptr<InputLogfile> >;
        static LogfileList Logfiles();

        using InputList = std::vector< std::shared_ptr<Input> >;
        static InputList Inputs();

        using DatabaseList = std::vector< std::shared_ptr<Database> >;
        static DatabaseList Databases();

#if 0
        static void AddTrigger(std::shared_ptr<bool> trigger);
        static void RemoveTrigger(std::shared_ptr<bool> trigger);
#endif

        static uint32_t serial; // increments whenever one of the lists change

    private:

        static std::mutex   _mutex;
        static ReceiverList _receivers;
        static LogfileList  _logfiles;
        static InputList    _inputs;
        static DatabaseList _databases;

#if 0
        static std::shared_ptr<bool> _triggers;
#endif

        static void _AddDatabase(std::shared_ptr<Database> database);
        static void _RemoveDatabase(std::shared_ptr<Database> database);
        static void _Update();
};

/* ****************************************************************************************************************** */

#endif // __GUI_DATA_HPP__
