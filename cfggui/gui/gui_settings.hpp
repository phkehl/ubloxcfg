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

#ifndef __GUI_SETTINGS_H__
#define __GUI_SETTINGS_H__

#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <map>

#include "ff_cpp.hpp"
#include "maptiles.hpp"
#include "imgui.h"
#include "implot.h"

/* ****************************************************************************************************************** */

#define SETTINGS_COLOURS(_P_) \
    _P_("App background",        APP_BACKGROUND,                     IM_COL32(0x73, 0x8c, 0x99, 0xff)) \
    _P_("Text title",            TEXT_TITLE,                         IM_COL32(0x55, 0xff, 0xff, 0xff)) \
    _P_("Text dim",              TEXT_DIM,                           IM_COL32(0xaa, 0xaa, 0xaa, 0xff)) \
    _P_("Text highlight",        TEXT_HIGHLIGHT,                     IM_COL32(0x55, 0xff, 0x55, 0xff)) \
    _P_("Fix invalid",           FIX_INVALID,                        IM_COL32(0xd9, 0x00, 0x00, 0xaa)) \
    _P_("Fix masked",            FIX_MASKED,                         IM_COL32(0x00, 0xaa, 0xff, 0xaa)) \
    _P_("Fix DR-only",           FIX_DRONLY,                         IM_COL32(0xb0, 0x00, 0xb0, 0xaa)) \
    _P_("Fix 2D",                FIX_S2D,                            IM_COL32(0x00, 0xaa, 0xaa, 0xaa)) \
    _P_("Fix 3D",                FIX_S3D,                            IM_COL32(0x00, 0xd9, 0x00, 0xaa)) \
    _P_("Fix 3D+DR",             FIX_S3D_DR,                         IM_COL32(0xd9, 0xd9, 0x00, 0xaa)) \
    _P_("Fix time",              FIX_TIME,                           IM_COL32(0xd0, 0xd0, 0xd0, 0xaa)) \
    _P_("Fix RTK float",         FIX_RTK_FLOAT,                      IM_COL32(0xff, 0xff, 0x00, 0xaa)) \
    _P_("Fix RTK fixed",         FIX_RTK_FIXED,                      IM_COL32(0xff, 0xa0, 0x00, 0xaa)) \
    _P_("Fix RTK float + DR",    FIX_RTK_FLOAT_DR,                   IM_COL32(0x9e, 0x9e, 0x00, 0xaa)) \
    _P_("Fix RTK fixed + DR",    FIX_RTK_FIXED_DR,                   IM_COL32(0x9e, 0x64, 0x00, 0xaa)) \
    _P_("Plot grid major",       PLOT_GRID_MAJOR,                    IM_COL32(0xaa, 0xaa, 0xaa, 0xff)) \
    _P_("Plot grid minor",       PLOT_GRID_MINOR,                    IM_COL32(0x55, 0x55, 0x55, 0xff)) \
    _P_("Plot grid label",       PLOT_GRID_LABEL,                    IM_COL32(0xff, 0xff, 0xff, 0xff)) \
    _P_("Plot stats min/max",    PLOT_STATS_MINMAX,                  IM_COL32(0xaa, 0x00, 0x00, 0xff)) \
    _P_("Plot stats mean",       PLOT_STATS_MEAN,                    IM_COL32(0xaa, 0x00, 0xaa, 0xff)) \
    _P_("Plot error ellipse",    PLOT_ERR_ELL,                       IM_COL32(0xaa, 0x00, 0x00, 0xaf)) \
    _P_("Plot highlight masked", PLOT_FIX_HL_MASKED,                 IM_COL32(0x55, 0xff, 0xff, 0xff)) \
    _P_("Plot highlight OK fix", PLOT_FIX_HL_OK,                     IM_COL32(0xff, 0x55, 0x55, 0xff)) \
    _P_("Plot crosshairs",       PLOT_FIX_CROSSHAIRS,                IM_COL32(0xff, 0x55, 0xff, 0xaf)) \
    _P_("Plot crosshairs label", PLOT_FIX_CROSSHAIRS_LABEL,          IM_COL32(0xff, 0x55, 0xff, 0xff)) \
    _P_("Plot histogram",        PLOT_HISTOGRAM,                     IM_COL32(0xb2, 0x81, 0x5e, 0xff)) \
    _P_("Map crosshairs",        PLOT_MAP_CROSSHAIRS,                IM_COL32(0xff, 0x44, 0x44, 0xaf)) \
    _P_("Map highlight masked",  PLOT_MAP_HL_MASKED,                 IM_COL32(0x55, 0xff, 0xff, 0xff)) \
    _P_("Map highlight OK fix",  PLOT_MAP_HL_OK,                     IM_COL32(0xff, 0x55, 0x55, 0xff)) \
    _P_("Map accuracy estimate", PLOT_MAP_ACC_EST,                   IM_COL32(0x40, 0x40, 0x40, 0x55)) \
    _P_("Map baseline",          PLOT_MAP_BASELINE,                  IM_COL32(0xff, 0x42, 0xf2, 0xcc)) /* hsla(304, 100%, 63%, 0.8) */ \
    _P_("Sky view satellite",    SKY_VIEW_SAT,                       IM_COL32(0x55, 0x86, 0xff, 0xa3)) \
    _P_("Signal used",           SIGNAL_USED,                        IM_COL32(0x00, 0xaa, 0x00, 0xaf)) \
    _P_("Signal unused",         SIGNAL_UNUSED,                      IM_COL32(0xaa, 0xaa, 0xaa, 0xaf)) \
    _P_("Signal 0 - 5 dBHz",     SIGNAL_00_05,                       IM_COL32(0xe0, 0x52, 0x52, 0xff)) /* hsl(  0, 70%, 60%) */ \
    _P_("Signal 5 - 10 dBHz",    SIGNAL_05_10,                       IM_COL32(0xe0, 0x69, 0x52, 0xff)) /* hsl( 10, 70%, 60%) */ \
    _P_("Signal 10 - 15 dBHz",   SIGNAL_10_15,                       IM_COL32(0xe0, 0x81, 0x52, 0xff)) /* hsl( 20, 70%, 60%) */ \
    _P_("Signal 15 - 20 dBHz",   SIGNAL_15_20,                       IM_COL32(0xe0, 0x99, 0x52, 0xff)) /* hsl( 30, 70%, 60%) */ \
    _P_("Signal 20 - 25 dBHz",   SIGNAL_20_25,                       IM_COL32(0xe0, 0xb1, 0x52, 0xff)) /* hsl( 40, 70%, 60%) */ \
    _P_("Signal 25 - 30 dBHz",   SIGNAL_25_30,                       IM_COL32(0xe0, 0xc9, 0x52, 0xff)) /* hsl( 50, 70%, 60%) */ \
    _P_("Signal 30 - 35 dBHz",   SIGNAL_30_35,                       IM_COL32(0xe0, 0xe0, 0x52, 0xff)) /* hsl( 60, 70%, 60%) */ \
    _P_("Signal 35 - 40 dBHz",   SIGNAL_35_40,                       IM_COL32(0xb3, 0xe0, 0x85, 0xff)) /* hsl( 90, 60%, 70%) */ \
    _P_("Signal 40 - 45 dBHz",   SIGNAL_40_45,                       IM_COL32(0x06, 0xf9, 0x06, 0xff)) /* hsl(120, 95%, 50%) */ \
    _P_("Signal 45 - 50 dBHz",   SIGNAL_45_50,                       IM_COL32(0x13, 0xec, 0x5b, 0xff)) /* hsl(140, 85%, 50%) */ \
    _P_("Signal 50 - 55 dBHz",   SIGNAL_50_55,                       IM_COL32(0x13, 0xec, 0xa4, 0xff)) /* hsl(160, 85%, 50%) */ \
    _P_("Signal 55 - oo dBHz",   SIGNAL_55_OO,                       IM_COL32(0x13, 0xec, 0xec, 0xff)) /* hsl(180, 85%, 50%) */

#define _SETTINGS_COLOUR_ENUM(_str, _enum, _col) _enum,

class GuiSettings
{
    public:
        GuiSettings(const std::string &cfgFile);
       ~GuiSettings();

        void                 LoadConf(const std::string &file);
        void                 SaveConf(const std::string &file);

        // Generic settings storage
        void                 SetValue(const std::string &key, const bool   value);
        void                 SetValue(const std::string &key, const int    value);
        void                 SetValue(const std::string &key, const float  value);
        void                 SetValue(const std::string &key, const double value);
        void                 SetValue(const std::string &key, const std::string &value);
        void                 GetValue(const std::string &key, bool   &value, const bool   def);
        void                 GetValue(const std::string &key, int    &value, const int    def);
        void                 GetValue(const std::string &key, float  &value, const float  def);
        void                 GetValue(const std::string &key, double &value, const double def);
        void                 GetValue(const std::string &key, std::string &value, const std::string &def);
        std::string          GetValue(const std::string &key);

        // Font
        enum Font_e : int { FONT_PROGGY_IX = 0, FONT_DEJAVU_IX = 1, NUM_FONTS = 2 };
        enum Font_e          fontIx;
        float                fontSize;

        // Colours
        enum Colour_e : int { _DUMMY = -1, SETTINGS_COLOURS(_SETTINGS_COLOUR_ENUM) _NUM_COLOURS };
        static ImU32         colours[_NUM_COLOURS];

        // GNSS fix colour
        ImU32                GetFixColour(const EPOCH_t *epoch);

        // Paths
        std::string          assetPath;
        std::string          cachePath;

        // Maps
        std::vector<MapParams> maps;

        // Helpers
        float                menuBarHeight;
        ImVec2               iconButtonSize;
        ImGuiStyle          &style;
        ImVec2               charSize;
        ImPlotStyle         &plotStyle;

        // See main()
        bool                 _UpdateFont(const bool force = false);
        bool                 _UpdateSizes(const bool force = false);

        // See GuiWinSettings
        void                 DrawSettingsEditor();

    private:
        static const char * const kColourLabels[_NUM_COLOURS];
        static const char * const kColourNames[_NUM_COLOURS];
        static ImU32              kColourDefaults[_NUM_COLOURS];

        static const char * const fontNames[NUM_FONTS];
        static constexpr float    fontSizeMin = 10.0;
        static constexpr float    fontSizeMax = 30.0;

        bool                 _fontDirty;
        bool                 _sizesDirty;
        float                _widgetOffs;

        Ff::ConfFile         _confFile;
};

#define GUI_COLOUR(which) GuiSettings::colours[GuiSettings::which]

/* ****************************************************************************************************************** */
#endif // __GUI_SETTINGS_H__
