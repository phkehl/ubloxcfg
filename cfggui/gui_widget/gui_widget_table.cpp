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

#include "gui_inc.hpp"

#include "gui_widget_table.hpp"

/* ****************************************************************************************************************** */

GuiWidgetTable::GuiWidgetTable() :
    _totalWidth{0.0f}
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::AddColumn(const char *title, const float width)
{
    _columns.push_back(Column(title, width));
    _totalWidth += width;
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWidgetTable::Column::Column(const char *_title, const float _width)
    : title{}, width{_width}
{
    std::strncpy(title, _title, sizeof(title) - 1);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::BeginDraw()
{
    ImGui::PushID(this);

    const float currWidth = ImGui::GetContentRegionAvail().x;

    // Child window of fixed width with horizontal scrolling
    ImGui::SetNextWindowContentSize(ImVec2(currWidth > _totalWidth ? currWidth : _totalWidth, 0.0f));
    ImGui::BeginChild("##Table", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

    // Header
    ImGui::BeginChild("##Head", ImVec2(0.0f, ImGui::GetTextLineHeight() * 1.5));

    // Setup columns
    ImGui::Columns(_columns.size(), "##HeadCols");
    int ix = 0;
    for (auto &col: _columns)
    {
        ImGui::SetColumnWidth(ix, col.width);
        ix++;
    }

    // Title
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
    for (auto &col: _columns)
    {
        ImGui::TextUnformatted(col.title);
        ImGui::NextColumn();
    }
    ImGui::PopStyleColor();

    ImGui::Separator();

    ImGui::EndChild(); // Head

    // Body
    ImGui::BeginChild("##Body", ImVec2(0.0f, 0.0f));

    // Setup columns
    ImGui::Columns(_columns.size(), "##BodyCols");
    ix = 0;
    for (auto &col: _columns)
    {
        ImGui::SetColumnWidth(ix, col.width);
        ix++;
    }

    // Col*()...
    // ...
    // EndDraw()
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::ColText(const char *text)
{
    ImGui::TextUnformatted(text);
    ImGui::NextColumn();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::ColTextF(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
    ImGui::NextColumn();
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetTable::ColSelectable(const char *label, bool selected)
{
    const bool res = ImGui::Selectable(label, selected, ImGuiSelectableFlags_SpanAllColumns);
    ImGui::NextColumn();
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::ColSkip()
{
    ImGui::NextColumn();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::EndDraw()
{
    ImGui::EndChild(); // Body
    ImGui::EndChild(); // Table
    ImGui::PopID();
}

/* ****************************************************************************************************************** */
