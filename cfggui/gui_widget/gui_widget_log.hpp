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

#ifndef __GUI_WIDGET_LOG_HPP__
#define __GUI_WIDGET_LOG_HPP__

#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <regex>

#include "gui_widget_filter.hpp"

/* ****************************************************************************************************************** */

class GuiWidgetLog
{
    public:
        GuiWidgetLog(const int maxLines = 10000);

        void        Clear();
        void        AddLine(const char *line, const ImU32 colour);
        void        AddLine(const std::string &line, const ImU32 colour);

        // Draw entire widget
        void        DrawWidget();

        // Draw controls and log separately
        void        DrawControls();
        void        DrawLog();

        std::string GetFilterStr();
        void        SetFilterStr(const std::string &str);

        std::string GetSettings();
        void        SetSettings(const std::string &settings);

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
        std::deque<LogLine>  _lines;
        int                  _maxLines;

        bool                 _logTimestamps;
        bool                 _logAutoscroll;
        bool                 _scrollToBottom;

        bool                 _logCopy;

        GuiWidgetFilter      _filterWidget;
        int                  _filterNumMatch;

        void                 _AddLogLine(LogLine logLine);
        void                 _HandleFilter();
};

/* ****************************************************************************************************************** */

#endif // __GUI_WIDGET_LOG_HPP__