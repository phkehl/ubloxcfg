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

#ifndef __GUI_WIDGET_FILTER_HPP__
#define __GUI_WIDGET_FILTER_HPP__

#include <string>
#include <regex>
#include <memory>

/* ****************************************************************************************************************** */

class GuiWidgetFilter
{
    public:
        GuiWidgetFilter(const std::string help = "");

        bool        DrawWidget(const float width = -1.0f);
        bool        Match(const std::string &str);
        bool        IsActive();
        bool        IsHighlight();
        void        SetHightlight(const bool highlight = true);
        void        SetUserMsg(const std::string &msg);
        std::string GetFilterStr();
        void        SetFilterStr(const std::string &str);

        std::string GetFilterSettings();
        void        SetFilterSettings(const std::string &settings);

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
        bool                 _filterChanged;
        std::unique_ptr<std::regex> _filterRegex;
        bool                 _filterMatch;
        std::string          _filterHelp;
        void                 _UpdateFilter();
};

/* ****************************************************************************************************************** */

#endif // __GUI_WIDGET_FILTER_HPP__
