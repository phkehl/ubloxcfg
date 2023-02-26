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

#ifndef __GUI_WIN_DATA_PLOT_HPP__
#define __GUI_WIN_DATA_PLOT_HPP__

#include <functional>
#include <unordered_map>
#include <vector>

#include "implot.h"

#include "gui_win_data.hpp"

/* ***** Plots ****************************************************************************************************** */

class GuiWinDataPlot : public GuiWinData
{
    public:
        GuiWinDataPlot(const std::string &name, std::shared_ptr<Database> database);
       ~GuiWinDataPlot();

    private:

        //void _ProcessData(const InputData &data) final;
        void _Loop(const uint32_t &frame, const double &now) final;
        void _DrawContent() final;
        // void _ClearData() final;
        void _DrawToolbar() final;

        void _SaveParams();
        void _LoadParams();

        struct DragDrop
        {
            enum Target : int {
                UNKNOWN = -99, TRASH = -4, LIST = -3, PLOT = -2, LEGEND = -1,
                X = ImAxis_X1, Y1 = ImAxis_Y1, Y2 = ImAxis_Y2, Y3 = ImAxis_Y3 };

            DragDrop(const Database::FieldDef &_field, const Target _src);
            DragDrop(const Database::FieldDef &_field, const Target _src, const Target _dst);
            DragDrop(const ImGuiPayload *imPayload, const Target _dst);
            Target src;
            Target dst;
            Database::FieldDef field;
        };

        std::unique_ptr<DragDrop> _CheckDrop(const ImAxis axis);
        void _HandleDrop(const DragDrop &dragdrop);
        void _UpdateVars();

        struct PlotVar
        {
            PlotVar(const ImAxis _axis, const Database::FieldDef &_field) : axis{_axis}, field{_field} {}
            ImAxis axis;
            Database::FieldDef field;
        };

        char                      _dndType[32]; // unique name for drag and drop functionality
        bool                      _dndHovered;
        ImPlotColormap            _colormap;

        std::unique_ptr<PlotVar>  _xVar;
        std::vector<PlotVar>      _yVars;
        std::string               _axisLabels[ImAxis_COUNT];
        bool                      _axisUsed[ImAxis_COUNT];
        std::string               _axisTooltips[ImAxis_COUNT];
        std::unordered_map<const char *, bool> _usedFields;
        enum FitMode { NONE, AUTOFIT, FOLLOW_X };
        FitMode                   _fitMode;
        ImPlotRect                _plotLimitsY1;
        ImPlotRect                _plotLimitsY2;
        ImPlotRect                _plotLimitsY3;
        static const ImPlotRect PLOT_LIMITS_DEF;
        double                    _followXmax;
        bool                      _setLimitsX;
        bool                      _setLimitsY;
        std::string               _plotTitleId;
        bool                      _showLegend;
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_INF_HPP__
