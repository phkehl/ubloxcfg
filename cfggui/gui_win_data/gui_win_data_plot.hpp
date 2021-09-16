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

#ifndef __GUI_WIN_DATA_PLOT_H__
#define __GUI_WIN_DATA_PLOT_H__

#include <functional>

#include "implot.h"

#include "gui_win_data.hpp"

/* ***** Plots ****************************************************************************************************** */

class GuiWinDataPlot : public GuiWinData
{
    public:
        GuiWinDataPlot(const std::string &name,
            std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile, std::shared_ptr<Database> database);
       ~GuiWinDataPlot();

      //void                 Loop(const uint32_t &frame, const double &now) override;
        void                 ProcessData(const Data &data) override;
        void                 ClearData() override;
        void                 DrawWindow() override;

    protected:

        void                 _DrawControls();
        void                 _DrawVars();
        void                 _DrawPlot();


        //! Plot variables
        typedef double (*EpochValueGetter_t)(const Database::Epoch &);
        struct PlotVar
        {
            PlotVar(const std::string &_label, const std::string &_tooltip, EpochValueGetter_t _getter);
            std::string        label;
            std::string        tooltip;
            bool               used;
            EpochValueGetter_t getter;
        };
        std::vector<PlotVar>   _plotVars;

        //! Data to plot
        enum class PlotType
        {
            LINE, SCATTER, STEP, BAR, // SHADED, ERRORBARS_V, ERRORBARS_H, STEMS, DIGITAL
        };
        typedef ImPlotPoint (*PlotVarGetter_t)(void *, int);
        struct PlotData
        {
            PlotData(PlotVar *_plotVarX, PlotVar *_plotVarY, PlotType _type, Database *_db);
            PlotVar           *plotVarX;
            PlotVar           *plotVarY;
            PlotType           type;
            Database          *db;
            ImPlotYAxis        axis;
            PlotVarGetter_t    getter;
        };
        std::vector<PlotData> _plotData;
        PlotVar             *_plotVarX;

        void                 _InitPlotVars();
        char                 _dndType[32]; // unique name for drag and drop functionality
        double               _epochPeriod; // [s]
        uint32_t             _epochPrevTs;
        bool                 _dndHovered;
        ImPlotFlags          _plotFlags;
        std::string          _yLabels[3];

        void _SetVarX(PlotVar *plotVar);
        void _AddToPlot(PlotVar *plotVar, const ImPlotYAxis yAxis = ImPlotYAxis_1);
        void _RemoveFromPlot(PlotVar *plotVar);
        void _SavePlots();
        void _LoadPlots();
        PlotVar *_PlotVarByLabel(const std::string label);
        void _UpdatePlotFlags();

        struct DndPayload
        {
            DndPayload(PlotVar *_plotVar, PlotData *_plotData);
            DndPayload(const ImGuiPayload *imPayload);
            PlotVar  *plotVar;
            PlotData *plotData;
        };
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_INF_H__
