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
#include <cmath>

#include "ff_stuff.h"
#include "config.h"

#include "gui_inc.hpp"
#include "gui_app.hpp"

#include "gui_win.hpp"

/* ****************************************************************************************************************** */

GuiWin::GuiWin(const std::string &name) :
    _winTitle         { "Default" },
    _winName          { "Default" },
    _winImguiName     { "Default###Default" },
    _winOpen          { false },
    _winDrawn         { false },
    _winFlags         { ImGuiWindowFlags_None },
    _winSize          { 30, 30 }, // > 0: units of fontsize, < 0: = fraction of screen width/height
    _winSizeMin       { 0, 0 },
    _winUid           { reinterpret_cast<std::uintptr_t>(this) },
    _winSettings      { GuiApp::GetInstance().GetSettings() },
    _winClass         { std::make_unique<ImGuiWindowClass>() }
{
    _winUidStr = Ff::Sprintf("%016lx", _winUid);
    _winName = name;
    SetTitle(name); // by default the (internal) name (= ID) is also the displayed title
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWin::Open()
{
    _winOpen = true;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWin::Close()
{
    _winOpen = false;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWin::IsOpen()
{
    return _winOpen;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWin::IsDrawn()
{
    return _winDrawn;
}

// ---------------------------------------------------------------------------------------------------------------------

bool *GuiWin::GetOpenFlag()
{
    return &_winOpen;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::string &GuiWin::GetName()
{
    return _winName;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::string &GuiWin::GetTitle()
{
    return _winTitle;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWin::SetTitle(const std::string &title)
{
    _winTitle = title;
    _winImguiName = _winTitle + std::string("###") + _winName;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWin::Focus()
{
    ImGui::SetWindowFocus(_winImguiName.c_str());
}

// ---------------------------------------------------------------------------------------------------------------------

const std::string &GuiWin::GetUidStr()
{
    return _winUidStr;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWin::Loop(const uint32_t &frame, const double &now)
{
    (void)frame;
    (void)now;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWin::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }
    ImGui::TextUnformatted("Hello!");
    _DrawWindowEnd();
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWin::_DrawWindowBegin()
{
    // Start drawing window

    if (!CHKBITS(_winFlags, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::SetNextWindowSize(_WinSizeToVec(_winSize), ImGuiCond_FirstUseEver);
        if (_winSizeMin.x != 0.0f)
        {
            ImGui::SetNextWindowSizeConstraints(_WinSizeToVec(_winSizeMin), ImVec2(FLT_MAX, FLT_MAX));
        }
    }

    ImGui::SetNextWindowClass(_winClass.get());

    _winDrawn = ImGui::Begin(_winImguiName.c_str(), &_winOpen, _winFlags);

    if (!_winDrawn)
    {
        ImGui::End();
    }

    return _winDrawn;
}

// ---------------------------------------------------------------------------------------------------------------------

ImVec2 GuiWin::_WinSizeToVec(ImVec2 size)
{
    auto &io = ImGui::GetIO();
    size.x *= size.x < 0.0 ? -io.DisplaySize.x : _winSettings->charSize.x;
    if (size.y != 0.0f)
    {
        size.y *= size.y < 0.0 ? -io.DisplaySize.y : _winSettings->charSize.y;
    }
    else
    {
        size.y = size.x;
    }
    return size;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWin::_DrawWindowEnd()
{
    ImGui::End();
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWin::ToggleButton(const char *labelOn, const char *labelOff, bool *toggle, const char *tooltipOn, const char *tooltipOff)
{
    bool res = false;
    const bool enabled = *toggle;
    if (labelOff == NULL)
    {
        if (!enabled) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_GREY)); }
        if (ImGui::Button(labelOn, _winSettings->iconButtonSize))
        {
            *toggle = !*toggle;
            res = true;
        }
        if (!enabled) { ImGui::PopStyleColor(); }
        if (tooltipOff == NULL)
        {
            Gui::ItemTooltip(tooltipOn);
        }
        else
        {
            Gui::ItemTooltip(enabled ? tooltipOn : tooltipOff);
        }
    }
    else
    {
        if (enabled)
        {
            if (ImGui::Button(labelOn,  _winSettings->iconButtonSize))
            {
                *toggle = false;
                res = true;
            }
            Gui::ItemTooltip(tooltipOn);
        }
        else
        {
            if (ImGui::Button(labelOff,  _winSettings->iconButtonSize))
            {
                *toggle = true;
                res = true;
            }
            Gui::ItemTooltip(tooltipOff != NULL ? tooltipOff : tooltipOn);
        }
    }
    return res;
}

/* ****************************************************************************************************************** */
