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
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataStats::_ProcessData(const InputData &data)
{
    // New epoch means database stats have updated
    if (data.type == InputData::DATA_EPOCH)
    {
        _dbinfo = _database->GetInfo();
        _table.ClearRows();

        for (const auto &e: _dbinfo.stats_list) {
            if (e.label && e.stats) {
                _table.AddCellText(e.label);
                _table.SetRowUid((uint32_t)(uint64_t)(void *)e.label);
                _table.AddCellTextF("%d", e.stats->count);
                _table.AddCellTextF(e.fmt ? e.fmt : "%g", e.stats->min);
                _table.AddCellTextF(e.fmt ? e.fmt : "%g", e.stats->mean);
                _table.AddCellTextF(e.fmt ? e.fmt : "%g", e.stats->std);
                _table.AddCellTextF(e.fmt ? e.fmt : "%g", e.stats->max);
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

void GuiWinDataStats::_DrawContent()
{
    bool drawVars = false;
    bool drawSignal = false;
    if (_tabbar.Begin())
    {
        if (_tabbar.Item("Variables"))
        {
            drawVars = true;
        }
        if (_tabbar.Item("Signal levels"))
        {
            drawSignal = true;
        }
        _tabbar.End();
    }
    if (drawVars)
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
    if (ImPlot::BeginPlot("##SignalLevels", ImVec2(-1,-1), ImPlotFlags_Crosshairs | ImPlotFlags_NoMenus | ImPlotFlags_NoFrame /*| ImPlotFlags_NoLegend*/))
    {
        // Data
        const auto &cno_trk = _dbinfo.cno_trk;
        const auto &cno_nav = _dbinfo.cno_nav;
        constexpr float bin_width = 4.5f;

        // Configure plot
        ImPlot::SetupAxis(ImAxis_X1, "Signal level [dbHz]", ImPlotAxisFlags_NoHighlight);
        ImPlot::SetupAxis(ImAxis_Y1, "Number of signals", ImPlotAxisFlags_NoHighlight);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0.0f, 55.0f, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0f, 35.0f, ImGuiCond_Always);
        ImPlot::SetupFinish();

        constexpr const char *label_trk = "Tracked (max/mean/std)";
        constexpr const char *label_nav = "Used    (max/mean/std)";

        // Maxima
        ImPlot::SetNextLineStyle(GUI_COLOUR4(SIGNAL_UNUSED));
        ImPlot::PlotStairs(label_trk, Database::Info::CNO_BINS_LO, cno_trk.maxs, Database::Info::CNO_BINS_NUM);
        ImPlot::SetNextLineStyle(GUI_COLOUR4(SIGNAL_USED));
        ImPlot::PlotStairs(label_nav, Database::Info::CNO_BINS_LO, cno_nav.maxs, Database::Info::CNO_BINS_NUM);

        // On plot per bin, so that we can cycle through the colourmap. Per bin plot #tracked on top of that #used.
        for (int ix = 0; ix < Database::Info::CNO_BINS_NUM; ix++)
        {
            // Plot x for centre of bin
            const float x = Database::Info::CNO_BINS_MI[ix];

            // Grey bar for tracked signals
            ImPlot::SetNextFillStyle(GUI_COLOUR4(SIGNAL_UNUSED));
            ImPlot::PlotBars(label_trk, &x, &cno_trk.means[ix], 1, bin_width);

            // Coloured bar for used signals
            ImPlot::SetNextFillStyle(GUI_COLOUR4(SIGNAL_00_05 + ix));
            ImPlot::PlotBars(label_nav, &x, &cno_nav.means[ix], 1, bin_width);

            if (cno_nav.means[ix] > 0.0f)
            {
                ImPlot::SetNextErrorBarStyle(GUI_COLOUR4(SIGNAL_USED), 15.0f, 3.0f);
                ImPlot::PlotErrorBars(label_nav, &x, &cno_nav.means[ix], &cno_nav.stds[ix], 1);
            }

            if (cno_trk.means[ix] > 0.0f)
            {
                ImPlot::SetNextErrorBarStyle(GUI_COLOUR4(SIGNAL_UNUSED), 10.0f, 2.0f);
                ImPlot::PlotErrorBars(label_trk, &x, &cno_trk.means[ix], &cno_trk.stds[ix], 1);
            }
        }

        // Last plot determines colour shown in the legend
        ImPlot::SetNextLineStyle(GUI_COLOUR4(SIGNAL_UNUSED));
        ImPlot::PlotDummy(label_trk);
        ImPlot::SetNextLineStyle(GUI_COLOUR4(SIGNAL_USED));
        ImPlot::PlotDummy(label_nav);

        ImPlot::EndPlot();
    }
}

/* ****************************************************************************************************************** */
