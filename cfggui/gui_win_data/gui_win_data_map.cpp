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

#include <cstdlib>

#include "ff_trafo.h"
#include "gui_win_data_map.hpp"

#include "gui_win_data_inc.hpp"
#include "gui_app.hpp"

/* ****************************************************************************************************************** */

GuiWinDataMap::GuiWinDataMap(const std::string &name,
            std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile, std::shared_ptr<Database> database) :
    _debugLayout{false}, _showInfo{true}, _map{nullptr}, _tiles{nullptr}
{
    _winSize  = { 80, 25 };
    _winFlags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    _receiver = receiver;
    _logfile  = logfile;
    _database = database;
    _winTitle = name;
    _winName  = name;

    ClearData();

    // Load settings
    auto cfg = Ff::StrSplit(_winSettings->GetValue(_winName + ".map"), ",");
    bool haveCfg = false;
    if (cfg.size() == 4)
    {
        std::size_t n1, n2, n3;
        const float zoom = std::stof(cfg[1], &n1);
        const double lat = std::stod(cfg[2], &n2);
        const double lon = std::stod(cfg[3], &n3);
        for (auto &map: _winSettings->maps)
        {
            if (map.name == cfg[0])
            {
                _SetMap(map, true);
                if (n1 > 0)
                {
                    _SetZoom(zoom);
                }
                if ( (n2 > 0) && (n3 > 0) )
                {
                    _SetCent(deg2rad(lat), deg2rad(lon));
                }
                _triggerAutosize = false;
                haveCfg = true;
                break;
            }
        }
    }
    if (!haveCfg && !_winSettings->maps.empty())
    {
        _SetMap(_winSettings->maps[0], true);
    }
}

GuiWinDataMap::~GuiWinDataMap()
{
    double lat, lon;
    _map->GetCentLatLon(lat, lon);
    _winSettings->SetValue(_winName + ".map", Ff::Sprintf("%s,%.2f,%.10f,%.10f",
        _map->GetParams().name.c_str(), _map->GetZoom(), rad2deg(lat), rad2deg(lon)));
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMap::Loop(const uint32_t &frame, const double &now)
{
    (void)frame;
    if (_tiles)
    {
        _tiles->Loop(now);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

//void GuiWinDataMap::ProcessData(const Data &data)
//{
//}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMap::ClearData()
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMap::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    if (!_tiles || !_map)
    {
        ImGui::TextUnformatted("No maps :-(");
        _DrawWindowEnd();
        return;
    }

    //ImGui::BeginChild("##Plot", ImVec2(0.0f, 0.0f), false,
    //    ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Update canvas size
    const ImVec2 canvasMin  = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    const ImVec2 canvasMax  = canvasMin + canvasSize;
    const ImVec2 canvasCent = ImVec2(canvasMin.x + std::floor(canvasSize.x * 0.5), canvasMin.y + std::floor(canvasSize.y * 0.5));

    _map->SetCanvas(canvasMin, canvasMax);
    //_mapMain->SetCanvas(canvasMin, canvasMin  + (canvasSize * ImVec2(0.5,1)));

    const ImVec2 cursorOrigin = ImGui::GetCursorPos();
    //ImVec2 cursorMax = ImGui::GetWindowContentRegionMax();

    ImDrawList *draw = ImGui::GetWindowDrawList();

    // Size to fill window
    if (_triggerAutosize)
    {
        _triggerAutosize = false;
        _AutoSize();
    }

    // Draw map
    _map->DrawMap(draw);

    // Draw points
    constexpr int _LAT_ = Database::_LAT_;
    constexpr int _LON_ = Database::_LON_;
    _database->ProcEpochs([&](const int ix, const Database::Epoch &epoch)
    {
        (void)ix;
        if (epoch.raw.havePos)
        {
            float x, y;
            if (_map->GetScreenXyFromLatLon(epoch.raw.llh[_LAT_], epoch.raw.llh[_LON_], x, y))
            {
                draw->AddRectFilled(ImVec2(x - 2, y - 2), ImVec2(x + 2, y + 2), _winSettings->GetFixColour(&epoch.raw));
            }
        }
        return true;
    });

    // Highlight last point, draw accuracy estimate circle
    const Database::Epoch *lastEpoch = nullptr;
    _database->ProcEpochs([&](const int ix, const Database::Epoch &epoch)
    {
        (void)ix;
        if (epoch.raw.havePos)
        {
            float x, y;
            if (_map->GetScreenXyFromLatLon(epoch.raw.llh[_LAT_], epoch.raw.llh[_LON_], x, y))
            {
                const ImU32 c = epoch.raw.fixOk ? GUI_COLOUR(PLOT_MAP_HL_OK) : GUI_COLOUR(PLOT_MAP_HL_MASKED);
                const double m2px = _map->GetPixelsPerMetre(epoch.raw.llh[_LAT_]);
                const float r = (0.5 * epoch.raw.horizAcc * m2px) + 0.5;
                if (r > 1.0f)
                {
                    draw->AddCircleFilled(ImVec2(x, y), r, GUI_COLOUR(PLOT_MAP_ACC_EST));
                }
                uint32_t age = TIME() - epoch.raw.ts;
                const float w = age < 100 ? 3.0 : 1.0;
                const float l = 20.0; //age < 100 ? 40.0 : 20.0; //20 + ((100 - MIN(age, 100)) / 2);
                draw->AddLine(ImVec2(x - l, y), ImVec2(x - 3, y), c, w);
                draw->AddLine(ImVec2(x + 3, y), ImVec2(x + l, y), c, w);
                draw->AddLine(ImVec2(x, y - l), ImVec2(x, y - 3), c, w);
                draw->AddLine(ImVec2(x, y + 3), ImVec2(x, y + l), c, w);
            }
            lastEpoch = &epoch;
            return false;
        }
        return true;
    }, true);

    // Draw basestation
    if (lastEpoch && lastEpoch->raw.havePos && lastEpoch->raw.haveRelPos)
    {
        {
            // Calculate approximate basestation coordinates given NED *at* base station
            // FIXE: is this correct?
            double llh[3];
            memcpy(llh, lastEpoch->raw.llh, sizeof(llh));

            // Approx. base lat/lon using rover lat/lon and North/East
            constexpr double kWgs84a = 6378137.0;
            const double m2r = 1.0 / kWgs84a; // metres per rad at equator
            const double m2rAtLat = m2r / cos( llh[0] ); // at our latitude
            llh[0] -= lastEpoch->raw.relNed[0] * m2r;
            llh[1] -= lastEpoch->raw.relNed[1] * m2rAtLat;
            llh[2] += lastEpoch->raw.relNed[2];
            // This is quite close already, but it gets better (more stable) if we do the following...

            // Get XYZ vector from NED vector at approx. base
            double xyz[3];
            xyz2ned_vec(lastEpoch->raw.relNed, llh, xyz);

            // Base ECEF
            xyz[0] = lastEpoch->raw.xyz[0] - xyz[0];
            xyz[1] = lastEpoch->raw.xyz[1] - xyz[1];
            xyz[2] = lastEpoch->raw.xyz[2] - xyz[2];
            // Base LLH
            xyz2llh_vec(xyz, llh);

            float xb, yb, xr, yr;
            const bool roverVis = _map->GetScreenXyFromLatLon(lastEpoch->raw.llh[0], lastEpoch->raw.llh[1], xr, yr, false);
            const bool baseVis  = _map->GetScreenXyFromLatLon(llh[0], llh[1], xb, yb, false);
            if (roverVis || baseVis)
            {
                draw->AddLine(ImVec2(xb, yb), ImVec2(xr, yr), GUI_COLOUR(PLOT_MAP_BASELINE), 2.0f);
            }
            if (baseVis)
            {
                draw->AddCircleFilled(ImVec2(xb, yb), 3.0f, GUI_COLOUR(PLOT_MAP_BASELINE));
            }
        }
    }
    // Controls
    bool controlsHovered = false;
    {
        ImGui::SetCursorPos(cursorOrigin + _winSettings->style.ItemSpacing);

        // Left click
        if (ImGui::Button(ICON_FK_MAP "##MapParam", _winSettings->iconButtonSize))
        {
            ImGui::OpenPopup("MapParams");
        }
        // Right click
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Checkbox("Debug map layout", &_debugLayout))
            {
                _map->SetDebug(_debugLayout);
            }
            ImGui::EndPopup();
        }

        // Left click: show full info/control
        if (ImGui::BeginPopup("MapParams"))
        {
            // Zoom slider
            float mapZoom = _mapZoom;
            ImGui::PushItemWidth(30 * _winSettings->charSize.x);
            if (ImGui::SliderFloat("##MapZoom", &mapZoom, _kMapZoomMin, _kMapZoomMax, "%.2f"))
            {
                _SetZoom(mapZoom);
            }
            ImGui::PopItemWidth();
            Gui::ItemTooltip("Map zoom level");

            // Map layer popup
            const auto map = _map->GetParams();
            ImGui::PushItemWidth(30 * _winSettings->charSize.x);
            if (ImGui::BeginCombo("###MapLayer", map.title.c_str()))
            {
                for (const auto &m: _winSettings->maps)
                {
                    if (ImGui::Selectable(m.title.c_str(), m.name == map.name))
                    {
                        if (m.name != map.name)
                        {
                            _SetMap(m);
                        }
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();

            // Map controls
            _map->DrawCtrl(_debugLayout);

            ImGui::SameLine();
            ToggleButton(ICON_FK_INFO "##ShowInfo", NULL, &_showInfo, "Showing info box", "Not showing info box");

            ImGui::EndPopup();
        }
        // Show minimal info/control
        else
        {
            Gui::ItemTooltip("Map config");
            if (ImGui::IsItemHovered())
            {
                controlsHovered = true;
            }

            ImGui::SameLine();

            ImGui::PushItemWidth((_winSettings->charSize.x * 6));
            float mapZoom = _mapZoom;
            if (ImGui::InputScalar("##MapZoom", ImGuiDataType_Float, &mapZoom, NULL, NULL, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue))
            {
                _SetZoom(mapZoom);
            }
            ImGui::PopItemWidth();
            Gui::ItemTooltip("Map zoom level");
            if (ImGui::IsItemHovered())
            {
                controlsHovered = true;
            }
        }

        const int numTilesInQueue = _tiles->NumTilesInQueue();
        if (numTilesInQueue > 0)
        {
            ImGui::SameLine();
            const float progress = (float)numTilesInQueue / (float)_tiles->kMaxtilesInQueue;
            char str[100];
            std::snprintf(str, sizeof(str), "Loading %d tiles", numTilesInQueue);
            ImGui::ProgressBar(progress, ImVec2(_winSettings->charSize.x * 20, 0.0f), str);
        }
    }

    // Attribution
    {
        const ImVec2 attr0 { canvasMin.x + _winSettings->style.ItemSpacing.x,
                             canvasMax.y - _winSettings->charSize.y - (3 * _winSettings->style.ItemSpacing.y) };
        ImGui::SetCursorScreenPos(attr0);
        auto p = _map->GetParams();
        if (ImGui::Button(p.attribution.c_str()))
        {
            std::string cmd = "xdg-open " + p.link;
            if (std::system(cmd.c_str()) != EXIT_SUCCESS)
            {
                WARNING("Command failed: %s", cmd.c_str());
            }
        }
        if (ImGui::IsItemHovered())
        {
            controlsHovered = true;
        }
    }

    // Place an invisible button on top of everything to capture mouse events (and disable windows moving)
    // Note the real buttons above must are placed first so that they will get the mouse events first (before this invisible button)
    ImGui::SetCursorPos(cursorOrigin);
    ImGui::InvisibleButton("canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft);
    const bool isHovered = !controlsHovered && ImGui::IsItemHovered();
    const bool isActive = ImGui::IsItemActive();
    auto &io = ImGui::GetIO();

    // Info box
    if (_showInfo)
    {
        const float lineSpacing = ImGui::GetTextLineHeightWithSpacing();
        const ImVec2 infoSize { _winSettings->charSize.x * 23, lineSpacing * 3 };
        const ImVec2 infoRectTopRight = ImVec2(canvasMax.x - _winSettings->style.ItemSpacing.x, canvasMin.y + _winSettings->style.ItemSpacing.y);
        const ImVec2 infoRect0 = infoRectTopRight - ImVec2(infoSize.x, 0);
        const ImVec2 infoRect1 = infoRectTopRight + ImVec2(0, infoSize.y);
        draw->AddRectFilled(infoRect0, infoRect1, ImGui::GetColorU32(ImGuiCol_FrameBg), _winSettings->style.FrameRounding);
        ImVec2 cursor = infoRect0 + _winSettings->style.FramePadding;

        // Lat/lon
        double lat;
        double lon;
        if (isHovered)
        {
            _map->GetLatLonAtMouse(io.MousePos, lat, lon);
        }
        else
        {
            _map->GetCentLatLon(lat, lon);
        }

        ImGui::SetCursorScreenPos(cursor); cursor.y += lineSpacing;

        if ( (lat < MapTiles::kMaxLat) && (lat > MapTiles::kMinLat) )
        {
            int d, m;
            double s;
            deg2dms(rad2deg(lat), &d, &m, &s);
            ImGui::Text(" %2d° %2d' %9.6f\" %c", ABS(d), m, s, d < 0 ? 'S' : 'N');
        }
        else
        {
            ImGui::TextUnformatted("Lat: n/a");
        }
        ImGui::SetCursorScreenPos(cursor); cursor.y += lineSpacing;
        if ( (lon < MapTiles::kMaxLon) && (lon > MapTiles::kMinLon) )
        {
            int d, m;
            double s;
            deg2dms(rad2deg(lon), &d, &m, &s);
            ImGui::Text("%3d° %2d' %9.6f\" %c", ABS(d), m, s, d < 0 ? 'W' : 'E');
        }
        else
        {
            ImGui::TextUnformatted("Lon: n/a");
        }

        // Scale bar
        ImGui::SetCursorScreenPos(cursor); cursor.y += lineSpacing;

        const double m2px = _map->GetPixelsPerMetre(lat);
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
            const float len = val * m2px;
            const ImVec2 offs = cursor + ImVec2(_winSettings->charSize.x * 7, (-0.5f * lineSpacing) - 2.0f);
            draw->AddLine(offs, offs + ImVec2(len, 0), GUI_COLOUR(C_BLACK), 4.0f);
            //DEBUG("lenM=%f log=%f val=%f len=%f", scaleLenM, n, val, len);
        }
    }

    // Crosshairs
    if (isHovered)
    {
        draw->AddLine(ImVec2(io.MousePos.x, canvasMin.y), ImVec2(io.MousePos.x, canvasMax.y), GUI_COLOUR(PLOT_MAP_CROSSHAIRS), 3.0);
        draw->AddLine(ImVec2(canvasMin.x, io.MousePos.y), ImVec2(canvasMax.x, io.MousePos.y), GUI_COLOUR(PLOT_MAP_CROSSHAIRS), 3.0);
    }

    // Map centre
    // {
    //     constexpr float len = 20;
    //     const float w = isHovered ? 1.0 : 3.0;
    //     draw->AddLine(ImVec2(canvasCent.x - len, canvasCent.y), ImVec2(canvasCent.x + len, canvasCent.y), GUI_COLOUR(PLOT_MAP_CROSSHAIRS), w);
    //     draw->AddLine(ImVec2(canvasCent.x, canvasCent.y - len), ImVec2(canvasCent.x, canvasCent.y + len), GUI_COLOUR(PLOT_MAP_CROSSHAIRS), w);
    // }

    // Dragging
    if (isHovered && isActive)
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            _DragStart();
        }
        else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            _DragEnd(ImGui::GetMouseDragDelta(ImGuiMouseButton_Left));
        }
    }

    // Zoom control using mouse wheel (but not while dragging)
    if (isHovered && !isActive)
    {
        if (io.MouseWheel != 0.0)
        {

            float step = _kMapZoomStep;
            if (io.KeyShift)
            {
                step = _kMapZoomStepShift;
            }
            else if (io.KeyCtrl)
            {
                step = _kMapZoomStepCtrl;
            }
            _SetZoomDelta(io.MousePos, io.MouseWheel * step, step);
        }
    }

    if (_debugLayout)
    {
        draw->AddRect(canvasMin, canvasMax, _cDebugLayout);
        draw->AddLine(canvasCent + ImVec2(0, -10), canvasCent + ImVec2(0, +10), _cDebugLayout);
        draw->AddLine(canvasCent + ImVec2(-10, 0), canvasCent + ImVec2(+10, 0), _cDebugLayout);
        draw->AddText(canvasMin + ImVec2(1,1), _cDebugLayout, Ff::Sprintf("%.1f/%.1f", canvasMin.x, canvasMin.y).c_str());
        draw->AddText(canvasMin + canvasSize + ImVec2(-80,-15), _cDebugLayout, Ff::Sprintf("%.1f/%.1f", canvasSize.x, canvasSize.y).c_str());
        draw->AddText(canvasCent + ImVec2(1,-15), _cDebugLayout, Ff::Sprintf("%.1f/%.1f", canvasCent.x, canvasCent.y).c_str());
        //draw->AddText(canvasCent + ImVec2(1,0), _cDebugLayout, Ff::Sprintf("%.1fx%.1f (%.1f)", _tileSize.x, _tileSize.y, _tileScale).c_str());
        //draw->AddText(canvasCent + ImVec2(1,15), _cDebugLayout, Ff::Sprintf("%d %.1f", _zoomLevel, _mapZoom).c_str());
        ImGui::Begin((_winName + "Tiles").c_str());
        ImGui::TextUnformatted(_tiles->GetDebugText().c_str());
        ImGui::End();
    }

    //ImGui::EndChild(); // ##Plot
    _DrawWindowEnd();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMap::_SetMap(const MapParams &map, const bool resetView)
{
    double lat, lon;
    if (!resetView)
    {
        _map->GetCentLatLon(lat, lon);
    }

    _tiles   = std::make_shared<MapTiles>(map);
    _map    = std::make_unique<GuiMap>(map, _tiles);

    if (resetView)
    {
        _SetCent(0.0, 0.0);
        _SetZoom(1.0);
        _triggerAutosize = true;
    }
    else
    {
        _SetCent(lat, lon);
        _SetZoom(_mapZoom);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMap::_AutoSize()
{
    const float zoom = _map->GetAutoSizeZoom();
    _SetZoom(zoom);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMap::_SetCent(const double lat, const double lon)
{
    _map->SetCentLatLon(lat, lon);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMap::_SetZoom(const float zoom, const float snap)
{
    _mapZoom = zoom;
    if (_mapZoom < _kMapZoomMin)
    {
        _mapZoom = _kMapZoomMin;
    }
    else if (_mapZoom > _kMapZoomMax)
    {
        _mapZoom = _kMapZoomMax;
    }

    if (snap > 0.0f)
    {
        _mapZoom = std::floor( (_mapZoom * (1.0f / snap)) + (snap / 2.0f)) * snap;
    }

    _map->SetZoom(_mapZoom);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMap::_SetZoomDelta(const ImVec2 &mousePos, const float delta, const float snap)
{
    double lat0, lon0, lat1, lon1, latC, lonC;
    _map->GetLatLonAtMouse(mousePos, lat0, lon0);
    const float mapZoom = _mapZoom + delta;
    _SetZoom(mapZoom, snap);
    _map->GetLatLonAtMouse(mousePos, lat1, lon1);
    _map->GetCentLatLon(latC, lonC);
    _SetCent(latC + (lat0 - lat1), lonC + (lon0 - lon1));
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMap::_DragStart()
{
    _dragStartTxyMain = _map->GetCentTxy();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMap::_DragEnd(const ImVec2 &totalDrag)
{
    if ( (totalDrag.x != 0.0) || (totalDrag.y != 0.0) )
    {
        const Ff::Vec2<double> deltaTxyMain = _map->GetDeltaTxyFromMouseDelta(totalDrag);
        _map->SetCentTxy(_dragStartTxyMain - deltaTxyMain);
    }
}

/* ****************************************************************************************************************** */
