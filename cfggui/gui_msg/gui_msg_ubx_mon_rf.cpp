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
#include <cmath>

#include "ff_ubx.h"

#include "imgui.h"
#include "implot.h"
#include "IconsForkAwesome.h"

#include "gui_settings.hpp"
#include "gui_widget.hpp"

#include "gui_msg_ubx_mon_rf.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxMonRf::GuiMsgUbxMonRf(std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile) :
    GuiMsg(receiver, logfile)
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxMonRf::Update(const std::shared_ptr<Ff::ParserMsg> &msg)
{
    if ( (UBX_MON_RF_VERSION_GET(msg->data) != UBX_MON_RF_V0_VERSION) || (UBX_MON_RF_V0_SIZE(msg->data) != msg->size) )
    {
        return;
    }
    UBX_MON_RF_V0_GROUP0_t rf;
    std::memcpy(&rf, &msg->data[UBX_HEAD_SIZE], sizeof(rf));

    if (_blockIqs.size() != rf.nBlocks)
    {
        _blockIqs.resize(rf.nBlocks);
    }

    for (int blockIx = 0, offs = UBX_HEAD_SIZE + (int)sizeof(UBX_MON_RF_V0_GROUP0_t); blockIx < (int)rf.nBlocks;
            blockIx++, offs += (int)sizeof(UBX_MON_RF_V0_GROUP1_t))
    {
        UBX_MON_RF_V0_GROUP1_t block;
        std::memcpy(&block, &msg->data[offs], sizeof(block));

        _blockIqs[blockIx].emplace_back(
            (float)block.magI * (1.0f/255.0f), (float)block.magQ * (1.0f/255.0f),
            (float)block.ofsI * (2.0f/255.0f), (float)block.ofsQ * (2.0f/255.0f));

        while (_blockIqs[blockIx].size() > NUM_IQS)
        {
            _blockIqs[blockIx].pop_front();
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxMonRf::Clear()
{
    _blockIqs.clear();
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxMonRf::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const ImVec2 &sizeAvail)
{
    if ( (UBX_MON_RF_VERSION_GET(msg->data) != UBX_MON_RF_V0_VERSION) || (UBX_MON_RF_V0_SIZE(msg->data) != msg->size) )
    {
        return false;
    }

    const ImVec2 blockSize { 0.5f * (sizeAvail.x - _winSettings->style.ItemSpacing.x), sizeAvail.y };

    UBX_MON_RF_V0_GROUP0_t rf;
    std::memcpy(&rf, &msg->data[UBX_HEAD_SIZE], sizeof(rf));
    const int nBlocks = CLIP(rf.nBlocks, 0, 3);

    const float dataOffs = _winSettings->charSize.x * 16;

    for (int blockIx = 0, offs = UBX_HEAD_SIZE + (int)sizeof(UBX_MON_RF_V0_GROUP0_t); blockIx < nBlocks;
            blockIx++, offs += (int)sizeof(UBX_MON_RF_V0_GROUP1_t))
    {
        UBX_MON_RF_V0_GROUP1_t block;
        std::memcpy(&block, &msg->data[offs], sizeof(block));

        ImGui::BeginChild(offs, blockSize);

        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
        ImGui::Text("RF block #%u", blockIx);
        ImGui::PopStyleColor();

        ImGui::TextUnformatted("Antenna status");
        ImGui::SameLine(dataOffs);
        const char * const aStatusStr[] = { "init", "unknown", "OK", "short", "open" };
        const bool aStatusWarn = (block.antStatus == UBX_MON_RF_V0_ANTSTATUS_SHORT) || (block.antStatus == UBX_MON_RF_V0_ANTSTATUS_OPEN);
        const bool aStatusOk   = (block.antStatus == UBX_MON_RF_V0_ANTSTATUS_OK);
        if (aStatusWarn || aStatusOk)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, aStatusWarn ? GUI_COLOUR(TEXT_WARNING) : GUI_COLOUR(TEXT_OK));
        }
        ImGui::TextUnformatted(block.antStatus < NUMOF(aStatusStr) ? aStatusStr[block.antStatus] : "???");
        if (aStatusWarn || aStatusOk)
        {
            ImGui::PopStyleColor();
        }

        ImGui::TextUnformatted("Antenna power");
        ImGui::SameLine(dataOffs);
        const char * const aPowerStr[]  = { "off", "on", "unknown" };
        if (block.antPower == UBX_MON_RF_V0_ANTPOWER_ON)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_OK));
        }
        ImGui::TextUnformatted(block.antPower < NUMOF(aPowerStr) ? aPowerStr[block.antPower] : "???");
        if (block.antPower == UBX_MON_RF_V0_ANTPOWER_ON)
        {
            ImGui::PopStyleColor();
        }

        ImGui::TextUnformatted("Jamming status");
        ImGui::SameLine(dataOffs);
        const char * const jammingStr[] = { "unknown", "OK", "warning", "critical" };
        const uint8_t jamming = UBX_MON_RF_V0_FLAGS_JAMMINGSTATE_GET(block.flags);
        const bool jammingWarning  = (jamming == UBX_MON_RF_V0_FLAGS_JAMMINGSTATE_WARN);
        const bool jammingCritical = (jamming == UBX_MON_RF_V0_FLAGS_JAMMINGSTATE_CRIT);
        if (jammingWarning || jammingCritical)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, jammingCritical ? GUI_COLOUR(TEXT_ERROR) : GUI_COLOUR(TEXT_WARNING));
        }
        ImGui::TextUnformatted(jamming < NUMOF(jammingStr) ? jammingStr[jamming] : "???");
        if (jammingWarning || jammingCritical)
        {
            ImGui::PopStyleColor();
        }

        ImGui::TextUnformatted("POST status");
        ImGui::SameLine(dataOffs);
        ImGui::Text("0x%08x", block.postStatus);

        char str[100];

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Noise level");
        ImGui::SameLine(dataOffs);
        const float noise = (float)block.noisePerMS / (float)UBX_MON_RF_V0_NOISEPERMS_MAX;
        snprintf(str, sizeof(str), "%u", block.noisePerMS);
        ImGui::ProgressBar(noise, ImVec2(-1.0f,0.0f), str);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("AGC monitor");
        ImGui::SameLine(dataOffs);
        const float agc = (float)block.agcCnt / (float)UBX_MON_RF_V0_AGCCNT_MAX;
        snprintf(str, sizeof(str), "%.1f%%", agc * 1e2f);
        ImGui::ProgressBar(agc, ImVec2(-1.0f,0.0f), str);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("CW jamming");
        ImGui::SameLine(dataOffs);
        const float jam = (float)block.jamInd / (float)UBX_MON_RF_V0_JAMIND_MAX;
        snprintf(str, sizeof(str), "%.1f%%", jam * 1e2f);
        ImGui::ProgressBar(jam, ImVec2(-1.0f,0.0f), str);

        GuiMsgUbxMonHw2::DrawIQ(ImGui::GetContentRegionAvail(), _blockIqs[blockIx]);

        ImGui::EndChild();

        if (blockIx < ((int)rf.nBlocks - 1))
        {
            ImGui::SameLine();
        }
    }

    return true;
}

/* ****************************************************************************************************************** */
