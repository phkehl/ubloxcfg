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
    _versions       { versions },
    _cacheDirLink   { std::string("file://") + GuiSettings::cacheDir },
    _configFileLink { std::string("file://") + GuiSettings::configFile }
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
    ImGui::Separator();  // -----------------------------------------------------------------------------
    ImGui::TextUnformatted("Versions:");
    for (const auto &version: _versions)
    {
        ImGui::TextWrapped("- %s", version.c_str());
    }
    ImGui::Separator();  // -----------------------------------------------------------------------------
    ImGui::TextUnformatted("Data:");
    ImGui::TextUnformatted("- Config file:"); ImGui::SameLine();
    Gui::TextLink(_configFileLink.c_str(), GuiSettings::configFile.c_str());
    ImGui::TextUnformatted("- Cache directory:"); ImGui::SameLine();
    Gui::TextLink(_cacheDirLink.c_str(), GuiSettings::cacheDir.c_str());

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
    GuiWinApp("Help"),
    _tabbar { WinName() }
{
    _winSize = { 70, 35 };
}

void GuiWinAppHelp::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    bool doHelpCfgGui = false;
    bool doHelpImGui  = false;
    bool doHelpImPlot = false;
    if (_tabbar.Begin())
    {
        if (_tabbar.Item("Help"))   { doHelpCfgGui = true; }
        if (_tabbar.Item("ImGui"))  { doHelpImGui  = true; }
        if (_tabbar.Item("ImPlot")) { doHelpImPlot = true; }
        _tabbar.End();
    }

    if (doHelpCfgGui)
    {
        ImGui::BeginChild("cfgguiHelp", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushFont(GuiSettings::fontSans);
        ImGui::TextUnformatted("In addition to the ImGui controls:");
        ImGui::BulletText("CTRL-F4 closes focused window");
        ImGui::BulletText("Hold SHIFT while moving window to dock into other windows.\n"
            "(VERY experimental. WILL crash sometimes!)");
        ImGui::TextUnformatted("In (some) tables:");
        ImGui::BulletText("Click to mark a line, CTRL-click to mark multiple lines");
        ImGui::BulletText("SHIFT-click colum headers for multi-sort");
        ImGui::PopFont();
        ImGui::EndChild();
    }
    if (doHelpImGui)
    {
        ImGui::BeginChild("ImGuiHelp", ImVec2(0, 0), false,  ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushFont(GuiSettings::fontSans);
        ImGui::ShowUserGuide();
        ImGui::PopFont();
        ImGui::EndChild();
    }
    if (doHelpImPlot)
    {
        ImGui::BeginChild("ImPlotHelp", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushFont(GuiSettings::fontSans);
        ImPlot::ShowUserGuide();
        ImGui::PopFont();
        ImGui::EndChild();
    }

    _DrawWindowEnd();
}

/* ****************************************************************************************************************** */

GuiWinAppLegend::GuiWinAppLegend() :
    GuiWinApp("Legend"),
    _tabbar { WinName() }
{
    _winFlags |= ImGuiWindowFlags_AlwaysAutoResize;
}

void GuiWinAppLegend::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    bool doFixColours   = false;
    bool doSignalLevels = false;
    if (_tabbar.Begin())
    {
        if (_tabbar.Item("Fix colours"))   { doFixColours = true; }
        if (_tabbar.Item("Signal levels")) { doSignalLevels  = true; }
        _tabbar.End();
    }

    if (doFixColours)
    {
        _DrawColourLegend(GUI_COLOUR(FIX_INVALID),      "Invalid (%d)",                    EPOCH_FIX_NOFIX);
        _DrawColourLegend(GUI_COLOUR(FIX_MASKED),       "Masked");
        _DrawColourLegend(GUI_COLOUR(FIX_DRONLY),       "Dead-reckoning only (%d)",        EPOCH_FIX_DRONLY);
        _DrawColourLegend(GUI_COLOUR(FIX_S2D),          "Single 2D (%d)",                  EPOCH_FIX_TIME);
        _DrawColourLegend(GUI_COLOUR(FIX_S3D),          "Single 3D (%d)",                  EPOCH_FIX_S2D);
        _DrawColourLegend(GUI_COLOUR(FIX_S3D_DR),       "Single 3D + dead-reckoning (%d)", EPOCH_FIX_S3D);
        _DrawColourLegend(GUI_COLOUR(FIX_TIME),         "Single time only (%d)",           EPOCH_FIX_S3D_DR);
        _DrawColourLegend(GUI_COLOUR(FIX_RTK_FLOAT),    "RTK float (%d)",                  EPOCH_FIX_RTK_FLOAT);
        _DrawColourLegend(GUI_COLOUR(FIX_RTK_FIXED),    "RTK fixed (%d)",                  EPOCH_FIX_RTK_FIXED);
        _DrawColourLegend(GUI_COLOUR(FIX_RTK_FLOAT_DR), "RTK float + dead-reckoning (%d)", EPOCH_FIX_RTK_FLOAT_DR);
        _DrawColourLegend(GUI_COLOUR(FIX_RTK_FIXED_DR), "RTK fixed + dead-reckoning (%d)", EPOCH_FIX_RTK_FIXED_DR);
    }
    if (doSignalLevels)
    {
        _DrawColourLegend(GUI_COLOUR(SIGNAL_00_05), "Signal 0 - 5 dBHz");
        _DrawColourLegend(GUI_COLOUR(SIGNAL_05_10), "Signal 5 - 10 dBHz");
        _DrawColourLegend(GUI_COLOUR(SIGNAL_10_15), "Signal 10 - 15 dBHz");
        _DrawColourLegend(GUI_COLOUR(SIGNAL_15_20), "Signal 15 - 20 dBHz");
        _DrawColourLegend(GUI_COLOUR(SIGNAL_20_25), "Signal 20 - 25 dBHz");
        _DrawColourLegend(GUI_COLOUR(SIGNAL_25_30), "Signal 25 - 30 dBHz");
        _DrawColourLegend(GUI_COLOUR(SIGNAL_30_35), "Signal 30 - 35 dBHz");
        _DrawColourLegend(GUI_COLOUR(SIGNAL_35_40), "Signal 35 - 40 dBHz");
        _DrawColourLegend(GUI_COLOUR(SIGNAL_40_45), "Signal 40 - 45 dBHz");
        _DrawColourLegend(GUI_COLOUR(SIGNAL_45_50), "Signal 45 - 50 dBHz");
        _DrawColourLegend(GUI_COLOUR(SIGNAL_50_55), "Signal 50 - 55 dBHz");
        _DrawColourLegend(GUI_COLOUR(SIGNAL_55_OO), "Signal 55 - oo dBHz");
    }

    _DrawWindowEnd();
}

void GuiWinAppLegend::_DrawColourLegend(const ImU32 colour, const char *fmt, ...)
{
    ImDrawList *draw = ImGui::GetWindowDrawList();
    FfVec2f offs = ImGui::GetCursorScreenPos();
    const FfVec2f size { GuiSettings::charSize.x * 1.5f, GuiSettings::charSize.y };
    const float textOffs = (2 * size.x) + (2 * GuiSettings::style->ItemSpacing.x);
    draw->AddRectFilled(offs, offs + size, colour | ((ImU32)(0xff) << IM_COL32_A_SHIFT) );
    offs.x += size.x;
    draw->AddRectFilled(offs, offs + size, colour);
    ImGui::SetCursorPosX(textOffs);

    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
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
