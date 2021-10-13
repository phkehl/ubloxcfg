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

#include "gui_settings.hpp"
#include "gui_widget.hpp"

#include "gui_msg_ubx_mon_hw.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxMonHw::GuiMsgUbxMonHw(std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile) :
    GuiMsg(receiver, logfile)
{
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxMonHw::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const ImVec2 &sizeAvail)
{
    if (msg->size != UBX_MON_HW_V0_SIZE)
    {
        return false;
    }
    UBX_MON_HW_V0_GROUP0_t hw;
    std::memcpy(&hw, &msg->data[UBX_HEAD_SIZE], sizeof(hw));

    const float topHeight = 6 * (_winSettings->charSize.y + _winSettings->style.ItemSpacing.y);
    const float topWidth = 0.5 * (sizeAvail.x - (2 * _winSettings->style.ItemSpacing.x));
    const ImVec2 topSize { topWidth, topHeight };
    const ImVec2 bottomSize { sizeAvail.x, sizeAvail.y - topSize.y -_winSettings->style.ItemSpacing.y };
    const float dataOffs = _winSettings->charSize.x * 16;

    ImGui::BeginChild("##Status", topSize);
    {
        ImGui::TextUnformatted("RTC");
        ImGui::SameLine(dataOffs);
        const bool rtcCalibrated = CHKBITS(hw.flags, UBX_MON_HW_V0_FLAGS_RTCCALIB);
        ImGui::PushStyleColor(ImGuiCol_Text, rtcCalibrated ? GUI_COLOUR(TEXT_OK) : GUI_COLOUR(TEXT_WARNING));
        ImGui::TextUnformatted(rtcCalibrated ? "calibrated" : "uncalibrated");
        ImGui::PopStyleColor();

        ImGui::TextUnformatted("RTC XTAL");
        ImGui::SameLine(dataOffs);
        const bool rtcXtalAbsent = CHKBITS(hw.flags, UBX_MON_HW_V0_FLAGS_XTALABSENT);
        if (rtcXtalAbsent) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_WARNING)); }
        ImGui::TextUnformatted(rtcXtalAbsent ? "absent" : "ok");
        if (rtcXtalAbsent) { ImGui::PopStyleColor(); }

        ImGui::TextUnformatted("Antenna status");
        ImGui::SameLine(dataOffs);
        const char * const aStatusStr[] = { "init", "unknown", "OK", "short", "open" };
        const bool aStatusWarn = (hw.aStatus == UBX_MON_HW_V0_ASTATUS_SHORT) || (hw.aStatus == UBX_MON_HW_V0_ASTATUS_OPEN);
        const bool aStatusOk   = (hw.aStatus == UBX_MON_HW_V0_ASTATUS_OK);
        if (aStatusWarn || aStatusOk)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, aStatusWarn ? GUI_COLOUR(TEXT_WARNING) : GUI_COLOUR(TEXT_OK));
        }
        ImGui::TextUnformatted(hw.aStatus < NUMOF(aStatusStr) ? aStatusStr[hw.aStatus] : "???");
        if (aStatusWarn || aStatusOk)
        {
            ImGui::PopStyleColor();
        }

        ImGui::TextUnformatted("Antenna power");
        ImGui::SameLine(dataOffs);
        const char * const aPowerStr[]  = { "off", "on", "unknown" };
        if (hw.aPower == UBX_MON_HW_V0_APOWER_ON)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_OK));
        }
        ImGui::TextUnformatted(hw.aPower < NUMOF(aPowerStr) ? aPowerStr[hw.aPower] : "???");
        if (hw.aPower == UBX_MON_HW_V0_APOWER_ON)
        {
            ImGui::PopStyleColor();
        }

        ImGui::TextUnformatted("Safeboot mode");
        ImGui::SameLine(dataOffs);
        const bool safeBootMode = CHKBITS(hw.flags, UBX_MON_HW_V0_FLAGS_SAFEBOOT);
        if (safeBootMode)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_WARNING));
        }
        ImGui::TextUnformatted(safeBootMode ? "active" : "inactive");
        if (safeBootMode)
        {
            ImGui::PopStyleColor();
        }

        ImGui::TextUnformatted("Jamming status");
        ImGui::SameLine(dataOffs);
        const char * const jammingStr[] = { "unknown", "OK", "warning", "critical" };
        const uint8_t jamming = UBX_MON_HW_V0_FLAGS_JAMMINGSTATE_GET(hw.flags);
        const bool jammingWarning  = (jamming == UBX_MON_HW_V0_FLAGS_JAMMINGSTATE_WARNING);
        const bool jammingCritical = (jamming == UBX_MON_HW_V0_FLAGS_JAMMINGSTATE_CRITICAL);
        if (jammingWarning || jammingCritical)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, jammingCritical ? GUI_COLOUR(TEXT_ERROR) : GUI_COLOUR(TEXT_WARNING));
        }
        ImGui::TextUnformatted(jamming < NUMOF(jammingStr) ? jammingStr[jamming] : "???");
        if (jammingWarning || jammingCritical)
        {
            ImGui::PopStyleColor();
        }

    }
    ImGui::EndChild();

    //Gui::VerticalSeparator();
    ImGui::SameLine();

    ImGui::BeginChild("##Gauges", topSize);
    {
        char str[100];

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Noise level");
        ImGui::SameLine(dataOffs);
        const float noise = (float)hw.noisePerMS / (float)UBX_MON_HW_V0_NOISEPERMS_MAX;
        snprintf(str, sizeof(str), "%u", hw.noisePerMS);
        ImGui::ProgressBar(noise, ImVec2(-1.0f,0.0f), str);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("AGC monitor");
        ImGui::SameLine(dataOffs);
        const float agc = (float)hw.agcCnt / (float)UBX_MON_HW_V0_AGCCNT_MAX;
        snprintf(str, sizeof(str), "%.1f%%", agc * 1e2f);
        ImGui::ProgressBar(agc, ImVec2(-1.0f,0.0f), str);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("CW jamming");
        ImGui::SameLine(dataOffs);
        const float jam = (float)hw.jamInd / (float)UBX_MON_HW_V0_JAMIND_MAX;
        snprintf(str, sizeof(str), "%.1f%%", jam * 1e2f);
        ImGui::ProgressBar(jam, ImVec2(-1.0f,0.0f), str);
    }

    ImGui::EndChild();

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody |
        ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedFit
        | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;

    const struct { const char *label; ImGuiTableColumnFlags flags; } columns[] =
    {
        { .label = "Pin",      .flags = ImGuiTableColumnFlags_NoReorder },
        { .label = "Function", .flags = 0 },
        { .label = "Type",     .flags = 0 },
        { .label = "Level",    .flags = 0 },
        { .label = "IRQ",      .flags = 0 },
        { .label = "Pull",     .flags = 0 },
        { .label = "Virtual",  .flags = 0 },
    };

    if (ImGui::BeginTable("pins", NUMOF(columns), tableFlags, bottomSize))
    {
        ImGui::TableSetupScrollFreeze(1, 1);
        for (int ix = 0; ix < NUMOF(columns); ix++)
        {
            ImGui::TableSetupColumn(columns[ix].label, columns[ix].flags);
        }
        ImGui::TableHeadersRow();

        for (uint32_t pinIx = 0, pinMask = 0x00000001; pinIx < 17; pinIx++, pinMask <<= 1)
        {
            ImGui::TableNextRow();
            int colIx = 0;

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::Selectable(pinNames[pinIx], false, ImGuiSelectableFlags_SpanAllColumns);

            ImGui::TableSetColumnIndex(colIx++);
            const uint8_t vPinIx = hw.VP[pinIx];
            if (CHKBITS(hw.pinSel, pinMask))
            {
                ImGui::TextUnformatted(pioNames[pinIx]);
            }
            else
            {
                ImGui::TextUnformatted(CHKBITS(hw.pinBank, pinMask) ? periphBfuncs[pinIx] : periphAfuncs[pinIx]);
            }

            ImGui::TableSetColumnIndex(colIx++);
            // (G)PIO
            if (CHKBITS(hw.pinSel, pinMask))
            {
                ImGui::TextUnformatted(CHKBITS(hw.pinDir, pinMask) ? "PIO_OUT" : "PIO_IN" );
            }
            // PERIPH function
            else
            {
                ImGui::TextUnformatted(CHKBITS(hw.pinBank, pinMask) ? "PERIPH_B" : "PERIPH_A" );
            }

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::TextUnformatted(CHKBITS(hw.pinVal, pinMask) ? "HIGH" : "LOW");

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::TextUnformatted(CHKBITS(hw.pinIrq, pinMask) ? "yes" : "no"); // FIXME: correct?

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::TextUnformatted(CHKBITS(hw.pullL, pinMask) ? "LOW" : "");
            if (CHKBITS(hw.pullH, pinMask))
            {
                ImGui::SameLine();
                ImGui::TextUnformatted("HIGH");
            }

            ImGui::TableSetColumnIndex(colIx++);
            if (CHKBITS(hw.usedMask, pinMask))
            {
                ImGui::Text("%u - %s", vPinIx, (vPinIx < NUMOF(virtFuncs)) && virtFuncs[vPinIx] ? virtFuncs[vPinIx] : "?");
            }
        }
        ImGui::EndTable();
    }

    return true;
}

/* ****************************************************************************************************************** */
