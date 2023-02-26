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

#ifndef __GUI_WIN_DATA_MAP_HPP__
#define __GUI_WIN_DATA_MAP_HPP__

#include "ff_stuff.h"
#include "gui_widget_map.hpp"
#include "gui_win_data.hpp"

/* ***** Map ******************************************************************************************************** */

class GuiWinDataMap : public GuiWinData
{
    public:

        GuiWinDataMap(const std::string &name, std::shared_ptr<Database> database);
       ~GuiWinDataMap();

    private:

        void _DrawContent() final;

        GuiWidgetMap _map;
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_MAP_HPP__
