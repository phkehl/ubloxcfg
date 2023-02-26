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

#include "gui_win_app.hpp"

/* ****************************************************************************************************************** */

GuiWinApp::GuiWinApp(const std::string &name) :
    GuiWin(name)
{
    _winFlags |= ImGuiWindowFlags_NoDocking;
}

/* ****************************************************************************************************************** */

GuiWinAppAbout::GuiWinAppAbout(const std::vector<std::string> &versions) :
    GuiWinApp("About"),
    _versions { versions }
{
    _winFlags |= ImGuiWindowFlags_AlwaysAutoResize;
}

void GuiWinAppAbout::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    ImGui::PushFont(GuiSettings::fontBold);
    Gui::TextTitle(         "cfggui " CONFIG_VERSION " (" CONFIG_GITHASH ")"
#ifdef FF_BUILD_DEBUG
        " -- debug"
#endif
    );
    ImGui::PopFont();

    ImGui::PushFont(GuiSettings::fontSans);
    ImGui::TextUnformatted( "Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org)");
    Gui::TextLink(          "https://oinkzwurgl.org/projaeggd/ubloxcfg/");
    ImGui::Separator();  // -----------------------------------------------------------------------------
    ImGui::TextUnformatted("This consists of the following parts:");


    _DrawEntry("'ubloxcfg' library",   "LGPLv3 license");
    _DrawEntry("'ff' library",         "GPLv3 license");
    _DrawEntry("'cfggui' application", "GPLv3 license");

    ImGui::Separator();  // -----------------------------------------------------------------------------
    ImGui::TextUnformatted( "Third-party code (see source code and the links for details):");
    _DrawEntry("Dear Imgui",                  "MIT license",          "https://github.com/ocornut/imgui");
    _DrawEntry("ImPlot",                      "MIT license",          "https://github.com/epezent/implot");
    _DrawEntry("PlatformFolders",             "MIT license",          "https://github.com/sago007/PlatformFolders");
    _DrawEntry("DejaVu fonts",                "unnamed license",      "https://dejavu-fonts.github.io");
    _DrawEntry("ForkAwesome font",            "MIT license",          "https://forkaweso.me", "https://github.com/ForkAwesome/Fork-Awesome");
    _DrawEntry("Tetris",                      "Revised BSD license",  "https://brennan.io/2015/06/12/tetris-reimplementation/");
    _DrawEntry("zfstream",                    "unnamed license",      "https://github.com/madler/zlib/tree/master/contrib/iostream3");
    _DrawEntry("Base64",                      "MIT license",          "https://gist.github.com/tomykaira/f0fd86b6c73063283afe550bc5d77594");
    _DrawEntry("glm",                         "Happy Bunny license",  "https://github.com/g-truc/glm");
    _DrawEntry("json",                        "Various licenses",     "https://github.com/nlohmann/json", "https://json.nlohmann.me/");
    ImGui::Separator();  // -----------------------------------------------------------------------------
    ImGui::TextUnformatted("Third-party libraries (first level dependencies, use ldd to see all):");
    _DrawEntry("GLFW",                        "zlib/libpng license",  "https://www.glfw.org", "https://github.com/glfw/glfw");
    _DrawEntry("libcurl",                     "unnamed license",      "https://curl.se/", "https://github.com/curl/curl");
    _DrawEntry("FreeType",                    "FreeType license",     "https://freetype.org/", "https://gitlab.freedesktop.org/freetype/freetype");
    ImGui::Separator();  // -----------------------------------------------------------------------------
    ImGui::TextUnformatted("Third-party data from the following public sources:");
    int nSources = 0;
    const char **sources = ubloxcfg_getSources(&nSources);
    for (int ix = 0; ix < nSources; ix++)
    {
        ImGui::Text("- %s", sources[ix]);
    }
    ImGui::Separator();
    ImGui::TextUnformatted("Versions:");
    for (const auto &version: _versions)
    {
        ImGui::Text("- %s", version.c_str());
    }
    ImGui::PopFont();

    _DrawWindowEnd();
}

void GuiWinAppAbout::_DrawEntry(const char *name, const char *license, const char *link, const char *link2)
{
    ImGui::PushFont(GuiSettings::fontSans);

    ImGui::TextUnformatted("- ");
    ImGui::SameLine(0, 0);
    ImGui::PushFont(GuiSettings::fontOblique);
    ImGui::TextUnformatted(name);
    ImGui::PopFont();

    if (license)
    {
        ImGui::SameLine(GuiSettings::charSize.x * 30);
        ImGui::TextUnformatted(license);
    }

    if (link)
    {
        ImGui::SameLine(GuiSettings::charSize.x * 51);
        Gui::TextLink(link);
    }

    if (link2)
    {
        ImGui::SameLine();
        Gui::TextLink(link2);
    }

    ImGui::PopFont();
}

/* ****************************************************************************************************************** */

GuiWinAppSettings::GuiWinAppSettings() :
    GuiWinApp("Settings"),
    _tabbar { WinName() }
{
    _winSize    = { 75, 25 };
    _winSizeMin = { 75, 15 };
}

void GuiWinAppSettings::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    if (_tabbar.Begin())
    {
        _tabbar.Item("Font",    GuiSettings::DrawSettingsEditorFont);
        _tabbar.Item("Colours", GuiSettings::DrawSettingsEditorColours);
        _tabbar.Item("Maps",    GuiSettings::DrawSettingsEditorMaps);
        _tabbar.Item("Misc",    GuiSettings::DrawSettingsEditorMisc);
        _tabbar.Item("Tools",   GuiSettings::DrawSettingsEditorTools);
        _tabbar.End();
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
            ImGui::BeginChild("cfgguiHelp", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::PushFont(GuiSettings::fontSans);
            ImGui::TextUnformatted("Yeah, right, ...");
            ImGui::PopFont();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("ImGui"))
        {
            ImGui::BeginChild("ImGuiHelp", ImVec2(0, 0), false,  ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::PushFont(GuiSettings::fontSans);
            ImGui::ShowUserGuide();
            ImGui::BulletText("CTRL-F4 closes focused window");
            ImGui::BulletText("Hold SHIFT while moving window to dock into other windows.\n"
                "(VERY experimental. WILL crash sometimes!)");
            ImGui::PopFont();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("ImPlot"))
        {
            ImGui::BeginChild("ImPlotHelp", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::PushFont(GuiSettings::fontSans);
            ImPlot::ShowUserGuide();
            ImGui::PopFont();
            ImGui::EndChild();
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
    FfVec2f offs = ImGui::GetCursorScreenPos();
    const FfVec2f size { GuiSettings::charSize.x * 1.5f, GuiSettings::charSize.y };
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
    ImGui::ShowDemoWindow(WinOpenFlag());
}

GuiWinAppImplotDemo::GuiWinAppImplotDemo() :
    GuiWinApp("ImPlotDemo")
{
    //SetTitle("ImPlot demo"); // not used, ImPlot sets it
};

void GuiWinAppImplotDemo::DrawWindow()
{
    ImPlot::ShowDemoWindow(WinOpenFlag());
}

#endif

/* ****************************************************************************************************************** */

#ifndef IMGUI_DISABLE_DEBUG_TOOLS

GuiWinAppImguiMetrics::GuiWinAppImguiMetrics() :
    GuiWinApp("DearImGuiMetrics")
{
    _winSize = { 70, 40 };
    //SetTitle("Dear ImGui metrics"); // not used, ImGui sets it
}

void GuiWinAppImguiMetrics::DrawWindow()
{
    ImGui::ShowMetricsWindow(WinOpenFlag());
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
    ImPlot::ShowMetricsWindow(WinOpenFlag());
}

/* ****************************************************************************************************************** */

GuiWinAppImguiStyles::GuiWinAppImguiStyles() :
    GuiWinApp("DearImGuiStyles")
{
    _winSize = { 70, 40 };
    WinSetTitle("Dear ImGui styles");
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
    WinSetTitle("ImPlot styles");
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
