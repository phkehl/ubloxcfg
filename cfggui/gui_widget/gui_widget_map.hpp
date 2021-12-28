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

#ifndef __GUI_WIDGET_MAP_HPP__
#define __GUI_WIDGET_MAP_HPP__

#include <memory>

#include "ff_utils.hpp"
#include "mapparams.hpp"
#include "maptiles.hpp"

/* ****************************************************************************************************************** */

class GuiWidgetMap
{
    public:
        GuiWidgetMap();
       ~GuiWidgetMap();

        void SetMap(const MapParams &mapParams, const bool resetView = false);
        void SetZoom(const float zoom);
        void SetPos(const double lat, const double lon); // [rad]

        std::string GetSettings();
        void        SetSettings(const std::string &settings);

        // 1. Draw base map
        bool BeginDraw();

        // 2. Draw custom stuff
        float PixelPerMetre(const float lat); // [rad]
        FfVec2 LonLatToScreen(const double lat, const double lon); // [rad]

        // 3. Draw controls, handle dragging, zooming, etc.
        void EndDraw();

    private:

        // Currently used map parameters and map tiles
        MapParams _mapParams;
        std::unique_ptr<MapTiles> _mapTiles;

        // Settings and state
        FfVec2  _centPosLonLat;    // Position (lon/lat) at canvas centre
        FfVec2  _centPosXy;        // Position (tile x/y) at canvas centre
        FfVec2  _centTileOffs;     // Centre tile (top left) offset from _canvasCent
        FfVec2  _tileSize;         // Tile size (_mapParams.tileSize[XY] with scaling applied)
        float   _mapZoom;          // Zoom level
        int     _zLevel;           // Map tiles zoom level
        int     _numTiles;         // Number of tiles in x and y (at _zLevel)
        bool    _autoSize;         // Auto-size map on next draw
        bool    _hideConfig;       // Hide map config (base layer, zoom, etc.)
        bool    _showInfo;         // Show map info (mouse position, scale bar)
        ImU32   _tintColour;       // Tint colour
        ImVec4  _tintColour4;      // Tint colour (for colour widget)
        bool    _debugLayout;      // Debug map layout
        bool    _debugTiles;       // Debug tiles
        FfVec2  _dragStartXy;      // Start of dragging map (tile coordinates)
        int     _isDragging;       // Dragging in progress
        void _SetPosAndZoom(const FfVec2 &lonLat, const float zoom, const int zLevel = -1, const float snap = ZOOM_STEP_SHIFT);
        void _DrawMapTile(ImDrawList *draw, const int ix, const int dx, const int dy);

        // Map canvas in screen coordinates
        FfVec2 _canvasMin;
        FfVec2 _canvasSize;
        FfVec2 _canvasMax;
        FfVec2 _canvasCent;
        static constexpr float CANVAS_MIN_WIDTH  = 100.0f;
        static constexpr float CANVAS_MIN_HEIGHT =  50.0f;

        static constexpr float ZOOM_MIN        =  0.0;
        static constexpr float ZOOM_MAX        = 25.0;
        static constexpr float ZOOM_STEP       =  0.1;
        static constexpr float ZOOM_STEP_SHIFT =  0.05;
        static constexpr float ZOOM_STEP_CTRL  =  0.5;
};

/* ****************************************************************************************************************** */

#endif // __GUI_WIDGET_MAP_HPP__