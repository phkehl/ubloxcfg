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

#include <cstring>

#include "ff_ubx.h"

#include "imgui.h"
#include "implot.h"
#include "IconsForkAwesome.h"

#include "gui_widget.hpp"

#include "gui_msg_ubx_mon_span.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxMonSpan::GuiMsgUbxMonSpan(std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile) :
    GuiMsg(receiver, logfile),
    _plotFlags{ImPlotFlags_AntiAliased | ImPlotFlags_Crosshairs}, _labels{}, _resetPlotRange{true}
{
    _labels.push_back(Label(1575.420000, "GPS/SBAS L1CA, GAL E1, QZSS L1CA/S"));
    _labels.push_back(Label(1561.098000, "BDS B1I"));
    _labels.push_back(Label(1602.000000, "GLO L1OF"));
    _labels.push_back(Label(1207.140000, "BDS B2I, GAL E5B"));
    _labels.push_back(Label(1227.600000, "GPS/QZSS L2C"));
    _labels.push_back(Label(1246.000000, "GLO L2OF"));
}

// ---------------------------------------------------------------------------------------------------------------------

GuiMsgUbxMonSpan::Label::Label(const double _freq, const char *_title) :
    freq{_freq}, title{_title}
{
    id = Ff::Sprintf("##%p", this);
}

// ---------------------------------------------------------------------------------------------------------------------

GuiMsgUbxMonSpan::SpectData::SpectData() :
    valid{false}, freq{}, ampl{}, min{}, max{}, mean{}, center{0.0}, span{0.0}, res{0.0}, pga{0.0}, count{0.0}
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxMonSpan::Clear()
{
    _spects.clear();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxMonSpan::Update(const std::shared_ptr<Ff::ParserMsg> &msg)
{
    if ( (UBX_MON_SPAN_VERSION_GET(msg->data) != UBX_MON_SPAN_V0_VERSION) || (msg->size < UBX_MON_SPAN_V0_SIZE(msg->data)) )
    {
        return;
    }

    UBX_MON_SPAN_V0_GROUP0_t head;
    std::memcpy(&head, &msg->data[UBX_HEAD_SIZE], sizeof(head));
    const int numRfBlocks = CLIP(head.numRfBlocks, 0, 5);
    _spects.resize(numRfBlocks);

    for (int spectIx = 0, offs = UBX_HEAD_SIZE + (int)sizeof(UBX_MON_SPAN_V0_GROUP0_t); spectIx < (int)numRfBlocks;
            spectIx++, offs += (int)sizeof(UBX_MON_SPAN_V0_GROUP1_t))
    {
        UBX_MON_SPAN_V0_GROUP1_t block;
        std::memcpy(&block, &msg->data[offs], sizeof(block));
        const int numBins = NUMOF(block.spectrum);
        auto &s = _spects[spectIx];
        s.freq.resize(numBins, NAN);
        s.ampl.resize(numBins, NAN);
        s.min.resize(numBins, NAN);
        s.max.resize(numBins, NAN);
        s.mean.resize(numBins, NAN);
        s.center = (double)block.center * 1e-6; // [MHz]
        s.span   = (double)block.span   * 1e-6; // [MHz]
        s.res    = (double)block.res    * 1e-6; // [MHz]
        s.pga    = (double)block.pga;
        for (int binIx = 0; binIx < numBins; binIx++)
        {
            s.freq[binIx] = (UBX_MON_SPAN_BIN_CENT_FREQ(block, binIx) * 1e-6) - s.center; // [MHz]
            const double ampl = (double)block.spectrum[binIx] * (100.0/255.0); // [%]
            s.ampl[binIx] = ampl;
            if (std::isnan(s.min[binIx]) || (ampl < s.min[binIx]))
            {
                s.min[binIx] = ampl;
            }
            if (std::isnan(s.max[binIx]) || (ampl > s.max[binIx]))
            {
                s.max[binIx] = ampl;
            }
            if (s.count > 0.0)
            {
                s.mean[binIx] += (ampl - s.mean[binIx]) / s.count;
            }
            else
            {
                s.mean[binIx] = ampl;
            }
        }
        s.count += 1.0;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxMonSpan::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const ImVec2 &sizeAvail)
{
    UNUSED(msg);
    if (!ImGui::BeginChild("##spectra", sizeAvail))
    {
        return true;
    }

    // Controls
    {
        if (ImGui::Button(ICON_FK_ARROWS_ALT "###ResetPlotRange"))
        {
            _resetPlotRange = true;
        }
        Gui::ItemTooltip("Reset plot range");

        ImGui::SameLine();

        // Clear
        if (ImGui::Button(ICON_FK_ERASER "##Clear", _winSettings->iconButtonSize))
        {
            Clear();
        }
        Gui::ItemTooltip("Clear all data");
    }

    // Plots
    if (!_spects.empty())
    {
        const int numSpects = _spects.size();
        const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        const float plotHeight = (canvasSize.y - ((numSpects - 1) * _winSettings->style.ItemSpacing.y)) / numSpects;
        for (int spectIx = 0; spectIx < numSpects; spectIx++)
        {
            auto &s = _spects[spectIx];
            char title[100];
            std::snprintf(title, sizeof(title), "RF%d (PGA %.0fdB)##%p", spectIx, s.pga, std::addressof(s));
            char xlabel[100];
            std::snprintf(xlabel, sizeof(xlabel), "Frequency (center %.6fMHz)", s.center);

            if (_resetPlotRange)
            {
                ImPlot::SetNextPlotLimitsX(-0.5 * s.span, 0.5 * s.span, ImGuiCond_Always);
                ImPlot::SetNextPlotLimitsY(1.0, 99.0, ImGuiCond_Always);
            }

            if (ImPlot::BeginPlot(title, xlabel, "Amplitude", ImVec2(-1, plotHeight), _plotFlags))
            {
                // Labels
                const ImPlotLimits limits = ImPlot::GetPlotLimits();
                for (auto &label: _labels)
                {
                    if (std::fabs(s.center - label.freq) < s.span)
                    {
                        double freq = s.center - label.freq;
                        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(0));
                        ImPlot::PlotVLines(label.id.c_str(), &freq, 1);
                        ImPlot::PopStyleColor();

                        const ImVec2 offs = ImGui::CalcTextSize(label.title.c_str());
                        ImPlot::PushStyleColor(ImPlotCol_InlayText, ImPlot::GetColormapColor(0));
                        ImPlot::PlotText(label.title.c_str(), freq, limits.Y.Min, true, ImVec2(offs.y, (-0.5f * offs.x) - 5.0f));
                        ImPlot::PopStyleColor();
                    }
                }

                // Aplitude with min/max
                ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(1));
                ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.3f);
                ImPlot::PlotShaded("##Amplitude", s.freq.data(), s.max.data(), s.min.data(), s.freq.size());
                ImPlot::PlotLine("##Amplitude", s.freq.data(), s.ampl.data(), s.freq.size());
                ImPlot::PopStyleVar();
                ImPlot::PopStyleColor();
                // Mean, initially hidden
                ImPlot::HideNextItem();
                ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(2));
                ImPlot::PlotLine("Mean", s.freq.data(), s.mean.data(), s.freq.size());
                ImPlot::PopStyleColor();

                ImPlot::EndPlot();
            }
        }
        _resetPlotRange = false;
    }
    // Dummy plots if no data is available
    else
    {
        const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        const float plotHeight = (canvasSize.y - _winSettings->style.ItemSpacing.y) / 2;
        if (ImPlot::BeginPlot("RF0", "Frequency", "Amplitude", ImVec2(-1, plotHeight), _plotFlags))
        {
            ImPlot::PlotDummy("##dummy0");
            ImPlot::EndPlot();
        }
        if (ImPlot::BeginPlot("RF1", "Frequency", "Amplitude", ImVec2(-1, plotHeight), _plotFlags))
        {
            ImPlot::PlotDummy("##dummy1");
            ImPlot::EndPlot();
        }
    }

    ImGui::EndChild();

    return true;
}

/* ****************************************************************************************************************** */
