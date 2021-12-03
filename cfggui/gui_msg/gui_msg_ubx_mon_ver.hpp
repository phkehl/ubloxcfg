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

#ifndef __GUI_MSG_UBX_MON_VER_HPP__
#define __GUI_MSG_UBX_MON_VER_HPP__

#include <memory>

#include "gui_inc.hpp"

#include "gui_msg.hpp"

/* ***** UBX-MON-VER renderer *************************************************************************************** */

class GuiMsgUbxMonVer : public GuiMsg
{
    public:
        GuiMsgUbxMonVer(std::shared_ptr<InputReceiver> receiver = nullptr, std::shared_ptr<InputLogfile> logfile = nullptr);

        bool Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2 &sizeAvail) final;

    private:
};

/* ****************************************************************************************************************** */
#endif // __GUI_MSG_UBX_MON_VER_HPP__
