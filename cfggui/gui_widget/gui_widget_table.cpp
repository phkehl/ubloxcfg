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

#include "gui_inc.hpp"

#include "gui_widget_table.hpp"

/* ****************************************************************************************************************** */

GuiWidgetTable::GuiWidgetTable() :
    _freezeCols     { 1 },
    _freezeRows     { 1 },
    _rowsSelectable { false },
    _rowFilter      { 0 },
    _dirty          { true },
    _hoveredRow     { 0 }
{
    std::snprintf(_tableName, sizeof(_tableName), "GuiWidgetTable##%p", this);
    ClearRows();
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWidgetTable::Column::Column(const std::string &_title, const float _width, const ColumnFlags _flags) :
    title    { _title },
    flags    { _flags },
    width    { _width },
    maxWidth { 0.0f }
{
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWidgetTable::Cell::Cell() :
    paddingLeft { 0.0f },
    drawCb      { nullptr },
    drawCbArg   { nullptr },
    colour      { NO_COLOUR },
    sort        { 0 }, // sort by content
    hide        { false }
{
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWidgetTable::Row::Row() :
    filter   { 0 },
    colour   { NO_COLOUR },
    sort     { defSort++ }, // sort by order added
    uid      { 0 }
{
}

/*static*/ uint32_t GuiWidgetTable::Row::defSort = 0;

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::AddColumn(const char *title, const float width, /*const enum ColumnFlags*/int flags)
{
    _columns.emplace_back(title, width, static_cast<ColumnFlags>(flags));
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

void GuiWidgetTable::AddCellText(const std::string &text, const uint32_t sort)
{
    Cell &cell = _NextCell();
    cell.content = text;
    cell.sort = sort;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::AddCellText(const char *text, const uint32_t sort)
{
    Cell &cell = _NextCell();
    cell.content = std::string(text);
    cell.sort = sort;
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

void GuiWidgetTable::AddCellEmpty(const uint32_t sort)
{
    Cell &cell = _NextCell();
    cell.sort = sort;
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

void GuiWidgetTable::SetCellSort(const uint32_t sort)
{
    if (!_rows.empty())
    {
        auto &row = _rows[ _rows.size() - 1 ];
        if (!row.cells.empty())
        {
            row.cells[ row.cells.size() - 1 ].sort = sort;
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

void GuiWidgetTable::SetRowSort(const uint32_t sort)
{
    if (!_rows.empty())
    {
        _rows[ _rows.size() - 1 ].sort = sort;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::SetRowUid(const uint32_t uid)
{
    if (!_rows.empty())
    {
        _rows[ _rows.size() - 1 ].uid = uid;
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

bool GuiWidgetTable::IsSelected(const uint32_t uid)
{
    return (uid != 0) && (_selectedRows.find(uid) != _selectedRows.end());
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetTable::IsHovered(const uint32_t uid)
{
    return (uid != 0) && (_hoveredRow == uid);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::_CheckData()
{
    // Check sorting
    bool doSort = _dirty;
    ImGuiTableSortSpecs *sortSpecs = ImGui::TableGetSortSpecs();
    if (sortSpecs && sortSpecs->SpecsDirty)
    {
        _sortSpecs.clear();
        const int maxSpecColIx = (int)_columns.size() - 1;
        for (int ix = 0; ix < sortSpecs->SpecsCount; ix++)
        {
            // FIXME: spurious bad sortspecs from imgui
            const int specColIx = sortSpecs->Specs[ix].ColumnIndex;
            if (specColIx > maxSpecColIx)
            {
                WARNING("Ignore fishy SortSpecs (%d > %d)!", specColIx, maxSpecColIx);
                continue;
            }
            if (!CHKBITS_ANY(_columns[specColIx].flags, ColumnFlags::SORTABLE))
            {
                WARNING("Ignore fishy SortSpecs (col %d no sort)!", specColIx);
                continue;
            }
            _sortSpecs.push_back(sortSpecs->Specs[ix]);
        }
        sortSpecs->SpecsDirty = false;
        doSort = true;
    }
    //DEBUG("sort specs %lu", _sortSpecs.size());
    if (doSort)
    {
        _SortData();
    }

    if (!_dirty)
    {
        return;
    }

    _tableFlags = TABLE_FLAGS;
    for (auto &col: _columns)
    {
        col.titleSize = ImGui::CalcTextSize(col.title.c_str());
        if (CHKBITS_ANY(col.flags, ColumnFlags::SORTABLE))
        {
            _tableFlags |=  ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_SortTristate;
            col.titleSize.x += 1.5f * GuiSettings::charSize.x; // add space for the sort indicator
        }
    }

    for (auto &row: _rows)
    {
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
                // Row (cell) uses custome draw callback, we cannot know max size
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


        if (CHKBITS_ANY(col.flags, ColumnFlags::HIDE_SAME))
        {
            int rowIx = 0;
            std::string prevContent;
            for (auto &row: _rows)
            {
                row.cells[colIx].hide = (rowIx > 0 ? (row.cells[colIx].content == prevContent) : false);
                prevContent = row.cells[colIx].content;
                rowIx++;
            }
        }

        colIx++;
    }

    _dirty = false;
    return;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::_SortData()
{
    // No specs -> default sort by row sort key
    if (_sortSpecs.empty())
    {
        std::sort(_rows.begin(), _rows.end(), [](const Row &a, const Row &b)
        {
            return a.sort < b.sort;
        });
        return;
    }

    // Sort by specs
    std::sort(_rows.begin(), _rows.end(), [this](const Row &rowA, const Row &rowB)
    {
        for (const auto &spec: _sortSpecs)
        {
            const auto &cellA = rowA.cells[spec.ColumnIndex];
            const auto &cellB = rowB.cells[spec.ColumnIndex];

            // Sort by numeric key given by user
            if ((cellA.sort != 0) && (cellB.sort != 0) && (cellA.sort != cellB.sort))
            {
                return spec.SortDirection == ImGuiSortDirection_Ascending ? cellA.sort < cellB.sort : cellB.sort < cellA.sort;
            }
            // Sort by content (string)
            else if (cellA.content != cellB.content)
            {
                return spec.SortDirection == ImGuiSortDirection_Ascending ? cellA.content < cellB.content : cellB.content < cellA.content;
            }
        }
        return false;
    });
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTable::DrawTable(ImVec2 size)
{
    if (_columns.empty())
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
            ImGui::TableSetupColumn(col.title.c_str(), flags, col.width > 0.0f ? col.width : col.maxWidth);
        }
        ImGui::TableSetupColumn("");
        ImGui::TableHeadersRow();

        // Check data (calculate column widths, sort data, etc.)
        _CheckData();

        // Draw table body
        _hoveredRow = 0;
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
                    if (row.uid != 0)
                    {
                        auto selEntry = _selectedRows.find(row.uid);
                        bool selected = (selEntry != _selectedRows.end() ? selEntry->second : false);
                        ImGui::PushID(row.uid);
                        if (ImGui::Selectable(cell.hide ? "" : cell.content.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
                        {
                            auto &io = ImGui::GetIO();
                            if (io.KeyCtrl)
                            {
                                _selectedRows[row.uid] = (selEntry != _selectedRows.end() ? !selected : true);
                            }
                            else
                            {
                                _selectedRows.clear();
                                if (!selected)
                                {
                                    _selectedRows[row.uid] = true;
                                }
                            }
                        }
                        ImGui::PopID();
                        if (ImGui::IsItemHovered())
                        {
                            _hoveredRow = row.uid;
                        }
                    }
                    else
                    {
                        ImGui::Selectable(cell.content.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
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
                        ImGui::TextUnformatted(cell.hide ? "" : cell.content.c_str());
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
