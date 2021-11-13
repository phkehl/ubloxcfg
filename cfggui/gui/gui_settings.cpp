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

#include <cmath>
#include <fstream>
#include <cerrno>
#include <filesystem>
#include <algorithm>

#include "ff_stuff.h"
#include "ff_debug.h"
#include "ff_trafo.h"
#include "ff_cpp.hpp"

#include "IconsForkAwesome.h"

#include "gui_fonts.hpp"
#include "gui_widget.hpp"
#include "gui_settings.hpp"
#include "platform.hpp"

/* ****************************************************************************************************************** */

//#define _SETTINGS_COLOUR_COL(_str, _enum, _col) _col,

GuiSettings::GuiSettings(const std::string &cfgFile) :
    // Settings
    fontIx{FONT_PROGGY_IX}, fontSize{13.0f},
    //colours{ SETTINGS_COLOURS(_SETTINGS_COLOUR_COL) },
    assetPath{}, cachePath{},
    maps{},
    // Helpers
    menuBarHeight{10.0},
    iconButtonSize{13.0, 13.0},
    style{ImGui::GetStyle()},
    charSize{8.0, 13.0},
    plotStyle{ImPlot::GetStyle()},

    // Private
    _fontDirty{true}, _sizesDirty{true}
{
    DEBUG("GuiSettings()");

    ImGuiIO &io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    //io.FontAllowUserScaling = true;
    //io.UserData = NULL;
    io.IniFilename = NULL;
    //io.IniFilename = "/tmp/meier.ini";
    //io.FontGlobalScale = 2.0;

    ImGui::SetColorEditOptions(
        ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_InputRGB |
        ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview |
        ImGuiColorEditFlags_AlphaPreviewHalf);

    // Tune style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    style.FrameBorderSize          = 1.0f;
    style.TabBorderSize            = 1.0f;
    style.WindowTitleAlign.x       = 0.5f;
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.FrameRounding            = 3.0f;
    //style.WindowBorderSize         = 5.0f;
    style.WindowRounding           = 5.0f;

    ImPlot::StyleColorsAuto();
    plotStyle.LineWeight = 2.0;
    //plotStyle.AntiAliasedLines = true;
    plotStyle.UseLocalTime = false;
    plotStyle.UseISO8601 = true;
    plotStyle.Use24HourClock = true;
    plotStyle.FitPadding = ImVec2(0.1, 0.1);

    const std::string baseDir = std::filesystem::weakly_canonical( std::filesystem::path(cfgFile).parent_path() ).string();
    assetPath = baseDir + "/../cfggui";
    cachePath = Platform::CacheDir("tiles");

    if (!cfgFile.empty())
    {
        LoadConf(cfgFile);
    }
}

GuiSettings::~GuiSettings()
{
    DEBUG("~GuiSettings()");
}

#define _SETTINGS_COLOUR_COL(_str, _enum, _col) _col,

/* static */ImU32 GuiSettings::colours[_NUM_COLOURS] = { SETTINGS_COLOURS(_SETTINGS_COLOUR_COL) };

// ---------------------------------------------------------------------------------------------------------------------

void GuiSettings::LoadConf(const std::string &file)
{
    DEBUG("GuiSettings::LoadConf(%s)", file.c_str());

    // Load ImGui stuff
    ImGui::LoadIniSettingsFromDisk(file.c_str());

    // Load our stuff
    if (_confFile.Load(file, "cfggui"))
    {
        // Colours
        for (int ix = 0; ix < _NUM_COLOURS; ix++)
        {
            uint32_t val;
            if (_confFile.Get(std::string("Colour.") + kColourNames[ix], val))
            {
                colours[ix] = val;
            }
        }
    }

    // Load maps
    {
        const std::string mapsConfFile = Platform::ConfigFile("maps.conf");

        // Create default maps.conf
        if (!Platform::FileExists(mapsConfFile))
        {
#include "maps_conf.c"
            Platform::FileSpew(mapsConfFile, cfggui_maps_conf, cfggui_maps_conf_len);
        }

        auto mapsConf = Ff::ConfFile();
        while (mapsConf.Load(mapsConfFile, "map"))
        {
            MapParams map;
            if (!mapsConf.Get("name",        map.name)        || map.name.empty()        ||
                !mapsConf.Get("title",       map.title)       || map.title.empty()       ||
                !mapsConf.Get("attribution", map.attribution) || map.attribution.empty() ||
                !mapsConf.Get("link",        map.link)        || map.link.empty()        ||
                !mapsConf.Get("zoomMin",     map.zoomMin)     || (map.zoomMin < 0)       ||
                !mapsConf.Get("zoomMax",     map.zoomMax)     || (map.zoomMax < map.zoomMin) ||
                !mapsConf.Get("tileSizeX",   map.tileSizeX)   || (map.tileSizeX < 100)   ||
                !mapsConf.Get("tileSizeY",   map.tileSizeY)   || (map.tileSizeY < 100)   ||
                !mapsConf.Get("downloadUrl", map.downloadUrl) || map.downloadUrl.empty() ||
                !mapsConf.Get("cachePath",   map.cachePath)   || map.cachePath.empty()   ||
                false
                )
            {
                WARNING("Incomplete map info in %s!", mapsConfFile.c_str());
                continue;
            }
            DEBUG("Map %s: %s", map.name.c_str(), map.title.c_str());
            mapsConf.Get("referer", map.referer);
            mapsConf.Get("threads", map.threads);
            mapsConf.Get("downloadTimeout", map.downloadTimeout);
            if      (map.downloadTimeout <  1000) { map.downloadTimeout =  5000; }
            else if (map.downloadTimeout > 60000) { map.downloadTimeout = 60000; }
            if      (map.threads <  1) { map.threads =  1; }
            else if (map.threads > 10) { map.threads = 10; }
            if (mapsConf.Get("minLat", map.minLat)) { map.minLat = deg2rad(map.minLat); }
            if (mapsConf.Get("maxLat", map.maxLat)) { map.maxLat = deg2rad(map.maxLat); }
            if (mapsConf.Get("minLon", map.minLon)) { map.minLon = deg2rad(map.minLon); }
            if (mapsConf.Get("maxLon", map.maxLon)) { map.maxLon = deg2rad(map.maxLon); }
            if ( (map.minLat < MapTiles::kMinLat) || (map.maxLat > MapTiles::kMaxLat) ||
                 (map.minLon < MapTiles::kMinLon) || (map.maxLon > MapTiles::kMaxLon) )
            {
                WARNING("Bad {min,max}{Lon,Lat} in %s!", mapsConfFile.c_str());
                continue;
            }
            std::string subDomains;
            if (mapsConf.Get("subDomains", subDomains))
            {
                map.subDomains = Ff::StrSplit(subDomains, ",");
            }
            maps.push_back(map);
            mapsConf.Clear();
        }
    }
}


// ---------------------------------------------------------------------------------------------------------------------

void GuiSettings::SetValue(const std::string &key, const bool value)
{
    _confFile.Set(key, value);
}

void GuiSettings::SetValue(const std::string &key, const int value)
{
    _confFile.Set(key, value);
}

void GuiSettings::SetValue(const std::string &key, const double value)
{
    _confFile.Set(key, value);
}

void GuiSettings::SetValue(const std::string &key, const float value)
{
    const double dbl = value;
    _confFile.Set(key, dbl);
}

void GuiSettings::SetValue(const std::string &key, const std::string &value)
{
    _confFile.Set(key, value);
}

void GuiSettings::SetValueList(const std::string &key, const std::vector<std::string> &list)
{
    _confFile.Set(key, Ff::StrJoin(list, ","));
}

void GuiSettings::GetValue(const std::string &key, bool &value, const bool def)
{
    if (!_confFile.Get(key, value))
    {
        value = def;
    }
}

void GuiSettings::GetValue(const std::string &key, int &value, const int def)
{
    if (!_confFile.Get(key, value))
    {
        value = def;
    }
}

void GuiSettings::GetValue(const std::string &key, double &value, const double def)
{
    if (!_confFile.Get(key, value))
    {
        value = def;
    }
}

void GuiSettings::GetValue(const std::string &key, float &value, const float def)
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

void GuiSettings::GetValue(const std::string &key, std::string &value, const std::string &def)
{
    if (!_confFile.Get(key, value))
    {
        value = def;
    }
}

std::string GuiSettings::GetValue(const std::string &key)
{
    std::string value;
    GetValue(key, value, "");
    return value;
}

std::vector<std::string> GuiSettings::GetValueList(const std::string &key)
{
    std::string value = GetValue(key);
    return value.empty() ? std::vector<std::string>() : Ff::StrSplit(value, ",");
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiSettings::SaveConf(const std::string &file)
{
    DEBUG("GuiSettings::SaveConf(%s)", file.c_str());

    // Save ImGui stuff
    ImGui::SaveIniSettingsToDisk(file.c_str());

    // Append our own stuff
    for (int ix = 0; ix < _NUM_COLOURS; ix++)
    {
        _confFile.Set(std::string("Colour.") + kColourNames[ix], colours[ix], true);
    }
    _confFile.Save(file, "cfggui", true);
}

// ---------------------------------------------------------------------------------------------------------------------

#define _SETTINGS_COLOUR_LABEL(_str, _enum, _col) _str,
#define _SETTINGS_COLOUR_NAME(_str, _enum, _col) STRINGIFY(_enum),

const char * const GuiSettings::kColourLabels[GuiSettings::_NUM_COLOURS] =
{
    SETTINGS_COLOURS(_SETTINGS_COLOUR_LABEL)
};

const char * const GuiSettings::kColourNames[GuiSettings::_NUM_COLOURS] =
{
    SETTINGS_COLOURS(_SETTINGS_COLOUR_NAME)
};

ImU32 GuiSettings::kColourDefaults[_NUM_COLOURS] =
{
    SETTINGS_COLOURS(_SETTINGS_COLOUR_COL)
};

// ---------------------------------------------------------------------------------------------------------------------

const char * const GuiSettings::fontNames[NUM_FONTS] =
{
    [GuiSettings::FONT_PROGGY_IX] = "ProggyClean",
    [GuiSettings::FONT_DEJAVU_IX] = "DejaVu Mono",
};

bool GuiSettings::_UpdateFont(const bool force)
{
    if (!force && !_fontDirty)
    {
        return false;
    }
    DEBUG("GuiSettings::_UpdateFont()");

    ImGuiIO &io = ImGui::GetIO();

    // Clear all font data
    io.Fonts->Clear();

    // Calculate glyphs to load
    //ImVector<ImWchar> glyphs;
    //ImFontGlyphRangesBuilder builder;
    //builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    //builder.AddText("σ");
    //builder.BuildRanges(&glyphs);

    // Load font
    ImFontConfig config;
    switch (fontIx)
    {
        case FONT_DEJAVU_IX:
            snprintf(config.Name, sizeof(config.Name), "DejaVu Mono %.1f", fontSize);
            config.GlyphOffset = ImVec2(0, -fontSize / 10.0);
            io.Fonts->AddFontFromMemoryCompressedBase85TTF(guiGetFontDejaVuSansMono(), fontSize, &config/*, glyphs.Data*/);
            break;
        case FONT_PROGGY_IX:
            snprintf(config.Name, sizeof(config.Name), "ProggyClean %.1f", fontSize);
            io.Fonts->AddFontFromMemoryCompressedBase85TTF(guiGetFontProggyClean(), fontSize, &config/*, glyphs.Data*/);
            break;
        case NUM_FONTS:
            break;
    }

    // Add icons (https://forkaweso.me/Fork-Awesome/icons/)
    static const ImWchar icons[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };
    config.MergeMode = true;
    config.GlyphOffset = ImVec2(0, fontSize / 10.0);
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(guiGetFontForkAwesome(), fontSize, &config, icons);

    // Build font atlas
    io.Fonts->Build();

    _fontDirty = false;
    _sizesDirty = true;

    // The remaining tasks (loading texture) will be done in main()
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiSettings::_UpdateSizes(const bool force)
{
    if (!force && !_sizesDirty)
    {
        return false;
    }
    DEBUG("GuiSettings::_UpdateSizes()");
    const float frameHeight = ImGui::GetFrameHeight();
    iconButtonSize = ImVec2(frameHeight, frameHeight);
    charSize = ImGui::CalcTextSize("X");
    _widgetOffs = 25 * charSize.x;

    _sizesDirty = false;
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

ImU32 GuiSettings::GetFixColour(const EPOCH_t *epoch)
{
    if (!epoch || !epoch->haveFix)
    {
        return colours[FIX_INVALID];
    }
    else if (!epoch->fixOk)
    {
        return colours[FIX_MASKED];
    }
    else
    {
        switch (epoch->fix)
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
    }
    return colours[FIX_INVALID];
}


// ---------------------------------------------------------------------------------------------------------------------

void GuiSettings::DrawSettingsEditor()
{
    // Font selection
    {
        bool changed = false;
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Font");
        ImGui::SameLine(_widgetOffs);
        ImGui::PushItemWidth(15 * charSize.x);
        Font_e newFontIx = fontIx;
        if (ImGui::Combo("##Font", (int *)&newFontIx, fontNames, GuiSettings::NUM_FONTS))
        {
            changed = true;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::PushItemWidth(12 * charSize.x);
        float newFontSize = fontSize;
        if (ImGui::InputFloat("##FontSize", &newFontSize, 1.0, 1.0, "%.1f", ImGuiInputTextFlags_EnterReturnsTrue))
        {
            changed = true;
        }
        ImGui::PopItemWidth();
        if ( changed && ( (newFontIx != fontIx) || (std::fabs(newFontSize - fontSize) > 0.05) ) )
        {
            DEBUG("new font: %s %.1f", fontNames[newFontIx], newFontSize);

            switch (newFontIx)
            {
                case FONT_DEJAVU_IX: fontIx = FONT_DEJAVU_IX; break;
                case FONT_PROGGY_IX:
                default:             fontIx = FONT_PROGGY_IX; break;
            }
            if (newFontSize < fontSizeMin)
            {
                fontSize = fontSizeMin;
            }
            else if (newFontSize > fontSizeMax)
            {
                fontSize = fontSizeMax;
            }
            else
            {
                fontSize = newFontSize;
            }
            fontSize = std::floor(fontSize * 10.0f) / 10.0f;
            _fontDirty = true; // trigger change in main()
        }
    }

    // Colours
    for (int ix = 0; ix < _NUM_COLOURS; ix++)
    {
        ImGui::Separator();

        ImGui::PushID(kColourLabels[ix]);

        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(kColourLabels[ix]);
        }
        ImGui::SameLine(_widgetOffs);
        {
            if (ImGui::Button(colours[ix] != kColourDefaults[ix] ? "#" : " ", iconButtonSize))
            {
                colours[ix] = kColourDefaults[ix];
            }
        }
        ImGui::SameLine();
        {
            ImVec4 col = ImGui::ColorConvertU32ToFloat4(colours[ix]);
            float col4[4] = { col.x, col.y, col.z, col.w };
            ImGui::PushItemWidth(charSize.x * 40);
            if (ImGui::ColorEdit4("##ColourEdit", col4, ImGuiColorEditFlags_AlphaPreviewHalf))
            {
                colours[ix] = ImGui::ColorConvertFloat4ToU32(ImVec4(col4[0], col4[1], col4[2], col4[3]));
            }
            ImGui::PopItemWidth();
        }

        ImGui::PopID();
    }
}

/* ****************************************************************************************************************** */
