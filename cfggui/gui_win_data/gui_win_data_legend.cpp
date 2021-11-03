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

#include "gui_win_data_legend.hpp"

#include "gui_win_data_inc.hpp"

/* ****************************************************************************************************************** */

GuiWinDataLegend::GuiWinDataLegend(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database)
{
    _winFlags |= ImGuiWindowFlags_AlwaysAutoResize;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataLegend::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTCYAN));
    ImGui::TextUnformatted("Fix colours");
    ImGui::PopStyleColor();
    _DrawFixColourLegend(EPOCH_FIX_NOFIX,        GUI_COLOUR(FIX_INVALID),      "Invalid");
    _DrawFixColourLegend(-1,                     GUI_COLOUR(FIX_MASKED),       "Masked");
    _DrawFixColourLegend(EPOCH_FIX_DRONLY,       GUI_COLOUR(FIX_DRONLY),       "Dead-reckoning only");
    _DrawFixColourLegend(EPOCH_FIX_TIME,         GUI_COLOUR(FIX_S2D),          "Single 2D");
    _DrawFixColourLegend(EPOCH_FIX_S2D,          GUI_COLOUR(FIX_S3D),          "Single 3D");
    _DrawFixColourLegend(EPOCH_FIX_S3D,          GUI_COLOUR(FIX_S3D_DR),       "Single 3D + dead-reckoning");
    _DrawFixColourLegend(EPOCH_FIX_S3D_DR,       GUI_COLOUR(FIX_TIME),         "Single time only");
    _DrawFixColourLegend(EPOCH_FIX_RTK_FLOAT,    GUI_COLOUR(FIX_RTK_FLOAT),    "RTK float");
    _DrawFixColourLegend(EPOCH_FIX_RTK_FIXED,    GUI_COLOUR(FIX_RTK_FIXED),    "RTK fixed");
    _DrawFixColourLegend(EPOCH_FIX_RTK_FLOAT_DR, GUI_COLOUR(FIX_RTK_FLOAT_DR), "RTK float + dead-reckoning");
    _DrawFixColourLegend(EPOCH_FIX_RTK_FIXED_DR, GUI_COLOUR(FIX_RTK_FIXED_DR), "RTK fixed + dead-reckoning");

    _DrawWindowEnd();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataLegend::_DrawFixColourLegend(const int value, const ImU32 colour, const char *label)
{
    ImDrawList *draw = ImGui::GetWindowDrawList();
    const float textOffs = (2 * _winSettings->charSize.x) + (2 * _winSettings->style.ItemSpacing.x);
    ImVec2 offs = ImGui::GetCursorScreenPos();
    draw->AddRectFilled(offs, offs + _winSettings->charSize, colour | ((ImU32)(0xff) << IM_COL32_A_SHIFT) );
    offs.x += _winSettings->charSize.x;
    draw->AddRectFilled(offs, offs + _winSettings->charSize, colour);
    ImGui::SetCursorPosX(textOffs);
    if (value < 0)
    {
        ImGui::TextUnformatted(label);
    }
    else
    {
        ImGui::Text("%s (%d)", label, value);
    }
}

/* ****************************************************************************************************************** */
