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

#ifndef __GUI_MSG_UBX_MON_RF_H__
#define __GUI_MSG_UBX_MON_RF_H__

#include <memory>
#include <vector>
#include <deque>

#include "imgui.h"
#include "gui_msg.hpp"

#include "gui_msg_ubx_mon_hw2.hpp"

/* ***** UBX-MON-RF renderer **************************************************************************************** */

class GuiMsgUbxMonRf : public GuiMsg
{
    public:
        GuiMsgUbxMonRf(std::shared_ptr<Receiver> receiver = nullptr, std::shared_ptr<Logfile> logfile = nullptr);

        void Update(const std::shared_ptr<Ff::ParserMsg> &msg) final;
        bool Render(const std::shared_ptr<Ff::ParserMsg> &msg, const ImVec2 &sizeAvail) final;
        void Clear();

    protected:

        static constexpr int NUM_IQS = 50;

        std::vector< std::deque<GuiMsgUbxMonHw2::IQ> > _blockIqs;

    private:
};

/* ****************************************************************************************************************** */
#endif // __GUI_MSG_UBX_MON_RF_H__
