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
    _freezeCols     { 1 },
    _freezeRows     { 1 },
    _rowsSelectable { true },
    _rowFilter      { 0 },
    _dirty          { true },
    _hoveredRow     { nullptr }
{
    std::snprintf(_tableName, sizeof(_tableName), "GuiWidgetTable##%p", this);
    ClearRows();
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWidgetTable::Column::Column(const std::string &_title, const float _width, const ColumnFlags _flags, const uint32_t _uid) :
    title    { _title },
    flags    { _flags },
    width    { _width },
    maxWidth { 0.0f },
    uid      { _uid }
{
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWidgetTable::Cell::Cell() :
    paddingLeft { 0.0f },
    drawCb      { nullptr },
    drawCbArg   { nullptr },
    colour      { NO_COLOUR }
{
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWidgetTable::Row::Row() :
    filter   { 0 },
    colour   { NO_COLOUR }
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::AddColumn(const char *title, const float width, const enum ColumnFlags flags, const uint32_t uid)
{
    _columns.push_back(Column(title, width, flags, uid));
    _dirty = true;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::SetTableScrollFreeze(const int cols, const int rows)
{
    _freezeCols = MAX(cols, 0);
    _freezeRows = MAX(rows, 0);
    _dirty = true;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::SetTableRowsSelectable(const bool enable)
{
    _rowsSelectable = enable;
    _dirty = true;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::SetTableSortFunc(GuiWidgetTable::SortFunc func)
{
    _sortFunc = func;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::ClearRows()
{
    _rows.clear();
    _dirty = true;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::ClearSelected()
{
    _selectedRows.clear();
    _dirty = true;
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWidgetTable::Cell &GuiWidgetTable::_NextCell()
{
    // Determine next cell
    IM_ASSERT(!_columns.empty());

    // No rows yet? Add one.
    if (_rows.empty())
    {
        _rows.emplace_back();
    }

    // Row full? Add one.
    if (_rows[ _rows.size() - 1 ].cells.size() >= _columns.size())
    {
        _rows.emplace_back();
    }

    // Get last row
    auto &row = _rows[ _rows.size() - 1 ];

    _dirty = true;

    // Add new cell and return it
    return row.cells.emplace_back();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::AddCellText(const std::string &text)
{
    Cell &cell = _NextCell();
    cell.content = text;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::AddCellText(const char *text)
{
    Cell &cell = _NextCell();
    cell.content = std::string(text);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::AddCellTextF(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Cell &cell = _NextCell();
    cell.content = Ff::Vsprintf(fmt, args);
    va_end(args);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::AddCellCb(GuiWidgetTable::CellDrawCb_t cb, void *arg)
{
    Cell &cell = _NextCell();
    cell.drawCb = cb;
    cell.drawCbArg = arg;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::AddCellEmpty()
{
    _NextCell();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::SetCellColour(const ImU32 colour)
{
    if (!_rows.empty())
    {
        auto &row = _rows[ _rows.size() - 1 ];
        if (!row.cells.empty())
        {
            row.cells[ row.cells.size() - 1 ].colour = colour;
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::SetRowFilter(const uint64_t filter)
{
    if (!_rows.empty())
    {
        _rows[ _rows.size() - 1 ].filter = filter;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::SetRowColour(const ImU32 colour)
{
    if (!_rows.empty())
    {
        _rows[ _rows.size() - 1 ].colour = colour;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::SetTableRowFilter(const uint64_t filter)
{
    _rowFilter = filter;
}

// ---------------------------------------------------------------------------------------------------------------------

int GuiWidgetTable::GetNumRows()
{
    return _rows.size();
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetTable::IsSelected(const std::string &content, const bool useHovered)
{
    if (_selectedRows.empty())
    {
        return useHovered && _hoveredRow && (content == *_hoveredRow);
    }
    else
    {
        return _selectedRows.find(content) != _selectedRows.end();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetTable::_CheckData()
{
    if (!_dirty)
    {
        return true;
    }
    if (_columns.empty())
    {
        return false;
    }

    _tableFlags = TABLE_FLAGS;

    for (auto &col: _columns)
    {
        col.titleSize = ImGui::CalcTextSize(col.title.c_str());
        if (CHKBITS_ANY(col.flags, ColumnFlags::SORTABLE))
        {
            _tableFlags |=  ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_SortTristate;
        }
    }

    std::unordered_map<std::string, bool> firstSeen;
    for (auto &row: _rows)
    {
        const auto &firstCellContent = row.cells[0].content;

        // Disable row selection if first column cells content are not unique
        if (firstCellContent.empty() || (firstSeen.find(firstCellContent) != firstSeen.end()))
        {
            _rowsSelectable = false;
        }
        firstSeen[firstCellContent] = true;

        // Add unique ID, for ImGui::Selectable() (unless given by the user)
        row.first = row.cells[0].content;
        if (row.first.find("##") == std::string::npos)
        {
            row.first += "##" + std::to_string(reinterpret_cast<std::uintptr_t>(std::addressof(row)));
        }
        // Calculate text size
        for (auto &cell: row.cells)
        {
            cell.contentSize = ImGui::CalcTextSize(cell.content.c_str(), nullptr, true);
            cell.paddingLeft = 0.0f;
        }
    }

    int colIx = 0;
    for (auto &col: _columns)
    {
        // Find row max width
        col.maxWidth = col.titleSize.x;
        bool noMaxWidth = false;
        for (const auto &row: _rows)
        {
            if ((int)row.cells.size() > colIx)
            {
                // Row (cell) uses custome draw callback, cannot know max size
                if (row.cells[colIx].drawCb)
                {
                    noMaxWidth = true;
                }
                else if (row.cells[colIx].contentSize.x > col.maxWidth)
                {
                    col.maxWidth  = row.cells[colIx].contentSize.x;
                }
            }
        }
        if (noMaxWidth)
        {
            col.maxWidth = 0.0; // let ImGui deal with it
        }

        // Determine left padding for content if align center or right is desired
        if (CHKBITS_ANY(col.flags, ColumnFlags::ALIGN_RIGHT | ColumnFlags::ALIGN_CENTER))
        {
            // Calculate and store padding for each cell
            for (auto &row: _rows)
            {
                if ((int)row.cells.size() >= colIx)
                {
                    auto &cell = row.cells[colIx];
                    const float delta = col.maxWidth - cell.contentSize.x;
                    cell.paddingLeft = CHKBITS(col.flags, ColumnFlags::ALIGN_CENTER) ? std::floor(0.5f * delta) : delta;
                }
            }
        }

        colIx++;
    }

    _dirty = false;
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::DrawTable(ImVec2 size)
{
    if (!_CheckData())
    {
        ImGui::TextUnformatted("no table?!");
        return;
    }

    if (ImGui::BeginTable(_tableName, _columns.size() + 1, _tableFlags, size))
    {
        // Enable first n columns and/or rows frozen (scroll only the remaining parts of the table)
        if ( (_freezeRows > 0) || (_freezeCols > 0) )
        {
            ImGui::TableSetupScrollFreeze(_freezeCols, _freezeRows);
        }

        // Table header
        for (const auto &col: _columns)
        {
            ImGuiTableColumnFlags flags = ImGuiTableColumnFlags_None;
            if (!CHKBITS_ANY(col.flags, ColumnFlags::SORTABLE))
            {
                flags |= ImGuiTableColumnFlags_NoSort;
            }
            ImGui::TableSetupColumn(col.title.c_str(), flags, col.width > 0.0f ? col.width : col.maxWidth, (ImGuiID)col.uid);
        }
        ImGui::TableSetupColumn("");
        ImGui::TableHeadersRow();

        // Data needs sorting?
        ImGuiTableSortSpecs *sortSpecs = ImGui::TableGetSortSpecs();
        if (sortSpecs && sortSpecs->SpecsDirty)
        {
            if (_sortFunc)
            {
                std::vector<SortInfo> info;
                for (int ix = 0; ix < sortSpecs->SpecsCount; ix++)
                {
                    info.emplace_back((uint32_t)sortSpecs->Specs[ix].ColumnUserID, sortSpecs->Specs[ix].SortDirection == ImGuiSortDirection_Descending ? SortOrder::DESC : SortOrder::ASC);
                }
                _sortFunc(info);
            }
            sortSpecs->SpecsDirty = false;
        }

        // Draw table body
        _hoveredRow = nullptr;
        for (const auto &row: _rows)
        {
            // Skip?
            if ( (_rowFilter != 0) && (row.filter != 0) )
            {
                if (row.filter != _rowFilter)
                {
                    continue;
                }
            }

            ImGui::TableNextRow();

            if (row.colour != NO_COLOUR)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, row.colour);
            }

            int ix = 0;
            for (const auto &cell: row.cells)
            {
                ImGui::TableSetColumnIndex(ix);

                if (cell.colour != NO_COLOUR)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, cell.colour);
                }

                // Left padding?
                if (cell.paddingLeft != 0.0f)
                {
                    ImGui::Dummy(ImVec2(cell.paddingLeft, 1));
                    ImGui::SameLine(0, 0);
                }

                // First column is a selectable, and handles selecting rows
                if (ix == 0)
                {
                    // Rows can be selected
                    if (_rowsSelectable)
                    {
                        auto selEntry = _selectedRows.find(cell.content);
                        bool selected =  selEntry != _selectedRows.end() ? selEntry->second : false;
                        if (ImGui::Selectable(row.first.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
                        {
                            auto &io = ImGui::GetIO();
                            if (io.KeyCtrl)
                            {
                                _selectedRows[cell.content] = selEntry != _selectedRows.end() ? !selected : true;
                            }
                            else
                            {
                                _selectedRows.clear();
                                if (!selected)
                                {
                                    _selectedRows[cell.content] = true;
                                }
                            }
                        }
                        if (ImGui::IsItemHovered())
                        {
                            _hoveredRow = &row.cells[0].content;
                        }
                    }
                    else
                    {
                        ImGui::Selectable(row.first.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
                    }
                }
                // Other columns
                else
                {
                    if (cell.drawCb)
                    {
                        cell.drawCb(cell.drawCbArg);
                    }
                    else
                    {
                        ImGui::TextUnformatted(cell.content.c_str());
                    }
                }

                if (cell.colour != NO_COLOUR)
                {
                    ImGui::PopStyleColor();
                }

                ix++;
            } // columns


            if (row.colour != NO_COLOUR)
            {
                ImGui::PopStyleColor();
            }
        } // rows

        ImGui::EndTable();
    }
}

/* ****************************************************************************************************************** */
