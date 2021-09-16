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

#ifndef __GUI_WIN_DATA_H__
#define __GUI_WIN_DATA_H__

#include <cinttypes>

#include "gui_win.hpp"

#include "receiver.hpp"
#include "logfile.hpp"
#include "database.hpp"

/* ***** Receiver data window base class **************************************************************************** */

class GuiWinData : public GuiWin
{
    public:
        GuiWinData() {}
        GuiWinData(const std::string &name,
            std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile, std::shared_ptr<Database> database);
        virtual ~GuiWinData() {}

        virtual void         Loop(const uint32_t &frame, const double &now);
        virtual void         ProcessData(const Data &data);
        virtual void         ClearData();
        virtual void         DrawWindow();

    protected:

        std::shared_ptr<Receiver> _receiver;  // either we get data from a receiver
        std::shared_ptr<Logfile>  _logfile;   // ..or from a logfile
        std::shared_ptr<Database> _database;
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_H__
