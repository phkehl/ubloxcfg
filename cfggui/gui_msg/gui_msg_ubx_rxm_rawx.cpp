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

#include "gui_msg_ubx_rxm_rawx.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxRxmRawx::GuiMsgUbxRxmRawx(std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile) :
    GuiMsg(receiver, logfile),
    _selected{0}
{
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxRxmRawx::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const ImVec2 &sizeAvail)
{
    if ( (UBX_RXM_RAWX_VERSION_GET(msg->data) != UBX_RXM_RAWX_V1_VERSION) || (UBX_RXM_RAWX_V1_SIZE(msg->data) != msg->size) )
    {
        return false;
    }

    // TODO: local time, leap seconds clock reset

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody |
        ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedFit
        | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;

    const struct { const char *label; ImGuiTableColumnFlags flags; } columns[] =
    {
        { .label = "SV",                      .flags = ImGuiTableColumnFlags_NoReorder },
        { .label = "Signal",                  .flags = 0 },
        { .label = "CNo",                     .flags = 0 },
        { .label = "Pseudo range [m]",        .flags = 0 },
        { .label = "Carrier phase [cycles]",  .flags = 0 },
        { .label = "Doppler [Hz]",            .flags = 0 },
        { .label = "Lock time [s]",           .flags = 0 },
    };

    // TODO: table sorting...
    if (ImGui::BeginTable("stats", NUMOF(columns), tableFlags, sizeAvail))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        for (int ix = 0; ix < NUMOF(columns); ix++)
        {
            ImGui::TableSetupColumn(columns[ix].label, columns[ix].flags);
        }
        ImGui::TableHeadersRow();

        UBX_RXM_RAWX_V1_GROUP0_t rawx;
        std::memcpy(&rawx, &msg->data[UBX_HEAD_SIZE], sizeof(rawx));

        for (int measIx = 0, offs = UBX_HEAD_SIZE + (int)sizeof(UBX_RXM_RAWX_V1_GROUP0_t); measIx < (int)rawx.numMeas;
                measIx++, offs += (int)sizeof(UBX_RXM_RAWX_V1_GROUP1_t))
        {
            UBX_RXM_RAWX_V1_GROUP1_t meas;
            std::memcpy(&meas, &msg->data[offs], sizeof(meas));
            const bool prValid    = CHKBITS(meas.trkStat, UBX_RXM_RAWX_V1_TRKSTAT_PRVALID);
            const bool cpValid    = CHKBITS(meas.trkStat, UBX_RXM_RAWX_V1_TRKSTAT_CPVALID);
            const bool halfCyc    = CHKBITS(meas.trkStat, UBX_RXM_RAWX_V1_TRKSTAT_HALFCYC);
            const bool subHalfCyc = CHKBITS(meas.trkStat, UBX_RXM_RAWX_V1_TRKSTAT_SUBHALFCYC);
            const uint32_t uid =
                ((uint32_t)meas.gnssId  << 24) |
                ((uint32_t)meas.svId    << 16) |
                ((uint32_t)meas.freqId  <<  8) |
                 (uint32_t)meas.sigId;

            ImGui::TableNextRow();
            int colIx = 0;
            char str[100];

            ImGui::TableSetColumnIndex(colIx++);
            snprintf(str, sizeof(str), "%s##%d", ubxSvStr(meas.gnssId, meas.svId), offs);
            if (ImGui::Selectable(str, _selected == uid, ImGuiSelectableFlags_SpanAllColumns))
            {
                _selected = (_selected == uid) ? 0 : uid;
            }

            ImGui::TableSetColumnIndex(colIx++);
            if (meas.gnssId == UBX_GNSSID_GLO)
            {
                ImGui::Text("%s (%+d)", ubxSigStr(meas.gnssId, meas.sigId), (int)meas.freqId - 7);
            }
            else
            {
                ImGui::TextUnformatted(ubxSigStr(meas.gnssId, meas.sigId));
            }

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::Text("%2u", meas.cno);

            ImGui::TableSetColumnIndex(colIx++);
            if (!prValid) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
            ImGui::Text("%11.2f %6.2f", meas.prMeas, UBX_RXM_RAWX_V1_PRSTD_SCALE(UBX_RXM_RAWX_V1_PRSTDEV_PRSTD_GET(meas.prStdev)));
            if (!prValid) { ImGui::PopStyleColor(); }

            ImGui::TableSetColumnIndex(colIx++);
            if (!cpValid) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
            ImGui::Text("%12.2f %5.3f", meas.cpMeas, UBX_RXM_RAWX_V1_CPSTD_SCALE(UBX_RXM_RAWX_V1_CPSTDEV_CPSTD_GET(meas.cpStdev)));
            if (!cpValid) { ImGui::PopStyleColor(); }

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::Text("%7.1f %5.3f", meas.doMeas, UBX_RXM_RAWX_V1_DOSTD_SCALE(UBX_RXM_RAWX_V1_DOSTDEV_DOSTD_GET(meas.doStdev)));

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::Text("%.3f", (double)meas.locktime * UBX_RXM_RAWX_V1_LOCKTIME_SCALE);

            // TODO
            UNUSED(halfCyc);
            UNUSED(subHalfCyc);

        }

        ImGui::EndTable();
    }

    return true;
}

/* ****************************************************************************************************************** */
