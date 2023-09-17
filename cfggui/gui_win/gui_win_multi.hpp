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

#ifndef __GUI_WIN_MULTI_HPP__
#define __GUI_WIN_MULTI_HPP__

#include <string>
#include <deque>
#include <memory>
#include <vector>
#include <unordered_map>
#include <cstdint>

#include "gui_win.hpp"
#include "gui_data.hpp"
#include "gui_widget_map.hpp"

/* ***** multiview window ******************************************************************************************* */

class GuiWinMulti : public GuiWin
{
    public:
        GuiWinMulti(const std::string &name);
       ~GuiWinMulti();

        void Loop(const uint32_t &frame, const double &now) final;
        void DrawWindow() final;

    private:

        uint32_t _guiDataSerial;

        GuiData::ReceiverList _allReceivers;
        GuiData::LogfileList  _allLogfiles;
        GuiData::ReceiverList _usedReceivers;
        GuiData::LogfileList  _usedLogfiles;

        GuiWidgetMap _map;

        struct Point
        {
            Point(const double _lat, const double _lon, const ImU32 _colour) :
                lat { _lat }, lon { _lon }, colour { _colour }
            {}
            double lat;
            double lon;
            ImU32  colour;
        };

        std::unordered_map<uint64_t, std::vector<Point>> _points;
        std::vector<uint64_t> _pointKeys;
        double _latestTow;

        void _Clear(const bool refresh = false);
        void _DrawControls();
        void _DrawMap();
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_MULTI_HPP__
