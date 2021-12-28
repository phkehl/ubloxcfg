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

#include "gui_inc.hpp"

#include "gui_msg_ubx_mon_rf.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxMonRf::GuiMsgUbxMonRf(std::shared_ptr<InputReceiver> receiver, std::shared_ptr<InputLogfile> logfile) :
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

/*static*/ const std::vector<GuiMsg::StatusFlags> GuiMsgUbxMonRf::_aStatusFlags =
{
    { UBX_MON_RF_V0_ANTSTATUS_INIT,     "init",           GUI_COLOUR_NONE },
    { UBX_MON_RF_V0_ANTSTATUS_DONTKNOW, "unknown",        GUI_COLOUR_NONE },
    { UBX_MON_RF_V0_ANTSTATUS_OK,       "OK",             GUI_COLOUR(TEXT_OK) },
    { UBX_MON_RF_V0_ANTSTATUS_SHORT,    "short",          GUI_COLOUR(TEXT_WARNING) },
    { UBX_MON_RF_V0_ANTSTATUS_OPEN,     "open",           GUI_COLOUR(TEXT_WARNING)  },
};

/*static*/ const std::vector<GuiMsg::StatusFlags> GuiMsgUbxMonRf::_aPowerFlags =
{
    { UBX_MON_HW_V0_APOWER_OFF,       "off",            GUI_COLOUR_NONE },
    { UBX_MON_HW_V0_APOWER_ON,        "on",             GUI_COLOUR(TEXT_OK) },
    { UBX_MON_HW_V0_APOWER_UNKNOWN,   "unknown",        GUI_COLOUR_NONE },
};

/*static*/ const std::vector<GuiMsg::StatusFlags> GuiMsgUbxMonRf::_jammingFlags =
{
    { UBX_MON_RF_V0_FLAGS_JAMMINGSTATE_UNKN,   "unknown",   GUI_COLOUR_NONE },
    { UBX_MON_RF_V0_FLAGS_JAMMINGSTATE_OK,     "ok",        GUI_COLOUR(TEXT_OK) },
    { UBX_MON_RF_V0_FLAGS_JAMMINGSTATE_WARN,   "warning",   GUI_COLOUR(TEXT_WARNING) },
    { UBX_MON_RF_V0_FLAGS_JAMMINGSTATE_CRIT,   "critical",  GUI_COLOUR(TEXT_ERROR) },
};

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxMonRf::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2 &sizeAvail)
{
    if ( (UBX_MON_RF_VERSION_GET(msg->data) != UBX_MON_RF_V0_VERSION) || (UBX_MON_RF_V0_SIZE(msg->data) != msg->size) )
    {
        return false;
    }

    const ImVec2 blockSize { 0.5f * (sizeAvail.x - GuiSettings::style->ItemSpacing.x), sizeAvail.y };

    UBX_MON_RF_V0_GROUP0_t rf;
    std::memcpy(&rf, &msg->data[UBX_HEAD_SIZE], sizeof(rf));
    const int nBlocks = CLIP(rf.nBlocks, 0, 3);

    const float dataOffs = GuiSettings::charSize.x * 16;

    for (int blockIx = 0, offs = UBX_HEAD_SIZE + (int)sizeof(UBX_MON_RF_V0_GROUP0_t); blockIx < nBlocks;
            blockIx++, offs += (int)sizeof(UBX_MON_RF_V0_GROUP1_t))
    {
        UBX_MON_RF_V0_GROUP1_t block;
        std::memcpy(&block, &msg->data[offs], sizeof(block));

        if (ImGui::BeginChild(offs, blockSize))
        {
            Gui::TextTitleF("RF block #%u", blockIx);

            _RenderStatusFlag(_aStatusFlags,  block.antStatus,                                   "Antenna status", dataOffs);
            _RenderStatusFlag(_aPowerFlags,   block.antPower,                                    "Antenna power",  dataOffs);
            _RenderStatusFlag(_jammingFlags,  UBX_MON_RF_V0_FLAGS_JAMMINGSTATE_GET(block.flags), "Jamming status", dataOffs);

            char str[100];

            snprintf(str, sizeof(str), "0x%08x", block.postStatus);
            _RenderStatusText("POST status", str, dataOffs);

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
        }
        ImGui::EndChild();

        if (blockIx < ((int)rf.nBlocks - 1))
        {
            ImGui::SameLine();
        }
    }

    return true;
}

/* ****************************************************************************************************************** */
