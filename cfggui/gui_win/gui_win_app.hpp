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

#ifndef __GUI_WIN_APP_H__
#define __GUI_WIN_APP_H__

#include "gui_win.hpp"

/* ***** About ****************************************************************************************************** */

class GuiWinAbout : public GuiWin
{
    public:
        GuiWinAbout();
        void DrawWindow() final;
};

/* ***** Settings *************************************************************************************************** */

class GuiWinSettings : public GuiWin
{
    public:
        GuiWinSettings();
        void DrawWindow() final;
};

/* ***** Help ******************************************************************************************************* */

class GuiWinHelp : public GuiWin
{
    public:
        GuiWinHelp();
        void DrawWindow() final;
};

/* ***** ImGui/ImPlot demos and debugging *************************************************************************** */

#ifndef IMGUI_DISABLE_DEMO_WINDOWS

class GuiWinImguiDemo : public GuiWin
{
    public:
        GuiWinImguiDemo();
        void DrawWindow() final;
};

class GuiWinImplotDemo : public GuiWin
{
    public:
        GuiWinImplotDemo();
        void DrawWindow() final;
};

#endif

#ifndef IMGUI_DISABLE_METRICS_WINDOW

class GuiWinImguiMetrics : public GuiWin
{
    public:
        GuiWinImguiMetrics();
        void DrawWindow() final;
};

#endif

class GuiWinImplotMetrics : public GuiWin
{
    public:
        GuiWinImplotMetrics();
        void DrawWindow() final;
};

class GuiWinImguiStyles : public GuiWin
{
    public:
        GuiWinImguiStyles();
        void DrawWindow() final;
};

class GuiWinImplotStyles : public GuiWin
{
    public:
        GuiWinImplotStyles();
        void DrawWindow() final;
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_APP_H__
