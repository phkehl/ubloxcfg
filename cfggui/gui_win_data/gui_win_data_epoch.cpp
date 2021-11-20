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

#include "ff_trafo.h"

#include "gui_inc.hpp"

#include "gui_win_data_epoch.hpp"

/* ****************************************************************************************************************** */

GuiWinDataEpoch::GuiWinDataEpoch(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database)
{
    _winSize = { 80, 25 };
    ClearData();
}

// ---------------------------------------------------------------------------------------------------------------------

#define _EPOCH_ROW_START(label) \
    ImGui::TableNextRow(); \
    ImGui::TableSetColumnIndex(0); \
    ImGui::Selectable(label, false, ImGuiSelectableFlags_SpanAllColumns); \
    ImGui::TableSetColumnIndex(1);

#define _EPOCH_ROW_FMT(label, fmt, ...) \
    _EPOCH_ROW_START(label); \
    ImGui::Text(fmt, ##__VA_ARGS__);

#define _EPOCH_ROW_IF_STR(label, pred, str) \
    _EPOCH_ROW_START(label); \
    if (pred) \
    { \
        ImGui::TextUnformatted(str); \
    } \
    else \
    { \
        ImGui::TextUnformatted("n/a"); \
    }

#define _EPOCH_ROW_IF_FMT(label, pred, fmt, ...) \
    _EPOCH_ROW_START(label); \
    if (pred) \
    { \
        ImGui::Text(fmt, ##__VA_ARGS__); \
    } \
    else \
    { \
        ImGui::TextUnformatted("n/a"); \
    }


void GuiWinDataEpoch::_DrawContent()
{
    const struct { const char *label; ImGuiTableColumnFlags flags; } columns[] =
    {
        { .label = "Field",     .flags = ImGuiTableColumnFlags_NoReorder },
        { .label = "Value",     .flags = ImGuiTableColumnFlags_NoReorder },
    };

    if (ImGui::BeginTable("stats", NUMOF(columns), TABLE_FLAGS, ImGui::GetContentRegionAvail()))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        for (int ix = 0; ix < NUMOF(columns); ix++)
        {
            ImGui::TableSetupColumn(columns[ix].label, columns[ix].flags);
        }
        ImGui::TableHeadersRow();

        if (_latestEpoch)
        {
            const EPOCH_t &e = _latestEpoch->epoch;

            _EPOCH_ROW_FMT(    "Sequence", "%i", e.seq);
            _EPOCH_ROW_IF_STR( "Fix type", e.haveFix, e.fixStr);
            _EPOCH_ROW_IF_FMT( "Position XYZ [m]", e.havePos, "%.3f %.3f %.3f", e.xyz[0], e.xyz[1], e.xyz[2]);
            _EPOCH_ROW_IF_FMT( "Position LLH [deg, m]", e.havePos, "%.9f %.9f %.3f", rad2deg(e.llh[0]), rad2deg(e.llh[1]), e.llh[2]);
            _EPOCH_ROW_IF_FMT( "Position accuracy", e.havePos, "%.3f (%.3f h, %.3f v)", e.posAcc, e.horizAcc, e.vertAcc);
            _EPOCH_ROW_IF_FMT( "Relative position NED [m]", e.haveRelPos, "%.3f %.3f %.3f (%.3f)", e.relNed[0], e.relNed[1], e.relNed[2], e.relLen);
            _EPOCH_ROW_IF_FMT( "Rel. position accuracy [m]", e.haveRelPos, "%.3f %.3f %.3f", e.relAcc[0], e.relAcc[1], e.relAcc[2]);

            // TODO... the rest of it...
        }

        ImGui::EndTable();
    }
}

/* ****************************************************************************************************************** */
