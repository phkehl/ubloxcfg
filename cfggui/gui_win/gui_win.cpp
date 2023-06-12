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
#include <cmath>

#include "ff_stuff.h"
#include "config.h"

#include "gui_inc.hpp"
#include "gui_app.hpp"

#include "gui_win.hpp"
#include "imgui_internal.h"

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
    _winResize        { -1, 0 },
    _winMoveTo        { -1, 0 },
    _winUid           { reinterpret_cast<std::uintptr_t>(this) },
    _winUidStr        { Ff::Sprintf("%016lx", _winUid) },
    _winClass         { std::make_unique<ImGuiWindowClass>() },
    _winIsDocked      { false },
    _winIsFocused     { true }
{
    _winName = name;
    _newWinInitPos = NEW_WIN_POS[_newWinPosIx++];
    _newWinPosIx %= NEW_WIN_POS.size();
    WinSetTitle(); // by default the (internal) name (= ID) is also the displayed title
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ const std::vector<FfVec2f> GuiWin::NEW_WIN_POS =
{
    { 20, 20 }, { 40, 40 }, { 60, 60 }, { 80, 80 }, { 100, 100 }, { 120, 120 }, { 140, 140 }, { 160, 160 }, { 180, 180 },
};

/*static*/ int GuiWin::_newWinPosIx = 0;

// ---------------------------------------------------------------------------------------------------------------------

void GuiWin::WinOpen()
{
    _winOpen = true;
    WinFocus();
    ImGui::SetWindowCollapsed(_winImguiName.c_str(), false);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWin::WinClose()
{
    _winOpen = false;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWin::WinIsOpen()
{
    return _winOpen;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWin::WinIsDrawn()
{
    return _winDrawn;
}

// ---------------------------------------------------------------------------------------------------------------------

bool *GuiWin::WinOpenFlag()
{
    return &_winOpen;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::string &GuiWin::WinName()
{
    return _winName;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::string &GuiWin::WinTitle()
{
    return _winTitle;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWin::WinSetTitle(const std::string &title)
{
    _winTitle = title.empty() ? _winName : title;
    _winImguiName = _winTitle + std::string("###") + _winName;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWin::WinFocus()
{
    ImGui::SetWindowFocus(_winImguiName.c_str());
}

// ---------------------------------------------------------------------------------------------------------------------

const std::string &GuiWin::WinUidStr()
{
    return _winUidStr;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWin::WinMoveTo(const ImVec2 &pos)
{
    _winMoveTo = pos;
}

void GuiWin::WinResize(const ImVec2 &size)
{
    if ( (size.x <= _winSizeMin.x) || (size.y <= _winSizeMin.y) )
    {
        _winResize = _winSize;
    }
    else
    {
        _winResize = size;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWin::WinIsDocked()
{
    return _winIsDocked;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWin::WinIsFocused()
{
    return _winIsFocused;
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
        if (_winResize.x > 0.0f)
        {
            ImGui::SetNextWindowSize(_WinSizeToVec(_winSize), ImGuiCond_Always);
            _winResize.x = -1.0f;
            _winResize.y = 0.0f;
        }
        else
        {
            ImGui::SetNextWindowSize(_WinSizeToVec(_winSize), ImGuiCond_FirstUseEver);
        }
        if (_winSizeMin.x != 0.0f)
        {
            ImGui::SetNextWindowSizeConstraints(_WinSizeToVec(_winSizeMin), ImVec2(FLT_MAX, FLT_MAX));
        }
    }

    ImGui::SetNextWindowClass(_winClass.get());
    if ( (_winMoveTo.x > 0) && (_winMoveTo.y > 0) )
    {
        ImGui::SetNextWindowPos(_winMoveTo, ImGuiCond_Always);
        _winMoveTo.x = -1.0f;
    }
    else
    {
        ImGui::SetNextWindowPos(_newWinInitPos, ImGuiCond_FirstUseEver);
    }

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
    size.x *= size.x < 0.0 ? -io.DisplaySize.x : GuiSettings::charSize.x;
    if (size.y != 0.0f)
    {
        size.y *= size.y < 0.0 ? -io.DisplaySize.y : GuiSettings::charSize.y;
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
    ImGuiWindow *win = ImGui::GetCurrentWindow();
    _winIsDocked = win->DockIsActive;
    _winIsFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    ImGui::End();
}

/* ****************************************************************************************************************** */
