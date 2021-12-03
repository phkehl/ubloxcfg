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

#ifndef __GUI_WIN_DATA_HPP__
#define __GUI_WIN_DATA_HPP__

#include <cinttypes>

#include "gui_win.hpp"

#include "input_data.hpp"
#include "input_receiver.hpp"
#include "input_logfile.hpp"
#include "database.hpp"

/* ***** Receiver data window base class **************************************************************************** */

class GuiWinData : public GuiWin
{
    public:
        GuiWinData(const std::string &name, std::shared_ptr<Database> database);
        virtual ~GuiWinData();

        void Loop(const uint32_t &frame, const double &now) final;
        void DrawWindow() final;

        void ProcessData(const InputData &data);
        void ClearData();

        void SetReceiver(std::shared_ptr<InputReceiver> receiver);
        void SetLogfile(std::shared_ptr<InputLogfile> logfile);

    protected:

        std::shared_ptr<InputReceiver> _receiver;  // either we get data from a receiver
        std::shared_ptr<InputLogfile>  _logfile;   // ..or from a logfile
        std::shared_ptr<Database> _database;

        // Common functionality
        bool                       _toolbarEna;               // Use common buttons toolbar?
        bool                       _latestEpochEna;           // Use latest epoch functionality?
        std::shared_ptr<Ff::Epoch> _latestEpoch;              // Latest epoch (or nullptr if n/a or expired)
        double                     _latestEpochTs;            // Latest epoch timestamp
        double                     _latestEpochAge;           // Age of latest epoch
        static constexpr double    _latestEpochExpire = 5.0;  // Max age to keep epoch (if _receiver)

        // Specific implementation
        virtual void _Loop(const uint32_t &frame, const double &now);
        virtual void _ProcessData(const InputData &data);
        virtual void _DrawToolbar();
        virtual void _DrawContent();
        virtual void _ClearData();
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_HPP__
