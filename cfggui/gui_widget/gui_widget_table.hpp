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
        enum ColumnFlags { NONE = 0, ALIGN_RIGHT = BIT(0), ALIGN_CENTER = BIT(1), SORTABLE = BIT(2), HIDE_SAME = BIT(3) };
        void AddColumn(const char *title, const float width = 0.0f, const /*enum ColumnFlags*/int = ColumnFlags::NONE);

        // Set table parameters (optional, can be done on setup and also changed later)
        void SetTableScrollFreeze(const int cols, const int rows);  // Set first n cols and/or rows frozen (never scroll)
        void SetTableRowFilter(const uint64_t filter = 0);          // Set filter

        // Add data. Call these for each cell until the table is full
        // The first cell can have an ##id to support row selection in case of duplicate or empty values for some of those cells
        void AddCellText(const std::string &text, const uint32_t sort = 0);
        void AddCellText(const char *text, const uint32_t sort = 0);
        void AddCellTextF(const char *fmt, ...);
        using CellDrawCb_t = std::function<void(void *)>;
        void AddCellCb(CellDrawCb_t cb, void *arg); // FIXME: not for the first column!
        void AddCellEmpty(const uint32_t sort = 0);

        // Cell stuff (must be called right after AddCell...())
        void SetCellColour(const ImU32 colour);
        void SetCellSort(const uint32_t sort); // Set sorting key for cell (default: content string)

        // Row stuff (must be called after first AddCell..() of the row and before first AddCell..() of the next row)
        void SetRowFilter(const uint64_t filter); // Set a filter for the current column
        void SetRowColour(const ImU32 colour);
        void SetRowSort(const uint32_t sort); // Set key for sorting rows if no column sort is active (default: in order added)
        void SetRowUid(const uint32_t uid); // Set a non-zero unique ID to enable row(s) selection and checking for selected row(s)

        // Table info, valid after adding data
        int GetNumRows();
        bool IsSelected(const uint32_t uid); // Must use SetRowUid() for this to work
        bool IsHovered(const uint32_t uid); // Must use SetRowUid() for this to work

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
            FfVec2f          titleSize;
            float            width;
            float            maxWidth;
        };

        struct Cell
        {
            Cell();
            std::string   content;
            FfVec2f       contentSize;
            float         paddingLeft;
            CellDrawCb_t  drawCb;
            void         *drawCbArg;
            ImU32         colour;
            uint32_t      sort;
            bool          hide;
        };

        struct Row
        {
            Row();
            std::vector<Cell> cells;
            uint64_t          filter;
            ImU32             colour;
            uint32_t          sort;
            uint32_t          uid;
            private:
                static uint32_t defSort;
        };

        static constexpr ImU32 NO_COLOUR = IM_COL32(0x00, 0x00, 0x00, 0x00); // = GUI_COLOUR_NONE

        static constexpr ImGuiTableFlags TABLE_FLAGS = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | //ImGuiTableFlags_NoBordersInBody |
            ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX |
            ImGuiTableFlags_ScrollY;

        ImGuiTableFlags _tableFlags;

        Cell &_NextCell();
        void _CheckData();
        void _SortData();

        std::vector<Column>  _columns;
        std::vector<Row>     _rows;
        std::unordered_map<uint32_t, bool> _selectedRows; // row.uid
        uint32_t _hoveredRow; // row.uid

        std::vector<ImGuiTableColumnSortSpecs> _sortSpecs;
};

/* ****************************************************************************************************************** */

#endif // __GUI_WIDGET_TABLE_HPP__

