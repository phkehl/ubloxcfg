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

#ifndef __GUI_MAP_H__
#define __GUI_MAP_H__

#include <cinttypes>
#include <cmath>

#include "ff_stuff.h"
#include "ff_cpp.hpp"
#include "maptiles.hpp"

#include "imgui.h"

/* ***** Map widget ************************************************************************************************* */

struct GuiMap
{
    GuiMap(MapParams params, std::shared_ptr<MapTiles> tiles);
    void                 SetZoom(const float zoom, const int level = -1);
    float                GetZoom();
    void                 SetCentLatLon(const double lat, const double lon);
    void                 GetCentLatLon(double &lat, double &lon);
    void                 SetCentTxy(const Ff::Vec2<double> &txy);
    Ff::Vec2<double>     GetCentTxy();
    void                 GetLatLonAtMouse(const ImVec2 &mouse, double &lat, double &lon);
    Ff::Vec2<double>     GetDeltaTxyFromMouseDelta(const ImVec2 &mouseDelta);
    void                 SetCanvas(const ImVec2 &min, const ImVec2 &max);
    void                 DrawMap(ImDrawList *draw);
    void                 DrawCtrl(const bool debug = false);
    float                GetAutoSizeZoom();
    const MapParams     &GetParams();
    void                 SetDebug(const bool debug);
    void                 SetTintColour(const ImVec4 &colour);
    bool                 GetScreenXyFromLatLon(const double lat, const double lon, float &x, float &y, const bool clip = true);
    double               GetPixelsPerMetre(const double lat);

    protected:
        void                 _DrawMapTile(ImDrawList *draw, const int ix, const int dx, const int dy);

        MapParams            _params;
        std::shared_ptr<MapTiles> _tiles;

        float                _zoom;         // Map zoom level
        double               _centLat;
        double               _centLon;
        ImU32                _tintColour;
        ImVec4               _tintColour4;

        bool                 _debug;

        int                  _zLevel;       // Map tiles zoom level
        Ff::Vec2<double>     _centTxy;      // _centLat/_centLon in tile coordinate
        int                  _numTiles;     // Number of tiles in x and y direction at current _zoomLevel
        float                _tileScale;    // Scale of tile
        ImVec2               _tileSize;     // Rendered tile size [px] (with _tileScale applied)
        bool                 _drawMap;

        int                  _centTx;       // Tile x coordinate [int] for centre tile
        int                  _centTy;       // Tile y coordinate [int] for centre tile
        ImVec2               _centTile0;    // Canvas coordinate for centre tile

        ImVec2               _canvasMin;
        ImVec2               _canvasMax;
        ImVec2               _canvasCent;
        ImVec2               _canvasSize;
        double              _canvasLatMin;
        double              _canvasLatMax;
        double              _canvasLonMin;
        double              _canvasLonMax;

        static constexpr ImU32 _cDebugTile   = IM_COL32(0xff, 0x00, 0x00, 0xaf);
};

/* ****************************************************************************************************************** */
#endif // __GUI_MAP_H__
