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

#include "gui_settings.hpp"
#include "gui_win_app.hpp"
#include "gui_app.hpp"

/* ****************************************************************************************************************** */

GuiWinAbout::GuiWinAbout()
{
    _winName     = "About";
    SetTitle("About");
    _winFlags    = ImGuiWindowFlags_AlwaysAutoResize;
}

void GuiWinAbout::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
    ImGui::TextUnformatted( "cfggui " CONFIG_VERSION " (" CONFIG_GITHASH ")");
    ImGui::PopStyleColor();
    ImGui::TextUnformatted( "Copyright (c) 2020-2021 Philippe Kehl (flipflip at oinkzwurgl dot org)");
    Gui::TextLink(          "https://oinkzwurgl.org/hacking/ubloxcfg/");
    ImGui::Separator();  // -----------------------------------------------------------------------------
    ImGui::TextUnformatted("This consists of the following parts:");
    ImGui::TextUnformatted("- 'ubloxcfg' library, under GPL");
    ImGui::TextUnformatted("- 'ff' library, under GPL");
    ImGui::TextUnformatted("- 'cfggui' application, under GPL");
    ImGui::Separator();  // -----------------------------------------------------------------------------
    ImGui::TextUnformatted( "Third-party code (see source code and the links for details):");
    ImGui::Text(            "- Dear ImGui %s by Omar Cornut and all Dear ImGui contributors, under a\n"
                            "  MIT license", ImGui::GetVersion());
    ImGui::TextUnformatted( "  "); ImGui::SameLine(0,0); Gui::TextLink("https://github.com/ocornut/imgui");
    ImGui::TextUnformatted( "- ImPlot " IMPLOT_VERSION " by Evan Pezent, under a MIT license");
    ImGui::TextUnformatted( "  "); ImGui::SameLine(0,0); Gui::TextLink("https://github.com/epezent/implot");
    ImGui::TextUnformatted( "- CRC24Q routines from the GPSD project, under a BSD-2-Clause license");
    ImGui::TextUnformatted( "  "); ImGui::SameLine(0,0); Gui::TextLink("https://gitlab.com/gpsd");
    ImGui::TextUnformatted( "- PlatformFolders by Poul Sander, under a MIT license");
    ImGui::TextUnformatted( "  "); ImGui::SameLine(0,0); Gui::TextLink("https://github.com/sago007/PlatformFolders");
    ImGui::TextUnformatted( "- DejaVu fonts, under a unnamed license");
    ImGui::TextUnformatted( "  "); ImGui::SameLine(0,0); Gui::TextLink("https://dejavu-fonts.github.io");
    ImGui::TextUnformatted( "- ProggyClean font by Tristan Grimmer, under a unnamed license");
    ImGui::TextUnformatted( "  "); ImGui::SameLine(0,0); Gui::TextLink("https://proggyfonts.net");
    ImGui::TextUnformatted( "- ForkAwesome font, under a MIT license");
    ImGui::TextUnformatted( "  "); ImGui::SameLine(0,0); Gui::TextLink("https://forkaweso.me");
    ImGui::Separator();  // -----------------------------------------------------------------------------

    ImGuiIO &io = ImGui::GetIO();
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
    ImGui::Text("Platform: %s, renderer: %s",
        io.BackendPlatformName ? io.BackendPlatformName : "?",
        io.BackendRendererName ? io.BackendRendererName : "?");
    ImGui::PopStyleColor();

    _DrawWindowEnd();
}

/* ****************************************************************************************************************** */

#ifndef IMGUI_DISABLE_DEMO_WINDOWS

GuiWinImguiDemo::GuiWinImguiDemo()
{
    _winTitle    = "Dear ImGui demo"; // not used, ImGui sets it
    _winName     = "DearImGuiDemo";
}

void GuiWinImguiDemo::DrawWindow()
{
    ImGui::ShowDemoWindow(GetOpenFlag());
}

GuiWinImplotDemo::GuiWinImplotDemo()
{
    _winTitle    = "ImPlot demo"; // not used, ImPlot sets it
    _winName     = "ImPlotDemo";
};

void GuiWinImplotDemo::DrawWindow()
{
    ImPlot::ShowDemoWindow(GetOpenFlag());
}

#endif

/* ****************************************************************************************************************** */

#ifndef IMGUI_DISABLE_METRICS_WINDOW

GuiWinImguiMetrics::GuiWinImguiMetrics()
{
    _winSize     = { 70, 40 };
    _winTitle    = "Dear ImGui metrics"; // not used, ImGui sets it
    _winName     = "DearImGuiMetrics";
}

void GuiWinImguiMetrics::DrawWindow()
{
    ImGui::ShowMetricsWindow(GetOpenFlag());
}
#endif

/* ****************************************************************************************************************** */

GuiWinImplotMetrics::GuiWinImplotMetrics()
{
    _winSize     = { 70, 40 };
    _winTitle    = "ImPlot metrics"; // not used, ImPlot sets it
    _winName     = "ImPlotMetrics";
}

void GuiWinImplotMetrics::DrawWindow()
{
    ImPlot::ShowMetricsWindow(GetOpenFlag());
}

/* ****************************************************************************************************************** */

GuiWinImguiStyles::GuiWinImguiStyles()
{
    _winSize     = { 70, 40 };
    _winName     = "DearImGuiStyles";
    SetTitle("Dear ImGui styles");
}

void GuiWinImguiStyles::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    ImGui::ShowStyleEditor();

    _DrawWindowEnd();
}

/* ****************************************************************************************************************** */

GuiWinImguiAbout::GuiWinImguiAbout()
{
    _winName     = "DearImGuiAbout";
    SetTitle("Dear ImGui about");
}

void GuiWinImguiAbout::DrawWindow()
{
    ImGui::ShowAboutWindow(GetOpenFlag());
}

/* ****************************************************************************************************************** */

GuiWinSettings::GuiWinSettings()
{
    _winName     = "Settings";
    _winSize     = { 75, 20 };
    SetTitle("Settings");
}

void GuiWinSettings::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    _winSettings->DrawSettingsEditor();

    _DrawWindowEnd();
}


/* ****************************************************************************************************************** */

GuiWinHelp::GuiWinHelp()
{
    _winName     = "Help";
    _winSize     = { 70, 35 };
    SetTitle("Help");
}

void GuiWinHelp::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    ImGui::ShowUserGuide();
    ImGui::Separator();
    ImPlot::ShowUserGuide();

    _DrawWindowEnd();
}

/* ****************************************************************************************************************** */

GuiWinDebug::GuiWinDebug() :
    _log{}
{
    _winName       = "DebugLog";
    _winOpen       = true;
    _winIniPos     = POS_SE;
    _winSize       = { 100, 15 };
    SetTitle("Debug log");
}

void GuiWinDebug::AddLog(const char *line, const ImU32 colour)
{
    _log.AddLine(line, colour);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDebug::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    _log.DrawWidget();

    _DrawWindowEnd();
}

/* ****************************************************************************************************************** */
