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

#ifndef __GUI_SETTINGS_COLOURS_HPP__
#define __GUI_SETTINGS_COLOURS_HPP__

#define GUI_SETTINGS_COLOURS(_P_) \
    _P_("App background",        APP_BACKGROUND,               IM_COL32(0x73, 0x8c, 0x99, 0xff)) \
    _P_("Text title",            TEXT_TITLE,                   IM_COL32(0x55, 0xff, 0xff, 0xff)) \
    _P_("Text dim",              TEXT_DIM,                     IM_COL32(0xaa, 0xaa, 0xaa, 0xff)) \
    _P_("Text highlight",        TEXT_HIGHLIGHT,               IM_COL32(0xff, 0xa2, 0x57, 0xff)) \
    _P_("Text okay",             TEXT_OK,                      IM_COL32(0x55, 0xff, 0x55, 0xff)) \
    _P_("Text warning",          TEXT_WARNING,                 IM_COL32(0xff, 0xff, 0x55, 0xff)) \
    _P_("Text error",            TEXT_ERROR,                   IM_COL32(0xff, 0x55, 0x55, 0xff)) \
    _P_("Fix invalid",           FIX_INVALID,                  IM_COL32(0xd9, 0x00, 0x00, 0xaa)) \
    _P_("Fix masked",            FIX_MASKED,                   IM_COL32(0x00, 0xaa, 0xff, 0xaa)) \
    _P_("Fix DR-only",           FIX_DRONLY,                   IM_COL32(0xb0, 0x00, 0xb0, 0xaa)) \
    _P_("Fix 2D",                FIX_S2D,                      IM_COL32(0x00, 0xaa, 0xaa, 0xaa)) \
    _P_("Fix 3D",                FIX_S3D,                      IM_COL32(0x00, 0xd9, 0x00, 0xaa)) \
    _P_("Fix 3D+DR",             FIX_S3D_DR,                   IM_COL32(0xd9, 0xd9, 0x00, 0xaa)) \
    _P_("Fix time",              FIX_TIME,                     IM_COL32(0xd0, 0xd0, 0xd0, 0xaa)) \
    _P_("Fix RTK float",         FIX_RTK_FLOAT,                IM_COL32(0xff, 0xff, 0x00, 0xaa)) \
    _P_("Fix RTK fixed",         FIX_RTK_FIXED,                IM_COL32(0xff, 0xa0, 0x00, 0xaa)) \
    _P_("Fix RTK float + DR",    FIX_RTK_FLOAT_DR,             IM_COL32(0x9e, 0x9e, 0x00, 0xaa)) \
    _P_("Fix RTK fixed + DR",    FIX_RTK_FIXED_DR,             IM_COL32(0x9e, 0x64, 0x00, 0xaa)) \
    _P_("Plot grid major",       PLOT_GRID_MAJOR,              IM_COL32(0xaa, 0xaa, 0xaa, 0xff)) \
    _P_("Plot grid minor",       PLOT_GRID_MINOR,              IM_COL32(0x55, 0x55, 0x55, 0xff)) \
    _P_("Plot grid label",       PLOT_GRID_LABEL,              IM_COL32(0xff, 0xff, 0xff, 0xff)) \
    _P_("Plot stats min/max",    PLOT_STATS_MINMAX,            IM_COL32(0xaa, 0x00, 0x00, 0xff)) \
    _P_("Plot stats mean",       PLOT_STATS_MEAN,              IM_COL32(0xaa, 0x00, 0xaa, 0xff)) \
    _P_("Plot error ellipse",    PLOT_ERR_ELL,                 IM_COL32(0xaa, 0x00, 0x00, 0xaf)) \
    _P_("Plot highlight masked", PLOT_FIX_HL_MASKED,           IM_COL32(0x55, 0xff, 0xff, 0xff)) \
    _P_("Plot highlight OK fix", PLOT_FIX_HL_OK,               IM_COL32(0xff, 0x55, 0x55, 0xff)) \
    _P_("Plot crosshairs",       PLOT_FIX_CROSSHAIRS,          IM_COL32(0xff, 0x55, 0xff, 0xaf)) \
    _P_("Plot crosshairs label", PLOT_FIX_CROSSHAIRS_LABEL,    IM_COL32(0xff, 0x55, 0xff, 0xff)) \
    _P_("Plot histogram",        PLOT_HISTOGRAM,               IM_COL32(0xb2, 0x81, 0x5e, 0xff)) \
    _P_("Map crosshairs",        PLOT_MAP_CROSSHAIRS,          IM_COL32(0xff, 0x44, 0x44, 0xaf)) \
    _P_("Map highlight masked",  PLOT_MAP_HL_MASKED,           IM_COL32(0x55, 0xff, 0xff, 0xff)) \
    _P_("Map highlight OK fix",  PLOT_MAP_HL_OK,               IM_COL32(0xff, 0x55, 0x55, 0xff)) \
    _P_("Map accuracy estimate", PLOT_MAP_ACC_EST,             IM_COL32(0x40, 0x40, 0x40, 0x55)) \
    _P_("Map baseline",          PLOT_MAP_BASELINE,            IM_COL32(0xff, 0x42, 0xf2, 0xcc)) /* hsla(304, 100%, 63%, 0.8) */ \
    _P_("Sky view satellite",    SKY_VIEW_SAT,                 IM_COL32(0x55, 0x86, 0xff, 0xa3)) \
    _P_("Signal used",           SIGNAL_USED,                  IM_COL32(0x00, 0xaa, 0x00, 0xaf)) \
    _P_("Signal unused",         SIGNAL_UNUSED,                IM_COL32(0xaa, 0xaa, 0xaa, 0xaf)) \
    _P_("Signal 0 - 5 dBHz",     SIGNAL_00_05,                 IM_COL32(0xe0, 0x52, 0x52, 0xff)) /* hsl(  0, 70%, 60%) */ \
    _P_("Signal 5 - 10 dBHz",    SIGNAL_05_10,                 IM_COL32(0xe0, 0x69, 0x52, 0xff)) /* hsl( 10, 70%, 60%) */ \
    _P_("Signal 10 - 15 dBHz",   SIGNAL_10_15,                 IM_COL32(0xe0, 0x81, 0x52, 0xff)) /* hsl( 20, 70%, 60%) */ \
    _P_("Signal 15 - 20 dBHz",   SIGNAL_15_20,                 IM_COL32(0xe0, 0x99, 0x52, 0xff)) /* hsl( 30, 70%, 60%) */ \
    _P_("Signal 20 - 25 dBHz",   SIGNAL_20_25,                 IM_COL32(0xe0, 0xb1, 0x52, 0xff)) /* hsl( 40, 70%, 60%) */ \
    _P_("Signal 25 - 30 dBHz",   SIGNAL_25_30,                 IM_COL32(0xe0, 0xc9, 0x52, 0xff)) /* hsl( 50, 70%, 60%) */ \
    _P_("Signal 30 - 35 dBHz",   SIGNAL_30_35,                 IM_COL32(0xe0, 0xe0, 0x52, 0xff)) /* hsl( 60, 70%, 60%) */ \
    _P_("Signal 35 - 40 dBHz",   SIGNAL_35_40,                 IM_COL32(0xb3, 0xe0, 0x85, 0xff)) /* hsl( 90, 60%, 70%) */ \
    _P_("Signal 40 - 45 dBHz",   SIGNAL_40_45,                 IM_COL32(0x06, 0xf9, 0x06, 0xff)) /* hsl(120, 95%, 50%) */ \
    _P_("Signal 45 - 50 dBHz",   SIGNAL_45_50,                 IM_COL32(0x13, 0xec, 0x5b, 0xff)) /* hsl(140, 85%, 50%) */ \
    _P_("Signal 50 - 55 dBHz",   SIGNAL_50_55,                 IM_COL32(0x13, 0xec, 0xa4, 0xff)) /* hsl(160, 85%, 50%) */ \
    _P_("Signal 55 - oo dBHz",   SIGNAL_55_OO,                 IM_COL32(0x13, 0xec, 0xec, 0xff)) /* hsl(180, 85%, 50%) */ \
    /* Generic colours (some from https://github.com/leiradel/ImGuiAl/blob/master/term/imguial_term.h) */ \
    _P_("Colour Black",          C_BLACK,                      IM_COL32(0x00, 0x00, 0x00, 0xff)) \
    _P_("Colour Grey",           C_GREY,                       IM_COL32(0x55, 0x55, 0x55, 0xff)) \
    _P_("Colour White",          C_WHITE,                      IM_COL32(0xaa, 0xaa, 0xaa, 0xff)) \
    _P_("Colour BrightWhite",    C_BRIGHTWHITE,                IM_COL32(0xff, 0xff, 0xff, 0xff)) \
    _P_("Colour Blue",           C_BLUE,                       IM_COL32(0x00, 0x00, 0xaa, 0xff)) \
    _P_("Colour BrightBlue",     C_BRIGHTBLUE,                 IM_COL32(0x55, 0x55, 0xff, 0xff)) \
    _P_("Colour Green",          C_GREEN,                      IM_COL32(0x00, 0xaa, 0x00, 0xff)) \
    _P_("Colour BrightGreen",    C_BRIGHTGREEN,                IM_COL32(0x55, 0xff, 0x55, 0xff)) \
    _P_("Colour Cyan",           C_CYAN,                       IM_COL32(0x00, 0xaa, 0xaa, 0xff)) \
    _P_("Colour BrightCyan",     C_BRIGHTCYAN,                 IM_COL32(0x55, 0xff, 0xff, 0xff)) \
    _P_("Colour Red",            C_RED,                        IM_COL32(0xaa, 0x00, 0x00, 0xff)) \
    _P_("Colour BrightRed",      C_BRIGHTRED,                  IM_COL32(0xff, 0x55, 0x55, 0xff)) \
    _P_("Colour Magenta",        C_MAGENTA,                    IM_COL32(0xaa, 0x00, 0xaa, 0xff)) \
    _P_("Colour BrightMagenta",  C_BRIGHTMAGENTA,              IM_COL32(0xff, 0x55, 0xff, 0xff)) \
    _P_("Colour Brown",          C_BROWN,                      IM_COL32(0xaa, 0x55, 0x00, 0xff)) \
    _P_("Colour Yellow",         C_YELLOW,                     IM_COL32(0xff, 0xff, 0x55, 0xff)) \
    _P_("Colour Orange",         C_ORANGE,                     IM_COL32(0xa8, 0x65, 0x00, 0xff)) \
    _P_("Colour BrightOrange",   C_BRIGHTORANGE,               IM_COL32(0xff, 0xa1, 0x14, 0xff)) \
    _P_("Colour None",           C_NONE,                       IM_COL32(0x00, 0x00, 0x00, 0x00)) \
    /* Log window */ \
    _P_("Log UBX messages",      LOG_MSGUBX,                   IM_COL32(0xff, 0x80, 0x80, 0xff)) \
    _P_("Log NMEA messages",     LOG_MSGNMEA,                  IM_COL32(0xab, 0xff, 0x80, 0xff)) \
    _P_("Log RTCM3 messages",    LOG_MSGRTCM3,                 IM_COL32(0xaf, 0x80, 0xff, 0xff)) \
    _P_("Log GARBAGE messages",  LOG_MSGGARBAGE,               IM_COL32(0x80, 0xce, 0xff, 0xff)) \
    _P_("Log Epochs",            LOG_EPOCH,                    IM_COL32(0xff, 0xfe, 0x80, 0xff)) \
    /* Inf window */ \
    _P_("Inf Debug",             INF_DEBUG,                    IM_COL32(0x00, 0xaa, 0xaa, 0xff)) \
    _P_("Inf Notice",            INF_NOTICE,                   IM_COL32(0xff, 0xff, 0xff, 0xff)) \
    _P_("Inf Warning",           INF_WARNING,                  IM_COL32(0xff, 0xff, 0x55, 0xff)) \
    _P_("Inf Error",             INF_ERROR,                    IM_COL32(0xff, 0x55, 0x55, 0xff)) \
    _P_("Inf Test",              INF_TEST,                     IM_COL32(0xff, 0x55, 0xff, 0xff)) \
    _P_("Inf Other",             INF_OTHER,                    IM_COL32(0xaa, 0x55, 0x00, 0xff)) \
    /* Debug logs */ \
    _P_("Debug Trace",           DEBUG_TRACE,                  IM_COL32(0xaa, 0x00, 0xaa, 0xff)) \
    _P_("Debug Debug",           DEBUG_DEBUG,                  IM_COL32(0x00, 0xaa, 0xaa, 0xff)) \
    _P_("Debug Print",           DEBUG_PRINT,                  IM_COL32(0xaa, 0xaa, 0xaa, 0xff)) \
    _P_("Debug Notice",          DEBUG_NOTICE,                 IM_COL32(0xff, 0xff, 0xff, 0xff)) \
    _P_("Debug Warning",         DEBUG_WARNING,                IM_COL32(0xff, 0xff, 0x55, 0xff)) \
    _P_("Debug Error",           DEBUG_ERROR,                  IM_COL32(0xff, 0x55, 0x55, 0xff))

#endif // __GUI_SETTINGS_COLOURS_HPP__
