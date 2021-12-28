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

#include <cinttypes>

#include "ff_trafo.h"

#include "gui_inc.hpp"
#include "implot.h"
#include "implot_internal.h"

#include "gui_win_data_plot.hpp"

/* ****************************************************************************************************************** */

#define _PLOT_ID "##Plot"

GuiWinDataPlot::GuiWinDataPlot(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database),
    _plotVars{}, _plotVarX{nullptr}, _dndHovered{false},
    _plotFlags{ImPlotFlags_AntiAliased | ImPlotFlags_Crosshairs}, _yLabels{"", "", ""},
    _colormap { ImPlotColormap_Deep }
{
    _winSize = { 80, 25 };

    _latestEpochEna = false;

    std::snprintf(_dndType, sizeof(_dndType), "PlotVar_%016" PRIx64, _winUid & 0xffffffffffffffff);
    _InitPlotVars();
    _LoadPlots();
}

GuiWinDataPlot::~GuiWinDataPlot()
{
    _SavePlots();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataPlot::_ProcessData(const InputData &data)
{
    if (data.type == InputData::DATA_EPOCH)
    {
        // Learn epoch period (1/rate)
        const uint32_t ts = data.epoch->epoch.ts;
        if (_epochPrevTs != 0)
        {
            const double dt = (double)(ts - _epochPrevTs) * 1e-3;
            if ( (dt > 0.01) && (dt < 5.0) )
            {
                _epochPeriod = dt;
            }
        }
        _epochPrevTs = ts;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataPlot::_ClearData()
{
    _epochPeriod = 1.0;
    _epochPrevTs = 0;
    // for (auto &d: _plotData)
    // {
    //     _RemoveFromPlot(d.plotVarY);
    // }
    // _plotVarX = nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataPlot::PlotData::PlotData(PlotVar *_plotVarX, PlotVar *_plotVarY, PlotType _type, Database *_db)
    : plotVarX{_plotVarX}, plotVarY{_plotVarY}, type{_type}, db{_db}
{
    axis = ImPlotYAxis_1;
    getter = [](void *arg, int ix) -> ImPlotPoint
    {
        PlotData *pd = (PlotData *)arg;
        const Database::Epoch &epoch = (*pd->db)[ix];
        return ImPlotPoint( pd->plotVarX->getter(epoch), pd->plotVarY->getter(epoch) );
    };
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataPlot::PlotVar::PlotVar(const std::string &_label, const std::string &_tooltip, EpochValueGetter_t _getter)
    : label{_label}, tooltip{_tooltip}, used{false}, getter{_getter}
{
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataPlot::DndPayload::DndPayload(PlotVar *_plotVar, PlotData *_plotData)
    : plotVar{_plotVar}, plotData{_plotData}
{
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataPlot::DndPayload::DndPayload(const ImGuiPayload *imPayload)
    : plotVar{ ((DndPayload *)imPayload->Data)->plotVar},
      plotData{((DndPayload *)imPayload->Data)->plotData}
{
}

// ---------------------------------------------------------------------------------------------------------------------

#define _EPOCH(_expr_) [](const Database::Epoch &epoch) { return (double)(_expr_); }

void GuiWinDataPlot::_InitPlotVars()
{
    //_plotVars.push_back( PlotVar( "NAN", "NAN", [](const Database::Epoch &epoch) { (void)epoch; return NAN; }) );
    _plotVars.push_back( PlotVar( "ts",      "Timestamp [s]",                         _EPOCH(epoch.ts)) );
    _plotVars.push_back( PlotVar( "TOW",     "GPS time of week [s]",                  _EPOCH(epoch.raw.haveGpsTow ? epoch.raw.gpsTow : NAN) ) );

    _plotVars.push_back( PlotVar( "East",    "East (relative to mean position) [m]",  _EPOCH(epoch.raw.havePos ? epoch.enuMean[Database::_E_] : NAN) ) );
    _plotVars.push_back( PlotVar( "North",   "North (relative to mean position) [m]", _EPOCH(epoch.raw.havePos ? epoch.enuMean[Database::_N_] : NAN) ) );
    _plotVars.push_back( PlotVar( "Up",      "Up (relative to mean position) [m]",    _EPOCH(epoch.raw.havePos ? epoch.enuMean[Database::_U_] : NAN) ) );

    _plotVars.push_back( PlotVar( "Lat",     "Latitude [deg]",                        _EPOCH(epoch.raw.havePos ? rad2deg(epoch.raw.llh[0]) : NAN) ) );
    _plotVars.push_back( PlotVar( "Lon",     "Longitude [deg]",                       _EPOCH(epoch.raw.havePos ? rad2deg(epoch.raw.llh[1]) : NAN) ) );
    _plotVars.push_back( PlotVar( "Height",  "Height [m]",                            _EPOCH(epoch.raw.havePos ? epoch.raw.llh[2] : NAN) ) );
    _plotVars.push_back( PlotVar( "MSL",     "Height MSL [m]",                        _EPOCH(epoch.raw.haveMsl ? epoch.raw.heightMsl : NAN) ) );

    _plotVars.push_back( PlotVar( "X",       "ECEF X [m]",                            _EPOCH(epoch.raw.havePos ? epoch.raw.xyz[0] : NAN) ) );
    _plotVars.push_back( PlotVar( "Y",       "ECEF Y [m]",                            _EPOCH(epoch.raw.havePos ? epoch.raw.xyz[1] : NAN) ) );
    _plotVars.push_back( PlotVar( "Z",       "ECEF Z [m]",                            _EPOCH(epoch.raw.havePos ? epoch.raw.xyz[2] : NAN) ) );

    _plotVars.push_back( PlotVar( "PDOP",    "Position DOP [-]",                      _EPOCH(epoch.raw.havePdop ? epoch.raw.pDOP : NAN) ) );
    _plotVars.push_back( PlotVar( "hAcc",    "Horizontal (2d) accuracy estimate [m]", _EPOCH(epoch.raw.havePos ? epoch.raw.horizAcc : NAN) ) );
    _plotVars.push_back( PlotVar( "vAcc",    "Vertical accuracy estimate [m]",        _EPOCH(epoch.raw.havePos ? epoch.raw.vertAcc : NAN) ) );
    _plotVars.push_back( PlotVar( "pAcc",    "Position (3d) accuracy estimate [m]",   _EPOCH(epoch.raw.havePos ? epoch.raw.posAcc : NAN) ) );

    _plotVars.push_back( PlotVar( "Fix",     "Fix type (+0.25 if fixok) [-]",         _EPOCH(epoch.raw.haveFix ? epoch.raw.fix + (epoch.raw.fixOk ? 0.25 : 0.0) : NAN) ) );

    _plotVars.push_back( PlotVar( "#sat",      "Number of satellits used in navigation [-]",       _EPOCH(epoch.raw.haveNumSv ? epoch.raw.numSatUsed     : NAN) ) );
    _plotVars.push_back( PlotVar( "#sig_GPS",  "Number of GPS signals used in navigation [-]",     _EPOCH(epoch.raw.haveNumSv ? epoch.raw.numSatUsedGps  : NAN) ) );
    _plotVars.push_back( PlotVar( "#sig_GLO",  "Number of GLONASS signals used in navigation [-]", _EPOCH(epoch.raw.haveNumSv ? epoch.raw.numSatUsedGlo  : NAN) ) );
    _plotVars.push_back( PlotVar( "#sig_GAL",  "Number of Galileo signals used in navigation [-]", _EPOCH(epoch.raw.haveNumSv ? epoch.raw.numSatUsedGal  : NAN) ) );
    _plotVars.push_back( PlotVar( "#sig_BDS",  "Number of BeiDou signals used in navigation [-]",  _EPOCH(epoch.raw.haveNumSv ? epoch.raw.numSatUsedBds  : NAN) ) );
    _plotVars.push_back( PlotVar( "#sig_SBAS", "Number of SBAS signals used in navigation [-]",    _EPOCH(epoch.raw.haveNumSv ? epoch.raw.numSatUsedSbas : NAN) ) );
    _plotVars.push_back( PlotVar( "#sig_QZSS", "Number of QZSS signals used in navigation [-]",    _EPOCH(epoch.raw.haveNumSv ? epoch.raw.numSatUsedQzss : NAN) ) );

    _plotVars.push_back( PlotVar( "#sig",      "Number of signals used in navigation [-]",         _EPOCH(epoch.raw.haveNumSig ? epoch.raw.numSigUsed     : NAN) ) );
    _plotVars.push_back( PlotVar( "#sig_GPS",  "Number of GPS signals used in navigation [-]",     _EPOCH(epoch.raw.haveNumSig ? epoch.raw.numSigUsedGps  : NAN) ) );
    _plotVars.push_back( PlotVar( "#sig_GLO",  "Number of GLONASS signals used in navigation [-]", _EPOCH(epoch.raw.haveNumSig ? epoch.raw.numSigUsedGlo  : NAN) ) );
    _plotVars.push_back( PlotVar( "#sig_GAL",  "Number of Galileo signals used in navigation [-]", _EPOCH(epoch.raw.haveNumSig ? epoch.raw.numSigUsedGal  : NAN) ) );
    _plotVars.push_back( PlotVar( "#sig_BDS",  "Number of BeiDou signals used in navigation [-]",  _EPOCH(epoch.raw.haveNumSig ? epoch.raw.numSigUsedBds  : NAN) ) );
    _plotVars.push_back( PlotVar( "#sig_SBAS", "Number of SBAS signals used in navigation [-]",    _EPOCH(epoch.raw.haveNumSig ? epoch.raw.numSigUsedSbas : NAN) ) );
    _plotVars.push_back( PlotVar( "#sig_QZSS", "Number of QZSS signals used in navigation [-]",    _EPOCH(epoch.raw.haveNumSig ? epoch.raw.numSigUsedQzss : NAN) ) );

    _plotVars.push_back( PlotVar( "relNorth", "Relative East (RTK) [m]",  _EPOCH(epoch.raw.haveRelPos ? epoch.raw.relNed[0] : NAN) ) );
    _plotVars.push_back( PlotVar( "relEast",  "Relative North (RTK) [m]", _EPOCH(epoch.raw.haveRelPos ? epoch.raw.relNed[1] : NAN) ) );
    _plotVars.push_back( PlotVar( "relDown",  "Relative down (RTK) [m]",  _EPOCH(epoch.raw.haveRelPos ? epoch.raw.relNed[2] : NAN) ) );
}

// ---------------------------------------------------------------------------------------------------------------------

constexpr float VAR_WIDTH = 15.0f;

void GuiWinDataPlot::_DrawContent()
{
    // List of variables
    ImGui::BeginChild("##Vars", ImVec2(GuiSettings::charSize.x * VAR_WIDTH, 0.0f));
    _DrawVars();
    ImGui::EndChild();

    ImGui::SameLine(0.0f, 2.0f);
    //Gui::VerticalSeparator();

    // Plot
    _DrawPlot();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataPlot::_DrawToolbar()
{
    ImGui::SameLine();

    // Help
    if (ImGui::Button(ICON_FK_QUESTION "###Help", GuiSettings::iconSize))
    {
        ImGui::OpenPopup("Help");
    }
    Gui::ItemTooltip("Help");
    if (ImGui::BeginPopup("Help"))
    {
        ImPlot::ShowUserGuide();
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    // Colormap
    ImGui::Button(ICON_FK_PAINT_BRUSH "###Colormap", GuiSettings::iconSize);
    Gui::ItemTooltip("Colours (click left/right to cycle maps)");
    if (ImGui::IsItemHovered())
    {
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            _colormap++;
            _colormap %= ImPlot::GetColormapCount();
            ImPlot::BustColorCache(_PLOT_ID);
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            _colormap--;
            if (_colormap < 0)
            {
                _colormap = ImPlot::GetColormapCount() - 1;
            }
            ImPlot::BustColorCache(_PLOT_ID);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataPlot::_DrawVars()
{
    // Variables currently used in plot
    Gui::TextTitle("Used:");
    ImGui::Separator();
    PlotVar *removeVar = nullptr;
    PlotData *src = nullptr;
    PlotData *dst = nullptr;
    for (auto &data: _plotData)
    {
        auto *varY = data.plotVarY;
        if (ImGui::Selectable(varY->label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
        {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                removeVar = varY;
            }
        }
        Gui::ItemTooltip(varY->tooltip.c_str());

        // This list is a drag and drop source for plot variables
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            DndPayload payload(varY, std::addressof(data));
            ImGui::SetDragDropPayload(_dndType, &payload, sizeof(payload));
            ImGui::TextUnformatted(varY->label.c_str());
            ImGui::EndDragDropSource();
        }
        // And the list is also a drop and drop target
        if (ImGui::BeginDragDropTarget())
        {
            const ImGuiPayload *imPayload = ImGui::AcceptDragDropPayload(_dndType);
            if (imPayload)
            {
                DndPayload payload(imPayload);
                // Source is this list
                if (payload.plotData)
                {
                    src = payload.plotData;
                    dst = std::addressof(data);
                }
                // Source is other (list of available)
                else
                {
                    _AddToPlot(payload.plotVar);
                }
            }
        }

        ImGui::SameLine(GuiSettings::charSize.x * (VAR_WIDTH - 4.0)); // reserve some space for vertical scrollbar
        ImGui::Text("y%d", data.axis + 1);
    }

    // Reorder list of plots, placing src in front of dst
    if (src && dst)
    {
        DEBUG("reorder");
        std::vector<PlotData> _newOrder;
        for (auto &data: _plotData)
        {
            auto *pData = std::addressof(data);
            if (pData == dst)
            {
                _newOrder.push_back(*src);
            }
            if (pData != src)
            {
                _newOrder.push_back(data);
            }
        }
        _plotData = _newOrder;
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    // Variables not currently used in plot
    ImGui::BeginGroup();
    Gui::TextTitle("Available:");
    ImGui::Separator();
    for (auto &var: _plotVars)
    {
        if (var.used)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
            ImGui::TextUnformatted(var.label.c_str());
            ImGui::PopStyleColor();
            continue;
        }
        if (ImGui::Selectable(var.label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
        {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                // Set as x variable if that is not set yet
                if (!_plotVarX)
                {
                    _SetVarX(&var);
                }
                // Otherwise add as plot
                else
                {
                    _AddToPlot(&var);
                }
            }
        }
        Gui::ItemTooltip(var.tooltip.c_str());

        // This list is a drag and drop source for plot variables
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            DndPayload payload(std::addressof(var), nullptr);
            ImGui::SetDragDropPayload(_dndType, &payload, sizeof(payload));
            ImGui::TextUnformatted(var.label.c_str());
            ImGui::EndDragDropSource();
        }
    }
    ImGui::EndGroup();
    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload *imPayload = ImGui::AcceptDragDropPayload(_dndType);
        if (imPayload)
        {
            DndPayload payload(imPayload);
            if (payload.plotVar)
            {
                _RemoveFromPlot(payload.plotVar);
            }
        }
    }

    // Remove only now after drawing both lists in order to avoid visual glitches
    if (removeVar)
    {
        _RemoveFromPlot(removeVar);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataPlot::_DrawPlot()
{
    PlotVar *droppedVar = nullptr;
    enum : int { DD_PLOT = -2, DD_X = -1, DD_Y1 = ImPlotYAxis_1, DD_Y2 = ImPlotYAxis_2, DD_Y3 = ImPlotYAxis_3 };
    ImPlotYAxis droppedAxis = DD_PLOT;

    const ImPlotFlags hoverPlotFlags = _dndHovered ? ImPlotFlags_YAxis2 | ImPlotFlags_YAxis3 : ImPlotFlags_None;
    ImPlot::SetNextPlotLimitsX(0, 60, ImGuiCond_Once);
    ImPlot::SetNextPlotLimitsY(-10, +10, ImGuiCond_Once);
    const char *xLabel = _plotVarX ? _plotVarX->tooltip.c_str() : NULL;
    ImPlot::PushColormap(_colormap);
    if (ImPlot::BeginPlot(_PLOT_ID, xLabel, _yLabels[ImPlotYAxis_1].empty() ? NULL : _yLabels[ImPlotYAxis_1].data(),
        ImVec2(-1,-1), _plotFlags | hoverPlotFlags, 0, 0, 0, 0,
        _yLabels[ImPlotYAxis_2].empty() ? NULL : _yLabels[ImPlotYAxis_2].data(),
        _yLabels[ImPlotYAxis_3].empty() ? NULL : _yLabels[ImPlotYAxis_3].data()))
    {
        // Add data
        _database->BeginGetEpoch();
        const int numEpochs = _database->NumEpochs();

        for (auto &data: _plotData)
        {
            const char *label = data.plotVarY->label.c_str();
            ImPlot::SetPlotYAxis(data.axis);
            switch (data.type)
            {
                case PlotType::LINE:
                    ImPlot::PlotLineG(label, data.getter, std::addressof(data), numEpochs);
                    break;
                case PlotType::SCATTER:
                    ImPlot::PlotScatterG(label, data.getter, std::addressof(data), numEpochs);
                    break;
                case PlotType::STEP:
                    ImPlot::PlotStairsG(label, data.getter, std::addressof(data), numEpochs);
                    break;
                case PlotType::BAR:
                    ImPlot::PlotBarsG(label, data.getter, std::addressof(data), numEpochs, 0.5 * _epochPeriod);
                    break;
            }
            // Allow legend labels to be dragged and dropped
            if (ImPlot::BeginDragDropSourceItem(label))
            {
                DndPayload payload(data.plotVarY, nullptr);
                ImGui::SetDragDropPayload(_dndType, &payload, sizeof(payload));
                ImGui::TextUnformatted(label);
                ImGui::EndDragDropSource();
            }
        }
        _database->EndGetEpoch();

        // Detect hovering with a drag and drop thingy to display all y axis, which may not all be used
        // FIXME: Isn't there a ImPlot::IsPlotHoveredWithADragAndDropThingy()?
        if (ImGui::BeginDragDropTarget())
        {
            _dndHovered = true;
            ImGui::EndDragDropTarget();
        }
        else
        {
            _dndHovered = false;
        }

        // Detect drops on the plot
        if (ImPlot::BeginDragDropTarget())
        {
            const ImGuiPayload *imPayload = ImGui::AcceptDragDropPayload(_dndType);
            if (imPayload)
            {
                droppedVar = DndPayload(imPayload).plotVar;
                droppedAxis = DD_PLOT;
            }
            ImPlot::EndDragDropTarget();
        }
        // Detect drops on the x axis
        if (ImPlot::BeginDragDropTargetX())
        {
            const ImGuiPayload *imPayload = ImGui::AcceptDragDropPayload(_dndType);
            if (imPayload)
            {
                droppedVar = DndPayload(imPayload).plotVar;
                droppedAxis = DD_X;
            }
            ImPlot::EndDragDropTarget();
        }
        // Detect drops on the y axes
        for (ImPlotYAxis yAxis = ImPlotYAxis_1; yAxis <= ImPlotYAxis_3; yAxis++)
        {
            if (ImPlot::BeginDragDropTargetY(yAxis))
            {
                const ImGuiPayload *imPayload = ImGui::AcceptDragDropPayload(_dndType);
                if (imPayload)
                {
                    droppedVar = DndPayload(imPayload).plotVar;
                    droppedAxis = yAxis;
                }
                ImPlot::EndDragDropTarget();
                break;
            }
        }

        ImPlot::EndPlot();
    }
    ImPlot::PopColormap();

    // Handle drops
    if (droppedVar)
    {
        DEBUG("Dropped %s on %d", droppedVar->label.c_str(), droppedAxis);

        // Dropped on x-axis, either explicitly or no x-axis variable yet and dropped on plot
        if ( (droppedAxis == DD_X) || (!_plotVarX && (droppedAxis < DD_Y1)) )
        {
            _SetVarX(droppedVar);
        }

        // X-axis is known
        else if (_plotVarX)
        {
            // Dropped on plot -> use y-axis 1
            if (droppedAxis == DD_PLOT)
            {
                droppedAxis = DD_Y1;
            }

            // Dropped on a particular y-axis
            if ( (droppedAxis >= DD_Y1) && (droppedAxis <= DD_Y3) )
            {
                _AddToPlot(droppedVar, droppedAxis);
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataPlot::_SetVarX(PlotVar *plotVar)
{
    DEBUG("Set x %s (was %s)", plotVar->label.c_str(), _plotVarX ? _plotVarX->label.c_str() : "-");

    // Save for future plots
    _plotVarX = plotVar;

    // Update existing plots
    for (auto &data: _plotData)
    {
        DEBUG("Update <%s><%s> -> <%s><%s>", data.plotVarX->label.c_str(), data.plotVarY->label.c_str(), plotVar->label.c_str(), data.plotVarY->label.c_str());
        data.plotVarX = plotVar;
    }

    // Remove possible plot of this
    _RemoveFromPlot(plotVar);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataPlot::_AddToPlot(PlotVar *plotVar, const ImPlotYAxis yAxis)
{
    // Update existing plot data
    bool addNew = true;
    for (auto &data: _plotData)
    {
        if (data.plotVarY->label == plotVar->label)
        {
            DEBUG("Update <%s><%s> Y%d->%d", data.plotVarX->label.c_str(), data.plotVarY->label.c_str(), data.axis, yAxis);
            data.axis = yAxis;
            addNew = false;
            break;
        }
    }
    // Create new data pair
    if (addNew)
    {
        DEBUG("Add <%s><%s> Y%d", _plotVarX->label.c_str(), plotVar->label.c_str(), yAxis);
        auto data = PlotData(_plotVarX, plotVar, PlotType::LINE, _database.get());
        data.axis = yAxis;
        plotVar->used = true;
        _plotData.push_back(std::move(data));
    }

    _UpdatePlotFlags();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataPlot::_UpdatePlotFlags()
{
    // Update plot flags and y axis labels
    CLRBITS(_plotFlags, ImPlotFlags_YAxis2 | ImPlotFlags_YAxis3);
    _yLabels[ImPlotYAxis_1].clear();
    _yLabels[ImPlotYAxis_2].clear();
    _yLabels[ImPlotYAxis_3].clear();
    for (auto &data: _plotData)
    {
        switch (data.axis)
        {
            case ImPlotYAxis_1:
                if (!_yLabels[ImPlotYAxis_1].empty())
                {
                    _yLabels[ImPlotYAxis_1] += " / ";
                }
                _yLabels[ImPlotYAxis_1] += data.plotVarY->label;
                break;
            case ImPlotYAxis_2:
                if (!_yLabels[ImPlotYAxis_2].empty())
                {
                    _yLabels[ImPlotYAxis_2] += " / ";
                }
                _yLabels[ImPlotYAxis_2] += data.plotVarY->label;
                _plotFlags |= ImPlotFlags_YAxis2;
                break;
            case ImPlotYAxis_3:
                if (!_yLabels[ImPlotYAxis_3].empty())
                {
                    _yLabels[ImPlotYAxis_3] += " / ";
                }
                _yLabels[ImPlotYAxis_3] += data.plotVarY->label;
                _plotFlags |= ImPlotFlags_YAxis3;
                break;
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataPlot::_RemoveFromPlot(PlotVar *plotVar)
{
    for (auto iter = _plotData.begin(); iter != _plotData.end(); )
    {
        if (iter->plotVarY->label == plotVar->label)
        {
            DEBUG("Remove plot <%s><%s>", iter->plotVarX->label.c_str(), iter->plotVarY->label.c_str());
            plotVar->used = false;
            iter = _plotData.erase(iter);
        }
        else
        {
            iter++;
        }
    }
    _UpdatePlotFlags();
}

// ---------------------------------------------------------------------------------------------------------------------

void  GuiWinDataPlot::_SavePlots()
{
    std::vector<std::string> plots;
    for (auto &data: _plotData)
    {
        std::string plot = data.plotVarX->label;
        plot += "/";
        plot += data.plotVarY->label;
        plot += "/";
        plot += std::to_string(data.axis);
        plots.push_back(plot);
    }
    GuiSettings::SetValue(_winName + ".plots", Ff::StrJoin(plots, ","));
}

// ---------------------------------------------------------------------------------------------------------------------

void  GuiWinDataPlot::_LoadPlots()
{
    std::string plots;
    GuiSettings::GetValue(_winName + ".plots", plots, "");
    for (const auto &plot: Ff::StrSplit(plots, ","))
    {
        const auto parts = Ff::StrSplit(plot, "/"); // x var / y var / y axis
        if (parts.size() != 3)
        {
            continue;
        }
        PlotVar *xVar = _PlotVarByLabel(parts[0]);
        PlotVar *yVar = _PlotVarByLabel(parts[1]);
        std::size_t num;
        int yAxis = std::stol(parts[2], &num, 0);
        if (!xVar || !yVar || (num <= 0) || (yAxis < ImPlotYAxis_1) || (yAxis > ImPlotYAxis_3))
        {
            continue;
        }

        if (!_plotVarX)
        {
            _plotVarX = xVar;
        }
        _AddToPlot(yVar, yAxis);
    }
}

GuiWinDataPlot::PlotVar *GuiWinDataPlot::_PlotVarByLabel(const std::string label)
{
    for (auto &plotVar: _plotVars)
    {
        if (plotVar.label == label)
        {
            return std::addressof(plotVar);
        }
    }
    return nullptr;
}

/* ****************************************************************************************************************** */
