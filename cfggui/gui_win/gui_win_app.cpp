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

#include "gui_win_app.hpp"

/* ****************************************************************************************************************** */

GuiWinApp::GuiWinApp(const std::string &name) :
    GuiWin(name)
{
    _winFlags |= ImGuiWindowFlags_NoDocking;
}

/* ****************************************************************************************************************** */

GuiWinAppAbout::GuiWinAppAbout() :
    GuiWinApp("About")
{
    _winFlags |= ImGuiWindowFlags_AlwaysAutoResize;
}

void GuiWinAppAbout::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    ImGui::PushFont(GuiSettings::fontSans);

    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
    ImGui::TextUnformatted( "cfggui " CONFIG_VERSION " (" CONFIG_GITHASH ")");
    ImGui::PopStyleColor();
    ImGui::TextUnformatted( "Copyright (c) 2020-2021 Philippe Kehl (flipflip at oinkzwurgl dot org)");
    Gui::TextLink(          "https://oinkzwurgl.org/hacking/ubloxcfg/");
    ImGui::Separator();  // -----------------------------------------------------------------------------
    ImGui::TextUnformatted("This consists of the following parts:");
    ImGui::TextUnformatted("- 'ubloxcfg' library, under LGPL");
    ImGui::TextUnformatted("- 'ff' library, under GPL");
    ImGui::TextUnformatted("- 'cfggui' application, under GPL");
    ImGui::Separator();  // -----------------------------------------------------------------------------
    ImGui::TextUnformatted( "Third-party code (see source code and the links for details):");
    ImGui::TextUnformatted( "- Dear ImGui, under a MIT license");
    ImGui::SameLine(); Gui::TextLink("https://github.com/ocornut/imgui");
    ImGui::TextUnformatted( "- ImPlot, under a MIT license");
    ImGui::SameLine(); Gui::TextLink("https://github.com/epezent/implot");
    ImGui::TextUnformatted( "- CRC24Q routines (from GPSD), under a BSD-2-Clause license");
    ImGui::SameLine(); Gui::TextLink("https://gitlab.com/gpsd");
    ImGui::TextUnformatted( "- PlatformFolders, under a MIT license");
    ImGui::SameLine(); Gui::TextLink("https://github.com/sago007/PlatformFolders");
    ImGui::TextUnformatted( "- DejaVu fonts, under a unnamed license");
    ImGui::SameLine(); Gui::TextLink("https://dejavu-fonts.github.io");
    // ImGui::TextUnformatted( "- ProggyClean font, under a unnamed license");
    // ImGui::SameLine(); Gui::TextLink("https://proggyfonts.net");
    ImGui::TextUnformatted( "- ForkAwesome font, under a MIT license");
    ImGui::SameLine(); Gui::TextLink("https://forkaweso.me");
    ImGui::TextUnformatted( "- Tetris, under a Revised BSD license");
    ImGui::SameLine(); Gui::TextLink("https://brennan.io/2015/06/12/tetris-reimplementation/");
    ImGui::TextUnformatted( "- NanoVG, under a Zlib license");
    ImGui::SameLine(); Gui::TextLink("https://github.com/memononen/nanovg");
    ImGui::SameLine(); Gui::TextLink("https://github.com/inniyah/nanovg");
    //ImGui::TextUnformatted( "- Miniz, under a MIT license");
    //ImGui::SameLine(); Gui::TextLink("https://github.com/richgel999/miniz");
    ImGui::Separator();  // -----------------------------------------------------------------------------
    ImGui::TextUnformatted("Third-party libraries (first level dependencies, use ldd to see all):");
    ImGui::TextUnformatted( "- GLFW");
    ImGui::SameLine(); Gui::TextLink("https://www.glfw.org");
    ImGui::TextUnformatted( "- libcurl");
    ImGui::SameLine(); Gui::TextLink("https://curl.se/");
    ImGui::TextUnformatted( "- FreeType");
    ImGui::SameLine(); Gui::TextLink("https://freetype.org/");
    ImGui::Separator();  // -----------------------------------------------------------------------------
    ImGui::TextUnformatted("Third-party data from the following public sources:");
    int nSources = 0;
    const char **sources = ubloxcfg_getSources(&nSources);
    for (int ix = 0; ix < nSources; ix++)
    {
        ImGui::Text("- %s", sources[ix]);
    }
    ImGui::PopFont();

    _DrawWindowEnd();
}

/* ****************************************************************************************************************** */

GuiWinAppSettings::GuiWinAppSettings(std::function<void()> drawCb) :
    GuiWinApp("Settings"),
    _drawCb { drawCb }
{
    _winSize    = { 72, 25 };
    _winSizeMin = { 72, 15 };
}

void GuiWinAppSettings::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    if (_drawCb)
    {
        _drawCb();
    }

    _DrawWindowEnd();
}


/* ****************************************************************************************************************** */

GuiWinAppHelp::GuiWinAppHelp() :
    GuiWinApp("Help")
{
    _winSize = { 70, 35 };
}

void GuiWinAppHelp::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    if (ImGui::BeginTabBar("Help", ImGuiTabBarFlags_FittingPolicyScroll))
    {
        if (ImGui::BeginTabItem("cfggui"))
        {
            ImGui::PushFont(GuiSettings::fontSans);
            ImGui::TextUnformatted("Yeah, right, ...");
            ImGui::EndTabItem();
            ImGui::PopFont();
        }
        if (ImGui::BeginTabItem("ImGui"))
        {
            ImGui::PushFont(GuiSettings::fontSans);
            ImGui::ShowUserGuide();
            ImGui::BulletText("Hold SHIFT while moving window to dock into other windows.\n"
                "(VERY experimental. WILL crash sometimes!");
            ImGui::PopFont();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("ImPlot"))
        {
            ImGui::PushFont(GuiSettings::fontSans);
            ImPlot::ShowUserGuide();
            ImGui::PopFont();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    _DrawWindowEnd();
}

/* ****************************************************************************************************************** */

GuiWinAppLegend::GuiWinAppLegend() :
    GuiWinApp("Legend")
{
    _winFlags |= ImGuiWindowFlags_AlwaysAutoResize;
}

void GuiWinAppLegend::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    if (ImGui::BeginTabBar("tabs"))
    {
        if (ImGui::BeginTabItem("Fix colours"))
        {
            _DrawFixColourLegend(EPOCH_FIX_NOFIX,        GUI_COLOUR(FIX_INVALID),      "Invalid");
            _DrawFixColourLegend(-1,                     GUI_COLOUR(FIX_MASKED),       "Masked");
            _DrawFixColourLegend(EPOCH_FIX_DRONLY,       GUI_COLOUR(FIX_DRONLY),       "Dead-reckoning only");
            _DrawFixColourLegend(EPOCH_FIX_TIME,         GUI_COLOUR(FIX_S2D),          "Single 2D");
            _DrawFixColourLegend(EPOCH_FIX_S2D,          GUI_COLOUR(FIX_S3D),          "Single 3D");
            _DrawFixColourLegend(EPOCH_FIX_S3D,          GUI_COLOUR(FIX_S3D_DR),       "Single 3D + dead-reckoning");
            _DrawFixColourLegend(EPOCH_FIX_S3D_DR,       GUI_COLOUR(FIX_TIME),         "Single time only");
            _DrawFixColourLegend(EPOCH_FIX_RTK_FLOAT,    GUI_COLOUR(FIX_RTK_FLOAT),    "RTK float");
            _DrawFixColourLegend(EPOCH_FIX_RTK_FIXED,    GUI_COLOUR(FIX_RTK_FIXED),    "RTK fixed");
            _DrawFixColourLegend(EPOCH_FIX_RTK_FLOAT_DR, GUI_COLOUR(FIX_RTK_FLOAT_DR), "RTK float + dead-reckoning");
            _DrawFixColourLegend(EPOCH_FIX_RTK_FIXED_DR, GUI_COLOUR(FIX_RTK_FIXED_DR), "RTK fixed + dead-reckoning");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    _DrawWindowEnd();
}

void GuiWinAppLegend::_DrawFixColourLegend(const int value, const ImU32 colour, const char *label)
{
    ImDrawList *draw = ImGui::GetWindowDrawList();
    FfVec2 offs = ImGui::GetCursorScreenPos();
    const FfVec2 size { GuiSettings::charSize.x * 1.5f, GuiSettings::charSize.y };
    const float textOffs = (2 * size.x) + (2 * GuiSettings::style->ItemSpacing.x);
    draw->AddRectFilled(offs, offs + size, colour | ((ImU32)(0xff) << IM_COL32_A_SHIFT) );
    offs.x += size.x;
    draw->AddRectFilled(offs, offs + size, colour);
    ImGui::SetCursorPosX(textOffs);
    if (value < 0)
    {
        ImGui::TextUnformatted(label);
    }
    else
    {
        ImGui::Text("%s (%d)", label, value);
    }
}

/* ****************************************************************************************************************** */

#ifndef IMGUI_DISABLE_DEMO_WINDOWS

GuiWinAppImguiDemo::GuiWinAppImguiDemo() :
    GuiWinApp("DearImGuiDemo")
{
    //SetTitle("Dear ImGui demo"); // not used, ImGui sets it
}

void GuiWinAppImguiDemo::DrawWindow()
{
    ImGui::ShowDemoWindow(GetOpenFlag());
}

GuiWinAppImplotDemo::GuiWinAppImplotDemo() :
    GuiWinApp("ImPlotDemo")
{
    //SetTitle("ImPlot demo"); // not used, ImPlot sets it
};

void GuiWinAppImplotDemo::DrawWindow()
{
    ImPlot::ShowDemoWindow(GetOpenFlag());
}

#endif

/* ****************************************************************************************************************** */

#ifndef IMGUI_DISABLE_METRICS_WINDOW

GuiWinAppImguiMetrics::GuiWinAppImguiMetrics() :
    GuiWinApp("DearImGuiMetrics")
{
    _winSize = { 70, 40 };
    //SetTitle("Dear ImGui metrics"); // not used, ImGui sets it
}

void GuiWinAppImguiMetrics::DrawWindow()
{
    ImGui::ShowMetricsWindow(GetOpenFlag());
}
#endif

/* ****************************************************************************************************************** */

GuiWinAppImplotMetrics::GuiWinAppImplotMetrics() :
    GuiWinApp("ImPlotMetrics")
{
    _winSize = { 70, 40 };
    //SetTitle("ImPlot metrics"); // not used, ImPlot sets it
}

void GuiWinAppImplotMetrics::DrawWindow()
{
    ImPlot::ShowMetricsWindow(GetOpenFlag());
}

/* ****************************************************************************************************************** */

GuiWinAppImguiStyles::GuiWinAppImguiStyles() :
    GuiWinApp("DearImGuiStyles")
{
    _winSize = { 70, 40 };
    SetTitle("Dear ImGui styles");
}

void GuiWinAppImguiStyles::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    ImGui::ShowStyleEditor();

    _DrawWindowEnd();
}

/* ****************************************************************************************************************** */

GuiWinAppImplotStyles::GuiWinAppImplotStyles() :
    GuiWinApp("ImPlotStyles")
{
    _winSize = { 70, 40 };
    SetTitle("ImPlot styles");
}

void GuiWinAppImplotStyles::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    ImPlot::ShowStyleEditor();

    _DrawWindowEnd();
}

/* ****************************************************************************************************************** */
