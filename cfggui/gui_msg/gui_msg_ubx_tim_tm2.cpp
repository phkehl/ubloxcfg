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
#include <algorithm>

#include "implot.h"

#include "ff_ubx.h"

#include "gui_inc.hpp"

#include "gui_msg_ubx_tim_tm2.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxTimTm2::GuiMsgUbxTimTm2(std::shared_ptr<InputReceiver> receiver, std::shared_ptr<InputLogfile> logfile) :
    GuiMsg(receiver, logfile),
    _lastRisingEdge    { NAN },
    _lastFallingEdge   { NAN },
    _periodRisingEdge  { NAN },
    _periodFallingEdge { NAN },
    _dutyCycle         { NAN }
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxTimTm2::Clear()
{
    _plotData.clear();
    _lastRisingEdge    = NAN;
    _lastFallingEdge   = NAN;
    _periodRisingEdge  = NAN;
    _periodFallingEdge = NAN;
    _dutyCycle         = NAN;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxTimTm2::Update(const std::shared_ptr<Ff::ParserMsg> &msg)
{
    if (msg->size != UBX_TIM_TM2_V0_SIZE)
    {
        return;
    }

    UBX_TIM_TM2_V0_GROUP0_t tm2;
    std::memcpy(&tm2, &msg->data[UBX_HEAD_SIZE], sizeof(tm2));

    const bool   newRisingEdge  = CHKBITS(tm2.flags, UBX_TIM_TM2_V0_FLAGS_NEWRISINGEDGE);
    const bool   newFallingEdge = CHKBITS(tm2.flags, UBX_TIM_TM2_V0_FLAGS_NEWFALLINGEDGE);
    const double towRisingEdge  = ((double)tm2.towMsR * UBX_TIM_TM2_V0_TOW_SCALE) + ((double)tm2.towSubMsR * UBX_TIM_TM2_V0_SUBMS_SCALE);
    const double towFallingEdge = ((double)tm2.towMsF * UBX_TIM_TM2_V0_TOW_SCALE) + ((double)tm2.towSubMsF * UBX_TIM_TM2_V0_SUBMS_SCALE);

    if (newRisingEdge)
    {
        if (!std::isnan(_lastRisingEdge))
        {
            _periodRisingEdge = towRisingEdge - _lastRisingEdge;
        }
        _lastRisingEdge = towRisingEdge;
    }
    if (newFallingEdge)
    {
        if (!std::isnan(_lastFallingEdge))
        {
            _periodFallingEdge = towFallingEdge - _lastFallingEdge;
        }
        _lastFallingEdge = towFallingEdge;

        if (!std::isnan(_lastRisingEdge) && !std::isnan(_periodRisingEdge) && (_periodRisingEdge > 1e-9))
        {
            _dutyCycle = (_lastFallingEdge - _lastRisingEdge) / _periodRisingEdge * 1e2;
        }
    }

    // edges can come in single messages or both in one message (in arbitrary order)
    if (newRisingEdge && newFallingEdge)
    {
        if (towRisingEdge < towFallingEdge)
        {
            _plotData.emplace_back(towRisingEdge,  true,  _periodRisingEdge, _periodFallingEdge, _dutyCycle);
            _plotData.emplace_back(towFallingEdge, false, _periodRisingEdge, _periodFallingEdge, _dutyCycle);
        }
        else
        {
            _plotData.emplace_back(towFallingEdge, false, _periodRisingEdge, _periodFallingEdge, _dutyCycle);
            _plotData.emplace_back(towRisingEdge,  true,  _periodRisingEdge, _periodFallingEdge, _dutyCycle);
        }
    }
    else if (newRisingEdge || newFallingEdge)
    {
        _plotData.emplace_back(newRisingEdge ? towRisingEdge : towFallingEdge, newRisingEdge,
            _periodRisingEdge, _periodFallingEdge, _dutyCycle);
    }

    while (_plotData.size() > MAX_PLOT_DATA)
    {
        _plotData.pop_front();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxTimTm2::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail)
{
    UNUSED(msg);
    UNUSED(sizeAvail);

    const float maxHeight = GuiSettings::charSize.y * 15.0f;

    if (ImPlot::BeginPlot("##plot", { sizeAvail.x, std::min(sizeAvail.y, maxHeight) },
            ImPlotFlags_Crosshairs | ImPlotFlags_NoFrame | ImPlotFlags_NoMenus))
    {
        ImPlot::SetupAxis(ImAxis_X1, "TOW [s]");

        ImPlot::SetupAxis(ImAxis_Y1, "Duty [%]", ImPlotAxisFlags_NoHighlight);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -5.0, 105.0, ImPlotCond_Always);

        ImPlot::SetupAxis(ImAxis_Y2, "Period [s]", ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoGridLines);
        ImPlot::SetupAxisLimits(ImAxis_Y2, 0.0, 2.5, ImPlotCond_Once);

        ImPlot::SetupLegend(ImPlotLocation_North, ImPlotLegendFlags_Horizontal | ImPlotLegendFlags_Outside);

        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
        ImPlot::PlotLineG("Period rising", [](int ix, void *arg)
            {
                const PlotData &pd = ((const std::deque<PlotData> *)arg)->at(ix);
                return ImPlotPoint(pd.tow_, pd.pRising_);
            }, &_plotData, _plotData.size());
        ImPlot::PlotLineG("Period falling", [](int ix, void *arg)
            {
                const PlotData &pd = ((const std::deque<PlotData> *)arg)->at(ix);
                return ImPlotPoint(pd.tow_, pd.pFalling_);
            }, &_plotData, _plotData.size());

        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
        ImPlot::PlotLineG("Duty cycle", [](int ix, void *arg)
            {
                const PlotData &pd = ((const std::deque<PlotData> *)arg)->at(ix);
                return ImPlotPoint(pd.tow_, pd.dutyCycle_);
            }, &_plotData, _plotData.size());
        ImPlot::PlotStairsG("Signal", [](int ix, void *arg)
            {
                const PlotData &pd = ((const std::deque<PlotData> *)arg)->at(ix);
                return ImPlotPoint(pd.tow_, pd.signal_ ? 100.0 : 0.0);
            }, &_plotData, _plotData.size());

        ImPlot::EndPlot();
    }

    return true;
}

/* ****************************************************************************************************************** */
