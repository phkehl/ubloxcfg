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
#include <cmath>

#include "ff_ubx.h"

#include "imgui.h"
#include "implot.h"
#include "IconsForkAwesome.h"

#include "gui_settings.hpp"
#include "gui_widget.hpp"

#include "gui_msg_ubx_mon_comms.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxMonComms::GuiMsgUbxMonComms(std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile) :
    GuiMsg(receiver, logfile)
{
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxMonComms::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const ImVec2 &sizeAvail)
{
    if ( (UBX_MON_COMMS_VERSION_GET(msg->data) != UBX_MON_COMMS_V0_VERSION) || (UBX_MON_COMMS_V0_SIZE(msg->data) != msg->size) )
    {
        return false;
    }

    UBX_MON_COMMS_V0_GROUP0_t comms;
    std::memcpy(&comms, &msg->data[UBX_HEAD_SIZE], sizeof(comms));

    const ImVec2 topSize = _CalcTopSize(1);

    if (ImGui::BeginChild("##Status", topSize))
    {
        const bool memError   = CHKBITS(comms.txErrors, UBX_MON_COMMS_V0_TXERRORS_MEM);
        const bool allocError = CHKBITS(comms.txErrors, UBX_MON_COMMS_V0_TXERRORS_MEM);
        if (memError || allocError)
        {
            ImGui::TextUnformatted("Errors:");
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_WARNING));
            if (memError)
            {
                ImGui::SameLine();
                ImGui::TextUnformatted("memory");
            }
            if (allocError)
            {
                ImGui::SameLine();
                ImGui::TextUnformatted("txbuf");
            }
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::TextUnformatted("Errors: none");
        }
        ImGui::EndChild();
    }

    const struct { const char *label; ImGuiTableColumnFlags flags; } columns[] =
    {
        { .label = "Port",     .flags = ImGuiTableColumnFlags_NoReorder },
        { .label = "TX pend",  .flags = 0 },
        { .label = "TX bytes", .flags = 0 },
        { .label = "TX usage", .flags = 0 },
        { .label = "RX pend",  .flags = 0 },
        { .label = "RX bytes", .flags = 0 },
        { .label = "RX usage", .flags = 0 },
        { .label = "Overruns", .flags = 0 },
        { .label = "Skipped",  .flags = 0 },
        { .label = "UBX",      .flags = 0 },
        { .label = "NMEA",     .flags = 0 },
        { .label = "RTCM3",    .flags = 0 },
        { .label = "SPARTN",   .flags = 0 },
        { .label = "Other",    .flags = 0 },
    };

    int msgsIxUbx    = -1;
    int msgsIxNmea   = -1;
    //int msgsIxRtcm2 = -1;
    int msgsIxRtcm3  = -1;
    int msgsIxSpartn = -1;
    int msgsIxOther  = -1;
    for (int ix = 0; ix < NUMOF(comms.protIds); ix++)
    {
        switch (comms.protIds[ix])
        {
            case UBX_MON_COMMS_V0_PROTIDS_UBX:    msgsIxUbx    = ix; break;
            case UBX_MON_COMMS_V0_PROTIDS_NMEA:   msgsIxNmea   = ix; break;
            //case UBX_MON_COMMS_V0_PROTIDS_RTCM2: msgsIxRtcm2 = ix; break;
            case UBX_MON_COMMS_V0_PROTIDS_RTCM3:  msgsIxRtcm3  = ix; break;
            case UBX_MON_COMMS_V0_PROTIDS_SPARTN: msgsIxSpartn = ix; break;
            case UBX_MON_COMMS_V0_PROTIDS_OTHER:  msgsIxOther  = ix; break;
        }
    }

    if (ImGui::BeginTable("ports", NUMOF(columns), TABLE_FLAGS, sizeAvail - topSize))
    {
        ImGui::TableSetupScrollFreeze(1, 1);
        for (int ix = 0; ix < NUMOF(columns); ix++)
        {
            ImGui::TableSetupColumn(columns[ix].label, columns[ix].flags);
        }
        ImGui::TableHeadersRow();

        for (int portIx = 0, offs = UBX_HEAD_SIZE + (int)sizeof(UBX_MON_COMMS_V0_GROUP0_t); portIx < (int)comms.nPorts;
                portIx++, offs += (int)sizeof(UBX_MON_COMMS_V0_GROUP1_t))
        {
            UBX_MON_COMMS_V0_GROUP1_t port;
            std::memcpy(&port, &msg->data[offs], sizeof(port));

            ImGui::TableNextRow();
            int colIx = 0;
            char str[100];

            ImGui::TableSetColumnIndex(colIx++);
            const uint8_t portBank = port.portId & 0xff;
            const uint8_t portNo   = (port.portId >> 8) & 0xff;
            const char * const portNames[] = { "I2C", "UART1", "UART2", "USB", "SPI" };
            snprintf(str, sizeof(str), "%u %s##%d", portBank, portNo < NUMOF(portNames) ? portNames[portNo] : "?", offs);
            ImGui::Selectable(str, false, ImGuiSelectableFlags_SpanAllColumns);

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::Text("%u", port.txPending);

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::Text("%u", port.txBytes);

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::Text("%3u%% (%3u%%)", port.txUsage, port.txPeakUsage);

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::Text("%u", port.rxPending);

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::Text("%u", port.rxBytes);

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::Text("%3u%% (%3u%%)", port.rxUsage, port.rxPeakUsage);

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::Text("%u", port.overrunErrors);

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::Text("%u", port.skipped);

            ImGui::TableSetColumnIndex(colIx++);
            if (msgsIxUbx < 0)
            {
                ImGui::TextUnformatted("?");
            }
            else
            {
                ImGui::Text("%u", port.msgs[msgsIxUbx]);
            }

            ImGui::TableSetColumnIndex(colIx++);
            if (msgsIxNmea < 0)
            {
                ImGui::TextUnformatted("?");
            }
            else
            {
                ImGui::Text("%u", port.msgs[msgsIxNmea]);
            }

            ImGui::TableSetColumnIndex(colIx++);
            if (msgsIxRtcm3 < 0)
            {
                ImGui::TextUnformatted("?");
            }
            else
            {
                ImGui::Text("%u", port.msgs[msgsIxRtcm3]);
            }

            ImGui::TableSetColumnIndex(colIx++);
            if (msgsIxOther < 0)
            {
                ImGui::TextUnformatted("?");
            }
            else
            {
                ImGui::Text("%u", port.msgs[msgsIxOther]);
            }

            ImGui::TableSetColumnIndex(colIx++);
            if (msgsIxSpartn < 0)
            {
                ImGui::TextUnformatted("?");
            }
            else
            {
                ImGui::Text("%u", port.msgs[msgsIxSpartn]);
            }
        }
        ImGui::EndTable();
    }

    return true;
}

/* ****************************************************************************************************************** */
