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

#include "gui_inc.hpp"

#include "gui_win_data_map.hpp"

/* ****************************************************************************************************************** */

GuiWinDataMap::GuiWinDataMap(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database)
{
    _winSize = { 80, 40 };
    _winFlags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    _latestEpochEna = false;
    _toolbarEna = false;

    ClearData();

    _map.SetSettings( GuiSettings::GetValue(_winName + ".map") );
}

GuiWinDataMap::~GuiWinDataMap()
{
    GuiSettings::SetValue( _winName + ".map", _map.GetSettings() );
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMap::_DrawContent()
{
    if (!_map.BeginDraw())
    {
        return;
    }

    ImDrawList *draw = ImGui::GetWindowDrawList();

    // Draw points
    constexpr int _LAT_ = Database::_LAT_;
    constexpr int _LON_ = Database::_LON_;
    _database->ProcEpochs([&](const int ix, const Database::Epoch &epoch)
    {
        (void)ix;
        if (epoch.raw.havePos)
        {
            const FfVec2f xy = _map.LonLatToScreen(epoch.raw.llh[_LAT_], epoch.raw.llh[_LON_]);
            draw->AddRectFilled(xy - FfVec2f(2,2), xy + FfVec2f(2,2), GuiSettings::GetFixColour(&epoch.raw));
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
            const FfVec2f xy = _map.LonLatToScreen(epoch.raw.llh[_LAT_], epoch.raw.llh[_LON_]);

            const ImU32 c = epoch.raw.fixOk ? GUI_COLOUR(PLOT_MAP_HL_OK) : GUI_COLOUR(PLOT_MAP_HL_MASKED);
            const double m2px = _map.PixelPerMetre(epoch.raw.llh[_LAT_]);
            const float r = (0.5 * epoch.raw.horizAcc * m2px) + 0.5;
            if (r > 1.0f)
            {
                draw->AddCircleFilled(xy, r, GUI_COLOUR(PLOT_MAP_ACC_EST));
            }
            uint32_t age = TIME() - epoch.raw.ts;
            const float w = age < 100 ? 3.0 : 1.0;
            const float l = 20.0; //age < 100 ? 40.0 : 20.0; //20 + ((100 - MIN(age, 100)) / 2);

            draw->AddLine(xy - ImVec2(l, 0), xy - ImVec2(3, 0), c, w);
            draw->AddLine(xy + ImVec2(3, 0), xy + ImVec2(l, 0), c, w);
            draw->AddLine(xy - ImVec2(0, l), xy - ImVec2(0, 3), c, w);
            draw->AddLine(xy + ImVec2(0, 3), xy + ImVec2(0, l), c, w);

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
            constexpr double WGS84_A = 6378137.0;
            const double m2r = 1.0 / WGS84_A; // metres per rad at equator
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

            const FfVec2f xyBase  = _map.LonLatToScreen(llh[0], llh[1]);
            const FfVec2f xyRover = _map.LonLatToScreen(lastEpoch->raw.llh[0], lastEpoch->raw.llh[1]);
            draw->AddLine(xyBase, xyRover, GUI_COLOUR(PLOT_MAP_BASELINE), 4.0f);
            draw->AddCircleFilled(xyBase, 6.0f, GUI_COLOUR(PLOT_MAP_BASELINE));
        }
    }

    _map.EndDraw();
}

/* ****************************************************************************************************************** */
