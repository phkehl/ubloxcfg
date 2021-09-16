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

#include <cinttypes>
#include <memory>
#include <map>
#include <string>

#include "imgui.h"

#include "ff_cpp.hpp"

#include "gui_win.hpp"

/* ***** About ********************************************************************************** */

class GuiWinAbout : public GuiWin
{
    public:
        GuiWinAbout();

        void DrawWindow() override;

    protected:
};

/* ***** ImGui Demos **************************************************************************** */

#ifndef IMGUI_DISABLE_DEMO_WINDOWS

class GuiWinImguiDemo : public GuiWin
{
    public:
        GuiWinImguiDemo();

        void DrawWindow() override;

    protected:
};

class GuiWinImplotDemo : public GuiWin
{
    public:
        GuiWinImplotDemo();

        void DrawWindow() override;

    protected:
};

#endif

#ifndef IMGUI_DISABLE_METRICS_WINDOW

class GuiWinImguiMetrics : public GuiWin
{
    public:
        GuiWinImguiMetrics();

        void DrawWindow() override;

    protected:
};

#endif

class GuiWinImplotMetrics : public GuiWin
{
    public:
        GuiWinImplotMetrics();

        void DrawWindow() override;

    protected:
};

class GuiWinImguiStyles : public GuiWin
{
    public:
        GuiWinImguiStyles();

        void DrawWindow() override;

    protected:
};

class GuiWinImguiAbout : public GuiWin
{
    public:
        GuiWinImguiAbout();

        void DrawWindow() override;

    protected:
};

/* ***** Settings ******************************************************************************* */

class GuiWinSettings : public GuiWin
{
    public:
        GuiWinSettings();

        void                 DrawWindow() override;

    protected:
};

/* ***** Help *********************************************************************************** */

class GuiWinHelp : public GuiWin
{
    public:
        GuiWinHelp();

        void                 DrawWindow() override;

    protected:
};

/* ***** Debug log ****************************************************************************** */

class GuiWinDebug : public GuiWin
{
    public:
        GuiWinDebug();

        void DrawWindow() override;
        void AddLog(const char *line, const ImU32 colour = Gui::White);

    protected:
        GuiWidgetLog _log;
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_APP_H__
