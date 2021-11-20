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

/* ****************************************************************************************************************** */

struct GuiWidgetTable
{
    GuiWidgetTable();
    void AddColumn(const char *title, const float width);
    void BeginDraw();
    void ColText(const char *text);
    void ColTextF(const char *fmt, ...);
    bool ColSelectable(const char *label, bool selected = false);
    void ColSkip();

    void EndDraw();

    protected:
        struct Column
        {
            Column(const char *_title, const float _width);
            char  title[100];
            float width;
        };

        std::vector<Column> _columns;
        float               _totalWidth;
};

/* ****************************************************************************************************************** */

#endif // __GUI_WIDGET_TABLE_HPP__

