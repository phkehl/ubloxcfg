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

#include <cinttypes>

#include "ff_trafo.h"

#include "gui_inc.hpp"
#include "implot.h"

#include "gui_win_data_plot.hpp"

/* ****************************************************************************************************************** */

/*static*/ const ImPlotRect GuiWinDataPlot::PLOT_LIMITS_DEF = { 0.0f, 100.0f, 0.0f, 10.0f };

GuiWinDataPlot::GuiWinDataPlot(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database),
    _colormap      { ImPlotColormap_Deep },
    _axisUsed      { false, false, false, false, false, false },
    _fitMode       { FitMode::NONE },
    _plotLimitsY1  { PLOT_LIMITS_DEF },
    _plotLimitsY2  { PLOT_LIMITS_DEF },
    _plotLimitsY3  { PLOT_LIMITS_DEF },
    _followXmax    { 0.0 },
    _setLimitsX    { true },
    _setLimitsY    { true },
    _plotTitleId   { "##" + _winName },
    _showLegend    { true }
{
    _winSize = { 80, 25 };

    std::snprintf(_dndType, sizeof(_dndType), "PlotVar_%016" PRIx64, _winUid & 0xffffffffffffffff);

    _LoadParams();
}

GuiWinDataPlot::~GuiWinDataPlot()
{
    _SaveParams();
}

// ---------------------------------------------------------------------------------------------------------------------

// void GuiWinDataPlot::_ClearData()
// {
// }

// ---------------------------------------------------------------------------------------------------------------------

// void GuiWinDataPlot::_ProcessData(const InputData &data)
// {
//     if (data.type == InputData::DATA_EPOCH)
//     {
//
//     }
// }

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataPlot::_Loop(const uint32_t &frame, const double &now)
{
    UNUSED(frame);
    UNUSED(now);

    if (!_database->Changed(this) || !_xVar)
    {
        return;
    }

    // Find most recent x value
    double xMax = NAN;
    const Database::FieldIx fieldIx = _xVar->field.field;
    _database->ProcRows([&](const Database::Row &row)
    {
        const double xValue = row[fieldIx];
        if (!std::isnan(xValue))
        {
            xMax = xValue;
            return false;
        }
        return true;
    }, true);

    if (!std::isnan(xMax))
    {
        // Update x axis range
        if ((_fitMode == FitMode::FOLLOW_X) && _xVar)
        {
            if (!std::isnan(_followXmax))
            {
                const double delta = xMax - _followXmax;
                if (delta != 0.0) {
                    _plotLimitsY1.X.Max += delta;
                    _plotLimitsY1.X.Min += delta;
                    _setLimitsX = true;
                }
            }
            _setLimitsX = true;
        }

        _followXmax = xMax;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataPlot::_DrawToolbar()
{
    Gui::VerticalSeparator();

    // Add variables to plot
    if (ImGui::Button(ICON_FK_PLUS "##AddVar", GuiSettings::iconSize))
    {
        ImGui::OpenPopup("AddVarMenu");
    }
    Gui::ItemTooltip("Add variables to plot.\nDouble-click items in this list or drag them to the plot or axes.");
    if (ImGui::IsPopupOpen("AddVarMenu"))
    {
        ImGui::SetNextWindowSize(ImVec2(-1, GuiSettings::lineHeight * 25));
        if (ImGui::BeginPopup("AddVarMenu"))
        {
            Gui::BeginMenuPersist();
            for (auto &field: Database::FIELDS)
            {
                // Skip entries without a title, they are not intended to be plotted
                if (!field.label) {
                    continue;
                }

                // Plot variable entry with tooltip
                const bool used = _usedFields[field.name];
                if (used) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
                if (ImGui::Selectable(field.label, false, ImGuiSelectableFlags_AllowDoubleClick)) {
                    // On double click, act like it was dragged onto the plot
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        _HandleDrop({ field, DragDrop::LIST, DragDrop::PLOT });
                    }
                }
                if (used) { ImGui::PopStyleColor(); }
                Gui::ItemTooltip(field.label);

                // Entry is a drag and drop source
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    DragDrop dragdrop(field, DragDrop::LIST);
                    ImGui::SetDragDropPayload(_dndType, &dragdrop, sizeof(dragdrop));
                    ImGui::TextUnformatted(field.label);
                    ImGui::EndDragDropSource();
                }
            }
            Gui::EndMenuPersist();
            ImGui::EndPopup();
        }
    }

    ImGui::SameLine();

    // Reset plot
    ImGui::BeginDisabled(!_xVar && _yVars.empty());
    if (ImGui::Button(ICON_FK_TRASH_O "##ResetPlot", GuiSettings::iconSize))
    {
        ImGui::OpenPopup("ResetPlot");
    }
    ImGui::EndDisabled();
    Gui::ItemTooltip("Reset plot (remove all vars, ...)");
    if (ImGui::IsPopupOpen("ResetPlot"))
    {
        if (ImGui::BeginPopup("ResetPlot"))
        {
            if (ImGui::Button("Reset plot"))
            {
                _xVar = nullptr;
                _yVars.clear();
                _plotLimitsY1 = PLOT_LIMITS_DEF;
                _plotLimitsY2 = PLOT_LIMITS_DEF;
                _plotLimitsY3 = PLOT_LIMITS_DEF;
                _setLimitsX = true;
                _setLimitsY = true;
                _UpdateVars();
            }
            ImGui::EndPopup();
        }
    }

    Gui::VerticalSeparator();

    // Colormap
    if (ImGui::Button(ICON_FK_PAINT_BRUSH "###Colormap", GuiSettings::iconSize))
    {
        ImGui::OpenPopup("ChooseColourmap");
    }
    Gui::ItemTooltip("Colours");
    if (ImGui::IsPopupOpen("ChooseColourmap"))
    {
        if (ImGui::BeginPopup("ChooseColourmap"))
        {
            Gui::BeginMenuPersist();
            for (ImPlotColormap map = 0; map < ImPlot::GetColormapCount(); map++)
            {
                if (ImPlot::ColormapButton(ImPlot::GetColormapName(map),ImVec2(225,0),map))
                {
                    _colormap = map;
                     ImPlot::BustColorCache(/*_plotTitleId.c_str()*/); // FIXME busting only our plot doesn't work
                }
            }

            Gui::EndMenuPersist();
            ImGui::EndPopup();
        }
    }

    Gui::VerticalSeparator();

    // Fit mode
    const bool fitFollowX = (_fitMode == FitMode::FOLLOW_X);
    const bool fitAutofit = (_fitMode == FitMode::AUTOFIT);
    // - Follow X
    if (!fitFollowX) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
    if (ImGui::Button(ICON_FK_ARROW_RIGHT "##FollowX", GuiSettings::iconSize))
    {
        _fitMode = (fitFollowX ? FitMode::NONE : FitMode::FOLLOW_X);
    }
    if (!fitFollowX) { ImGui::PopStyleColor(); }
    Gui::ItemTooltip("Follow x (automatically move x axis)");
    ImGui::SameLine();
    // - Autofit
    if (!fitAutofit) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
    if (ImGui::Button(ICON_FK_ARROWS_ALT "##Autofit", GuiSettings::iconSize))
    {
        _fitMode = (fitAutofit ? FitMode::NONE : FitMode::AUTOFIT);
    }
    if (!fitAutofit) { ImGui::PopStyleColor(); }
    Gui::ItemTooltip("Automatically fit axes ranges to data");

    ImGui::SameLine();

    // Toggle legend
    Gui::ToggleButton(ICON_FK_LIST "##ToggleLegend", nullptr, &_showLegend, "Toggle legend", nullptr, GuiSettings::iconSize);


    // ImGui::SameLine();
    // ImGui::Text("x1: %.1f %.1f, y1: %.1f %.1f, y2: %.1f %.1f, y3: %.1f %.1f, xMax %.1f, color %d", _plotLimitsY1.X.Min, _plotLimitsY1.X.Max,
    //     _plotLimitsY1.Y.Min, _plotLimitsY1.Y.Max, _plotLimitsY2.Y.Min, _plotLimitsY2.Y.Max, _plotLimitsY3.Y.Min, _plotLimitsY3.Y.Max, _followXmax, _colormap);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataPlot::_DrawContent()
{
    std::unique_ptr<DragDrop> dragdrop;

    // Wrapper so that we can detect drop target on the entire content
    ImGui::BeginChild("PlotWrapper");

    // Plot
    ImPlot::PushColormap(_colormap);
    switch (_fitMode)
    {
        case FitMode::NONE:
            break;
        case FitMode::AUTOFIT:
            ImPlot::SetNextAxesToFit();
            break;
        case FitMode::FOLLOW_X:
            break;
    }
    ImPlotFlags plotFlags = ImPlotFlags_Crosshairs | ImPlotFlags_NoFrame /*| ImPlotFlags_NoMenus*/;
    if (!_showLegend) { plotFlags |= ImPlotFlags_NoLegend; }
    if (ImPlot::BeginPlot(_plotTitleId.c_str(), ImVec2(-1,-1), plotFlags))
    {
        // X axis
        ImPlot::SetupAxis(ImAxis_X1, _axisLabels[ImAxis_X1].c_str());
        ImPlot::SetupAxisLimits(ImAxis_X1, _plotLimitsY1.X.Min, _plotLimitsY1.X.Max, _setLimitsX ? ImPlotCond_Always: ImPlotCond_None);
        // Y axes
        const bool showY1 = _axisUsed[ImAxis_Y1] || (_dndHovered && _axisUsed[ImAxis_X1]) || _setLimitsY;
        const bool showY2 = _axisUsed[ImAxis_Y2] || (_dndHovered && _axisUsed[ImAxis_X1]) || _setLimitsY;
        const bool showY3 = _axisUsed[ImAxis_Y3] || (_dndHovered && _axisUsed[ImAxis_X1]) || _setLimitsY;
        ImPlot::SetupAxis(ImAxis_Y1, _axisLabels[ImAxis_Y1].c_str());
        ImPlot::SetupAxisLimits(ImAxis_Y1, _plotLimitsY1.Y.Min, _plotLimitsY1.Y.Max, _setLimitsY ? ImPlotCond_Always: ImPlotCond_None);
        if (showY2)
        {
            ImPlot::SetupAxis(ImAxis_Y2, _axisLabels[ImAxis_Y2].c_str(), ImPlotAxisFlags_AuxDefault);
            ImPlot::SetupAxisLimits(ImAxis_Y2, _plotLimitsY2.Y.Min, _plotLimitsY2.Y.Max, _setLimitsY ? ImPlotCond_Always: ImPlotCond_None);
        }
        if (showY3)
        {
            ImPlot::SetupAxis(ImAxis_Y3, _axisLabels[ImAxis_Y3].c_str(), ImPlotAxisFlags_AuxDefault);
            ImPlot::SetupAxisLimits(ImAxis_Y3, _plotLimitsY3.Y.Min, _plotLimitsY3.Y.Max, _setLimitsY ? ImPlotCond_Always: ImPlotCond_None);
        }
        ImPlot::SetupFinish();
        _setLimitsX = false;
        _setLimitsY = false;

        // Plot data
        if (_xVar && !_yVars.empty())
        {
            _database->BeginGetRows();

            struct Helper { Database::FieldIx xIx; Database::FieldIx yIx; Database *db; } helper =
                { _xVar->field.field, Database::FieldIx::ix_none, _database.get() };
            for (const auto &yVar: _yVars)
            {
                // Plot
                helper.yIx = yVar.field.field;
                ImPlot::SetAxes(_xVar->axis, yVar.axis);
                ImPlot::PlotLineG(yVar.field.label, [](int ix, void *arg) {
                    Helper *h = (Helper *)arg;
                    const Database &db = *h->db;
                    return ImPlotPoint(db[ix][h->xIx], db[ix][h->yIx]);
                }, &helper, _database->Size());

                if (ImPlot::BeginLegendPopup(yVar.field.label, ImGuiMouseButton_Right))
                {
                    if (ImGui::Button("Remove from plot"))
                    {
                        dragdrop = std::make_unique<DragDrop>(yVar.field, DragDrop::LEGEND, DragDrop::TRASH);
                        ImGui::CloseCurrentPopup();
                    }
                    ImPlot::EndLegendPopup();
                }

                // Allow legend labels to be dragged and dropped
                if (ImPlot::BeginDragDropSourceItem(yVar.field.label))
                {
                    DragDrop dd(yVar.field, DragDrop::LEGEND);
                    ImGui::SetDragDropPayload(_dndType, &dd, sizeof(dd));
                    ImPlot::ItemIcon(ImPlot::GetLastItemColor());
                    ImGui::SameLine();
                    ImGui::TextUnformatted(yVar.field.label);
                    ImGui::EndDragDropSource();
                }
            }

            _database->EndGetRows();
        }

        _plotLimitsY1 = ImPlot::GetPlotLimits(ImAxis_X1, ImAxis_Y1);
        _plotLimitsY2 = ImPlot::GetPlotLimits(ImAxis_X1, ImAxis_Y2);
        _plotLimitsY3 = ImPlot::GetPlotLimits(ImAxis_X1, ImAxis_Y3);

        // Drop on plot (only if free axis available)
        if (!dragdrop && (!_axisUsed[ImAxis_X1] || !_axisUsed[ImAxis_Y1] || !_axisUsed[ImAxis_Y2] || !_axisUsed[ImAxis_Y3]) &&
             ImPlot::BeginDragDropTargetPlot())
        {
            const ImGuiPayload *imPayload = ImGui::AcceptDragDropPayload(_dndType);
            if (imPayload)
            {
                dragdrop = std::make_unique<DragDrop>(imPayload, DragDrop::PLOT);
            }
            ImPlot::EndDragDropTarget();
        }
        // Drop on X1 axis
        if (!dragdrop) { dragdrop = _CheckDrop(ImAxis_X1); }
        // Drop on Y axes
        if (!dragdrop && showY1) { dragdrop = _CheckDrop(ImAxis_Y1); }
        if (!dragdrop && showY2) { dragdrop = _CheckDrop(ImAxis_Y2); }
        if (!dragdrop && showY3) { dragdrop = _CheckDrop(ImAxis_Y3); }

        // Axis tooltips
        if (showY1 && !_axisTooltips[ImAxis_Y1].empty() && ImPlot::IsAxisHovered(ImAxis_Y1))
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(_axisTooltips[ImAxis_Y1].c_str());
            ImGui::EndTooltip();
        }
        if (showY2 && !_axisTooltips[ImAxis_Y2].empty() && ImPlot::IsAxisHovered(ImAxis_Y2))
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(_axisTooltips[ImAxis_Y2].c_str());
            ImGui::EndTooltip();
        }
        if (showY1 && !_axisTooltips[ImAxis_Y3].empty() && ImPlot::IsAxisHovered(ImAxis_Y3))
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(_axisTooltips[ImAxis_Y2].c_str());
            ImGui::EndTooltip();
        }

        ImPlot::EndPlot();
    }
    ImPlot::PopColormap();

    // Trash
    if (_dndHovered)
    {
        const auto pad = FfVec2f(GuiSettings::style->WindowPadding) * 2.0f;
        const auto size = FfVec2f(GuiSettings::iconSize) * 2.0f;
        ImGui::SetCursorPos(FfVec2f(ImGui::GetWindowSize()) - size - pad);
        ImGui::Button(ICON_FK_TRASH_O "##Trash", size);
        if (ImGui::BeginDragDropTarget())
        {
            const ImGuiPayload *imPayload = ImGui::AcceptDragDropPayload(_dndType);
            if (imPayload)
            {
                dragdrop = std::make_unique<DragDrop>(imPayload, DragDrop::TRASH);
            }
            ImGui::EndDragDropTarget();
        }
    }

    // End wrapper
    ImGui::EndChild();

    // Detect hovering with a drag and drop thingy to display all axes to reveal them as dnd targets
    if (ImGui::BeginDragDropTarget())
    {
        _dndHovered = true;
        ImGui::EndDragDropTarget();
    }
    else
    {
        _dndHovered = false;
    }

    // Handle drops (*after* plotting!)
    if (dragdrop)
    {
        _HandleDrop(*dragdrop);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataPlot::DragDrop::DragDrop(const Database::FieldDef &_field, Target _src) :
    src    { _src },
    dst    { UNKNOWN },
    field  { _field }
{
}

GuiWinDataPlot::DragDrop::DragDrop(const Database::FieldDef &_field, const Target _src, const Target _dst) :
    src    { _src },
    dst    { _dst },
    field  { _field }
{
}

GuiWinDataPlot::DragDrop::DragDrop(const ImGuiPayload *imPayload, const Target _dst) :
    src    { ((DragDrop *)imPayload->Data)->src },
    dst    { _dst },
    field  { ((DragDrop *)imPayload->Data)->field }
{
}

// ---------------------------------------------------------------------------------------------------------------------

std::unique_ptr<GuiWinDataPlot::DragDrop> GuiWinDataPlot::_CheckDrop(const ImAxis axis)
{
    std::unique_ptr<DragDrop> res;
    if (ImPlot::BeginDragDropTargetAxis(axis))
    {
        const ImGuiPayload *imPayload = ImGui::AcceptDragDropPayload(_dndType);
        if (imPayload)
        {
            res = std::make_unique<DragDrop>(imPayload, (DragDrop::Target)axis);
        }
        ImPlot::EndDragDropTarget();
    }
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------


void GuiWinDataPlot::_HandleDrop(const DragDrop &dragdrop)
{
    //DEBUG("dragdrop %d -> %d", dragdrop.src, dragdrop.dst);

    DragDrop::Target dst = dragdrop.dst;

    // Drops on plot go to an axis if possible
    if (dst == DragDrop::PLOT)
    {
        // No x-axis yet -> set as x axis
        if      (!_axisUsed[ImAxis_X1]) { dst = DragDrop::X; }
        // Have x-axis -> use next free y axis
        else if (!_axisUsed[ImAxis_Y1]) { dst = DragDrop::Y1; }
        else if (!_axisUsed[ImAxis_Y2]) { dst = DragDrop::Y2; }
        else if (!_axisUsed[ImAxis_Y3]) { dst = DragDrop::Y3; }
        // Invalid drop
        else
        {
            return;
        }
    }

    // Remove (only y vars), also for drag from legend
    if ( (dst == DragDrop::TRASH) || (dragdrop.src == DragDrop::LEGEND) )
    {
        for (auto iter = _yVars.begin(); iter != _yVars.end(); )
        {
            auto &yVar = *iter;
            if (yVar.field.name == dragdrop.field.name)
            {
                iter = _yVars.erase(iter);
            }
            else
            {
                iter++;
            }
        }
    }

    // Drop on X axis
    if (dst == DragDrop::X)
    {
        _xVar = std::make_unique<PlotVar>(dst, dragdrop.field);
    }
    // Drop on Y axis
    else if ((dst >= DragDrop::Y1) && (dst <= DragDrop::Y3))
    {
        _yVars.emplace_back(dst, dragdrop.field);
    }
    // else: Invalid

    _UpdateVars();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataPlot::_UpdateVars()
{
    _usedFields.clear();
    if (_xVar)
    {
        _axisLabels[ImAxis_X1] = _xVar->field.label;
        _usedFields[_xVar->field.name] = true;
        _axisUsed[ImAxis_X1] = true;
    }
    else
    {
        _axisLabels[ImAxis_X1].clear();
        _axisUsed[ImAxis_X1] = false;
    }

    for (ImAxis axis = ImAxis_Y1; axis <= ImAxis_Y3; axis++)
    {
        _axisUsed[axis] = false;
        _axisLabels[axis].clear();
        _axisTooltips[axis].clear();

        // Find relevant variables
        std::vector<const Database::FieldDef *> fieldDefs;
        for (const auto &var: _yVars)
        {
            if (var.axis == axis)
            {
                fieldDefs.push_back(std::addressof(var.field));
                _axisUsed[axis] = true;
                _usedFields[var.field.name] = true;
            }
        }

        // Set label, use short variable names resp. full variable label depending on the number of vars
        if (fieldDefs.size() > 1)
        {
            std::string label;
            std::string tooltip;
            for (auto fd: fieldDefs)
            {
                label += " / ";
                label += fd->name;
                tooltip += "\n";
                tooltip += fd->label;
            }
            _axisLabels[axis] = label.substr(3);
            _axisTooltips[axis] = tooltip.substr(1);
        }
        else if (fieldDefs.size() > 0)
        {
            _axisLabels[axis] = fieldDefs[0]->label;
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void  GuiWinDataPlot::_SaveParams()
{
    json params = { { "vars", json::array() } };
    if (_xVar)
    {
        params["vars"].push_back({ { "field", _xVar->field.name }, { "axis", _xVar->axis } });
        for (const auto &yVar: _yVars)
        {
            params["vars"].push_back({ { "field", yVar.field.name }, { "axis", yVar.axis } });
        }
    }
    params["limits"] = { _plotLimitsY1.X.Min, _plotLimitsY1.X.Max, _plotLimitsY1.Y.Min, _plotLimitsY1.Y.Max,
        _plotLimitsY2.Y.Min, _plotLimitsY2.Y.Max, _plotLimitsY3.Y.Min, _plotLimitsY3.Y.Max };
    params["colormap"] = _colormap;
    params["legend"] = _showLegend;

    GuiSettings::SetJson(_winName, params);
}

// ---------------------------------------------------------------------------------------------------------------------

void  GuiWinDataPlot::_LoadParams()
{
    const auto params = GuiSettings::GetJson(_winName);
    if (!params.is_object())
    {
        DEBUG("params not ok");
        return;
    }

    for (const auto &var: params["vars"])
    {
        const std::string field = var["field"];
        const int axis = var["axis"];

        // Check field
        bool haveField = false;
        Database::FieldDef fieldDef;
        for (const auto &candDef: Database::FIELDS)
        {
            if (candDef.name == field)
            {
                fieldDef = candDef;
                haveField = true;
                break;
            }
        }
        if (!haveField)
        {
            continue;
        }

        // First need x variable
        if (!_xVar)
        {
            if (axis != ImAxis_X1)
            {
                continue;
            }
            _xVar = std::make_unique<PlotVar>(axis, fieldDef);
            continue;
        }

        // Then can add y variables
        _yVars.emplace_back(axis, fieldDef);
    }

    const auto &limits = params["limits"];
    _plotLimitsY1.X.Min = limits[0];
    _plotLimitsY1.X.Max = limits[1];
    _plotLimitsY1.Y.Min = limits[2];
    _plotLimitsY1.Y.Max = limits[3];
    _plotLimitsY2.Y.Min = limits[4];
    _plotLimitsY2.Y.Max = limits[5];
    _plotLimitsY3.Y.Min = limits[6];
    _plotLimitsY3.Y.Max = limits[7];

    _colormap = params["colormap"];
    _showLegend = params["legend"];

    _UpdateVars();
}

/* ****************************************************************************************************************** */
