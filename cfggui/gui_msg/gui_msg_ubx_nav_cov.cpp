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

#include <cstring>
#include <cmath>

#include "ff_ubx.h"

#include "gui_inc.hpp"

#include "gui_msg_ubx_nav_cov.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxNavCov::GuiMsgUbxNavCov(std::shared_ptr<InputReceiver> receiver, std::shared_ptr<InputLogfile> logfile) :
    GuiMsg(receiver, logfile)
{
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxNavCov::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail)
{
    UNUSED(sizeAvail);
    if ( (UBX_NAV_COV_VERSION_GET(msg->data) != UBX_NAV_COV_V0_VERSION) || (msg->size != UBX_NAV_COV_V0_SIZE) )
    {
        return false;
    }

    UBX_NAV_COV_V0_GROUP0_t cov;
    std::memcpy(&cov, &msg->data[UBX_HEAD_SIZE], sizeof(cov));

    const float valuesPos[] =
    {
        cov.posCovNN, cov.posCovNE, cov.posCovND,
        cov.posCovNE, cov.posCovEE, cov.posCovED,
        cov.posCovND, cov.posCovED, cov.posCovDD,
    };

    const float valuesVel[] =
    {
        cov.velCovNN, cov.velCovNE, cov.velCovND,
        cov.velCovNE, cov.velCovEE, cov.velCovED,
        cov.velCovND, cov.velCovED, cov.velCovDD,
    };

    const float valuesFail[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

    const ImPlotAxisFlags axesFlags  = ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks;
    const ImPlotFlags     plotFlags  = ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus |
                                       ImPlotFlags_NoBoxSelect | ImPlotFlags_NoFrame;
    const char * const    xyLabels[] = { "N", "E", "D" };
    const float           scaleMin   = 0; //-1e-3 * 1e-3; // mm
    const float           scaleMax   = 0; //1e3 * 1e3;    // km
    const ImPlotColormap  colMap     = ImPlotColormap_Cool; // ImPlotColormap_Plasma
    const char           *numFmt     = "%.1e";
    const float           plotWidth  = 0.5f * (sizeAvail.x - GuiSettings::style->ItemSpacing.x);
    const ImVec2          plotSize   { MIN(plotWidth, sizeAvail.y), MIN(plotWidth, sizeAvail.y) };

    ImPlot::PushColormap(colMap);

    if (ImPlot::BeginPlot("Position NED [m^2]##pos", plotSize, plotFlags))
    {
        ImPlot::SetupAxisTicks(ImAxis_X1, 0 + 1.0/6.0, 1 - 1.0/6.0, 3, xyLabels);
        ImPlot::SetupAxisTicks(ImAxis_Y1, 1 - 1.0/6.0, 0 + 1.0/6.0, 3, xyLabels);
        ImPlot::SetupAxis(ImAxis_X1, nullptr, axesFlags);
        ImPlot::SetupAxis(ImAxis_Y1, nullptr, axesFlags);
        ImPlot::SetupFinish();
        ImPlot::PlotHeatmap("heat", cov.posCovValid ? valuesPos : valuesFail, 3, 3, scaleMin, scaleMax, numFmt);
        ImPlot::EndPlot();
    }

    ImGui::SameLine();

    ImPlot::PushColormap(colMap);
    if (ImPlot::BeginPlot("Velocity NED [m^2/s^2]##vel", plotSize, plotFlags))
    {
        ImPlot::SetupAxisTicks(ImAxis_X1, 0 + 1.0/6.0, 1 - 1.0/6.0, 3, xyLabels);
        ImPlot::SetupAxisTicks(ImAxis_Y1, 1 - 1.0/6.0, 0 + 1.0/6.0, 3, xyLabels);
        ImPlot::SetupAxis(ImAxis_X1, nullptr, axesFlags);
        ImPlot::SetupAxis(ImAxis_Y1, nullptr, axesFlags);
        ImPlot::SetupFinish();
        ImPlot::PlotHeatmap("heat", cov.velCovValid ? valuesVel : valuesFail, 3, 3, scaleMin, scaleMax, numFmt);
        ImPlot::EndPlot();
    }

    return true;
}

/* ****************************************************************************************************************** */
