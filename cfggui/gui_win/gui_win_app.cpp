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

GuiWinAbout::GuiWinAbout() :
    GuiWin("About")
{
    _winFlags = ImGuiWindowFlags_AlwaysAutoResize;
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
    ImGui::Text(            "- Dear ImGui %s by Omar Cornut and all Dear ImGui contributors, under a MIT license", ImGui::GetVersion());
    ImGui::SameLine(); Gui::TextLink("https://github.com/ocornut/imgui");
    ImGui::TextUnformatted( "- ImPlot " IMPLOT_VERSION " by Evan Pezent, under a MIT license");
    ImGui::SameLine(); Gui::TextLink("https://github.com/epezent/implot");
    ImGui::TextUnformatted( "- CRC24Q routines from the GPSD project, under a BSD-2-Clause license");
    ImGui::SameLine(); Gui::TextLink("https://gitlab.com/gpsd");
    ImGui::TextUnformatted( "- PlatformFolders by Poul Sander, under a MIT license");
    ImGui::SameLine(); Gui::TextLink("https://github.com/sago007/PlatformFolders");
    ImGui::TextUnformatted( "- DejaVu fonts, under a unnamed license");
    ImGui::SameLine(); Gui::TextLink("https://dejavu-fonts.github.io");
    ImGui::TextUnformatted( "- ProggyClean font by Tristan Grimmer, under a unnamed license");
    ImGui::SameLine(); Gui::TextLink("https://proggyfonts.net");
    ImGui::TextUnformatted( "- ForkAwesome font, under a MIT license");
    ImGui::SameLine(); Gui::TextLink("https://forkaweso.me");
    ImGui::Separator();  // -----------------------------------------------------------------------------
    ImGui::TextUnformatted("Third-party data from the following public sources:");
    int nSources = 0;
    const char **sources = ubloxcfg_getSources(&nSources);
    for (int ix = 0; ix < nSources; ix++)
    {
        ImGui::TextUnformatted(sources[ix]);
    }
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

GuiWinSettings::GuiWinSettings() :
    GuiWin("Settings")
{
    _winSize = { 75, 20 };
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

GuiWinHelp::GuiWinHelp() :
    GuiWin("Help")
{
    _winSize = { 70, 35 };
}

void GuiWinHelp::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    if (ImGui::BeginTabBar("Help", ImGuiTabBarFlags_FittingPolicyScroll))
    {
        if (ImGui::BeginTabItem("cfggui"))
        {
            ImGui::TextUnformatted("Yeah, right, ...");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("ImGui"))
        {
            ImGui::ShowUserGuide();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("ImPlot"))
        {
            ImPlot::ShowUserGuide();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    _DrawWindowEnd();
}

/* ****************************************************************************************************************** */

#ifndef IMGUI_DISABLE_DEMO_WINDOWS

GuiWinImguiDemo::GuiWinImguiDemo() :
    GuiWin("DearImGuiDemo")
{
    //SetTitle("Dear ImGui demo"); // not used, ImGui sets it
}

void GuiWinImguiDemo::DrawWindow()
{
    ImGui::ShowDemoWindow(GetOpenFlag());
}

GuiWinImplotDemo::GuiWinImplotDemo() :
    GuiWin("ImPlotDemo")
{
    //SetTitle("ImPlot demo"); // not used, ImPlot sets it
};

void GuiWinImplotDemo::DrawWindow()
{
    ImPlot::ShowDemoWindow(GetOpenFlag());
}

#endif

/* ****************************************************************************************************************** */

#ifndef IMGUI_DISABLE_METRICS_WINDOW

GuiWinImguiMetrics::GuiWinImguiMetrics() :
    GuiWin("DearImGuiMetrics")
{
    _winSize = { 70, 40 };
    //SetTitle("Dear ImGui metrics"); // not used, ImGui sets it
}

void GuiWinImguiMetrics::DrawWindow()
{
    ImGui::ShowMetricsWindow(GetOpenFlag());
}
#endif

/* ****************************************************************************************************************** */

GuiWinImplotMetrics::GuiWinImplotMetrics() :
    GuiWin("ImPlotMetrics")
{
    _winSize = { 70, 40 };
    //SetTitle("ImPlot metrics"); // not used, ImPlot sets it
}

void GuiWinImplotMetrics::DrawWindow()
{
    ImPlot::ShowMetricsWindow(GetOpenFlag());
}

/* ****************************************************************************************************************** */

GuiWinImguiStyles::GuiWinImguiStyles() :
    GuiWin("DearImGuiStyles")
{
    _winSize = { 70, 40 };
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

GuiWinImplotStyles::GuiWinImplotStyles() :
    GuiWin("ImPlotStyles")
{
    _winSize = { 70, 40 };
    SetTitle("ImPlot styles");
}

void GuiWinImplotStyles::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    ImPlot::ShowStyleEditor();

    _DrawWindowEnd();
}

/* ****************************************************************************************************************** */
