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
#include "ff_rtcm3.h"

#include "gui_inc.hpp"

#include "gui_msg_ubx_rxm_rtcm.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxRxmRtcm::GuiMsgUbxRxmRtcm(std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile) :
    GuiMsg(receiver, logfile)
{
}

// ---------------------------------------------------------------------------------------------------------------------

GuiMsgUbxRxmRtcm::RtcmInfo::RtcmInfo(const int _msgType, const int _subType, const int _refStation) :
    msgType{_msgType}, subType{_subType}, refStation{_refStation == 0xffff ? -1 : _refStation},
    nUsed{0}, nUnused{0}, nUnknown{0}, nCrcFailed{0}
{
    name = std::to_string(msgType) + (msgType == 4072 ? "." + std::to_string(subType) : "");
    tooltip = rtcm3TypeDesc(msgType, subType);
    if (tooltip == NULL)
    {
        tooltip = "Unknown RTCM3 message";
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxRxmRtcm::Clear()
{
    _rtcmInfos.clear();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxRxmRtcm::Update(const std::shared_ptr<Ff::ParserMsg> &msg)
{
    if ( (msg->size == UBX_RXM_RTCM_V2_SIZE) && (UBX_RXM_RTCM_VERSION_GET(msg->data) == UBX_RXM_RTCM_V2_VERSION) )
    {
        UBX_RXM_RTCM_V2_GROUP0_t rtcm;
        std::memcpy(&rtcm, &msg->data[UBX_HEAD_SIZE], sizeof(rtcm));

        // UID for this entry, also the std::map sort
        const uint32_t uid =
            ((uint32_t)MAX(rtcm.refStation, 0) << 24) |
            ((uint32_t)rtcm.subType            << 12) |
             (uint32_t)rtcm.msgType;

        RtcmInfo *info = nullptr;
        auto entry = _rtcmInfos.find(uid);
        if (entry != _rtcmInfos.end())
        {
            info = &entry->second;
        }
        else
        {
            auto foo = _rtcmInfos.insert({ uid, RtcmInfo(rtcm.msgType, rtcm.subType, rtcm.refStation) });
            info = &foo.first->second;
        }

        // Update statistics
        const bool crcFailed  = CHKBITS(rtcm.flags, UBX_RXM_RTCM_V2_FLAGS_CRCFAILED);
        if (crcFailed)
        {
            info->nCrcFailed++;
        }
        else
        {
            switch (UBX_RXM_RTCM_V2_FLAGS_MSGUSED_GET(rtcm.flags))
            {
                case UBX_RXM_RTCM_V2_FLAGS_MSGUSED_UNUSED:  info->nUnused++;  break;
                case UBX_RXM_RTCM_V2_FLAGS_MSGUSED_USED:    info->nUsed++;    break;
                case UBX_RXM_RTCM_V2_FLAGS_MSGUSED_UNKNOWN:
                default:                                    info->nUnknown++; break;
            }
        }
        info->lastTs = msg->ts;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxRxmRtcm::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2 &sizeAvail)
{
    UNUSED(msg);

    const uint32_t now = TIME();

    const struct { const char *label; ImGuiTableColumnFlags flags; } columns[] =
    {
        { .label = "Message",   .flags = ImGuiTableColumnFlags_NoReorder },
        { .label = "Ref",       .flags = 0 },
        { .label = "#Used",     .flags = 0 },
        { .label = "#Unused",   .flags = 0 },
        { .label = "#Unknown",  .flags = 0 },
        { .label = "#CrcFail",  .flags = 0 },
        { .label = "Age",       .flags = 0 },
        { .label = "Desc",      .flags = 0 },
    };

    if (ImGui::BeginTable("stats", NUMOF(columns), TABLE_FLAGS, sizeAvail))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        for (int ix = 0; ix < NUMOF(columns); ix++)
        {
            ImGui::TableSetupColumn(columns[ix].label, columns[ix].flags);
        }
        ImGui::TableHeadersRow();

        for (auto &entry: _rtcmInfos)
        {
            int ix = 0;
            auto &info = entry.second;
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(ix++);
            ImGui::Selectable(info.name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
            //Gui::ItemTooltip(info.tooltip);

            ImGui::TableSetColumnIndex(ix++);
            ImGui::Text("%d", info.refStation);

            ImGui::TableSetColumnIndex(ix++);
            ImGui::Text("%u", info.nUsed);

            ImGui::TableSetColumnIndex(ix++);
            ImGui::Text("%u", info.nUnused);

            ImGui::TableSetColumnIndex(ix++);
            ImGui::Text("%u", info.nUnknown);

            ImGui::TableSetColumnIndex(ix++);
            ImGui::Text("%u", info.nCrcFailed);

            ImGui::TableSetColumnIndex(ix++);
            if (_receiver)
            {
                ImGui::Text("%.1f", (now - info.lastTs) * 1e-3);
            }

            ImGui::TableSetColumnIndex(ix++);
            ImGui::TextUnformatted(info.tooltip);
        }

        ImGui::EndTable();
    }

    return true;
}

/* ****************************************************************************************************************** */
