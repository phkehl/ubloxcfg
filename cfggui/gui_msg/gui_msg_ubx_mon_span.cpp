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
// See the GNU General Public License for more detailspect.
//
// You should have received a copy of the GNU General Public License along with this program.
// If not, see <https://www.gnu.org/licenses/>.

#include <cstring>

#include "ff_ubx.h"

#include "gui_inc.hpp"

#include "gui_msg_ubx_mon_span.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxMonSpan::GuiMsgUbxMonSpan(std::shared_ptr<InputReceiver> receiver, std::shared_ptr<InputLogfile> logfile) :
    GuiMsg(receiver, logfile),
    _resetPlotRange { true }
{
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ const std::vector<GuiMsgUbxMonSpan::Label> GuiMsgUbxMonSpan::FREQ_LABELS =
{
    { 1575.420000, "##1", "GPS/SBAS L1CA, GAL E1, QZSS L1CA/S" }, // 24 MHz
    { 1561.098000, "##2", "BDS B1I" },
  //{ 1602.000000, "##3", "GLO L1OF" }, // -7*0.5625=-3.9375 .. +6*0.5625=3.375 ==> 1598.0625 .. 1605.375
    { 1598.062500, "##3lo", "GLO L1OF lo" },
    { 1605.375000, "##3hi", "GLO L1OF hi" },
    { 1207.140000, "##4", "BDS B2I, GAL E5B" },
    { 1227.600000, "##5", "GPS/QZSS L2C" },
  //{ 1246.000000, "##6", "GLO L2OF" }, // -7*0.4375=-3.0625 .. +6*0.4375=2.625 ==> 1242.9375 .. 1248.625
    { 1242.937500, "##6lo", "GLO L2OF (-7)" },
    { 1248.625000, "##6hi", "GLO L2OF (+6)" },
};

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
        auto &spect = _spects[spectIx];
        spect.freq.resize(numBins, NAN);
        spect.ampl.resize(numBins, NAN);
        spect.min.resize(numBins, NAN);
        spect.max.resize(numBins, NAN);
        spect.mean.resize(numBins, NAN);
        spect.center = (double)block.center * 1e-6; // [MHz]
        spect.span   = (double)block.span   * 1e-6; // [MHz]
        spect.res    = (double)block.res    * 1e-6; // [MHz]
        spect.pga    = (double)block.pga;
        for (int binIx = 0; binIx < numBins; binIx++)
        {
            spect.freq[binIx] = (UBX_MON_SPAN_BIN_CENT_FREQ(block, binIx) * 1e-6) - spect.center; // [MHz]
            const double ampl = (double)block.spectrum[binIx] * (100.0/255.0); // [%]
            spect.ampl[binIx] = ampl;
            if (std::isnan(spect.min[binIx]) || (ampl < spect.min[binIx]))
            {
                spect.min[binIx] = ampl;
            }
            if (std::isnan(spect.max[binIx]) || (ampl > spect.max[binIx]))
            {
                spect.max[binIx] = ampl;
            }
            if (spect.count > 0.0)
            {
                spect.mean[binIx] += (ampl - spect.mean[binIx]) / spect.count;
            }
            else
            {
                spect.mean[binIx] = ampl;
            }
        }
        spect.count += 1.0;

        spect.tab = Ff::Sprintf("RF%d (%.6fMHz)", spectIx, spect.center);
        spect.title = Ff::Sprintf("RF%d (PGA %.0fdB)###%p", spectIx, spect.pga, std::addressof(spect));
        spect.xlabel = Ff::Sprintf("Frequency (center %.6fMHz)", spect.center);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxMonSpan::Buttons()
{
    ImGui::SameLine();
    ImGui::BeginDisabled(_spects.empty());
    if (ImGui::Button(ICON_FK_ARROWS_ALT "###ResetPlotRange"))
    {
        _resetPlotRange = true;
    }
    ImGui::EndDisabled();
    Gui::ItemTooltip("Reset plot range");
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxMonSpan::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail)
{
    UNUSED(msg);
    if (_spects.empty())
    {
        return false;
    }
    if (ImGui::BeginChild("##spects", sizeAvail))
    {
        const int numSpects = _spects.size();
        int spectIx0 = 0;
        int spectIx1 = numSpects - 1;

        if (ImGui::BeginTabBar("tabs"))
        {
            if (ImGui::BeginTabItem("All"))
            {
                ImGui::EndTabItem();
            }
            for (int spectIx = 0; spectIx < numSpects; spectIx++)
            {
                if (ImGui::BeginTabItem(_spects[spectIx].tab.c_str()))
                {
                    spectIx0 = spectIx;
                    spectIx1 = spectIx;
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

        FfVec2f plotSize = ImGui::GetContentRegionAvail();
        plotSize.y -= (float)(spectIx1 - spectIx0 + 1 - 1) * GuiSettings::style->ItemInnerSpacing.y;
        plotSize.y /= (float)(spectIx1 - spectIx0 + 1);
        for (int spectIx = spectIx0; spectIx <= spectIx1; spectIx++)
        {
            _DrawSpect(_spects[spectIx], plotSize);
        }
        _resetPlotRange = false;

    }
    ImGui::EndChild();

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxMonSpan::_DrawSpect(const SpectData &spect, const ImVec2 size)
{
    if (ImPlot::BeginPlot(spect.title.c_str(), size, ImPlotFlags_Crosshairs | ImPlotFlags_NoFrame))
    {
        ImPlot::SetupAxis(ImAxis_X1, spect.xlabel.c_str());
        ImPlot::SetupAxis(ImAxis_Y1, "Amplitude");
        if (_resetPlotRange)
        {
            ImPlot::SetupAxisLimits(ImAxis_X1, -0.5 * spect.span, 0.5 * spect.span, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 1.0, 99.0, ImGuiCond_Always);
        }
        ImPlot::SetupFinish();

        // Labels
        const ImPlotRect limits = ImPlot::GetPlotLimits();
        for (auto &label: FREQ_LABELS)
        {
            if (std::fabs(spect.center - label.freq) < spect.span)
            {
                double freq = label.freq - spect.center;
                ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(0));
                ImPlot::PlotInfLines(label.id.c_str(), &freq, 1);
                ImPlot::PopStyleColor();

                const ImVec2 offs = ImGui::CalcTextSize(label.title.c_str());
                ImPlot::PushStyleColor(ImPlotCol_InlayText, ImPlot::GetColormapColor(0));
                // FIXME: label placement
                ImPlot::PlotText(label.title.c_str(), freq, limits.Y.Min, ImVec2(offs.y, (-0.5f * offs.x) - 5.0f), ImPlotTextFlags_Vertical);

                ImPlot::PopStyleColor();
            }
        }

        // Aplitude with min/max
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(1, ImPlotColormap_Deep));
        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.3f);
        ImPlot::PlotShaded("##Amplitude", spect.freq.data(), spect.max.data(), spect.min.data(), spect.freq.size());
        ImPlot::PlotLine("##Amplitude", spect.freq.data(), spect.ampl.data(), spect.freq.size());
        ImPlot::PopStyleVar();
        ImPlot::PopStyleColor();
        // Mean, initially hidden
        ImPlot::HideNextItem();
        ImPlot::PushStyleColor(ImPlotCol_Line, ImPlot::GetColormapColor(2, ImPlotColormap_Deep));
        ImPlot::PlotLine("Mean", spect.freq.data(), spect.mean.data(), spect.freq.size());
        ImPlot::PopStyleColor();

        ImPlot::EndPlot();
    }
}


/* ****************************************************************************************************************** */
