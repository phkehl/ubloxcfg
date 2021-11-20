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

#ifndef __GUI_SETTINGS_HPP__
#define __GUI_SETTINGS_HPP__

#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <cstdint>

#include "ff_epoch.h"
#include "ff_conffile.hpp"
#include "mapparams.hpp"
#include "imgui.h"
#include "implot.h"

#include "gui_settings_colours.hpp"

/* ****************************************************************************************************************** */

#define _SETTINGS_COLOUR_ENUM(_str, _enum, _col) _enum,

class GuiSettings
{
    public:
        GuiSettings(const std::string &cfgFile);
       ~GuiSettings();

        void LoadConf(const std::string &file);
        void SaveConf(const std::string &file);

        // Generic settings storage
        void SetValue(const std::string &key, const bool     value);
        void SetValue(const std::string &key, const int      value);
        void SetValue(const std::string &key, const uint32_t value);
        void SetValue(const std::string &key, const float    value);
        void SetValue(const std::string &key, const double   value);
        void SetValue(const std::string &key, const std::string &value);
        void SetValueMult(const std::string &key, const std::vector<std::string> &list, const int maxNum);
        void SetValueList(const std::string &key, const std::vector<std::string> &list, const std::string &sep, const int maxNum);
        void GetValue(const std::string &key, bool     &value, const bool     def);
        void GetValue(const std::string &key, int      &value, const int      def);
        void GetValue(const std::string &key, uint32_t &value, const uint32_t def);
        void GetValue(const std::string &key, float    &value, const float    def);
        void GetValue(const std::string &key, double   &value, const double   def);
        void GetValue(const std::string &key, std::string &value, const std::string &def);
        std::string GetValue(const std::string &key);
        std::vector<std::string> GetValueMult(const std::string &key, const int maxNum);
        std::vector<std::string> GetValueList(const std::string &key, const std::string &sep, const int maxNum);

        // Font
        float                fontSize;
        ImFont              *fontMono; // default
        ImFont              *fontSans;

        // Colours
        enum Colour_e : int { _DUMMY = -1, GUI_SETTINGS_COLOURS(_SETTINGS_COLOUR_ENUM) _NUM_COLOURS };
        static ImU32         colours[_NUM_COLOURS]; // use GUI_COLOUR() macro to access

        // GNSS fix colour
        ImU32                GetFixColour(const EPOCH_t *epoch);

        // Paths
        std::string          cachePath;

        // Maps
        std::vector<MapParams> maps;

        // Helpers
        float                menuBarHeight;  // Height of main window menu bar (and probably any other menu bar...)
        ImVec2               iconButtonSize; // Size for ImGui::Button() with just an icon (ICON_FK_...)
        ImGuiStyle          &style;          // Shortcut to ImGui styles
        ImVec2               charSize;       // Char size (of a fontMono character)
        ImPlotStyle         &plotStyle;      // Shortcut to ImPlot styles

        // See main()
        bool                 _UpdateFont();
        bool                 _UpdateSizes();

        void                 _DrawSettingsEditor(); // for GuiWinSettings

    private:
        static const char * const COLOUR_LABELS[_NUM_COLOURS];
        static const char * const COLOUR_NAMES[_NUM_COLOURS];
        static ImU32              COLOUR_DEFAULTS[_NUM_COLOURS];

        static constexpr float    FONT_SIZE_DEF = 13.0;
        static constexpr float    FONT_SIZE_MIN = 10.0;
        static constexpr float    FONT_SIZE_MAX = 30.0;

        bool                      _fontDirty;
        bool                      _sizesDirty;
        float                     _widgetOffs;

        // Freetype2 config
        uint32_t                  _ftBuilderFlags;
        float                     _ftRasterizerMultiply;
        static constexpr uint32_t FT_BUILDER_FLAGS_DEF = (1 << 2); // ImGuiFreeTypeBuilderFlags_ForceAutoHint
        static constexpr float    FT_RASTERIZER_MULTIPLY_DEF = 1.0f;
        static constexpr float    FT_RASTERIZER_MULTIPLY_MIN = 0.2f;
        static constexpr float    FT_RASTERIZER_MULTIPLY_MAX = 2.0f;

        Ff::ConfFile              _confFile;
        bool                      _clearSettingsOnExit;
};

#define GUI_COLOUR(which) GuiSettings::colours[GuiSettings::which]

/* ****************************************************************************************************************** */
#endif // __GUI_SETTINGS_HPP__
