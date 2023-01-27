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
    GuiWinData(name, database),
    _tabbar { WinName(), ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_TabListPopupButton }
{
    _winSize = { 80, 25 };

    _table.AddColumn("Variable");
    _table.AddColumn("Count", 0.0f, GuiWidgetTable::ALIGN_RIGHT);
    _table.AddColumn("Min",   0.0f, GuiWidgetTable::ALIGN_RIGHT);
    _table.AddColumn("Mean",  0.0f, GuiWidgetTable::ALIGN_RIGHT);
    _table.AddColumn("Std",   0.0f, GuiWidgetTable::ALIGN_RIGHT);
    _table.AddColumn("Max",   0.0f, GuiWidgetTable::ALIGN_RIGHT);

    _rows.emplace_back("Latitude [deg]",
        [](const Database::EpochStats &es) { return es.llh[Database::_LAT_]; },
        [](const double value) { return Ff::Sprintf("%.12f", rad2deg(value)); });
    _rows.emplace_back("Longitude [deg]",
        [](const Database::EpochStats &es) { return es.llh[Database::_LON_]; },
        [](const double value) { return Ff::Sprintf("%.12f", rad2deg(value)); });
    _rows.emplace_back("Height [m]",
        [](const Database::EpochStats &es) { return es.llh[Database::_HEIGHT_]; },
        [](const double value) { return Ff::Sprintf("%.3f", value); });
    _rows.emplace_back("Latitude [deg, min, sec]",
        [](const Database::EpochStats &es) { return es.llh[Database::_LAT_]; },
        [](const double value)
        {
            int d, m;
            double s;
            deg2dms(rad2deg(value), &d, &m, &s);
            return Ff::Sprintf("%3d° %2d' %9.6f\" %c", ABS(d), m, s, d < 0 ? 'S' : 'N');
        });
    _rows.emplace_back("Longitude [deg, min, sec]",
        [](const Database::EpochStats &es) { return es.llh[Database::_LON_]; },
        [](const double value)
        {
            int d, m;
            double s;
            deg2dms(rad2deg(value), &d, &m, &s);
            return Ff::Sprintf("%3d° %2d' %9.6f\" %c", ABS(d), m, s, d < 0 ? 'S' : 'N');
        });
    _rows.emplace_back("East (ref) [m]",
        [](const Database::EpochStats &es) { return es.enuRef[Database::_E_]; },
        [](const double value) { return Ff::Sprintf("%.3f", rad2deg(value)); });
    _rows.emplace_back("North (ref) [m]",
        [](const Database::EpochStats &es) { return es.enuRef[Database::_N_]; },
        [](const double value) { return Ff::Sprintf("%.3f", rad2deg(value)); });
    _rows.emplace_back("Up (ref) [m]",
        [](const Database::EpochStats &es) { return es.enuRef[Database::_U_]; },
        [](const double value) { return Ff::Sprintf("%.3f", rad2deg(value)); });
    _rows.emplace_back("East (mean) [m]",
        [](const Database::EpochStats &es) { return es.enuMean[Database::_E_]; },
        [](const double value) { return Ff::Sprintf("%.3f", rad2deg(value)); });
    _rows.emplace_back("North (mean) [m]",
        [](const Database::EpochStats &es) { return es.enuMean[Database::_N_]; },
        [](const double value) { return Ff::Sprintf("%.3f", rad2deg(value)); });
    _rows.emplace_back("Up (mean) [m]",
        [](const Database::EpochStats &es) { return es.enuMean[Database::_U_]; },
        [](const double value) { return Ff::Sprintf("%.3f", rad2deg(value)); });
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataStats::_ProcessData(const InputData &data)
{
    // New epoch means database stats have updated, render row contents
    if (data.type == InputData::DATA_EPOCH)
    {
        _table.ClearRows();
        const Database::EpochStats stats = _database->GetStats();
        for (auto &row: _rows)
        {
            if (row.getter && row.formatter)
            {
                auto &st = row.getter(stats);
                _table.AddCellText(row.label);
                _table.AddCellTextF("%d", st.count);
                _table.AddCellText(row.formatter(st.min));
                _table.AddCellText(row.formatter(st.mean));
                _table.AddCellText(row.formatter(st.std));
                _table.AddCellText(row.formatter(st.max));
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataStats::_ClearData()
{
    _table.ClearRows();
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataStats::Row::Row(const char *_label, StatsGetter _getter, ValFormatter _formatter) :
    label     { _label },
    getter    { _getter },
    formatter { _formatter }
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataStats::_DrawContent()
{
    bool drawPosition = false;
    bool drawSignal = false;
    if (_tabbar.Begin())
    {
        if (_tabbar.Item("Position"))
        {
            drawPosition = true;
        }
        if (_tabbar.Item("Signal levels"))
        {
            drawSignal = true;
        }
        _tabbar.End();
    }
    if (drawPosition)
    {
        _table.DrawTable();
    }
    if (drawSignal)
    {
        _DrawSiglevelPlot();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataStats::_DrawSiglevelPlot()
{
    if (ImPlot::BeginPlot("##SignalLevels", ImVec2(-1,-1), ImPlotFlags_Crosshairs | ImPlotFlags_NoMenus /*| ImPlotFlags_NoLegend*/))
    {
        // Data
        const auto stats = _database->GetStats();
        const auto &cnoNav = stats.cnoNav;
        const auto &cnoTrk = stats.cnoTrk;
        constexpr float binWidth = 4.5f;

        // Configure plot
        ImPlot::SetupAxis(ImAxis_X1, "Signal level [dbHz]", ImPlotAxisFlags_NoHighlight);
        ImPlot::SetupAxis(ImAxis_Y1, "Number of signals", ImPlotAxisFlags_NoHighlight);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0.0f, 55.0f, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0f, 25.0f, ImGuiCond_Always);
        ImPlot::SetupFinish();

        constexpr const char *labelTrk = "Tracked (max/mean/std)";
        constexpr const char *labelNav = "Used    (max/mean/std)";

        // Maxima
        ImPlot::SetNextLineStyle(GUI_COLOUR4(SIGNAL_UNUSED));
        ImPlot::PlotStairs(labelTrk, cnoTrk.cnosLo.data(), cnoTrk.maxs.data(), cnoTrk.count);
        ImPlot::SetNextLineStyle(GUI_COLOUR4(SIGNAL_USED));
        ImPlot::PlotStairs(labelNav, cnoNav.cnosLo.data(), cnoNav.maxs.data(), cnoNav.count);

        // On plot per bin, so that we can cycle through the colourmap. Per bin plot #tracked on top of that #used.
        for (int ix = 0; ix < cnoNav.count; ix++)
        {
            ImPlot::SetNextFillStyle(GUI_COLOUR4(SIGNAL_UNUSED));
            ImPlot::PlotBars(labelTrk, &cnoTrk.cnosMi[ix], &cnoTrk.means[ix], 1, binWidth);

            ImPlot::SetNextFillStyle(GUI_COLOUR4(SIGNAL_00_05 + ix));
            ImPlot::PlotBars(labelNav, &cnoNav.cnosMi[ix], &cnoNav.means[ix], 1, binWidth);

            if (cnoNav.means[ix] > 0.0f)
            {
                ImPlot::SetNextErrorBarStyle(GUI_COLOUR4(SIGNAL_USED), 15.0f, 3.0f);
                ImPlot::PlotErrorBars(labelNav, &cnoNav.cnosMi[ix], &cnoNav.means[ix], &cnoNav.stds[ix], 1);
            }

            if (cnoTrk.means[ix] > 0.0f)
            {
                ImPlot::SetNextErrorBarStyle(GUI_COLOUR4(SIGNAL_UNUSED), 10.0f, 2.0f);
                ImPlot::PlotErrorBars(labelTrk, &cnoTrk.cnosMi[ix], &cnoTrk.means[ix], &cnoTrk.stds[ix], 1);
            }
        }

        // Last plot determines colour shown in the legend
        ImPlot::SetNextLineStyle(GUI_COLOUR4(SIGNAL_UNUSED));
        ImPlot::PlotDummy(labelTrk);
        ImPlot::SetNextLineStyle(GUI_COLOUR4(SIGNAL_USED));
        ImPlot::PlotDummy(labelNav);

        ImPlot::EndPlot();
    }
}

/* ****************************************************************************************************************** */
