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

#include <cmath>
#include <fstream>
#include <cerrno>
#include <filesystem>
#include <algorithm>

#include "ff_stuff.h"
#include "ff_debug.h"
#include "ff_trafo.h"
#include "ff_cpp.hpp"

#include "platform.hpp"
#include "database.hpp"

#include "gui_inc.hpp"
#include "imgui_internal.h"
#include "implot_internal.h"

#ifdef IMGUI_ENABLE_FREETYPE
#  include "imgui_freetype.h"
#endif

#include "gui_fonts.hpp"

#include "gui_settings.hpp"

/* ****************************************************************************************************************** */

/*static*/ void GuiSettings::Init(const std::string &configName)
{
    DEBUG("GuiSettings() %s", configFile.c_str());

    ImGuiIO &io = ImGui::GetIO();

    io.ConfigFlags                   |= ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigFlags                 |= ImGuiConfigFlags_NavEnableGamepad;
    //io.FontAllowUserScaling         = true;
    //io.UserData                     = NULL;
    io.IniFilename                    = NULL; // Don't auto-load, we do it ourselves in LoadConf() below
    //io.FontGlobalScale              = 2.0;
    io.ConfigDockingWithShift         = true;
    //io.ConfigViewportsNoAutoMerge   = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    io.ConfigWindowsResizeFromEdges   = true;

    ImGui::SetColorEditOptions(
        ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_InputRGB |
        ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview |
        ImGuiColorEditFlags_AlphaPreviewHalf);

    // Select ImGui and ImPlot styles
    ImGui::StyleColorsDark();
    ImPlot::StyleColorsAuto();

    // Tune ImGui styles
    ImGuiStyle *imguiStyle = &(ImGui::GetCurrentContext()->Style);
    imguiStyle->Colors[ImGuiCol_PopupBg]          = ImColor(IM_COL32(0x34, 0x34, 0x34, 0xf8));
    imguiStyle->Colors[ImGuiCol_TitleBg]          = ImVec4(0.232f, 0.201f, 0.271f, 1.00f);
    imguiStyle->Colors[ImGuiCol_TitleBgActive]    = ImVec4(0.502f, 0.075f, 0.256f, 1.00f);
    imguiStyle->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.200f, 0.220f, 0.270f, 0.75f);
    imguiStyle->Colors[ImGuiCol_TabActive]        = ImVec4(0.502f, 0.075f, 0.256f, 1.00f);
    imguiStyle->FrameBorderSize                   = 1.0f;
    imguiStyle->TabBorderSize                     = 1.0f;
    imguiStyle->WindowTitleAlign.x                = 0.0f;
    imguiStyle->WindowMenuButtonPosition          = ImGuiDir_Left;
    imguiStyle->FrameRounding                     = 3.0f;
    //imguiStyle->WindowBorderSize                  = 5.0f;
    imguiStyle->WindowRounding                    = 5.0f;

    // Tune ImPlot styles
    ImPlotStyle *implotStyle = &(ImPlot::GetCurrentContext()->Style);
    implotStyle->LineWeight              = 2.0;
    //implotStyle->AntiAliasedLines      = true; // TODO: enable MSAA (how?!), see ImGui FAQ
    implotStyle->UseLocalTime            = false;
    implotStyle->UseISO8601              = true;
    implotStyle->Use24HourClock          = true;
    implotStyle->FitPadding              = ImVec2(0.1, 0.1);
    //implotStyle->PlotPadding             = ImVec2(0, 0);
    implotStyle->Colors[ImPlotCol_AxisBgHovered] = ImVec4(0.060f, 0.530f, 0.980f, 0.200f);
    implotStyle->Colors[ImPlotCol_AxisBgActive]  = ImVec4(0.060f, 0.530f, 0.980f, 0.400f);

    // Store style shortcuts
    style     = imguiStyle;
    plotStyle = implotStyle;

    configFile = Platform::ConfigFile((configName.empty() ? std::string("cfggui") : configName) + std::string(".conf"));
    cacheDir = Platform::CacheDir();

    // Calculate CNo colours
    const int cnoColoursCount = (&GUI_COLOUR(SIGNAL_55_OO) - &GUI_COLOUR(SIGNAL_00_05)) + 1;
    ImPlotColormap cnoColourMap = ImPlot::AddColormap("CnoColours", &GUI_COLOUR(SIGNAL_00_05), cnoColoursCount, false);
    for (int cno = 0; cno <= CNO_COLOUR_MAX; cno++)
    {
        const float t = (float)cno / (float)CNO_COLOUR_MAX;
        CNO_COLOURS[cno] = ImGui::ColorConvertFloat4ToU32(ImPlot::SampleColormap(t, cnoColourMap));
    }
}

// ---------------------------------------------------------------------------------------------------------------------

// Static member variables, set/updated at run-time

/*static*/ float   GuiSettings::fontSize    = GuiSettings::FONT_SIZE_DEF;
/*static*/ float   GuiSettings::lineHeight  = GuiSettings::FONT_SIZE_DEF + 2.0f;
/*static*/ ImFont *GuiSettings::fontMono    = nullptr;
/*static*/ ImFont *GuiSettings::fontSans    = nullptr;
/*static*/ ImFont *GuiSettings::fontBold    = nullptr;
/*static*/ ImFont *GuiSettings::fontOblique = nullptr;
/*static*/ FfVec2f  GuiSettings::charSize    = { GuiSettings::FONT_SIZE_DEF, GuiSettings::FONT_SIZE_DEF };
/*static*/ FfVec2f  GuiSettings::iconSize    = { GuiSettings::FONT_SIZE_DEF, GuiSettings::FONT_SIZE_DEF };

/* static */ImU32 GuiSettings::colours[_NUM_COLOURS] =
{
#  define _SETTINGS_COLOUR_COL(_str, _enum, _col) _col,
    GUI_SETTINGS_COLOURS(_SETTINGS_COLOUR_COL)
};

/*static*/ ImVec4 GuiSettings::colours4[_NUM_COLOURS] =
{

#  define _SETTINGS_COLOUR_COL4(_str, _enum, _col) \
    { (float)((_col >> IM_COL32_R_SHIFT) & 0xff) / 255.0f, \
      (float)((_col >> IM_COL32_G_SHIFT) & 0xff) / 255.0f, \
      (float)((_col >> IM_COL32_B_SHIFT) & 0xff) / 255.0f, \
      (float)((_col >> IM_COL32_A_SHIFT) & 0xff) / 255.0f },
    GUI_SETTINGS_COLOURS(_SETTINGS_COLOUR_COL4)
};

/*static*/ const ImGuiStyle   *GuiSettings::style = nullptr;
/*static*/ const ImPlotStyle  *GuiSettings::plotStyle = nullptr;

/*static*/ std::string GuiSettings::configFile = "";
/*static*/ std::string GuiSettings::cacheDir = "";

/*static*/ std::vector<MapParams> GuiSettings::maps = { };

/*static*/ Ff::ConfFile GuiSettings::_confFile;

/*static*/ int GuiSettings::dbNumRows = DB_NUM_ROWS_DEF;

/*static*/ int GuiSettings::minFrameRate = MIN_FRAME_RATE_DEF;

/*static*/ bool               GuiSettings::_fontDirty            = true;
/*static*/ bool               GuiSettings::_sizesDirty           = true;
/*static*/ uint32_t           GuiSettings::_ftBuilderFlags       = FT_BUILDER_FLAGS_DEF;
/*static*/ float              GuiSettings::_ftRasterizerMultiply = FT_RASTERIZER_MULTIPLY_DEF;
/*static*/ bool               GuiSettings::_clearSettingsOnExit  = false;
/*static*/ float              GuiSettings::_widgetOffs = 0;

/*static */ImU32              GuiSettings::CNO_COLOURS[CNO_COLOUR_MAX + 1] = { }; // Initialised in Init()

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ void GuiSettings::LoadConf()
{
    if (configFile.empty())
    {
        WARNING("GuiSettings::LoadConf() no config file defined!"); // Init() not called?
        return;
    }

    DEBUG("GuiSettings::LoadConf() %s", configFile.c_str());

    // Load ImGui stuff
    ImGui::LoadIniSettingsFromDisk(configFile.c_str());

    // Load our stuff
    if (_confFile.Load(configFile, "cfggui"))
    {
        // Colours
        for (int ix = 0; ix < _NUM_COLOURS; ix++)
        {
            GetValue(std::string("Settings.colour.") + COLOUR_NAMES[ix], colours[ix], COLOUR_DEFAULTS[ix]);
        }

        // Font
        GetValue("Settings.fontSize", fontSize, FONT_SIZE_DEF);
        fontSize = CLIP(fontSize, FONT_SIZE_MIN, FONT_SIZE_MAX);

        // Freetype renderer config
        GetValue("Settings.ftBuilderFlags", _ftBuilderFlags, FT_BUILDER_FLAGS_DEF);
        GetValue("Settings.ftRasterizerMultiply", _ftRasterizerMultiply, FT_RASTERIZER_MULTIPLY_DEF);
        _ftRasterizerMultiply = CLIP(_ftRasterizerMultiply, FT_RASTERIZER_MULTIPLY_MIN, FT_RASTERIZER_MULTIPLY_MAX);

        LoadRecentItems(RECENT_LOGFILES);
        LoadRecentItems(RECENT_RECEIVERS);
        LoadRecentItems(RECENT_NTRIP_CASTERS);

        GetValue("Settings.dbNumRows", dbNumRows, DB_NUM_ROWS_DEF);
        dbNumRows = CLIP(dbNumRows, DB_NUM_ROWS_MIN, DB_NUM_ROWS_MAX);

        GetValue("Settings.minFrameRate", minFrameRate, MIN_FRAME_RATE_DEF);
        minFrameRate = CLIP(minFrameRate, MIN_FRAME_RATE_MIN, MIN_FRAME_RATE_MAX);
    }

    // Load maps
    {
        maps.clear();
        auto conf = Ff::ConfFile();
        while (conf.Load(configFile, "map"))
        {
            MapParams map;
            bool mapOk = true;
            if (!conf.Get("name",        map.name)        || map.name.empty()        ||
                !conf.Get("title",       map.title)       || map.title.empty()       ||
                !conf.Get("zoomMin",     map.zoomMin)     || (map.zoomMin < 0)       ||
                !conf.Get("zoomMax",     map.zoomMax)     || (map.zoomMax < map.zoomMin) ||
                !conf.Get("tileSizeX",   map.tileSizeX)   || (map.tileSizeX < 100)   || (map.tileSizeX > 16384) ||
                !conf.Get("tileSizeY",   map.tileSizeY)   || (map.tileSizeY < 100)   || (map.tileSizeY > 16384) ||
                !conf.Get("downloadUrl", map.downloadUrl) || map.downloadUrl.empty() ||
                !conf.Get("cachePath",   map.cachePath)   || map.cachePath.empty()   ||
                false
                )
            {
                mapOk = false;
            }

            for (int i = 1; i <= MapParams::MAX_MAP_LINKS; i++)
            {
                MapParams::MapLink link;
                if (conf.Get(Ff::Sprintf("link%d.label", i), link.label) &&
                    conf.Get(Ff::Sprintf("link%d.url", i), link.url))
                {
                    map.mapLinks.push_back(link);
                }
            }
            if (map.mapLinks.empty())
            {
                mapOk = false;
            }

            map.enabled = true;
            conf.Get("enabled",         map.enabled);
            conf.Get("referer",         map.referer);
            conf.Get("threads",         map.threads);
            conf.Get("downloadTimeout", map.downloadTimeout);

            map.threads = CLIP(map.threads, 1, 10);
            map.downloadTimeout = CLIP(map.downloadTimeout, 1000, 30000);

            std::string subDomains;
            if (conf.Get("subDomains", subDomains))
            {
                map.subDomains = Ff::StrSplit(subDomains, ",");
            }

            if (conf.Get("minLat", map.minLat)) { map.minLat = deg2rad(map.minLat); } else { mapOk = false; }
            if (conf.Get("maxLat", map.maxLat)) { map.maxLat = deg2rad(map.maxLat); } else { mapOk = false; }
            if (conf.Get("minLon", map.minLon)) { map.minLon = deg2rad(map.minLon); } else { mapOk = false; }
            if (conf.Get("maxLon", map.maxLon)) { map.maxLon = deg2rad(map.maxLon); } else { mapOk = false; }
            const double minDelta = deg2rad(1.0);
            if ( (map.minLat < (MapParams::MIN_LAT_RAD - 1e-12)) || (map.maxLat > (MapParams::MAX_LAT_RAD + 1e-12)) ||
                 (map.minLon < (MapParams::MIN_LON_RAD - 1e-12)) || (map.maxLon > (MapParams::MAX_LON_RAD + 1e-12)) ||
                 ((map.maxLat - map.minLat) < minDelta) || ((map.maxLon - map.minLon) < minDelta) )
            {
                mapOk = false;
            }

            if (mapOk)
            {
                maps.push_back(map);
            }
            else
            {
                WARNING("%s:%d: Bad map info!", configFile.c_str(), conf.GetSectionBeginLine());
            }
        }

        // Add / update from built-in maps
        for (auto &builtin: MapParams::BUILTIN_MAPS)
        {
            auto entry = std::find_if(maps.begin(), maps.end(), [&builtin](MapParams &p) { return p.name == builtin.name; });
            if (entry == maps.end())
            {
                maps.push_back(builtin);
            }
            else
            {
                const bool enabled = entry->enabled;
                *entry = builtin;
                entry->enabled = enabled;
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ void GuiSettings::SaveConf()
{
    if (configFile.empty())
    {
        WARNING("GuiSettings::LoadConf() no config file defined!"); // Init() not called?
        return;
    }

    DEBUG("GuiSettings::SaveConf() %s", configFile.c_str());

    if (_clearSettingsOnExit)
    {
        Platform::FileSpew(configFile, nullptr, 0);
        return;
    }

    // Save ImGui stuff
    ImGui::SaveIniSettingsToDisk(configFile.c_str());

    // Append our own stuff
    for (int ix = 0; ix < _NUM_COLOURS; ix++)
    {
        SetValue(std::string("Settings.colour.") + COLOUR_NAMES[ix], colours[ix]);
    }
    SetValue("Settings.fontSize",             fontSize);
    SetValue("Settings.ftBuilderFlags",       _ftBuilderFlags);
    SetValue("Settings.ftRasterizerMultiply", _ftRasterizerMultiply);
    SetValue("Settings.dbNumRows",          dbNumRows);
    SetValue("Settings.minFrameRate",         minFrameRate);

    SaveRecentItems(RECENT_LOGFILES);
    SaveRecentItems(RECENT_RECEIVERS);
    SaveRecentItems(RECENT_NTRIP_CASTERS);

    _confFile.Save(configFile, "cfggui", true);

    // Save maps
    for (const auto &map: maps)
    {
        Ff::ConfFile conf;
        conf.Set("enabled",         map.enabled);
        conf.Set("name",            map.name);
        conf.Set("title",           map.title);
        for (int ix = 0; ix < MapParams::MAX_MAP_LINKS; ix++)
        {
            if ((int)map.mapLinks.size() > ix)
            {
                conf.Set(Ff::Sprintf("link%d.label", ix + 1), map.mapLinks[ix].label);
                conf.Set(Ff::Sprintf("link%d.url", ix + 1), map.mapLinks[ix].url);
            }
        }
        conf.Set("zoomMin",         map.zoomMin);
        conf.Set("zoomMax",         map.zoomMax);
        conf.Set("tileSizeX",       map.tileSizeX);
        conf.Set("tileSizeY",       map.tileSizeY);
        conf.Set("downloadUrl",     map.downloadUrl);
        conf.Set("subDomains",      Ff::StrJoin(map.subDomains, ","));
        conf.Set("downloadTimeout", map.downloadTimeout);
        conf.Set("referer",         map.referer);
        conf.Set("cachePath",       map.cachePath);
        conf.Set("minLat",          rad2deg(map.minLat));
        conf.Set("maxLat",          rad2deg(map.maxLat));
        conf.Set("minLon",          rad2deg(map.minLon));
        conf.Set("maxLon",          rad2deg(map.maxLon));
        conf.Set("threads",         CLIP(map.threads, 1, 10));
        conf.Save(configFile, "map", true);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ void GuiSettings::SetValue(const std::string &key, const bool value)
{
    _confFile.Set(key, value);
}

/*static*/ void GuiSettings::SetValue(const std::string &key, const int value)
{
    _confFile.Set(key, value);
}

/*static*/ void GuiSettings::SetValue(const std::string &key, const uint32_t value)
{
    _confFile.Set(key, value, true);
}

/*static*/ void GuiSettings::SetValue(const std::string &key, const double value)
{
    _confFile.Set(key, value);
}

/*static*/ void GuiSettings::SetValue(const std::string &key, const float value)
{
    const double dbl = value;
    _confFile.Set(key, dbl);
}

/*static*/ void GuiSettings::SetValue(const std::string &key, const std::string &value)
{
    _confFile.Set(key, value);
}

/*static*/ void GuiSettings::SetValueMult(const std::string &key, const std::vector<std::string> &list, const int maxNum)
{
    for (int n = 1; n <= maxNum; n++)
    {
        SetValue(key + std::to_string(n), n <= (int)list.size() ? list[n - 1] : "");
    }
}

/*static*/ void GuiSettings::SetValueList(const std::string &key, const std::vector<std::string> &list, const std::string &sep, const int maxNum)
{
    if (!list.empty())
    {
        const std::vector<std::string> copy {
            list.begin(), list.begin() + (maxNum < (int)list.size() ? maxNum : (int)list.size()) };
        SetValue(key, Ff::StrJoin(copy, sep));
    }
    else
    {
        SetValue(key, std::string(""));
    }
}

/*static*/ void GuiSettings::GetValue(const std::string &key, bool &value, const bool def)
{
    if (!_confFile.Get(key, value))
    {
        value = def;
    }
}

/*static*/ void GuiSettings::GetValue(const std::string &key, int &value, const int def)
{
    if (!_confFile.Get(key, value))
    {
        value = def;
    }
}

/*static*/ void GuiSettings::GetValue(const std::string &key, uint32_t &value, const uint32_t def)
{
    if (!_confFile.Get(key, value))
    {
        value = def;
    }
}

/*static*/ void GuiSettings::GetValue(const std::string &key, double &value, const double def)
{
    if (!_confFile.Get(key, value))
    {
        value = def;
    }
}

/*static*/ void GuiSettings::GetValue(const std::string &key, float &value, const float def)
{
    double dbl;
    if (!_confFile.Get(key, dbl))
    {
        value = def;
    }
    else
    {
        value = dbl;
    }
}

/*static*/ void GuiSettings::GetValue(const std::string &key, std::string &value, const std::string &def)
{
    if (!_confFile.Get(key, value))
    {
        value = def;
    }
}

/*static*/ std::string GuiSettings::GetValue(const std::string &key)
{
    std::string value;
    GetValue(key, value, "");
    return value;
}

/*static*/ std::vector<std::string> GuiSettings::GetValueMult(const std::string &key, const int maxNum)
{
    std::vector<std::string> list;
    for (int n = 1; n <= maxNum; n++)
    {
        std::string str = GetValue(key + std::to_string(n));
        if (!str.empty())
        {
            list.push_back(str);
        }
    }
    return list;
}

/*static*/ std::vector<std::string> GuiSettings::GetValueList(const std::string &key, const std::string &sep, const int maxNum)
{
    const std::vector<std::string> strs = Ff::StrSplit(GetValue(key), sep);

    if ((int)strs.size() > maxNum)
    {
        return std::vector<std::string>(strs.begin(), strs.begin() + maxNum);
    }
    else
    {
        return strs;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ json GuiSettings::GetJson(const std::string &key)
{
    const std::string json_str = GetValue(key);
    if (json_str.empty())
    {
        return json();
    }

    try
    {
        return json::parse(json_str);
    }
    catch (json::exception &e)
    {
        WARNING("Corrupt config %s: %s", key.c_str(), e.what());
        return json();
    }
}

/*static*/ void GuiSettings::SetJson(const std::string &key, const json &data)
{
    SetValue(key, data.dump());
}


// ---------------------------------------------------------------------------------------------------------------------

/*static*/ bool GuiSettings::UpdateFonts()
{
    if (!_fontDirty)
    {
        return false;
    }
    DEBUG("GuiSettings::UpdateFonts()");

    ImGuiIO &io = ImGui::GetIO();

    // Clear all font data
    io.Fonts->Clear();

    // Calculate glyphs to load
    //ImVector<ImWchar> glyphs;
    //ImFontGlyphRangesBuilder builder;
    //builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    //builder.AddText("σ");
    //builder.BuildRanges(&glyphs);

    // Monospace font, the default
    {
        ImFontConfig config;
        snprintf(config.Name, sizeof(config.Name), "DejaVu Mono %.1f", fontSize);
        config.FontBuilderFlags = _ftBuilderFlags;
        config.RasterizerMultiply = _ftRasterizerMultiply;
        fontMono = io.Fonts->AddFontFromMemoryCompressedBase85TTF(guiGetFontDejaVuSansMono(), fontSize, &config/*, glyphs.Data*/);

        // Add icons (https://forkaweso.me/Fork-Awesome/icons/)
        snprintf(config.Name, sizeof(config.Name), "Fork Awesome %.1f", fontSize);
        static const ImWchar icons[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };
        config.MergeMode = true;
        config.GlyphOffset = ImVec2(0.0f, std::floor(fontSize / 10.0f));
        io.Fonts->AddFontFromMemoryCompressedBase85TTF(guiGetFontForkAwesome(), fontSize, &config, icons);
    }

    // Normal font
    {
        ImFontConfig config;
        snprintf(config.Name, sizeof(config.Name), "DejaVu Sans %.1f", fontSize + 1.0f);
        config.FontBuilderFlags = _ftBuilderFlags;
        config.RasterizerMultiply = _ftRasterizerMultiply;
        fontSans = io.Fonts->AddFontFromMemoryCompressedBase85TTF(guiGetFontDejaVuSans(), fontSize + 1.0f, &config/*, glyphs.Data*/);
    }

    // Bold font
    {
        ImFontConfig config;
        snprintf(config.Name, sizeof(config.Name), "DejaVu Sans Bold %.1f", fontSize + 1.0f);
        config.FontBuilderFlags = _ftBuilderFlags;
        config.RasterizerMultiply = _ftRasterizerMultiply;
        fontBold = io.Fonts->AddFontFromMemoryCompressedBase85TTF(guiGetFontDejaVuSansBold(), fontSize + 1.0f, &config/*, glyphs.Data*/);
    }

    // Oblique font
    {
        ImFontConfig config;
        snprintf(config.Name, sizeof(config.Name), "DejaVu Sans Oblique %.1f", fontSize + 1.0f);
        config.FontBuilderFlags = _ftBuilderFlags;
        config.RasterizerMultiply = _ftRasterizerMultiply;
        fontOblique = io.Fonts->AddFontFromMemoryCompressedBase85TTF(guiGetFontDejaVuSansOblique(), fontSize + 1.0f, &config/*, glyphs.Data*/);
    }

    // Build font atlas
    io.Fonts->Build();

    _fontDirty = false;
    _sizesDirty = true;

    // The remaining tasks (loading texture) will be done in main()
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ bool GuiSettings::UpdateSizes()
{
    if (!_sizesDirty)
    {
        return false;
    }
    DEBUG("GuiSettings::UpdateSizes()");
    const float frameHeight = ImGui::GetFrameHeight();
    iconSize = ImVec2(frameHeight, frameHeight);
    charSize = ImGui::CalcTextSize("X");
    lineHeight = ImGui::GetTextLineHeightWithSpacing();
    _widgetOffs = 25 * charSize.x;

    _sizesDirty = false;
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ ImU32 GuiSettings::FixColour(const EPOCH_FIX_t fix, const bool fixok)
{
    if (!fixok)
    {
        return colours[FIX_MASKED];
    }
    switch (fix)
    {
        case EPOCH_FIX_UNKNOWN:
        case EPOCH_FIX_NOFIX:         return colours[FIX_INVALID];
        case EPOCH_FIX_DRONLY:        return colours[FIX_DRONLY];
        case EPOCH_FIX_S2D:           return colours[FIX_S2D];
        case EPOCH_FIX_S3D:           return colours[FIX_S3D];
        case EPOCH_FIX_S3D_DR:        return colours[FIX_S3D_DR];
        case EPOCH_FIX_TIME:          return colours[FIX_TIME];
        case EPOCH_FIX_RTK_FLOAT:     return colours[FIX_RTK_FLOAT];
        case EPOCH_FIX_RTK_FIXED:     return colours[FIX_RTK_FIXED];
        case EPOCH_FIX_RTK_FLOAT_DR:  return colours[FIX_RTK_FLOAT_DR];
        case EPOCH_FIX_RTK_FIXED_DR:  return colours[FIX_RTK_FIXED_DR];
    }
    return colours[FIX_INVALID];
}

/*static*/ const ImVec4 &GuiSettings::FixColour4(const EPOCH_FIX_t fix, const bool fixok)
{
    if (!fixok)
    {
        return colours4[FIX_MASKED];
    }
    switch (fix)
    {
        case EPOCH_FIX_UNKNOWN:
        case EPOCH_FIX_NOFIX:         return colours4[FIX_INVALID];
        case EPOCH_FIX_DRONLY:        return colours4[FIX_DRONLY];
        case EPOCH_FIX_S2D:           return colours4[FIX_S2D];
        case EPOCH_FIX_S3D:           return colours4[FIX_S3D];
        case EPOCH_FIX_S3D_DR:        return colours4[FIX_S3D_DR];
        case EPOCH_FIX_TIME:          return colours4[FIX_TIME];
        case EPOCH_FIX_RTK_FLOAT:     return colours4[FIX_RTK_FLOAT];
        case EPOCH_FIX_RTK_FIXED:     return colours4[FIX_RTK_FIXED];
        case EPOCH_FIX_RTK_FLOAT_DR:  return colours4[FIX_RTK_FLOAT_DR];
        case EPOCH_FIX_RTK_FIXED_DR:  return colours4[FIX_RTK_FIXED_DR];
    }
    return colours4[FIX_INVALID];
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ ImU32 GuiSettings::CnoColour(const float cno)
{
    return CNO_COLOURS[(int)std::clamp(cno, 0.0f, (float)CNO_COLOUR_MAX)];
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ std::map<std::string, std::vector<std::string>> GuiSettings::_recentItems;

/*static*/ void GuiSettings::LoadRecentItems(const std::string &name)
{
    _recentItems[name] = GuiSettings::GetValueMult(name + ".recentItems", MAX_RECENT);
    Ff::MakeUnique(_recentItems[name]);
}

/*static*/ void GuiSettings::SaveRecentItems(const std::string &name)
{
    GuiSettings::SetValueMult(name + ".recentItems", _recentItems[name], MAX_RECENT);
}

/*static*/ void GuiSettings::AddRecentItem(const std::string &name, const std::string &input)
{
    _recentItems[name].insert(_recentItems[name].begin(), input);
    Ff::MakeUnique(_recentItems[name]);
    while (_recentItems[name].size() > MAX_RECENT)
    {
        _recentItems[name].pop_back();
    }
}

/*static*/ void GuiSettings::ClearRecentItems(const std::string &name)
{
    _recentItems[name].clear();
}

/*static*/ const std::vector<std::string> &GuiSettings::GetRecentItems(const std::string &name)
{
    return _recentItems[name];
}

// ---------------------------------------------------------------------------------------------------------------------

#define _SETTINGS_COLOUR_LABEL(_str, _enum, _col) _str,
#define _SETTINGS_COLOUR_NAME(_str, _enum, _col) STRINGIFY(_enum),
/*static*/ const char * const GuiSettings::COLOUR_LABELS[GuiSettings::_NUM_COLOURS] =
{
    GUI_SETTINGS_COLOURS(_SETTINGS_COLOUR_LABEL)
};

/*static*/ const char * const GuiSettings::COLOUR_NAMES[GuiSettings::_NUM_COLOURS] =
{
    GUI_SETTINGS_COLOURS(_SETTINGS_COLOUR_NAME)
};

/*static*/ ImU32 GuiSettings::COLOUR_DEFAULTS[_NUM_COLOURS] =
{
    GUI_SETTINGS_COLOURS(_SETTINGS_COLOUR_COL)
};

/*static*/ void GuiSettings::DrawSettingsEditorFont()
{
    ImGui::AlignTextToFramePadding();

    ImGui::TextUnformatted("Font size");
    ImGui::SameLine(_widgetOffs);
    ImGui::PushItemWidth(12 * charSize.x);
    float newFontSize = fontSize;
    if (ImGui::InputFloat("##FontSize", &newFontSize, 1.0, 1.0, "%.1f", ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (std::fabs(newFontSize - fontSize) > 0.05)
        {
            newFontSize = std::floor(newFontSize * 10.0f) / 10.0f;
            fontSize = CLIP(newFontSize, FONT_SIZE_MIN, FONT_SIZE_MAX);
            _fontDirty = true; // trigger change in main()
        }
    }
    ImGui::PopItemWidth();

#ifdef IMGUI_ENABLE_FREETYPE
    ImGui::Separator();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Font renderer");
    ImGui::SameLine(_widgetOffs);

    const float offs = charSize.x * 16;

    if (ImGui::CheckboxFlags("NoHinting", &_ftBuilderFlags, ImGuiFreeTypeBuilderFlags_NoHinting))
    {
        _fontDirty = true;
    }
    ImGui::SameLine(_widgetOffs + offs);
    if (ImGui::CheckboxFlags("NoAutoHint", &_ftBuilderFlags, ImGuiFreeTypeBuilderFlags_NoAutoHint))
    {
        _fontDirty = true;
    }
    ImGui::SameLine(_widgetOffs + offs + offs);
    if (ImGui::CheckboxFlags("ForceAutoHint", &_ftBuilderFlags, ImGuiFreeTypeBuilderFlags_ForceAutoHint))
    {
        _fontDirty = true;
    }
    ImGui::NewLine();
    ImGui::SameLine(_widgetOffs);
    if (ImGui::CheckboxFlags("LightHinting", &_ftBuilderFlags, ImGuiFreeTypeBuilderFlags_LightHinting))
    {
        _fontDirty = true;
    }
    ImGui::SameLine(_widgetOffs + offs);
    if (ImGui::CheckboxFlags("MonoHinting", &_ftBuilderFlags, ImGuiFreeTypeBuilderFlags_MonoHinting))
    {
        _fontDirty = true;
    }
    // if (ImGui::CheckboxFlags("Bold", &_ftBuilderFlags, ImGuiFreeTypeBuilderFlags_Bold))
    // {
    //     _fontDirty = true;
    // }
    // if (ImGui::CheckboxFlags("Oblique", &_ftBuilderFlags, ImGuiFreeTypeBuilderFlags_Oblique))
    // {
    //     _fontDirty = true;
    // }
    ImGui::SameLine(_widgetOffs + offs + offs);
    if (ImGui::CheckboxFlags("Monochrome", &_ftBuilderFlags, ImGuiFreeTypeBuilderFlags_Monochrome))
    {
        _fontDirty = true;
    }

    ImGui::Separator();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Rasterizer multiply");
    ImGui::SameLine(_widgetOffs);
    ImGui::PushItemWidth(12 * charSize.x);
    float newMultiply = _ftRasterizerMultiply;
    if (ImGui::DragFloat("##RasterizerMultiply", &newMultiply, 0.01f, FT_RASTERIZER_MULTIPLY_MIN, FT_RASTERIZER_MULTIPLY_MAX, "%.2f"))
    {
        if (std::fabs(newMultiply - _ftRasterizerMultiply) > 0.01)
        {
            newMultiply = std::floor(newMultiply * 100.0f) / 100.0f;
            _ftRasterizerMultiply = CLIP(newMultiply, FT_RASTERIZER_MULTIPLY_MIN, FT_RASTERIZER_MULTIPLY_MAX);
            _fontDirty = true;
        }
    }
#endif
    // ImFontAtlas *atlas = ImGui::GetIO().Fonts;
    // if (ImGui::DragInt("TexGlyphPadding", &atlas->TexGlyphPadding, 0.1f, 1, 16))
    // {
    //     _fontDirty = true;
    // }
    // if (ImGui::DragFloat("RasterizerMultiply", &_ftRasterizerMultiply, 0.001f, 0.0f, 2.0f))
    // {
    //     _fontDirty = true;
    // }

    ImGui::Separator();

    ImGui::TextUnformatted("Default font");
    ImGui::SameLine(_widgetOffs);
    ImGui::PushFont(GuiSettings::fontMono);
    ImGui::TextUnformatted(GuiSettings::fontMono->ConfigData->Name);
    ImGui::NewLine();
    ImGui::SameLine(_widgetOffs);
    ImGui::TextUnformatted("abcdefghijklmnopqrstuvwxyz");
    ImGui::NewLine();
    ImGui::SameLine(_widgetOffs);
    ImGui::TextUnformatted("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    ImGui::NewLine();
    ImGui::SameLine(_widgetOffs);
    ImGui::TextUnformatted("0123456789+*%&/()=?{}[];,:.-_<>");
    ImGui::PopFont();

    ImGui::TextUnformatted("Sans font");
    ImGui::SameLine(_widgetOffs);
    ImGui::PushFont(GuiSettings::fontSans);
    ImGui::TextUnformatted(GuiSettings::fontSans->ConfigData->Name);
    ImGui::NewLine();
    ImGui::SameLine(_widgetOffs);
    ImGui::TextUnformatted("abcdefghijklmnopqrstuvwxyz");
    ImGui::NewLine();
    ImGui::SameLine(_widgetOffs);
    ImGui::TextUnformatted("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    ImGui::NewLine();
    ImGui::SameLine(_widgetOffs);
    ImGui::TextUnformatted("0123456789+*%&/()=?{}[];,:.-_<>");
    ImGui::PopFont();

    ImGui::TextUnformatted("Bold font");
    ImGui::SameLine(_widgetOffs);
    ImGui::PushFont(GuiSettings::fontBold);
    ImGui::TextUnformatted(GuiSettings::fontBold->ConfigData->Name);
    ImGui::NewLine();
    ImGui::SameLine(_widgetOffs);
    ImGui::TextUnformatted("abcdefghijklmnopqrstuvwxyz");
    ImGui::NewLine();
    ImGui::SameLine(_widgetOffs);
    ImGui::TextUnformatted("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    ImGui::NewLine();
    ImGui::SameLine(_widgetOffs);
    ImGui::TextUnformatted("0123456789+*%&/()=?{}[];,:.-_<>");
    ImGui::PopFont();

    ImGui::TextUnformatted("Oblique font");
    ImGui::SameLine(_widgetOffs);
    ImGui::PushFont(GuiSettings::fontOblique);
    ImGui::TextUnformatted(GuiSettings::fontOblique->ConfigData->Name);
    ImGui::NewLine();
    ImGui::SameLine(_widgetOffs);
    ImGui::TextUnformatted("abcdefghijklmnopqrstuvwxyz");
    ImGui::NewLine();
    ImGui::SameLine(_widgetOffs);
    ImGui::TextUnformatted("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    ImGui::NewLine();
    ImGui::SameLine(_widgetOffs);
    ImGui::TextUnformatted("0123456789+*%&/()=?{}[];,:.-_<>");
    ImGui::PopFont();
}

/*static*/ void GuiSettings::DrawSettingsEditorColours()
{
    const float _widgetOffs = 25 * charSize.x;

    if (ImGui::BeginChild("Colours"))
    {
        for (int ix = 0; ix < _NUM_COLOURS; ix++)
        {
            if (ix != 0)
            {
                ImGui::Separator();
            }

            ImGui::PushID(COLOUR_LABELS[ix]);

            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(COLOUR_LABELS[ix]);
            }
            ImGui::SameLine(_widgetOffs);
            {
                ImGui::BeginDisabled(colours[ix] == COLOUR_DEFAULTS[ix]);
                if (ImGui::Button(colours[ix] != COLOUR_DEFAULTS[ix] ? ICON_FK_TIMES : " ", iconSize))
                {
                    colours[ix] = COLOUR_DEFAULTS[ix];
                    colours4[ix] = ImGui::ColorConvertU32ToFloat4(COLOUR_DEFAULTS[ix]);
                }
                Gui::ItemTooltip("Reset to default");
                ImGui::EndDisabled();
            }
            ImGui::SameLine();
            {
                ImGui::PushItemWidth(charSize.x * 40);
                if (ImGui::ColorEdit4("##ColourEdit", &colours4[ix].x, ImGuiColorEditFlags_AlphaPreviewHalf))
                {
                    colours[ix] = ImGui::ColorConvertFloat4ToU32(colours4[ix]);
                }
                ImGui::PopItemWidth();
            }

            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}

/*static*/ void GuiSettings::DrawSettingsEditorMaps()
{
    if (ImGui::BeginChild("Maps"))
    {
        for (auto &map: maps)
        {
            char str[100];
            std::snprintf(str, sizeof(str), "%s (%s)##%p", map.title.c_str(), map.name.c_str(), &map);
            ImGui::Checkbox(str, &map.enabled);
            ImGui::Separator();
        }
    }
    ImGui::EndChild();
}

/*static*/ void GuiSettings::DrawSettingsEditorMisc()
{
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Database size");
    ImGui::SameLine(_widgetOffs);
    ImGui::BeginDisabled(dbNumRows == DB_NUM_ROWS_DEF);
    if (ImGui::Button(dbNumRows != DB_NUM_ROWS_DEF ? ICON_FK_TIMES : " ", iconSize))
    {
        dbNumRows = DB_NUM_ROWS_DEF;
    }
    Gui::ItemTooltip("Reset to default");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::PushItemWidth(25 * charSize.x);
    if (ImGui::SliderInt("##dbNumRows", &dbNumRows, DB_NUM_ROWS_MIN, DB_NUM_ROWS_MAX))
    {
        dbNumRows = CLIP(dbNumRows, DB_NUM_ROWS_MIN, DB_NUM_ROWS_MAX);
    }
    ImGui::SameLine();
    ImGui::Text("(~%.0fMB per rx or log)", (double)(sizeof(Database::Row) * dbNumRows) * (1.0/1024.0/1024.0));
    ImGui::PopItemWidth();

    ImGui::TextUnformatted("Min framerate");
    ImGui::SameLine(_widgetOffs);
    ImGui::BeginDisabled(minFrameRate == MIN_FRAME_RATE_DEF);
    if (ImGui::Button(minFrameRate != MIN_FRAME_RATE_DEF ? ICON_FK_TIMES : " ", iconSize))
    {
        minFrameRate = MIN_FRAME_RATE_DEF;
    }
    Gui::ItemTooltip("Reset to default");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::PushItemWidth(25 * charSize.x);
    if (ImGui::SliderInt("##minFrameRate", &minFrameRate, MIN_FRAME_RATE_MIN, MIN_FRAME_RATE_MAX))
    {
        minFrameRate = CLIP(minFrameRate, MIN_FRAME_RATE_MIN, MIN_FRAME_RATE_MAX);
    }
    ImGui::SameLine();
    ImGui::PopItemWidth();
}

/*static*/ void GuiSettings::DrawSettingsEditorTools()
{
    ImGui::BeginDisabled(_clearSettingsOnExit);
    if (ImGui::Button("Clear settings on exit"))
    {
        _clearSettingsOnExit = true;
    }
    ImGui::EndDisabled();
    if (_clearSettingsOnExit)
    {
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            _clearSettingsOnExit = false;
        }
        ImGui::TextUnformatted("Config file will be wiped on exit!");
    }
}

/* ****************************************************************************************************************** */
