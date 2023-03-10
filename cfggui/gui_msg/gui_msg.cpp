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

#include "gui_inc.hpp"

#include "gui_app.hpp"

#include "gui_msg.hpp"

#include "gui_msg_ubx_esf_meas.hpp"
#include "gui_msg_ubx_esf_status.hpp"
#include "gui_msg_ubx_mon_comms.hpp"
#include "gui_msg_ubx_mon_hw.hpp"
#include "gui_msg_ubx_mon_hw2.hpp"
#include "gui_msg_ubx_mon_hw3.hpp"
#include "gui_msg_ubx_mon_rf.hpp"
#include "gui_msg_ubx_mon_span.hpp"
#include "gui_msg_ubx_mon_ver.hpp"
#include "gui_msg_ubx_nav_cov.hpp"
#include "gui_msg_ubx_nav_dop.hpp"
#include "gui_msg_ubx_rxm_rawx.hpp"
#include "gui_msg_ubx_rxm_rtcm.hpp"
#include "gui_msg_ubx_rxm_sfrbx.hpp"
#include "gui_msg_ubx_rxm_spartn.hpp"
#include "gui_msg_ubx_tim_tm2.hpp"

/* ****************************************************************************************************************** */

GuiMsg::GuiMsg(std::shared_ptr<InputReceiver> receiver, std::shared_ptr<InputLogfile> logfile) :
    _receiver     { receiver },
    _logfile      { logfile }
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsg::Update(const std::shared_ptr<Ff::ParserMsg> &msg)
{
    UNUSED(msg);
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsg::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail)
{
    UNUSED(msg);
    UNUSED(sizeAvail);
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsg::Buttons()
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsg::Clear()
{
}

// ---------------------------------------------------------------------------------------------------------------------

ImVec2 GuiMsg::_CalcTopSize(const int nLinesOfTopText)
{
    const float topHeight = nLinesOfTopText * (GuiSettings::charSize.y + GuiSettings::style->ItemSpacing.y);
    return ImVec2(0.0f, topHeight);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsg::_RenderStatusText(const std::string &label, const std::string &text, const float dataOffs)
{
    ImGui::TextUnformatted(label.c_str());
    ImGui::SameLine(dataOffs);
    ImGui::TextUnformatted(text.c_str());
}

void GuiMsg::_RenderStatusText(const std::string &label, const char        *text, const float dataOffs)
{
    ImGui::TextUnformatted(label.c_str());
    ImGui::SameLine(dataOffs);
    ImGui::TextUnformatted(text);
}

void GuiMsg::_RenderStatusText(const char        *label, const std::string &text, const float dataOffs)
{
    ImGui::TextUnformatted(label);
    ImGui::SameLine(dataOffs);
    ImGui::TextUnformatted(text.c_str());
}

void GuiMsg::_RenderStatusText(const char        *label, const char        *text, const float dataOffs)
{
    ImGui::TextUnformatted(label);
    ImGui::SameLine(dataOffs);
    ImGui::TextUnformatted(text);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsg::_RenderStatusFlag(const std::vector<StatusFlags> &flags, const int value, const char *label, const float offs)
{
    ImGui::TextUnformatted(label);
    ImGui::SameLine(offs);

    for (const auto &f: flags)
    {
        if (value == f.value)
        {
            const bool colour = (f.colour != GUI_COLOUR_NONE);
            if (colour) { ImGui::PushStyleColor(ImGuiCol_Text, f.colour); }
            ImGui::TextUnformatted(f.text);
            if (colour) { ImGui::PopStyleColor(); }
            return;
        }
    }

    char str[100];
    snprintf(str, sizeof(str), "? (%d)", value);
    ImGui::TextUnformatted(str);
}


/* ****************************************************************************************************************** */

/* static */ std::unique_ptr<GuiMsg> GuiMsg::GetRenderer(const std::string &msgName,
    std::shared_ptr<InputReceiver> receiver, std::shared_ptr<InputLogfile> logfile)
{
    if      (msgName == "UBX-ESF-MEAS")      { return std::make_unique<GuiMsgUbxEsfMeas>(receiver, logfile);     }
    else if (msgName == "UBX-ESF-STATUS")    { return std::make_unique<GuiMsgUbxEsfStatus>(receiver, logfile);   }
    else if (msgName == "UBX-MON-COMMS")     { return std::make_unique<GuiMsgUbxMonComms>(receiver, logfile);    }
    else if (msgName == "UBX-MON-HW")        { return std::make_unique<GuiMsgUbxMonHw>(receiver, logfile);       }
    else if (msgName == "UBX-MON-HW2")       { return std::make_unique<GuiMsgUbxMonHw2>(receiver, logfile);      }
    else if (msgName == "UBX-MON-HW3")       { return std::make_unique<GuiMsgUbxMonHw3>(receiver, logfile);      }
    else if (msgName == "UBX-MON-RF")        { return std::make_unique<GuiMsgUbxMonRf>(receiver, logfile);       }
    else if (msgName == "UBX-MON-SPAN")      { return std::make_unique<GuiMsgUbxMonSpan>(receiver, logfile);     }
    else if (msgName == "UBX-MON-VER")       { return std::make_unique<GuiMsgUbxMonVer>(receiver, logfile);      }
    else if (msgName == "UBX-NAV-COV")       { return std::make_unique<GuiMsgUbxNavCov>(receiver, logfile);      }
    else if (msgName == "UBX-NAV-DOP")       { return std::make_unique<GuiMsgUbxNavDop>(receiver, logfile);      }
    else if (msgName == "UBX-RXM-RAWX")      { return std::make_unique<GuiMsgUbxRxmRawx>(receiver, logfile);     }
    else if (msgName == "UBX-RXM-RTCM")      { return std::make_unique<GuiMsgUbxRxmRtcm>(receiver, logfile);     }
    else if (msgName == "UBX-RXM-SFRBX")     { return std::make_unique<GuiMsgUbxRxmSfrbx>(receiver, logfile);    }
    else if (msgName == "UBX-RXM-SPARTN")    { return std::make_unique<GuiMsgUbxRxmSpartn>(receiver, logfile);   }
    else if (msgName == "UBX-TIM-TM2")       { return std::make_unique<GuiMsgUbxTimTm2>(receiver, logfile);      }
    else                                     { return std::make_unique<GuiMsg>(receiver, logfile);               }
}

/* ****************************************************************************************************************** */
