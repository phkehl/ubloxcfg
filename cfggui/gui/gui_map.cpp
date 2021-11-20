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

#include "gui_inc.hpp"

#include "gui_map.hpp"

/* ****************************************************************************************************************** */

GuiMap::GuiMap(MapParams params, std::shared_ptr<MapTiles> tiles) :
    _params{params}, _tiles{tiles},
    _tintColour{IM_COL32(0xff, 0xff, 0xff, 0xff)}, _tintColour4{1.0f, 1.0f, 1.0f, 1.0f},
    //_tintColour{IM_COL32(0x66, 0x66, 0x66, 0xff)}, _tintColour4{1.0f, 1.0f, 1.0f, 1.0f},
    _debug{false}
{
    SetZoom(1.0);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMap::SetZoom(const float zoom, const int level)
{
    _zoom = zoom;
    if (_zoom < 0.0f)
    {
        _zoom = 0.0f;
    }
    else if (_zoom > 100.0f)
    {
        _zoom = 100.0f;
    }

    // Use corresponding integer zoom level (tile z) or custom
    _zLevel = level < 0 ? (int)std::floor(_zoom + 0.01) : level;

    // Don't draw map below minimal available tile zoom level (to not download/draw LOTS of tiles)
    _drawMap = _zLevel >= _params.zoomMin;

    // Clip zoom level to min/max available for the map
    if (_zLevel < _params.zoomMin)
    {
        _zLevel = _params.zoomMin;
    }
    else if (_zLevel > _params.zoomMax)
    {
        _zLevel = _params.zoomMax;
    }

    // Number of tiles in x and y (so there are _numTiles*_numTiles tiles in total)
    _numTiles  = 1 << _zLevel;

    // Mpa centre in tile coordinate
    _centTxy.x = MapTiles::LonToTx(_centLon, _zLevel);
    _centTxy.y = MapTiles::LatToTy(_centLat, _zLevel);

    // Scale tiles for zoom level
    const float deltaScale = _zoom - (float)_zLevel; // can be > 1, e.g. when zoom > max. level the map has
    if (std::fabs(deltaScale) > (1.0 - FLT_EPSILON))
    {
        _tileScale = std::pow(2.0, deltaScale);
    }
    else
    {
        _tileScale = 1.0 + deltaScale;
    }
    _tileSize = FfVec2(_params.tileSizeX, _params.tileSizeY) * _tileScale;

    //DEBUG("_zoom=%.1f _zLevel=%d deltaScale=%.1f _tileScale=%.1f _tileSize=%.1fx%.1f",
    //    _zoom, _zLevel, deltaScale, _tileScale, _tileSize.x, _tileSize.y);
}

// ---------------------------------------------------------------------------------------------------------------------

float GuiMap::GetZoom()
{
    return _zoom;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMap::GetCentLatLon(double &lat, double &lon)
{
    lat = _centLat;
    lon = _centLon;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMap::SetCentLatLon(const double lat, const double lon)
{
    _centLat = lat;
    _centLon = lon;
    if (_centLat < MapParams::MIN_LAT)
    {
        _centLat = MapParams::MIN_LAT;
    }
    else if (_centLat > MapParams::MAX_LAT)
    {
        _centLat = MapParams::MAX_LAT;
    }
    if (_centLon < MapParams::MIN_LON)
    {
        _centLon = MapParams::MIN_LON;
    }
    else if (_centLon > MapParams::MAX_LON)
    {
        _centLon = MapParams::MAX_LON;
    }

    _centTxy.x = MapTiles::LonToTx(_centLon, _zLevel);
    _centTxy.y = MapTiles::LatToTy(_centLat, _zLevel);
    //DEBUG("SetCentLatLon() %.6f %.6f, %.1f %.1f (%d)", rad2deg(_centLat), rad2deg(_centLon), _centTxy.x, _centTxy.y, _zLevel);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMap::SetCentTxy(const Ff::Vec2<double> &txy)
{
    _centTxy = txy;
    _centLat = MapTiles::TyToLat(_centTxy.y, _zLevel);
    _centLon = MapTiles::TxToLon(_centTxy.x, _zLevel);
}

// ---------------------------------------------------------------------------------------------------------------------

Ff::Vec2<double> GuiMap::GetCentTxy()
{
    return _centTxy;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMap::GetLatLonAtMouse(const FfVec2 &mouse, double &lat, double &lon)
{
    const FfVec2 deltaT = (mouse - _canvasCent + FfVec2(0.5, 0.5)) / _tileSize;
    lat = MapTiles::TyToLat(_centTxy.y + deltaT.y, _zLevel);
    lon = MapTiles::TxToLon(_centTxy.x + deltaT.x, _zLevel);
}

// ---------------------------------------------------------------------------------------------------------------------

Ff::Vec2<double> GuiMap::GetDeltaTxyFromMouseDelta(const FfVec2 &mouseDelta)
{
    return Ff::Vec2<double> { mouseDelta.x / _tileSize.x, mouseDelta.y / _tileSize.y };
}

// ---------------------------------------------------------------------------------------------------------------------

float GuiMap::GetAutoSizeZoom()
{
    const int numTilesX = (_canvasSize.x / _params.tileSizeX) + 1;
    const int numTilesY = (_canvasSize.y / _params.tileSizeY) + 1;
    const int numTiles = MAX(numTilesX, numTilesY);
    const float zoom = std::ceil(std::sqrt((float)numTiles));
    DEBUG("GetAutoSizeZoom() %.1fx%.1f %dx%d -> %dx%d -> %d -> %.1f",
        _canvasSize.x, _canvasSize.y, _params.tileSizeX, _params.tileSizeY, numTilesX, numTilesY, numTiles, zoom);
    return zoom;
}

// ---------------------------------------------------------------------------------------------------------------------

const MapParams &GuiMap::GetParams()
{
    return _params;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMap::SetDebug(const bool debug)
{
    _debug = debug;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMap::SetTintColour(const ImVec4 &colour)
{
    _tintColour4 = colour;
    _tintColour = ImGui::ColorConvertFloat4ToU32(_tintColour4);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMap::DrawCtrl(const bool debug)
{
    ImGui::PushID(this);

    // Tint colour
    {
        if (ImGui::ColorEdit4("##ColourEdit", (float *)&_tintColour4, ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_NoInputs))
        {
            _tintColour = ImGui::ColorConvertFloat4ToU32(_tintColour4);
        }
        Gui::ItemTooltip("Tint colour");
    }

    if (debug)
    {
        ImGui::SameLine();

        // Debug layout
        ImGui::Checkbox("##Debug", &_debug);
        Gui::ItemTooltip("Debug tiles");

        if (_debug)
        {
            ImGui::SameLine();

            // Tile zoom level
            int zLevel = _zLevel;
            ImGui::PushItemWidth(150);
            if (ImGui::SliderInt("##Zoom", &zLevel, 0, 25))
            {
                SetZoom(_zoom, zLevel);
            }
            ImGui::PopItemWidth();
            Gui::ItemTooltip("Tile zoom level");
        }
    }

    ImGui::PopID();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMap::SetCanvas(const FfVec2 &min, const FfVec2 &max)
{
    _canvasMin = min;
    _canvasMax = max;
    _canvasCent = (_canvasMin + _canvasMax) * 0.5;
    _canvasCent.x = std::floor(_canvasCent.x + 0.5f);
    _canvasCent.y = std::floor(_canvasCent.y + 0.5f);
    _canvasSize = _canvasMax - _canvasMin;

    const Ff::Vec2<double> minTxy = _centTxy - GetDeltaTxyFromMouseDelta(_canvasCent - _canvasMin);
    const Ff::Vec2<double> maxTxy = _centTxy + GetDeltaTxyFromMouseDelta(_canvasMax - _canvasCent);
    _canvasLatMin = MapTiles::TyToLat(maxTxy.y, _zLevel);
    _canvasLatMax = MapTiles::TyToLat(minTxy.y, _zLevel);
    _canvasLonMin = MapTiles::TxToLon(minTxy.x, _zLevel);
    _canvasLonMax = MapTiles::TxToLon(maxTxy.x, _zLevel);
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMap::GetScreenXyFromLatLon(const double lat, const double lon, float &x, float &y, const bool clip)
{
    const bool visible = !( (lat < _canvasLatMin) || (lat > _canvasLatMax) || (lon < _canvasLonMin) || (lon > _canvasLonMax) );
    if (clip && !visible)
    {
        return false;
    }
    const double ty = MapTiles::LatToTy(lat, _zLevel);
    const double tx = MapTiles::LonToTx(lon, _zLevel);
    x = std::floor( _canvasCent.x + ((tx - _centTxy.x) * _tileSize.x) + 0.5f );
    y = std::floor( _canvasCent.y + ((ty - _centTxy.y) * _tileSize.y) + 0.5f );
    return visible;
}

// ---------------------------------------------------------------------------------------------------------------------

double GuiMap::GetPixelsPerMetre(const double lat)
{
    const double cosLatPowZoom = std::cos(lat) * std::pow(2.0, -_zLevel);
    const double sTile = MapTiles::WGS84_C * cosLatPowZoom;        // Tile width in [m]
    return _tileSize.x / sTile;                       // 1 [m] in [px]
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMap::DrawMap(ImDrawList *draw)
{
    if (!_drawMap)
    {
        if ((_tintColour & IM_COL32_A_MASK) != 0)
        {
            char msg[200];
            std::snprintf(msg, sizeof(msg), "Zoom level %.2f outside of\nrange for map (%d..%d)",
                _zoom, _params.zoomMin, _params.zoomMax);
            const FfVec2 sz = ImGui::CalcTextSize(msg);
            ImGui::SetCursorScreenPos( _canvasMin + (_canvasSize / 2) - (sz / 2) );
            ImGui::TextUnformatted(msg);
        }
        return;
    }

    // Calculate centre tile screen coordinates
    const Ff::Vec2<double> centTileXyInt { (float)std::floor(_centTxy.x), (float)std::floor(_centTxy.y) };
    const Ff::Vec2<double> centTileXyFrac = _centTxy - centTileXyInt;
    _centTx = (int)centTileXyInt.x;
    _centTy = (int)centTileXyInt.y;
    _centTile0 = _canvasCent - ImVec2( std::round(centTileXyFrac.x * _tileSize.x), std::round(centTileXyFrac.y * _tileSize.y) );

    draw->PushClipRect(_canvasMin, _canvasMax);

    // Draw centre tile
    _DrawMapTile(draw, 0, 0, 0);

    // Draw tiles around centre tile, in a spiral...
    //
    //                                     dist  ix0        length
    //                              81     5     9*9 9=5+4  11 =5+6
    //   73 74 75 76 77 78 79 80 49        4     7*7 7=4+3   9 =4+5
    //   72 43 44 45 46 47 48 25 50   -3   3     5*5 5=3+2   7 =3+4
    //   71 42 21 22 23 24  9 26 51   -2   2     3*3 3=2+1   5 =2+3
    //   70 41 20  7  8  1 10 27 52   -1   1     1*1 1=1+0   3 =1+2
    //   69 40 19  6 [0] 2 11 28 53
    //   68 39 18  5  4  3 12 29 54              ix0 = (d+(d-1))*(d+(d-1)) = (2d-1)*(2d-1) = 4dd-4d+1 = 4(dd-d)+1
    //   67 38 17 16 15 14 13 30 55
    //   66 37 36 35 34 33 32 31 56
    //   65 64 63 62 61 60 59 58 57
    //

    const int maxDx = ( (_canvasSize.x / _tileSize.x) / 2) + 1.5;
    const int maxDy = ( (_canvasSize.y / _tileSize.y) / 2) + 1.5;
    const int maxDist = MAX(maxDx, maxDy);
    for (int dist = 1, ix = 1; dist <= maxDist; dist++)           //  1  2  3  4  5 ...
    {
        //int ix = 4 * ((dist * dist) - dist) + 1;   //  1  9 25 49 81 ...
        // top right -> bottom right
        for (int dx = dist, dy = -dist; dy <= dist; dy++, ix++)
        {
            _DrawMapTile(draw, ix, dx, dy);
        }
        // bottom right -> bottom left
        for (int dx = dist - 1, dy = dist; dx >= -dist; dx--, ix++)
        {
            _DrawMapTile(draw, ix, dx, dy);
        }
        // bottom left -> top left
        for (int dx = -dist, dy = dist - 1; dy >= -dist; dy--, ix++)
        {
            _DrawMapTile(draw, ix, dx, dy);
        }
        // top left -> top right
        for (int dx = 1 - dist, dy = -dist; dx <= (dist - 1); dx++, ix++)
        {
            _DrawMapTile(draw, ix, dx, dy);
        }
    }

    draw->PopClipRect();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMap::_DrawMapTile(ImDrawList *draw, const int ix, const int dx, const int dy)
{
    const int tx = _centTx + dx;
    const int ty = _centTy + dy;
    if ( (tx >= 0) && (tx < _numTiles) && (ty >= 0) && (ty < _numTiles) )
    {
        const FfVec2 tile0 = _centTile0 + (FfVec2(dx, dy) * _tileSize);
        const FfVec2 tile1 = tile0 + _tileSize;
        if ( (tile0.x < _canvasMax.x) && (tile0.y < _canvasMax.y) && (tile1.x > _canvasMin.x) && (tile1.y > _canvasMin.y) )
        {
            const bool skipDraw = (_tintColour & IM_COL32_A_MASK) == 0;
            if (skipDraw)
            {
                _tiles->GetTileTex(tx, ty, _zLevel);
            }
            else
            {
                draw->AddImage(_tiles->GetTileTex(tx, ty, _zLevel), tile0, tile1, FfVec2(0,0), FfVec2(1.0,1.0), _tintColour);
                if (_debug)
                {
#if 0
                    draw->AddRect(tile0, tile1, _cDebugTile);
#else
                    draw->AddLine(ImVec2(tile0.x, tile0.y), ImVec2(tile0.x + 25, tile0.y), _cDebugTile);
                    draw->AddLine(ImVec2(tile0.x, tile0.y), ImVec2(tile0.x, tile0.y + 25), _cDebugTile);

                    draw->AddLine(ImVec2(tile1.x, tile0.y), ImVec2(tile1.x - 25, tile0.y), _cDebugTile);
                    draw->AddLine(ImVec2(tile1.x, tile0.y), ImVec2(tile1.x, tile0.y + 25), _cDebugTile);

                    draw->AddLine(ImVec2(tile0.x, tile1.y), ImVec2(tile0.x + 25, tile1.y), _cDebugTile);
                    draw->AddLine(ImVec2(tile0.x, tile1.y), ImVec2(tile0.x, tile1.y - 25), _cDebugTile);

                    draw->AddLine(ImVec2(tile1.x, tile1.y), ImVec2(tile1.x - 25, tile1.y), _cDebugTile);
                    draw->AddLine(ImVec2(tile1.x, tile1.y), ImVec2(tile1.x, tile1.y - 25), _cDebugTile);
#endif
                    draw->AddText(tile0 + (_tileSize / 2) - ImVec2(40,30), _cDebugTile, Ff::Sprintf("#%d %d/%d %.2f %d\n%.1fx%.1f\n%.1f/%.1f\n%.1f/%.1f",
                        ix, tx, ty, _zoom, _zLevel, _tileSize.x, _tileSize.y, tile0.x, tile0.y, tile1.x, tile1.y).c_str());
                }
            }
        }
    }
}

/* ****************************************************************************************************************** */
