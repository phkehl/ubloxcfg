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
#include "gui_msg_ubx_mon_hw3.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxMonHw3::GuiMsgUbxMonHw3(std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile) :
    GuiMsg(receiver, logfile)
{
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxMonHw3::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const ImVec2 &sizeAvail)
{
    if ( (UBX_MON_HW3_VERSION_GET(msg->data) != UBX_MON_HW3_V0_VERSION) || (UBX_MON_HW3_V0_SIZE(msg->data) != msg->size) )
    {
        return false;
    }
    UBX_MON_HW3_V0_GROUP0_t hw;
    std::memcpy(&hw, &msg->data[UBX_HEAD_SIZE], sizeof(hw));

    const float topHeight = 4 * (_winSettings->charSize.y + _winSettings->style.ItemSpacing.y);
    const ImVec2 topSize { 0.0f, topHeight };
    const ImVec2 bottomSize { sizeAvail.x, sizeAvail.y - topSize.y -_winSettings->style.ItemSpacing.y };

    ImGui::BeginChild("##Status", topSize);
    {
        ImGui::TextUnformatted("RTC:             ");
        ImGui::SameLine();
        const bool rtcCalibrated = CHKBITS(hw.flags, UBX_MON_HW3_V0_FLAGS_RTCCALIB);
        ImGui::PushStyleColor(ImGuiCol_Text, rtcCalibrated ? GUI_COLOUR(TEXT_OK) : GUI_COLOUR(TEXT_WARNING));
        ImGui::TextUnformatted(rtcCalibrated ? "calibrated" : "uncalibrated");
        ImGui::PopStyleColor();

        ImGui::TextUnformatted("RTC XTAL:        ");
        ImGui::SameLine();
        const bool rtcXtalAbsent = CHKBITS(hw.flags, UBX_MON_HW3_V0_FLAGS_XTALABSENT);
        if (rtcXtalAbsent) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_WARNING)); }
        ImGui::TextUnformatted(rtcXtalAbsent ? "absent" : "ok");
        if (rtcXtalAbsent) { ImGui::PopStyleColor(); }

        ImGui::TextUnformatted("Safeboot mode:   ");
        ImGui::SameLine();
        const bool safeBootMode = CHKBITS(hw.flags, UBX_MON_HW3_V0_FLAGS_SAFEBOOT);
        if (safeBootMode)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_WARNING));
        }
        ImGui::TextUnformatted(safeBootMode ? "active" : "inactive");
        if (safeBootMode)
        {
            ImGui::PopStyleColor();
        }

        ImGui::TextUnformatted("Hardware version:");
        ImGui::SameLine();
        char str[sizeof(hw.hwVersion) + 1];
        std::memcpy(str, hw.hwVersion, sizeof(hw.hwVersion));
        str[sizeof(str) - 1] = '\0';
        ImGui::TextUnformatted(str);
    }
    ImGui::EndChild();

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody |
        ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedFit
        | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;

    const struct { const char *label; ImGuiTableColumnFlags flags; } columns[] =
    {
        { .label = "Pin",         .flags = ImGuiTableColumnFlags_NoReorder },
        { .label = "Direction",   .flags = 0 },
        { .label = "State",       .flags = 0 },
        { .label = "Peripheral",  .flags = 0 },
        { .label = "IRQ",         .flags = 0 },
        { .label = "Pull",        .flags = 0 },
        { .label = "Virtual",     .flags = 0 },
    };

    if (ImGui::BeginTable("pins", NUMOF(columns), tableFlags, bottomSize))
    {
        ImGui::TableSetupScrollFreeze(1, 1);
        for (int ix = 0; ix < NUMOF(columns); ix++)
        {
            ImGui::TableSetupColumn(columns[ix].label, columns[ix].flags);
        }
        ImGui::TableHeadersRow();

        for (int pinIx = 0, offs = UBX_HEAD_SIZE + (int)sizeof(UBX_MON_HW3_V0_GROUP0_t); pinIx < (int)hw.nPins;
             pinIx++, offs += (int)sizeof(UBX_MON_HW3_V0_GROUP1_t))
        {
            UBX_MON_HW3_V0_GROUP1_t pin;
            std::memcpy(&pin, &msg->data[offs], sizeof(pin));

            ImGui::TableNextRow();
            int colIx = 0;
            char str[100];

            ImGui::TableSetColumnIndex(colIx++);
            const uint8_t pinBank = pin.pinId & 0xff;
            const uint8_t pinNo   = (pin.pinId >> 8) & 0xff;
            snprintf(str, sizeof(str), "%d %2d##%d", pinBank, pinNo, pinIx);
            ImGui::Selectable(str, false, ImGuiSelectableFlags_SpanAllColumns);

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::TextUnformatted(CHKBITS(pin.pinMask, UBX_MON_HW3_V0_PINMASK_DIRECTION) ? "output" : "input");

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::TextUnformatted(CHKBITS(pin.pinMask, UBX_MON_HW3_V0_PINMASK_VALUE) ? "high" : "low");

            ImGui::TableSetColumnIndex(colIx++);
            if (CHKBITS(pin.pinMask, UBX_MON_HW3_V0_PINMASK_PERIPHPIO))
            {
                ImGui::TextUnformatted("PIO");
            }
            else
            {
                const uint8_t periph = UBX_MON_HW3_V0_PINMASK_PINBANK_GET(pin.pinMask);
                const char periphChars[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' };
                ImGui::Text("%c", periph < NUMOF(periphChars) ? periphChars[periph] : '?');
            }

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::TextUnformatted(CHKBITS(pin.pinMask, UBX_MON_HW3_V0_PINMASK_PIOIRQ) ? "yes" : "no");

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::TextUnformatted(CHKBITS(pin.pinMask, UBX_MON_HW3_V0_PINMASK_PIOPULLLOW) ? "low" : "");
            if (CHKBITS(pin.pinMask, UBX_MON_HW3_V0_PINMASK_PIOPULLHIGH))
            {
                ImGui::SameLine();
                ImGui::TextUnformatted("high");
            }

            ImGui::TableSetColumnIndex(colIx++);
            if (CHKBITS(pin.pinMask, UBX_MON_HW3_V0_PINMASK_VPMANAGER))
            {
                ImGui::Text("%u - %s", pin.VP,
                    (pin.VP < NUMOF(GuiMsgUbxMonHw::virtFuncs)) && GuiMsgUbxMonHw::virtFuncs[pin.VP] ?
                    GuiMsgUbxMonHw::virtFuncs[pin.VP] : "?");
            }
        }

        ImGui::EndTable();
    }

    return true;
}

/* ****************************************************************************************************************** */
