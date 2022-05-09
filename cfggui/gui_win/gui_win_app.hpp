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

#ifndef __GUI_WIN_APP_HPP__
#define __GUI_WIN_APP_HPP__

#include <string>
#include <vector>

#include "gui_widget_tabbar.hpp"
#include "gui_win.hpp"

/* ***** Base class ************************************************************************************************* */

class GuiWinApp : public GuiWin
{
    public:
        GuiWinApp(const std::string &name);
};

/* ***** About ****************************************************************************************************** */

class GuiWinAppAbout : public GuiWinApp
{
    public:
        GuiWinAppAbout(const std::vector<std::string> &versions);
        void DrawWindow() final;
    private:
        std::vector<std::string> _versions;
        void _DrawEntry(const char *name, const char *license, const char *link = nullptr, const char *link2 = nullptr);
};

/* ***** Settings *************************************************************************************************** */

class GuiWinAppSettings : public GuiWinApp
{
    public:
        GuiWinAppSettings();
        void DrawWindow() final;
    private:
        GuiWidgetTabbar _tabbar;
};

/* ***** Help ******************************************************************************************************* */

class GuiWinAppHelp : public GuiWinApp
{
    public:
        GuiWinAppHelp();
        void DrawWindow() final;
};

/* ***** Legend ***************************************************************************************************** */

class GuiWinAppLegend : public GuiWinApp
{
    public:
        GuiWinAppLegend();
        void DrawWindow() final;
    protected:
        void _DrawFixColourLegend(const int value, const ImU32 colour, const char *label);
};

/* ***** ImGui/ImPlot demos and debugging *************************************************************************** */

#ifndef IMGUI_DISABLE_DEMO_WINDOWS

class GuiWinAppImguiDemo : public GuiWinApp
{
    public:
        GuiWinAppImguiDemo();
        void DrawWindow() final;
};

class GuiWinAppImplotDemo : public GuiWinApp
{
    public:
        GuiWinAppImplotDemo();
        void DrawWindow() final;
};

#endif

#ifndef IMGUI_DISABLE_METRICS_WINDOW

class GuiWinAppImguiMetrics : public GuiWinApp
{
    public:
        GuiWinAppImguiMetrics();
        void DrawWindow() final;
};

#endif

class GuiWinAppImplotMetrics : public GuiWinApp
{
    public:
        GuiWinAppImplotMetrics();
        void DrawWindow() final;
};

class GuiWinAppImguiStyles : public GuiWinApp
{
    public:
        GuiWinAppImguiStyles();
        void DrawWindow() final;
};

class GuiWinAppImplotStyles : public GuiWinApp
{
    public:
        GuiWinAppImplotStyles();
        void DrawWindow() final;
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_APP_HPP__
