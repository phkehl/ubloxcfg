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

#include "gui_inc.hpp"

#include "gui_widget_tabbar.hpp"

/* ****************************************************************************************************************** */

GuiWidgetTabbar::GuiWidgetTabbar(const std::string &name, const int flags) :
    _name        { name },
    _tabbarFlags { flags }
{
    _setSelected = GuiSettings::GetValue("GuiWidgetTabbar." + _name);
}

GuiWidgetTabbar::~GuiWidgetTabbar()
{
    GuiSettings::SetValue("GuiWidgetTabbar." + _name, _selected);
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetTabbar::Begin()
{
    return ImGui::BeginTabBar(_name.c_str(), _tabbarFlags);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTabbar::End()
{
    _setSelected.clear();
    ImGui::EndTabBar();
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetTabbar::Item(const std::string &label)
{
    bool res = false;
    ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
    auto hashPos = label.rfind('#');
    if ( !_setSelected.empty() &&
        (
            ( ( (hashPos == std::string::npos) && (label == _setSelected) ) ||
              ( (hashPos != std::string::npos) && (label.substr(hashPos + 1) == _setSelected) ) )
        ))
    {
        _setSelected.clear();
        flags |= ImGuiTabItemFlags_SetSelected;
    }
    if (ImGui::BeginTabItem(label.c_str(), nullptr, flags))
    {
        if (_selected != label)
        {
            _selected = hashPos == std::string::npos ? label : label.substr(hashPos + 1);
        }
        res = true;
        ImGui::EndTabItem();
    }
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetTabbar::Item(const std::string &label, std::function<void(void)> cb)
{
    if (Item(label))
    {
        cb();
    }
}

/* ****************************************************************************************************************** */
