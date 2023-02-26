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

#include <cstring>
#include <cmath>

#include "ff_ubx.h"

#include "gui_inc.hpp"

#include "gui_msg_ubx_mon_hw2.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxMonHw2::GuiMsgUbxMonHw2(std::shared_ptr<InputReceiver> receiver, std::shared_ptr<InputLogfile> logfile) :
    GuiMsg(receiver, logfile)
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxMonHw2::Update(const std::shared_ptr<Ff::ParserMsg> &msg)
{
    if (msg->size == UBX_MON_HW2_V0_SIZE)
    {
        UBX_MON_HW2_V0_GROUP0_t hw;
        std::memcpy(&hw, &msg->data[UBX_HEAD_SIZE], sizeof(hw));

        // Measurement
        _iqs.emplace_back((float)hw.magI * (1.0f/255.0f), (float)hw.magQ * (1.0f/255.0f),
                          (float)hw.ofsI * (2.0f/255.0f), (float)hw.ofsQ * (2.0f/255.0f));

        while (_iqs.size() > NUM_IQS)
        {
            _iqs.pop_front();
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxMonHw2::Clear()
{
    _iqs.clear();
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxMonHw2::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail)
{
    UNUSED(msg);
    DrawIQ(sizeAvail, _iqs);
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

/* static */ void GuiMsgUbxMonHw2::DrawIQ(const ImVec2 &size, const std::deque<GuiMsgUbxMonHw2::IQ> &iqs)
{
    if (ImGui::BeginChild("##iqplot", size))
    {

        // Measurement
        const IQ &iqLatest = iqs[iqs.size() - 1];

        // Canvas
        const FfVec2f offs = ImGui::GetCursorScreenPos();
        const FfVec2f cent = FfVec2f(offs.x + std::floor(size.x * 0.5), offs.y + std::floor(size.y * 0.5));

        // ImGui::Text("I: %5.1f%% @ %+5.1f%%", iqLatest.magI * 1e2f, iqLatest.offsI * 1e2f);
        // ImGui::Text("Q: %5.1f%% @ %+5.1f%%", iqLatest.magQ * 1e2f, iqLatest.offsQ * 1e2f);

        ImGui::Text("I: %5.1f%%", iqLatest.magI  * 1e2f);
        ImGui::Text("  %+6.1f%%", iqLatest.offsI * 1e2f);
        ImGui::Text("Q: %5.1f%%", iqLatest.magQ  * 1e2f);
        ImGui::Text("  %+6.1f%%", iqLatest.offsQ * 1e2f);

        ImDrawList *draw = ImGui::GetWindowDrawList();
        const float radiusPx = 0.4f * MIN(size.x, size.y);

        // Draw grid
        draw->AddLine(cent - ImVec2(radiusPx, 0), cent + ImVec2(radiusPx, 0), GUI_COLOUR(PLOT_GRID_MAJOR));
        draw->AddLine(cent - ImVec2(0, radiusPx), cent + ImVec2(0, radiusPx), GUI_COLOUR(PLOT_GRID_MAJOR));
        draw->AddCircle(cent,        radiusPx, GUI_COLOUR(PLOT_GRID_MAJOR), 0);
        draw->AddCircle(cent, 0.25 * radiusPx, GUI_COLOUR(PLOT_GRID_MINOR), 0);
        draw->AddCircle(cent, 0.50 * radiusPx, GUI_COLOUR(PLOT_GRID_MINOR), 0);
        draw->AddCircle(cent, 0.75 * radiusPx, GUI_COLOUR(PLOT_GRID_MINOR), 0);

        // Labels
        const ImVec2 charSize = ImGui::CalcTextSize("X");
        ImGui::SetCursorScreenPos(cent + ImVec2(radiusPx + charSize.x, -0.5 * charSize.y));
        ImGui::TextUnformatted("+I");
        ImGui::SetCursorScreenPos(cent - ImVec2(radiusPx + (3 * charSize.x), +0.5 * charSize.y));
        ImGui::TextUnformatted("-I");
        ImGui::SetCursorScreenPos(cent + ImVec2(-charSize.x, radiusPx));
        ImGui::TextUnformatted("-Q");
        ImGui::SetCursorScreenPos(cent + ImVec2(-charSize.x, -radiusPx - charSize.y));
        ImGui::TextUnformatted("+Q");

        // Ellipses
        const ImU32 colBg = (GUI_COLOUR(C_RED) & 0xffffff00) | 0x000000aa;
        for (auto iqIt = iqs.begin(); iqIt != iqs.end(); iqIt++)
        {
            const auto &iq = *iqIt;
            const bool isLatest = (iqIt == (iqs.end() - 1));
            const float lw = isLatest ? 3.0f : 2.0f;
            const ImU32 col = isLatest ? GUI_COLOUR(C_BRIGHTORANGE) : colBg;

            const float radiusX = radiusPx * iq.magQ;
            const float radiusY = radiusPx * iq.magI;
            const FfVec2f offsXY = FfVec2f(iq.offsQ * radiusPx, -iq.offsI * radiusPx);
            const FfVec2f centXY = cent + offsXY;
            draw->AddLine(centXY - FfVec2f(radiusX, 0), centXY + FfVec2f(radiusX, 0), col, lw);
            draw->AddLine(centXY - FfVec2f(0, radiusY), centXY + FfVec2f(0, radiusY), col, lw);
            ImVec2 points[30];
            for (int ix = 0; ix < NUMOF(points); ix++)
            {
                const double t = (double)ix * (2 * M_PI / (double)NUMOF(points));
                const double dx = radiusX * std::cos(t);
                const double dy = radiusY * std::sin(t);
                points[ix] = centXY + FfVec2f(dx, dy);
            }
            draw->AddPolyline(points, NUMOF(points), col, true, lw);
        }
    }
    ImGui::EndChild();
}

/* ****************************************************************************************************************** */
