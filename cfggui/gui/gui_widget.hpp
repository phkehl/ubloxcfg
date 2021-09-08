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

#ifndef __GUI_WIDGETS_H__
#define __GUI_WIDGETS_H__

#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <regex>

#include "imgui.h"

/* ****************************************************************************************************************** */

namespace Gui
{
    // Some colours (based on https://github.com/leiradel/ImGuiAl/blob/master/term/imguial_term.h)
    enum : ImU32
    {
        Black           = IM_COL32(0x00, 0x00, 0x00, 0xff),
        Blue            = IM_COL32(0x00, 0x00, 0xaa, 0xff),
        Green           = IM_COL32(0x00, 0xaa, 0x00, 0xff),
        Cyan            = IM_COL32(0x00, 0xaa, 0xaa, 0xff),
        Red             = IM_COL32(0xaa, 0x00, 0x00, 0xff),
        Magenta         = IM_COL32(0xaa, 0x00, 0xaa, 0xff),
        Brown           = IM_COL32(0xaa, 0x55, 0x00, 0xff),
        White           = IM_COL32(0xaa, 0xaa, 0xaa, 0xff),
        Gray            = IM_COL32(0x55, 0x55, 0x55, 0xff),
        BrightBlue      = IM_COL32(0x55, 0x55, 0xff, 0xff),
        BrightGreen     = IM_COL32(0x55, 0xff, 0x55, 0xff),
        BrightCyan      = IM_COL32(0x55, 0xff, 0xff, 0xff),
        BrightRed       = IM_COL32(0xff, 0x55, 0x55, 0xff),
        BrightMagenta   = IM_COL32(0xff, 0x55, 0xff, 0xff),
        Yellow          = IM_COL32(0xff, 0xff, 0x55, 0xff),
        BrightWhite     = IM_COL32(0xff, 0xff, 0xff, 0xff),
        Orange          = IM_COL32(0xa8, 0x65, 0x00, 0xff),
        BrightOrange    = IM_COL32(0xff, 0xa1, 0x14, 0xff),

        // Log window
        MsgUbx          = IM_COL32(0xff, 0x80, 0x80, 0xff),
        MsgNmea         = IM_COL32(0xab, 0xff, 0x80, 0xff),
        MsgRtcm3        = IM_COL32(0xaf, 0x80, 0xff, 0xff),
        MsgGarbage      = IM_COL32(0x80, 0xce, 0xff, 0xff),
        Epoch           = IM_COL32(0xff, 0xfe, 0x80, 0xff),

        // Inf window
        InfDebug        = IM_COL32(0x00, 0xaa, 0xaa, 0xff),
        InfNotice       = IM_COL32(0xff, 0xff, 0xff, 0xff),
        InfWarning      = IM_COL32(0xff, 0xff, 0x55, 0xff),
        InfError        = IM_COL32(0xff, 0x55, 0x55, 0xff),
        InfTest         = IM_COL32(0xff, 0x55, 0xff, 0xff),
        InfOther        = IM_COL32(0xaa, 0x55, 0x00, 0xff),

        None            = IM_COL32(0x00, 0x00, 0x00, 0x00),
    };

    void BeginDisabled();
    void EndDisabled();
    ImU32 ColourHSV(const float h, const float s, const float v, const float a = 1.0f);
    void SetWindowScale(const float scale = 1.0f);
    void VerticalSeparator(const float offset_from_start_x = 0.0f);
    bool ItemTooltip(const char *text, const double delay = 0.3);
    bool ItemTooltipBegin(const double delay = 0.3);
    void ItemTooltipEnd();
    void DebugTooltipThingy(const char *fmt, ...) IM_FMTARGS(1);

    enum Tri_e { TRI_IGNORE = -1, TRI_DISABLED = 0, TRI_ENABLED = 1 };
    bool CheckBoxTristate(const char *label, Tri_e *state, const bool cycleAllThree = false);
    bool CheckboxFlagsX8(const char *label, uint64_t *flags, uint64_t flags_value);

    void BeginMenuPersist();
    void EndMenuPersist();

    void TextLink(const char *url, const char *text = nullptr);
};

/* ****************************************************************************************************************** */

struct GuiWidgetFilter
{
    GuiWidgetFilter(const std::string help = "");

    bool DrawWidget(const float width = -1.0f);
    bool Match(const std::string &str);
    bool IsActive();
    bool IsHighlight();
    void SetUserMsg(const std::string &msg);

    protected:
        enum FILTER_e { FILTER_NONE, FILTER_BAD, FILTER_OK };
        enum FILTER_e        _filterState;
        std::string          _filterStr;
        double               _filterUpdated;
        std::string          _filterErrorMsg;
        std::string          _filterUserMsg;
        bool                 _filterCaseSensitive;
        bool                 _filterHighlight;
        bool                 _filterInvert;
        std::unique_ptr<std::regex> _filterRegex;
        bool                 _filterMatch;
        std::string          _filterHelp;
        void                 _UpdateFilter();
};

/* ****************************************************************************************************************** */

struct GuiWidgetLog
{
    GuiWidgetLog(const int maxLines = 10000, const bool controls = true);

    void                     Clear();
    void                     AddLine(const char *line, const ImU32 colour);
    void                     AddLine(const std::string &line, const ImU32 colour);

    void                     DrawWidget();

    protected:
        struct LogLine
        {
            LogLine(const char *_str, const ImU32 _colour);
            LogLine(const std::string &_str, const ImU32 _colour);
            std::string str;
            int         len;
            double      time;
            ImU32       colour;
            bool        match;
        };
        std::deque<LogLine>  _lines; // FIXME: should this be unique_ptr?!
        int                  _maxLines;

        bool                 _drawControls;

        bool                 _logTimestamps;
        bool                 _logAutoscroll;

        bool                 _logCopy;

        GuiWidgetFilter      _filterWidget;
        int                  _filterNumMatch;

        void                 _AddLogLine(LogLine logLine);
        void                 _DrawLogControls();
        void                 _DrawLogLines();
        void                 _HandleFilter();
};

/* ****************************************************************************************************************** */

struct GuiWidgetTable
{
    GuiWidgetTable();
    void AddColumn(const char *title, const float width);
    void BeginDraw();
    void ColText(const char *text);
    void ColTextF(const char *fmt, ...);
    bool ColSelectable(const char *label, bool selected = false, ImGuiSelectableFlags flags = ImGuiSelectableFlags_SpanAllColumns);
    void ColSkip();

    void EndDraw();

    protected:
        struct Column
        {
            Column(const char *_title, const float _width);
            char  title[100];
            float width;
        };

        std::vector<Column> _columns;
        float               _totalWidth;
};

/* ****************************************************************************************************************** */

#endif // __GUI_WIDGETS_H__