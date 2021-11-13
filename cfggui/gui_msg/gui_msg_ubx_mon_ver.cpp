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

#include <cstring>

#include "ff_ubx.h"

#include "imgui.h"
#include "implot.h"
#include "IconsForkAwesome.h"

#include "gui_widget.hpp"

#include "gui_msg_ubx_mon_ver.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxMonVer::GuiMsgUbxMonVer(std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile) :
    GuiMsg(receiver, logfile)
{
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxMonVer::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const ImVec2 &sizeAvail)
{
    UNUSED(sizeAvail);
    if (msg->size >= UBX_MON_VER_V0_MIN_SIZE)
    {
        UBX_MON_VER_V0_GROUP0_t gr0;
        int offs = UBX_HEAD_SIZE;
        std::memcpy(&gr0, &msg->data[offs], sizeof(gr0));
        gr0.swVersion[sizeof(gr0.swVersion) - 1] = '\0';
        gr0.hwVersion[sizeof(gr0.hwVersion) - 1] = '\0';

        ImGui::Text("swVersion = %s", gr0.swVersion);
        ImGui::Text("hwVersion = %s", gr0.hwVersion);

        offs += sizeof(gr0);
        int extIx = 0;
        UBX_MON_VER_V0_GROUP1_t gr1;
        while (offs <= (msg->size - 2 - (int)sizeof(gr1)))
        {
            std::memcpy(&gr1, &msg->data[offs], sizeof(gr1));
            gr1.extension[sizeof(gr1.extension) - 1] = '\0';
            ImGui::Text("extension[%d] = %s", extIx, gr1.extension);
            extIx++;
            offs += sizeof(gr1);
        }
    }
    // Poll message
    else if (msg->size == UBX_FRAME_SIZE)
    {
        return false;
    }
    else
    {
        ImGui::Text("Unknown message version!");
    }

    return true;
}

/* ****************************************************************************************************************** */
