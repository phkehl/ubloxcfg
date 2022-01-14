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

#ifndef __GUI_WIDGET_TABLE_HPP__
#define __GUI_WIDGET_TABLE_HPP__

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

#include "imgui.h"

/* ****************************************************************************************************************** */

class GuiWidgetTable
{
    public:
        GuiWidgetTable();

        // Setup: Add columns (first thing, and only once)
        enum ColumnFlags { NONE = 0, ALIGN_RIGHT = BIT(0), ALIGN_CENTER = BIT(1) };
        void AddColumn(const char *title, const float width = 0.0f, const enum ColumnFlags = ColumnFlags::NONE);

        // Set table parameters (optional, can be done on setup and also changed later)
        void SetTableScrollFreeze(const int cols, const int rows);  // Set first n cols and/or rows frozen (never scroll)
        void SetTableRowsSelectable(const bool enable = true);      // Enable/disable row selection by user (click, CTRL-click)
        void SetTableRowFilter(const uint64_t filter = 0);          // Set filter

        // Add data. Call these for each cell until the table is full
        // The first cell can have an ##id to support row selection in case of duplicate or empty values for some of those cells
        void AddCellText(const std::string &text);
        void AddCellText(const char *text);
        void AddCellTextF(const char *fmt, ...);
        using CellDrawCb_t = std::function<void(void *)>;
        void AddCellCb(CellDrawCb_t cb, void *arg);
        void AddCellEmpty();

        // Cell stuff (must be called right after AddCell...())
        void SetCellColour(const ImU32 colour);

        // Row stuff (must be called after first AddCell..() of the row and before first AddCell..() of the next row)
        void SetRowFilter(const uint64_t filter); // Set a filter for the current column
        void SetRowColour(const ImU32 colour);

        // Table info, valid after adding data
        int GetNumRows();
        bool IsSelected(const std::string &content, const bool useHovered = true);

        // Clear data. Must add new data afterwards.
        void ClearRows();

        // Draw table
        void DrawTable(const ImVec2 size = ImVec2(0.0f, 0.0f));

        // Clear selected rows
        void ClearSelected();

    protected:

        char _tableName[40];

        int      _freezeCols;
        int      _freezeRows;
        bool     _rowsSelectable;
        uint64_t _rowFilter;

        bool _dirty;

        struct Column
        {
            Column(const std::string &_title, const float _width = 0.0f, const ColumnFlags _flags = ColumnFlags::NONE);
            std::string      title;
            enum ColumnFlags flags;
            FfVec2f           titleSize;
            float            width;
            float            maxWidth;
        };

        struct Cell
        {
            Cell();
            std::string   content;
            FfVec2f        contentSize;
            float         paddingLeft;
            CellDrawCb_t  drawCb;
            void         *drawCbArg;
            ImU32         colour;
        };

        struct Row
        {
            Row();
            std::vector<Cell> cells;
            std::string       first;
            uint64_t          filter;
            ImU32             colour;
        };

        static constexpr ImU32 NO_COLOUR = IM_COL32(0x00, 0x00, 0x00, 0x00); // = GUI_COLOUR_NONE

        static constexpr ImGuiTableFlags TABLE_FLAGS = ImGuiTableFlags_RowBg |ImGuiTableFlags_Borders | //ImGuiTableFlags_NoBordersInBody |
            ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX |
            ImGuiTableFlags_ScrollY;

        Cell &_NextCell();
        bool _CheckData();

        std::vector<Column>  _columns;
        std::vector<Row>     _rows;
        std::unordered_map<std::string, bool> _selectedRows;
        const std::string   *_hoveredRow;
};

/* ****************************************************************************************************************** */

#endif // __GUI_WIDGET_TABLE_HPP__

