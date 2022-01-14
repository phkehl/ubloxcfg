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
#include "implot.h"

#include "ff_trafo.h"

#include "gui_widget_map.hpp"

/* ****************************************************************************************************************** */

GuiWidgetMap::GuiWidgetMap() :
    //_mapParams   { MapParams::NO_MAP },
    _hideConfig  { true },
    _showInfo    { true },
    _tintColour  { IM_COL32(0xff, 0xff, 0xff, 0xff) },
    _tintColour4 { 1.0f, 1.0f, 1.0f, 1.0f },
    _debugLayout { false },
    _debugTiles  { false },
    _isDragging  { ImGuiMouseButton_COUNT }
{
    bool haveMap = false;
    for (auto &map: GuiSettings::maps)
    {
        if (map.enabled)
        {
            SetMap(map, true);
            haveMap = true;
            break;
        }
    }
    if (!haveMap)
    {
        SetMap(MapParams::NO_MAP, true);
    }
}

GuiWidgetMap::~GuiWidgetMap()
{
}

// ---------------------------------------------------------------------------------------------------------------------

std::string GuiWidgetMap::GetSettings()
{
    return Ff::Sprintf("%s:%.8f:%.8f:%.2f:0x%08x",
        _mapParams.name.c_str(), rad2deg(_centPosLonLat.x), rad2deg(_centPosLonLat.y), _mapZoom, _tintColour);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetMap::SetSettings(const std::string &settings)
{
    bool haveMap = false;
    const auto set = Ff::StrSplit(settings, ":", 5);
    if (set.size() == 5)
    {
        const std::string name = set[0];
        const float lon  = deg2rad(std::atof(set[1].c_str()));
        const float lat  = deg2rad(std::atof(set[2].c_str()));
        const float zoom = std::atof(set[3].c_str());
        uint32_t tint = std::atoi(set[4].c_str());
        try { tint = std::stoul(set[4], nullptr, 16); } catch (...) { tint = 0xffffff; }
        for (auto &map: GuiSettings::maps)
        {
            if (map.name == name)
            {
                if (map.enabled)
                {
                    SetMap(map, true);
                    _SetPosAndZoom({ lon, lat }, zoom);
                    haveMap = true;
                    _tintColour = tint;
                    _tintColour4 = ImGui::ColorConvertU32ToFloat4(tint);
                }
                break;
            }
        }
    }
    if (!haveMap)
    {
        for (auto &map: GuiSettings::maps)
        {
            if (map.enabled)
            {
                SetMap(map, true);
                haveMap = true;
                break;
            }
        }
    }
    if (!haveMap)
    {
        SetMap(MapParams::NO_MAP, true);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetMap::SetMap(const MapParams &mapParams, const bool resetView)
{
    _mapParams = mapParams;
    _mapTiles = std::make_unique<MapTiles>(mapParams);

    if (resetView)
    {
        _SetPosAndZoom({ 0.0, 0.0 }, 1.0);
    }
    else
    {
        _SetPosAndZoom(_centPosLonLat, _mapZoom, _zLevel);
    }
    _autoSize = resetView;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetMap::SetPos(const double lat, const double lon)
{
    _SetPosAndZoom({ lon, lat }, _mapZoom, _zLevel);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetMap::SetZoom(const float zoom)
{
    _SetPosAndZoom(_centPosLonLat, zoom);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetMap::_SetPosAndZoom(const FfVec2d &pos, const float zoom, const int zLevel, const float snap)
{
    // Clamp and snap zoom level
    _mapZoom = std::clamp(zoom, ZOOM_MIN, ZOOM_MAX);
    if (snap > 0.0f)
    {
        _mapZoom = std::floor( (_mapZoom * (1.0f / snap)) + (snap / 2.0f)) * snap;
    }

    // The corresponding integer zoom level (tile z)
    const int zLevelZoom = (int)std::floor(_mapZoom + 0.01);

    // Choose closest map tile z level
    if (zLevel < 0)
    {
        _zLevel = zLevelZoom;
    }
    // Custom map tile z level
    else
    {
        // Don't allow zooming more than two levels closer (to not download/draw LOTS of tiles)
        const int minZ = zLevelZoom - 10;
        const int maxZ = zLevelZoom + 2;
        _zLevel = CLIP(zLevel, minZ, maxZ);
    }

    // Clip to range supported by the map
    _zLevel = CLIP(_zLevel, _mapParams.zoomMin, _mapParams.zoomMax);

    _numTiles  = 1 << _zLevel;

    // Update center position
    _centPosLonLat.x = std::clamp(pos.x, _mapParams.minLon, _mapParams.maxLon);
    _centPosLonLat.y = std::clamp(pos.y, _mapParams.minLat, _mapParams.maxLat);
    _centPosXy = MapTiles::LonLatToTileXy(_centPosLonLat, _zLevel);

    // Scale tiles for zoom level
    const float deltaScale = _mapZoom - (float)_zLevel; // can be > 1, e.g. when zoom > max. level the map has
    const float tileScale = (std::fabs(deltaScale) > (1.0 - FLT_EPSILON) ? std::pow(2.0, deltaScale) : 1.0 + deltaScale);
    _tileSize = FfVec2d(_mapParams.tileSizeX, _mapParams.tileSizeY) * tileScale;

    // Centre tile top-left corner offset from canvas centre
    const float fracX = _centPosXy.x - std::floor(_centPosXy.x);
    const float fracY = _centPosXy.y - std::floor(_centPosXy.y);
    _centTileOffs = ImVec2( -std::round(fracX * _tileSize.x), -std::round(fracY * _tileSize.y) );

    _autoSize = false;
}

// ---------------------------------------------------------------------------------------------------------------------

float GuiWidgetMap::PixelPerMetre(const float lat)
{
    const float cosLatPowZoom = std::cos(lat) * std::pow(2.0f, -_zLevel);
    const float sTile = MapTiles::WGS84_C * cosLatPowZoom;  // Tile width in [m]
    return _tileSize.x / sTile;                             // 1 [m] in [px]
}

// ---------------------------------------------------------------------------------------------------------------------

FfVec2f GuiWidgetMap::LonLatToScreen(const double lat, const double lon)
{
    // Note: double precision calculations here!
    const Ff::Vec2<double> tXy = MapTiles::LonLatToTileXy(Ff::Vec2<double>(lon, lat), _zLevel);
    return _canvasCent + FfVec2f(
        (tXy.x - (double)_centPosXy.x) * (double)_tileSize.x,
        (tXy.y - (double)_centPosXy.y) * (double)_tileSize.y );

    // FIXME: check visibility? Return 0,0 if outside of canvas?
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetMap::BeginDraw()
{
    _canvasMin  = ImGui::GetCursorScreenPos();
    _canvasSize = ImGui::GetContentRegionAvail();
    if ( ((int)_canvasSize.x) & 0x1 ) { _canvasSize.x -= 1.0f; } // Make canvas size even number..
    if ( ((int)_canvasSize.y) & 0x1 ) { _canvasSize.y -= 1.0f; } // ..so that the centre is even, too
    _canvasMax  = _canvasMin + _canvasSize;
    _canvasCent = (_canvasMin + _canvasMax) * 0.5;
    if ( (_canvasSize.x < CANVAS_MIN_WIDTH) || (_canvasSize.y < CANVAS_MIN_HEIGHT) )
    {
        return false;
    }

    if (_autoSize)
    {
        const int numTilesX = (_canvasSize.x / _mapParams.tileSizeX) + 1;
        const int numTilesY = (_canvasSize.y / _mapParams.tileSizeY) + 1;
        const int numTiles = MAX(numTilesX, numTilesY);
        const float zoom = std::ceil(std::sqrt((float)numTiles));
        SetZoom(zoom);
    }

    ImDrawList *draw = ImGui::GetWindowDrawList();
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

    //draw->PopClipRect(); // nope, later..

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetMap::_DrawMapTile(ImDrawList *draw, const int ix, const int dx, const int dy)
{
    // TODO: wrap around? Hmmm... would only work for global maps
    const int tx = (int)_centPosXy.x + dx;
    const int ty = (int)_centPosXy.y + dy;
    if ( (tx >= 0) && (tx < _numTiles) && (ty >= 0) && (ty < _numTiles) )
    {
        const FfVec2f tile0 = _canvasCent + _centTileOffs + (FfVec2f(dx, dy) * _tileSize);
        const FfVec2f tile1 = tile0 + _tileSize;
        if ( (tile0.x < _canvasMax.x) && (tile0.y < _canvasMax.y) && (tile1.x > _canvasMin.x) && (tile1.y > _canvasMin.y) )
        {
            const bool skipDraw = (_tintColour & IM_COL32_A_MASK) == 0;
            if (skipDraw)
            {
                if (_mapTiles)
                {
                    _mapTiles->GetTileTex(tx, ty, _zLevel);
                }
            }
            else
            {
                if (_mapTiles)
                {
                    draw->AddImage(_mapTiles->GetTileTex(tx, ty, _zLevel), tile0, tile1, FfVec2f(0,0), FfVec2f(1.0,1.0), _tintColour);
                }
                if (_debugTiles)
                {
#if 0
                    draw->AddRect(tile0, tile1, GUI_COLOUR(MAP_DEBUG));
#else
                    draw->AddLine(ImVec2(tile0.x, tile0.y), ImVec2(tile0.x + 25, tile0.y), GUI_COLOUR(MAP_DEBUG));
                    draw->AddLine(ImVec2(tile0.x, tile0.y), ImVec2(tile0.x, tile0.y + 25), GUI_COLOUR(MAP_DEBUG));

                    draw->AddLine(ImVec2(tile1.x, tile0.y), ImVec2(tile1.x - 25, tile0.y), GUI_COLOUR(MAP_DEBUG));
                    draw->AddLine(ImVec2(tile1.x, tile0.y), ImVec2(tile1.x, tile0.y + 25), GUI_COLOUR(MAP_DEBUG));

                    draw->AddLine(ImVec2(tile0.x, tile1.y), ImVec2(tile0.x + 25, tile1.y), GUI_COLOUR(MAP_DEBUG));
                    draw->AddLine(ImVec2(tile0.x, tile1.y), ImVec2(tile0.x, tile1.y - 25), GUI_COLOUR(MAP_DEBUG));

                    draw->AddLine(ImVec2(tile1.x, tile1.y), ImVec2(tile1.x - 25, tile1.y), GUI_COLOUR(MAP_DEBUG));
                    draw->AddLine(ImVec2(tile1.x, tile1.y), ImVec2(tile1.x, tile1.y - 25), GUI_COLOUR(MAP_DEBUG));
#endif
                    draw->AddText(tile0 + (_tileSize / 2) - ImVec2(40,30), GUI_COLOUR(MAP_DEBUG), Ff::Sprintf("#%d %d/%d %.2f %d\n%.1fx%.1f\n%.1f/%.1f\n%.1f/%.1f",
                        ix, tx, ty, _mapZoom, _zLevel, _tileSize.x, _tileSize.y, tile0.x, tile0.y, tile1.x, tile1.y).c_str());
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

#define _CHECK_HOVER() \
    if (ImGui::IsItemHovered()) \
    { \
        controlsHovered = true; \
    }

void GuiWidgetMap::EndDraw()
{
    ImGui::SetCursorScreenPos(_canvasMin + GuiSettings::style->ItemSpacing);

    bool controlsHovered = false;

    // Map control
    {
        Gui::ToggleButton(ICON_FK_MAP "##ShowConfig", nullptr, &_hideConfig, "Map config", nullptr, GuiSettings::iconSize);
        _CHECK_HOVER();

        ImGui::SameLine();

        // Zoom level input
        {
            ImGui::PushItemWidth((GuiSettings::charSize.x * 6));
            float mapZoom = _mapZoom;
            if (ImGui::InputScalar("##MapZoom", ImGuiDataType_Float, &mapZoom, NULL, NULL, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue))
            {
                SetZoom(mapZoom);
            }
            ImGui::PopItemWidth();
            Gui::ItemTooltip("Map zoom level");
            _CHECK_HOVER();
        }

        // Tile download progress bar
        const int numTilesInQueue = _mapTiles ? _mapTiles->NumTilesInQueue() : 0;
        if (numTilesInQueue > 0)
        {
            ImGui::SameLine();

            const float progress = (float)numTilesInQueue / (float)_mapTiles->MAX_TILES_IN_QUEUE;
            char str[100];
            std::snprintf(str, sizeof(str), "Loading %d tiles", numTilesInQueue);
            ImGui::ProgressBar(progress, ImVec2(GuiSettings::charSize.x * 20, 0.0f), str);
            _CHECK_HOVER();
        }

        // Display full map config
        if (!_hideConfig)
        {
            ImGui::NewLine();
            ImGui::SameLine();
            //if (ImGui::BeginChildFrame(1234, GuiSettings::charSize * FfVec2f(30, 5.5), ImGuiWindowFlags_NoScrollbar))
            if (ImGui::BeginChild(1234, GuiSettings::charSize * FfVec2f(30, 5.5), false, ImGuiWindowFlags_NoScrollbar))
            {
                // Zoom slider
                {
                    float mapZoom = _mapZoom;
                    ImGui::PushItemWidth(-1);
                    if (ImGui::SliderFloat("##MapZoom", &mapZoom, ZOOM_MIN, ZOOM_MAX, "%.2f"))
                    {
                        SetZoom(mapZoom);
                    }
                    ImGui::PopItemWidth();
                    Gui::ItemTooltip("Map zoom level");
                }

                // Base map select
                {
                    ImGui::PushItemWidth(-1);
                    if (ImGui::BeginCombo("###BaseMap", _mapParams.title.c_str()))
                    {
                        for (const auto &map: GuiSettings::maps)
                        {
                            if (map.enabled)
                            {
                                const bool selected = (map.name == _mapParams.name);
                                if (ImGui::Selectable(map.title.c_str(), selected))
                                {
                                    if (!selected)
                                    {
                                        SetMap(map);
                                    }
                                }
                                if (selected)
                                {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::PopItemWidth();
                }

                // Show info box
                {
                    Gui::ToggleButton(ICON_FK_INFO "##ShowInfo", NULL, &_showInfo, "Showing info box", "Not showing info box", GuiSettings::iconSize);
                }

                ImGui::SameLine();

                // Tint colour
                {
                    if (ImGui::ColorEdit4("##ColourEdit", (float *)&_tintColour4, ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_NoInputs))
                    {
                        _tintColour = ImGui::ColorConvertFloat4ToU32(_tintColour4);
                    }
                    Gui::ItemTooltip("Tint colour");
                }

                ImGui::SameLine();

                // Debug layout
                {
                    ImGui::Checkbox("##DebugLayout", &_debugLayout);
                    Gui::ItemTooltip("Debug map layout");
                }

                if (_debugLayout)
                {
                    ImGui::SameLine();

                    // Debug tiles
                    ImGui::Checkbox("##Debug", &_debugTiles);
                    Gui::ItemTooltip("Debug tiles");

                    ImGui::SameLine();

                    // Tile zoom level
                    ImGui::BeginDisabled(!_debugTiles);
                    int zLevel = _zLevel;
                    ImGui::PushItemWidth(-1);
                    if (ImGui::SliderInt("##TileZ", &zLevel, ZOOM_MIN, ZOOM_MAX))
                    {
                        _SetPosAndZoom(_centPosLonLat, _mapZoom, zLevel);
                    }
                    ImGui::PopItemWidth();
                    Gui::ItemTooltip("Tile zoom level");
                    ImGui::EndDisabled();
                }
            }
            //ImGui::EndChildFrame();
            ImGui::EndChild();
            _CHECK_HOVER();
        }
    }

    // Map links
    {
        const ImVec2 attr0 { _canvasMin.x + GuiSettings::style->ItemSpacing.x,
                             _canvasMax.y - GuiSettings::charSize.y - (3 * GuiSettings::style->ItemSpacing.y) };
        ImGui::SetCursorScreenPos(attr0);
        const std::string *link = nullptr;
        for (const auto &info: _mapParams.mapLinks)
        {
            if (ImGui::Button(info.label.c_str()))
            {
                link = &info.url;
            }
            if (ImGui::IsItemHovered())
            {
                controlsHovered = true;
            }
            ImGui::SameLine();
        }
        if (link && !link->empty())
        {
            std::string cmd = "xdg-open " + *link;
            if (std::system(cmd.c_str()) != EXIT_SUCCESS)
            {
                WARNING("Command failed: %s", cmd.c_str());
            }
        }
    }

    ImDrawList *draw = ImGui::GetWindowDrawList();
    auto &io = ImGui::GetIO();

    // Info box (part 1)
    FfVec2f infoBoxCursor;
    if (_showInfo)
    {
        const FfVec2f infoSize { GuiSettings::charSize.x * 23, (GuiSettings::charSize.y * 3) + (2 * GuiSettings::style->ItemSpacing.y) };
        const FfVec2f infoRectTopRight = FfVec2f(_canvasMax.x - GuiSettings::style->ItemSpacing.x, _canvasMin.y + GuiSettings::style->ItemSpacing.y);
        const FfVec2f infoRect0 = infoRectTopRight - FfVec2f(infoSize.x, 0);
        const FfVec2f infoRect1 = infoRectTopRight + FfVec2f(0, infoSize.y);
        infoBoxCursor = infoRect0 + GuiSettings::style->FramePadding;
        draw->AddRectFilled(infoRect0, infoRect1, ImGui::GetColorU32(ImGuiCol_FrameBg), GuiSettings::style->FrameRounding);
        ImGui::SetCursorScreenPos(infoRect0);
        ImGui::InvisibleButton("infobox", infoRect1 - infoRect0, ImGuiButtonFlags_MouseButtonLeft);
        _CHECK_HOVER();
    }

    // Place an invisible button on top of everything to capture mouse events (and disable windows moving)
    // Note the real buttons above must are placed first so that they will get the mouse events first (before this invisible button)
    ImGui::SetCursorScreenPos(_canvasMin);
    ImGui::InvisibleButton("canvas", _canvasSize, ImGuiButtonFlags_MouseButtonLeft);
    const bool isHovered = !controlsHovered && ImGui::IsItemHovered();
    const bool isActive = isHovered && (io.MouseDown[ImGuiMouseButton_Left] || io.MouseDown[ImGuiMouseButton_Right]);

    // Info box (part 2)
    if (_showInfo)
    {
        FfVec2f pos = _centPosLonLat; // use center pos
        if (isHovered && !isActive)  // unless hovering but not dragging
        {
            const FfVec2f delta = (FfVec2f(io.MousePos) - _canvasCent) / _tileSize;
            pos = MapTiles::TileXyToLonLat(_centPosXy + delta, _zLevel);
        }

        ImGui::SetCursorScreenPos(infoBoxCursor);
        infoBoxCursor.y += GuiSettings::charSize.y;

        if ( (pos.y < MapParams::MAX_LAT_RAD) && (pos.y > MapParams::MIN_LAT_RAD) )
        {
            int d, m;
            double s;
            deg2dms(rad2deg(pos.y), &d, &m, &s);
            ImGui::Text(" %2d° %2d' %9.6f\" %c", ABS(d), m, s, d < 0 ? 'S' : 'N');
        }
        else
        {
            ImGui::TextUnformatted("Lat: n/a");
        }
        ImGui::SetCursorScreenPos(infoBoxCursor);
        infoBoxCursor.y += GuiSettings::charSize.y;
        if ( (pos.x < MapParams::MAX_LON_RAD) && (pos.x > MapParams::MIN_LON_RAD) )
        {
            int d, m;
            double s;
            deg2dms(rad2deg(pos.x), &d, &m, &s);
            ImGui::Text("%3d° %2d' %9.6f\" %c", ABS(d), m, s, d < 0 ? 'W' : 'E');
        }
        else
        {
            ImGui::TextUnformatted("Lon: n/a");
        }

        // Scale bar
        ImGui::SetCursorScreenPos(infoBoxCursor);
        infoBoxCursor.y += GuiSettings::charSize.y;

        const double m2px = PixelPerMetre(pos.y);
        if (m2px > 0.0)
        {
            const float px2m = 1.0 / m2px;
            const float scaleLenPx = 100.0f;
            const float scaleLenM = scaleLenPx * px2m;

            const float n = std::floor( std::log10(scaleLenM) );
            const float val = std::pow(10, n);
            if (val > 1000.0f)
            {
                ImGui::Text("%.0f km", val * 1e-3f);
            }
            else if (val < 1.0f)
            {
                ImGui::Text("%.2f m", val);
            }
            else
            {
                ImGui::Text("%.0f m", val);
            }
            const float len = std::round(val * m2px);
            const FfVec2f offs = infoBoxCursor + FfVec2f(GuiSettings::charSize.x * 8, std::floor(-0.5f * GuiSettings::charSize.y) - 2.0f);
            draw->AddRectFilled(offs, offs + FfVec2f(len, 6), GUI_COLOUR(C_BLACK));
            draw->AddRect(offs, offs + FfVec2f(len, 6), GUI_COLOUR(C_WHITE));
            //DEBUG("lenM=%f log=%f val=%f len=%f", scaleLenM, n, val, len);
        }
    }

    // Crosshairs (but not while dragging)
    if (isHovered && !isActive)
    {
        draw->AddLine(ImVec2(io.MousePos.x, _canvasMin.y), ImVec2(io.MousePos.x, _canvasMax.y), GUI_COLOUR(MAP_CROSSHAIRS), 3.0);
        draw->AddLine(ImVec2(_canvasMin.x, io.MousePos.y), ImVec2(_canvasMax.x, io.MousePos.y), GUI_COLOUR(MAP_CROSSHAIRS), 3.0);
    }

    // Detect drag start
    if (isHovered && (_isDragging == ImGuiMouseButton_COUNT))
    {
        if (io.MouseClicked[ImGuiMouseButton_Left])
        {
            _isDragging = ImGuiMouseButton_Left;
            _dragStartXy = _centPosXy;
        }
        else if (io.MouseClicked[ImGuiMouseButton_Right])
        {
            _isDragging = ImGuiMouseButton_Right;
            //_dragStartXy = _centPosXy;
        }
    }
    // Dragging left: move map
    else if (_isDragging == ImGuiMouseButton_Left)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        const FfVec2d delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
        if ( (delta.x != 0.0) || (delta.y != 0.0) )
        {
            const FfVec2d pos = MapTiles::TileXyToLonLat(_dragStartXy - (delta / _tileSize), _zLevel);
            _SetPosAndZoom(pos, _mapZoom, _zLevel);
        }
        if (io.MouseReleased[ImGuiMouseButton_Left])
        {
            _isDragging = ImGuiMouseButton_COUNT;
        }
    }
    else if (_isDragging == ImGuiMouseButton_Right)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        const FfVec2f delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);

        if ( (delta.x != 0.0) && (delta.y != 0.0) )
        {
            draw->AddRectFilled(io.MousePos, FfVec2f(io.MousePos) - delta, GUI_COLOUR(MAP_ZOOM_RECT));
            draw->AddRect(io.MousePos, FfVec2f(io.MousePos) - delta, GUI_COLOUR(MAP_CROSSHAIRS));
        }

        if (io.MouseReleased[ImGuiMouseButton_Right])
        {
            _isDragging = ImGuiMouseButton_COUNT;
            if ( (delta.x != 0.0) && (delta.y != 0.0) )
            {
                const FfVec2f deltaMin = (FfVec2f(io.MousePos) - delta - _canvasCent) / _tileSize;
                const FfVec2f llMin = MapTiles::TileXyToLonLat(_centPosXy + deltaMin, _zLevel);
                const FfVec2f deltaMax = (FfVec2f(io.MousePos) - _canvasCent) / _tileSize;
                const FfVec2f llMax = MapTiles::TileXyToLonLat(_centPosXy + deltaMax, _zLevel);
                const FfVec2f llCent = (llMax + llMin) * 0.5;
                const float deltaLon = std::abs(llMax.x - llMin.x);
                const float deltaLat = std::abs(llMax.y - llMin.y);
                const float zoom = std::log( 2 * M_PI / MIN(deltaLon, deltaLat) ) / std::log(2.0f);
                _SetPosAndZoom(llCent, zoom + 0.5f);
            }
        }
    }

    // Zoom control using mouse wheel (but not while dragging)
    if (isHovered && !isActive)
    {
        if (io.MouseWheel != 0.0)
        {
            float step = ZOOM_STEP;
            if (io.KeyShift)
            {
                step = ZOOM_STEP_SHIFT;
            }
            else if (io.KeyCtrl)
            {
                step = ZOOM_STEP_CTRL;
            }

            // Position at mouse before zoom
            const FfVec2f delta0 = (FfVec2f(io.MousePos) - _canvasCent) / _tileSize;
            const FfVec2f pos0 = MapTiles::TileXyToLonLat(_centPosXy + delta0, _zLevel);

            // Zoom
            const float deltaZoom = io.MouseWheel * step;
            _SetPosAndZoom(_centPosLonLat, _mapZoom + deltaZoom, -1, step);

            // Position at mouse after zoom
            const FfVec2f delta1 = (FfVec2f(io.MousePos) - _canvasCent) / _tileSize;
            const FfVec2f pos1 = MapTiles::TileXyToLonLat(_centPosXy + delta1, _zLevel);

            // Update map position so that the original position is at the mouse position
            _SetPosAndZoom(_centPosLonLat + (pos0 - pos1), _mapZoom, _zLevel, step);
        }
    }

    // Map layout debugging
    if (_debugLayout)
    {
        // Entire canvas
        draw->AddRect(_canvasMin, _canvasMax, GUI_COLOUR(MAP_DEBUG));

        // Centre
        draw->AddLine(_canvasCent + FfVec2f(0, -10), _canvasCent + FfVec2f(0, +10), GUI_COLOUR(MAP_DEBUG));
        draw->AddLine(_canvasCent + FfVec2f(-10, 0), _canvasCent + FfVec2f(+10, 0), GUI_COLOUR(MAP_DEBUG));
        draw->AddText(_canvasCent + FfVec2f(2,-GuiSettings::charSize.y), GUI_COLOUR(MAP_DEBUG), Ff::Sprintf("%.1f/%.1f", _canvasCent.x, _canvasCent.y).c_str());
        draw->AddText(_canvasCent + FfVec2f(2,0), GUI_COLOUR(MAP_DEBUG), Ff::Sprintf("%.6f/%.6f", rad2deg(_centPosLonLat.x), rad2deg(_centPosLonLat.y)).c_str());
        draw->AddText(_canvasCent + FfVec2f(2, GuiSettings::charSize.y), GUI_COLOUR(MAP_DEBUG), Ff::Sprintf("%.1f/%.1f", _centPosXy.x, _centPosXy.y).c_str());

        // Top left
        draw->AddText(_canvasMin + FfVec2f(1,1), GUI_COLOUR(MAP_DEBUG), Ff::Sprintf("%.1f/%.1f", _canvasMin.x, _canvasMin.y).c_str());

        // Bottom right
        draw->AddText(_canvasMax + FfVec2f(-12 * GuiSettings::charSize.x, -GuiSettings::charSize.y - 1), GUI_COLOUR(MAP_DEBUG), Ff::Sprintf("%.1f/%.1f", _canvasSize.x, _canvasSize.y).c_str());

        //draw->AddText(canvasCent + FfVec2f(1,0), GUI_COLOURS(MAP_DEBUG), Ff::Sprintf("%.1fx%.1f (%.1f)", _tileSize.x, _tileSize.y, _tileScale).c_str());
        //draw->AddText(canvasCent + ImVec2(1,15), GUI_COLOURS(MAP_DEBUG), Ff::Sprintf("%d %.1f", _zoomLevel, _mapZoom).c_str());

        // Bottom right
        if (_mapTiles)
        {
            const auto stats = _mapTiles->GetStats();
            draw->AddText({ _canvasMin.x + 1, _canvasMax.y - GuiSettings::charSize.y - 1 }, GUI_COLOUR(MAP_DEBUG),
                Ff::Sprintf("#tiles %d (%d load, %d avail, %d fail, %d outside)",
                    stats.numTiles, stats.numLoad, stats.numAvail, stats.numFail, stats.numOutside).c_str());
        }
    }


    draw->PopClipRect();
}

/* ****************************************************************************************************************** */
// eof