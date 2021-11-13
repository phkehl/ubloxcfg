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

#ifndef __GUI_WIN_DATA_MESSAGEs_H__
#define __GUI_WIN_DATA_MESSAGES_H__

#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <cstdint>

#include "gui_msg.hpp"

#include "gui_win_data.hpp"

/* ***** Receiver messages ****************************************************************************************** */

class GuiWinDataMessages : public GuiWinData
{
    public:
        GuiWinDataMessages(const std::string &name, std::shared_ptr<Database> database);
        ~GuiWinDataMessages();

        void Loop(const uint32_t &frame, const double &now) final;
        void ProcessData(const Data &data) final;
        void ClearData() final;
        void DrawWindow() final;

    protected:
        bool _showList;
        bool _showSubSec;
        bool _showHexDump;

        struct MsgInfo
        {
            MsgInfo(std::unique_ptr<GuiMsg> _renderer);
            void Clear();
            void Update(const std::shared_ptr<Ff::ParserMsg> &_msg);
            std::shared_ptr<Ff::ParserMsg> msg;
            uint32_t count;
            uint32_t dt[30];
            int      dtIx;
            float    rate;
            float    age;
            bool     flag;
            std::vector<std::string> hexdump;
            std::unique_ptr<GuiMsg> renderer;
        };

        using msgsMap_t = std::map< std::string, MsgInfo >;

        msgsMap_t           _messages;
        msgsMap_t::iterator _selectedEntry;
        std::string         _selectedName;
        msgsMap_t::iterator _displayedEntry;
        double   _nowIm;
        uint32_t _nowTs;
        std::vector<std::string> _classNames;

        struct MsgRate
        {
            MsgRate(const UBLOXCFG_MSGRATE_t *_rate);
            std::string               msgName;
            std::string               menuNameEnabled;
            std::string               menuNameDisabled;
            const UBLOXCFG_MSGRATE_t *rate;
            bool                      haveIds;
            uint8_t                   clsId;
            uint8_t                   msgId;
        };
        std::unordered_map<std::string, std::vector<MsgRate>> _msgRates; // FIXME: make static?
        struct MsgPoll
        {
            MsgPoll(const uint8_t _clsId, const uint8_t _msgId);
            uint8_t clsId;
            uint8_t msgId;
        };
        std::unordered_map<std::string, MsgPoll> _msgPolls;
        void _InitMsgRatesAndPolls();
        void _SetRate(const MsgRate &def, const int rate);
        void _PollMsg(const MsgPoll &def);

        void _UpdateInfo();
        void _DrawMessagesMenu();
        void _DrawList();
        void _DrawListButtons();
        void _DrawMessage();
        void _DrawMessageButtons();

};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_MESSAGES_H__
