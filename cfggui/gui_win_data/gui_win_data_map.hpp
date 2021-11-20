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

#ifndef __GUI_WIN_DATA_MAP_HPP__
#define __GUI_WIN_DATA_MAP_HPP__

#include "ff_stuff.h"
#include "maptiles.hpp"
#include "gui_map.hpp"
#include "gui_win_data.hpp"

/* ***** Map ******************************************************************************************************** */

class GuiWinDataMap : public GuiWinData
{
    public:

        GuiWinDataMap(const std::string &name, std::shared_ptr<Database> database);
       ~GuiWinDataMap();

    protected:

        void _Loop(const uint32_t &frame, const double &now) final;
        void _DrawContent() final;

        // Map display parameters
        bool                 _debugLayout;
        bool                 _triggerAutosize;
        bool                 _showInfo;
        float                _mapZoom;

        static constexpr float _kMapZoomMin       =  0.0;
        static constexpr float _kMapZoomMax       = 25.0;
        static constexpr float _kMapZoomStep      =  0.1;
        static constexpr float _kMapZoomStepShift =  0.05;
        static constexpr float _kMapZoomStepCtrl  =  0.5;

        std::unique_ptr<GuiMap> _map;
        std::shared_ptr<MapTiles> _tiles;

        // Dragging map start
        Ff::Vec2<double>     _dragStartTxyMain;

        void                 _SetMap(const MapParams &map, const bool resetView = false);
        void                 _AutoSize();
        void                 _SetCent(const double lat, const double lon);
        void                 _SetZoom(const float zoom, const float snap = _kMapZoomStepShift);
        void                 _SetZoomDelta(const ImVec2 &mousePos, const float delta, const float snap = _kMapZoomStepShift);
        void                 _DragStart();
        void                 _DragEnd(const ImVec2 &totalDrag);

        static constexpr ImU32 _cDebugLayout = IM_COL32(0xff, 0x00, 0x00, 0xaf);
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_MAP_HPP__
