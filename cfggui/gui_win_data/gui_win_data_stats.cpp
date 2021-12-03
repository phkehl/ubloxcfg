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

#include "gui_win_data_stats.hpp"

/* ****************************************************************************************************************** */

GuiWinDataStats::GuiWinDataStats(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database)
{
    _winSize = { 80, 25 };

    ClearData();

    _rows.push_back(Row("Latitude [deg]", [](const Database::EpochStats &es) { return es.llh[Database::_LAT_]; },
        [](const Database::Stats &st, Row &row)
        {
            row.count = Ff::Sprintf("%d", st.count);
            row.min   = Ff::Sprintf("%+17.12f", rad2deg(st.min));
            row.max   = Ff::Sprintf("%+17.12f", rad2deg(st.max));
            row.mean  = Ff::Sprintf("%+17.12f", rad2deg(st.mean));
            row.std   = std::isnan(st.std) ? "0" : Ff::Sprintf("%.12f", rad2deg(st.std));
        }));
    _rows.push_back(Row("Longitude [deg]", [](const Database::EpochStats &es) { return es.llh[Database::_LON_]; },
        [](const Database::Stats &st, Row &row)
        {
            row.count = Ff::Sprintf("%d", st.count);
            row.min   = Ff::Sprintf("%+17.12f", rad2deg(st.min));
            row.max   = Ff::Sprintf("%+17.12f", rad2deg(st.max));
            row.mean  = Ff::Sprintf("%+17.12f", rad2deg(st.mean));
            row.std   = std::isnan(st.std) ? "0" : Ff::Sprintf("%.12f", rad2deg(st.std));
        }));
    _rows.push_back(Row("Height [m]", [](const Database::EpochStats &es) { return es.llh[Database::_HEIGHT_]; },
        [](const Database::Stats &st, Row &row)
        {
            row.count = Ff::Sprintf("%d", st.count);
            row.min   = Ff::Sprintf("%.3f", st.min);
            row.max   = Ff::Sprintf("%.3f", st.max);
            row.mean  = Ff::Sprintf("%.3f", st.mean);
            row.std   = Ff::Sprintf("%.3f", st.std);
        }));
    _rows.push_back(Row("Latitude [deg, min, sec]",  [](const Database::EpochStats &es) { return es.llh[Database::_LAT_]; },
        [](const Database::Stats &st, Row &row)
        {
            auto fmt = [](const double r)
            {
                int d, m;
                double s;
                deg2dms(rad2deg(r), &d, &m, &s);
                return Ff::Sprintf("%3d° %2d' %9.6f\" %c", ABS(d), m, s, d < 0 ? 'S' : 'N');
            };
            row.count = Ff::Sprintf("%d", st.count);
            row.min   = fmt(st.min);
            row.max   = fmt(st.max);
            row.mean  = fmt(st.mean);
            row.std   = std::isnan(st.std) ? "0" : fmt(st.std);
        }
    ));
    _rows.push_back(Row("Longitude [deg, min, sec]",  [](const Database::EpochStats &es) { return es.llh[Database::_LON_]; },
        [](const Database::Stats &st, Row &row)
        {
            auto fmt = [](const double r)
            {
                int d, m;
                double s;
                deg2dms(rad2deg(r), &d, &m, &s);
                return Ff::Sprintf("%3d° %2d' %9.6f\" %c", ABS(d), m, s, d < 0 ? 'W' : 'E');
            };
            row.count = Ff::Sprintf("%d", st.count);
            row.min   = fmt(st.min);
            row.max   = fmt(st.max);
            row.mean  = fmt(st.mean);
            row.std   = std::isnan(st.std) ? "0" : fmt(st.std);
        }));

    _rows.push_back(Row("East (abs) [m]", [](const Database::EpochStats &es) { return es.enuAbs[Database::_E_]; },
        [](const Database::Stats &st, Row &row)
        {
            row.count = Ff::Sprintf("%d", st.count);
            row.min   = Ff::Sprintf("%8.3f", st.min);
            row.max   = Ff::Sprintf("%8.3f", st.max);
            row.mean  = Ff::Sprintf("%8.3f", st.mean);
            row.std   = Ff::Sprintf("%8.3f", st.std);
        }));
    _rows.push_back(Row("North (abs) [m]", [](const Database::EpochStats &es) { return es.enuAbs[Database::_N_]; },
        [](const Database::Stats &st, Row &row)
        {
            row.count = Ff::Sprintf("%d", st.count);
            row.min   = Ff::Sprintf("%8.3f", st.min);
            row.max   = Ff::Sprintf("%8.3f", st.max);
            row.mean  = Ff::Sprintf("%8.3f", st.mean);
            row.std   = Ff::Sprintf("%8.3f", st.std);
        }));
    _rows.push_back(Row("Up (abs) [m]", [](const Database::EpochStats &es) { return es.enuAbs[Database::_U_]; },
        [](const Database::Stats &st, Row &row)
        {
            row.count = Ff::Sprintf("%d", st.count);
            row.min   = Ff::Sprintf("%8.3f", st.min);
            row.max   = Ff::Sprintf("%8.3f", st.max);
            row.mean  = Ff::Sprintf("%8.3f", st.mean);
            row.std   = Ff::Sprintf("%8.3f", st.std);
        }));

    _rows.push_back(Row("East (mean) [m]", [](const Database::EpochStats &es) { return es.enuMean[Database::_E_]; },
        [](const Database::Stats &st, Row &row)
        {
            row.count = Ff::Sprintf("%d", st.count);
            row.min   = Ff::Sprintf("%8.3f", st.min);
            row.max   = Ff::Sprintf("%8.3f", st.max);
            row.mean  = Ff::Sprintf("%8.3f", st.mean);
            row.std   = Ff::Sprintf("%8.3f", st.std);
        }));
    _rows.push_back(Row("North (mean) [m]", [](const Database::EpochStats &es) { return es.enuMean[Database::_N_]; },
        [](const Database::Stats &st, Row &row)
        {
            row.count = Ff::Sprintf("%d", st.count);
            row.min   = Ff::Sprintf("%8.3f", st.min);
            row.max   = Ff::Sprintf("%8.3f", st.max);
            row.mean  = Ff::Sprintf("%8.3f", st.mean);
            row.std   = Ff::Sprintf("%8.3f", st.std);
        }));
    _rows.push_back(Row("Up (mean) [m]", [](const Database::EpochStats &es) { return es.enuMean[Database::_U_]; },
        [](const Database::Stats &st, Row &row)
        {
            row.count = Ff::Sprintf("%d", st.count);
            row.min   = Ff::Sprintf("%8.3f", st.min);
            row.max   = Ff::Sprintf("%8.3f", st.max);
            row.mean  = Ff::Sprintf("%8.3f", st.mean);
            row.std   = Ff::Sprintf("%8.3f", st.std);
        }));
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataStats::_ProcessData(const InputData &data)
{
    // New epoch means database stats have updated, render row contents
    if (data.type == InputData::DATA_EPOCH)
    {
        const auto stats = _database->GetStats();
        for (auto &row: _rows)
        {
            if (row.getter && row.formatter)
            {
                auto &s = row.getter(stats);
                row.formatter(s, row);
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataStats::_ClearData()
{
    for (auto &row: _rows)
    {
        row.Clear();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataStats::Row::Row(const char *_label, StatsGetter _getter, RowFormatter _formatter) :
    label{_label}, getter{_getter}, formatter{_formatter}
{
    Clear();
}

void GuiWinDataStats::Row::Clear()
{
    count = "-";
    min   = "-";
    mean  = "-";
    std   = "-";
    max   = "-";
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataStats::_DrawContent()
{
    constexpr ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_Hideable | /*ImGuiTableFlags_Sortable |*/
        ImGuiTableFlags_BordersV /* ImGuiTableFlags_Borders */ |
        ImGuiTableFlags_RowBg;

    if (ImGui::BeginTable("stats", 6, flags))
    {
        ImGui::TableSetupColumn("Variable");
        ImGui::TableSetupColumn("Count");
        ImGui::TableSetupColumn("Min");
        ImGui::TableSetupColumn("Mean");
        ImGui::TableSetupColumn("Std");
        ImGui::TableSetupColumn("Max");
        ImGui::TableHeadersRow();

        for (auto &row: _rows)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(row.label.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(row.count.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(row.min.c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted(row.mean.c_str());
            ImGui::TableSetColumnIndex(4);
            ImGui::TextUnformatted(row.std.c_str());
            ImGui::TableSetColumnIndex(5);
            ImGui::TextUnformatted(row.max.c_str());
        }

        ImGui::EndTable();
    }
}

/* ****************************************************************************************************************** */
