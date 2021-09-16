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

#include "imgui.h"
#include "implot.h"
#include "IconsForkAwesome.h"

#include "ff_stuff.h"
#include "config.h"

#include "gui_win.hpp"
#include "gui_app.hpp"

/* ****************************************************************************************************************** */

GuiWin::GuiWin() :
    _winTitle        { "Default" },
    _winName         { "Default" },
    _winImguiName    { "Default###Default" },
    _winOpen         { false },
    _winPos          { POS_USER },
    _winRePos        { POS_NONE },
    _winIniPos       { POS_USER },
    _winFlags        { 0 },
    _winSize         { 30, 30 }, // > 0: units of fontsize, < 0: = fraction of screen width/height
    _winSizeMin      { 0, 0 },
    _winUid          { reinterpret_cast<std::uintptr_t>(this) },
    _winSettings     { GuiApp::GetInstance().GetSettings() }
{
}

GuiWin::GuiWin(const std::string &name) :
    _winSettings { GuiApp::GetInstance().GetSettings() }
{
    _winName = name;
    SetTitle(_winTitle);
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
    ImGui::SetWindowFocus(_winTitle.c_str());
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
    auto &io = ImGui::GetIO();

    // Set window position
    {
        const float padding = 10.0f;
        const float topOffs = _winSettings->menuBarHeight;

        enum POS_e position;
        ImGuiCond cond = ImGuiCond_Always;
        if (_winRePos != POS_NONE)
        {
            position    = _winRePos;
            _winRePos = POS_NONE;
        }
        else if (_winIniPos != POS_NONE)
        {
            position = _winIniPos;
            _winIniPos = POS_NONE;
            cond = ImGuiCond_FirstUseEver;
        }
        else
        {
            position = _winPos;
        }
        switch (position)
        {
            case POS_NW:
                ImGui::SetNextWindowPos(
                    ImVec2(padding, topOffs + padding), cond, ImVec2(0.0f, 0.0f));
                break;
            case POS_NE:
                ImGui::SetNextWindowPos(
                    ImVec2(io.DisplaySize.x - padding, topOffs + padding), cond, ImVec2(1.0f, 0.0f));
                break;
            case POS_SE:
                ImGui::SetNextWindowPos(
                    ImVec2(io.DisplaySize.x - padding, io.DisplaySize.y - padding), cond, ImVec2(1.0f, 1.0f));
                break;
            case POS_SW:
                ImGui::SetNextWindowPos(
                    ImVec2(padding, io.DisplaySize.y - padding), cond, ImVec2(0.0f, 1.0f));
                break;

            case POS_N:
                ImGui::SetNextWindowPos(
                    ImVec2(io.DisplaySize.x / 2.0f, topOffs + padding), cond, ImVec2(0.5f, 0.0f));
                break;
            case POS_S:
                ImGui::SetNextWindowPos(
                    ImVec2(io.DisplaySize.x / 2.0f, io.DisplaySize.y - padding), cond, ImVec2(0.5f, 1.0f));
                break;

            case POS_E:
                ImGui::SetNextWindowPos(
                    ImVec2(io.DisplaySize.x - padding, io.DisplaySize.y / 2.0f), cond, ImVec2(1.0f, 0.5f));
                break;

            case POS_W:
                ImGui::SetNextWindowPos(
                    ImVec2(padding, io.DisplaySize.y / 2.0f), cond, ImVec2(0.0f, 0.5f));
                break;

            case POS_USER:
            case POS_NONE:
                break;
        }
    }

    // Start drawing window
    {
        if ( (_winFlags & ImGuiWindowFlags_AlwaysAutoResize) == 0)
        {
            // > 0 = units of character width/height, < 0 = fraction of display (main window) size
            /*ImVec2 size = _winSize;
            size.x *= _winSize.x < 0.0 ? -io.DisplaySize.x : _winSettings->charSize.x;
            if (size.y != 0.0f)
            {
                size.y *= _winSize.y < 0.0 ? -io.DisplaySize.y : _winSettings->charSize.y;
            }
            else
            {
                size.y = size.x;
            }*/
            ImGui::SetNextWindowSize(_WinSizeToVec(_winSize), ImGuiCond_FirstUseEver);
            if (_winSizeMin.x != 0.0)
            {
                ImGui::SetNextWindowSizeConstraints(_WinSizeToVec(_winSizeMin), ImVec2(FLT_MAX, FLT_MAX));
            }
        }

        if (!ImGui::Begin(_winImguiName.c_str(), &_winOpen, _winFlags))
        {
            ImGui::End();
            return false;
        }
    }

    // Add window context menu
    {
        if ( (_winFlags & ImGuiWindowFlags_NoTitleBar) == 0 ?
            ImGui::BeginPopupContextItem() : ImGui::BeginPopupContextWindow() )
        {
            ImGui::BeginPopupContextItem();
            _DrawWindowContextMenuItems();
            ImGui::EndPopup();
        }
    }

    return true;
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

void GuiWin::_DrawWindowContextMenuItems(const bool lockPosition)
{
    enum POS_e position = _winPos;
    ImGui::TextUnformatted(lockPosition ? "Position" : "Move to");
    ImGui::RadioButton("##North-west", (int *)&position, POS_NW);   ImGui::SameLine();
    ImGui::RadioButton("##North",      (int *)&position, POS_N);    ImGui::SameLine();
    ImGui::RadioButton("##North-east", (int *)&position, POS_NE);

    ImGui::RadioButton("##West",       (int *)&position, POS_W);    ImGui::SameLine();
    ImGui::RadioButton("##User",       (int *)&position, POS_USER); ImGui::SameLine();
    ImGui::RadioButton("##East",       (int *)&position, POS_E);

    ImGui::RadioButton("##South-west", (int *)&position, POS_SW);   ImGui::SameLine();
    ImGui::RadioButton("##South",      (int *)&position, POS_S);    ImGui::SameLine();
    ImGui::RadioButton("##South-east", (int *)&position, POS_SE);

    if (lockPosition)
    {
        _winPos   = position;
        _winRePos = POS_NONE;
    }
    else
    {
        _winRePos = position;
    }

    ImGui::Separator();

    //ImGui::SetNextItemWidth(100.0);
    //ImGui::DragFloat("##scale", &_scale, 0.01f, 0.25f, 4.0f, "%.1f");
    //ImGui::Separator();

    if (ImGui::MenuItem("Close"))
    {
        Close();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWin::ToggleButton(const char *labelOn, const char *labelOff, bool *toggle, const char *tooltipOn, const char *tooltipOff)
{
    bool res = false;
    const bool enabled = *toggle;
    if (labelOff == NULL)
    {
        if (!enabled) { ImGui::PushStyleColor(ImGuiCol_Text, Gui::Gray); }
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
