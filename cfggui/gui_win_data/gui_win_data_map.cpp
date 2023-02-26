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
    _map.EnableFollowButton();
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
    _database->ProcRows([&](const Database::Row &row)
    {
        if (row.pos_avail)
        {
            const FfVec2f xy = _map.LonLatToScreen(row.pos_llh_lat, row.pos_llh_lon);
            draw->AddRectFilled(xy - FfVec2f(2,2), xy + FfVec2f(2,2), row.fix_colour);
        }
        return true;
    });

    // Highlight last point, draw accuracy estimate circle
    Database::Row last_row;
    _database->ProcRows([&](const Database::Row &row)
    {
        if (row.pos_avail)
        {
            const FfVec2f xy = _map.LonLatToScreen(row.pos_llh_lat, row.pos_llh_lon);

            const ImU32 c = row.fix_ok ? GUI_COLOUR(PLOT_MAP_HL_OK) : GUI_COLOUR(PLOT_MAP_HL_MASKED);
            const double m2px = _map.PixelPerMetre(row.pos_llh_lat);
            const float r = (0.5 * row.pos_acc_horiz * m2px) + 0.5;
            if (r > 1.0f)
            {
                draw->AddCircleFilled(xy, r, GUI_COLOUR(PLOT_MAP_ACC_EST));
            }
            uint32_t age = TIME() - row.time_ts;
            const float w = age < 100 ? 3.0 : 1.0;
            const float l = 20.0; //age < 100 ? 40.0 : 20.0; //20 + ((100 - MIN(age, 100)) / 2);

            draw->AddLine(xy - ImVec2(l, 0), xy - ImVec2(3, 0), c, w);
            draw->AddLine(xy + ImVec2(3, 0), xy + ImVec2(l, 0), c, w);
            draw->AddLine(xy - ImVec2(0, l), xy - ImVec2(0, 3), c, w);
            draw->AddLine(xy + ImVec2(0, 3), xy + ImVec2(0, l), c, w);

            last_row = row;
            return false;
        }
        return true;
    }, true);

    // Draw approximate position of the basestation
    if (last_row.pos_avail && last_row.relpos_avail)
    {
        // Calculate approximate basestation coordinates given NED *at* base station
        // FIXE: is this correct?
        double llh[3] = { last_row.pos_llh_lat, last_row.pos_llh_lon, last_row.pos_llh_height };

        // Approx. base lat/lon using rover lat/lon and North/East
        constexpr double WGS84_A = 6378137.0;
        const double m2r = 1.0 / WGS84_A; // metres per rad at equator
        const double m2rAtLat = m2r / cos( llh[0] ); // at our latitude
        llh[0] -= last_row.relpos_ned_north * m2r;
        llh[1] -= last_row.relpos_ned_east * m2rAtLat;
        llh[2] += last_row.relpos_ned_down;
        // This is quite close already, but it gets better (more stable) if we do the following...

        // Get XYZ vector from NED vector at approx. base
        double xyz[3];
        xyz2ned_vec(&last_row.relpos_ned_north, llh, xyz);

        // Base ECEF
        xyz[0] = last_row.pos_ecef_x - xyz[0];
        xyz[1] = last_row.pos_ecef_y - xyz[1];
        xyz[2] = last_row.pos_ecef_z - xyz[2];
        // Base LLH
        xyz2llh_vec(xyz, llh);

        const FfVec2f xyBase  = _map.LonLatToScreen(llh[0], llh[1]);
        const FfVec2f xyRover = _map.LonLatToScreen(last_row.pos_llh_lat, last_row.pos_llh_lon);
        draw->AddLine(xyBase, xyRover, GUI_COLOUR(PLOT_MAP_BASELINE), 4.0f);
        draw->AddCircleFilled(xyBase, 6.0f, GUI_COLOUR(PLOT_MAP_BASELINE));
    }

    if (last_row.pos_avail && _map.FollowEnabled())
    {
        _map.SetPos(last_row.pos_llh_lat, last_row.pos_llh_lon);
    }

    _map.EndDraw();
}

/* ****************************************************************************************************************** */
