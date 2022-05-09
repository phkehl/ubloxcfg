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

#ifndef __GUI_WIDGET_TABBAR_HPP__
#define __GUI_WIDGET_TABBAR_HPP__

#include <string>
#include <functional>

/* ****************************************************************************************************************** */

class GuiWidgetTabbar
{
    public:

        GuiWidgetTabbar(const std::string &name, const int flags = 0 /* ImGuiTabBarFlags_None */);
       ~GuiWidgetTabbar();

        bool Begin();
        void Item(const std::string &label, std::function<void(void)> cb);
        void End();

    private:

        std::string _name;
        int         _tabbarFlags;
        std::string _selected;
        std::string _setSelected;
};

/* ****************************************************************************************************************** */

#endif // __GUI_WIDGET_TABBAR_HPP__
