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

#ifndef __GUI_WIN_DATA_MESSAGEs_H__
#define __GUI_WIN_DATA_MESSAGES_H__

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <cstdint>

#include "gui_win_data.hpp"

/* ***** Receiver messages ****************************************************************************************** */

class GuiWinDataMessages : public GuiWinData
{
    public:
        GuiWinDataMessages(const std::string &name,
            std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile, std::shared_ptr<Database> database);

      //void                 Loop(const uint32_t &frame, const double &now) override;
        void                 ProcessData(const Data &data) override;
        void                 ClearData() override;
        void                 DrawWindow() override;

    protected:
        bool                 _showSubSec;
        struct MsgInfo
        {
            MsgInfo(const std::shared_ptr<Ff::ParserMsg> &_msg) : msg{_msg}, count{1}, dt{}, dtIx{0}, rate{0.0} {}
            void Update(const std::shared_ptr<Ff::ParserMsg> &_msg);
            std::shared_ptr<Ff::ParserMsg> msg;
            uint32_t count;
            uint32_t dt[10];
            int      dtIx;
            float    rate;
        };
        std::map< std::string, MsgInfo > _messages;
        std::map< std::string, MsgInfo >::iterator _selectedEntry;
        std::map< std::string, MsgInfo >::iterator _displayedEntry;
        std::string                      _hexDump;
        void                             _UpdateHexdump();
        std::vector<std::string>         _classNames;
        void                             _SetRate(const UBLOXCFG_MSGRATE_t *def, const uint8_t rate);
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_MESSAGES_H__
