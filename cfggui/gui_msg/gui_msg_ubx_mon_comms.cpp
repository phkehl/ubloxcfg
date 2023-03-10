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

#include "gui_msg_ubx_mon_comms.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxMonComms::GuiMsgUbxMonComms(std::shared_ptr<InputReceiver> receiver, std::shared_ptr<InputLogfile> logfile) :
    GuiMsg(receiver, logfile),
    _valid { false }
{
    _table.AddColumn("Port");
    _table.AddColumn("TX pend", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("TX bytes", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("TX usage", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("RX pend", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("RX bytes", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("RX usage", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("Overruns", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("Skipped", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("UBX", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("NMEA", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("RTCM3", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("SPARTN", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("Other", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxMonComms::Clear()
{
    _table.ClearRows();
    _valid = false;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxMonComms::Update(const std::shared_ptr<Ff::ParserMsg> &msg)
{
    _table.ClearRows();

    if ( (UBX_MON_COMMS_VERSION_GET(msg->data) != UBX_MON_COMMS_V0_VERSION) || (UBX_MON_COMMS_V0_SIZE(msg->data) != msg->size) )
    {
        return;
    }

    UBX_MON_COMMS_V0_GROUP0_t comms;
    std::memcpy(&comms, &msg->data[UBX_HEAD_SIZE], sizeof(comms));

    _valid = true;
    _txErrors = comms.txErrors;

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

    _table.ClearRows();
    for (int portIx = 0, offs = UBX_HEAD_SIZE + (int)sizeof(UBX_MON_COMMS_V0_GROUP0_t); portIx < (int)comms.nPorts;
            portIx++, offs += (int)sizeof(UBX_MON_COMMS_V0_GROUP1_t))
    {
        UBX_MON_COMMS_V0_GROUP1_t port;
        std::memcpy(&port, &msg->data[offs], sizeof(port));

        const uint8_t portBank = port.portId & 0xff;
        const uint8_t portNo   = (port.portId >> 8) & 0xff;
        const char * const portNames[] = { "I2C", "UART1", "UART2", "USB", "SPI" };

        _table.AddCellTextF("%u %s", portBank, portNo < NUMOF(portNames) ? portNames[portNo] : "?");
        _table.AddCellTextF("%u", port.txPending);
        _table.AddCellTextF("%u", port.txBytes);
        _table.AddCellTextF("%u%% (%3u%%)", port.txUsage, port.txPeakUsage);
        _table.AddCellTextF("%u", port.rxPending);
        _table.AddCellTextF("%u", port.rxBytes);
        _table.AddCellTextF("%u%% (%3u%%)", port.rxUsage, port.rxPeakUsage);
        _table.AddCellTextF("%u", port.overrunErrors);
        _table.AddCellTextF("%u", port.skipped);
        if (msgsIxUbx < 0)
        {
            _table.AddCellEmpty();
        }
        else
        {
            _table.AddCellTextF("%u", port.msgs[msgsIxUbx]);
        }
        if (msgsIxNmea < 0)
        {
            _table.AddCellEmpty();
        }
        else
        {
            _table.AddCellTextF("%u", port.msgs[msgsIxNmea]);
        }
        if (msgsIxRtcm3 < 0)
        {
            _table.AddCellEmpty();
        }
        else
        {
            _table.AddCellTextF("%u", port.msgs[msgsIxRtcm3]);
        }
        if (msgsIxOther < 0)
        {
            _table.AddCellEmpty();
        }
        else
        {
            _table.AddCellTextF("%u", port.msgs[msgsIxOther]);
        }
        if (msgsIxSpartn < 0)
        {
            _table.AddCellEmpty();
        }
        else
        {
            _table.AddCellTextF("%u", port.msgs[msgsIxSpartn]);
        }
        _table.SetRowUid(port.portId);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxMonComms::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail)
{
    UNUSED(msg);
    if (!_valid)
    {
        return false;
    }

    const ImVec2 topSize = _CalcTopSize(1);

    const bool memError   = CHKBITS(_txErrors, UBX_MON_COMMS_V0_TXERRORS_MEM);
    const bool allocError = CHKBITS(_txErrors, UBX_MON_COMMS_V0_TXERRORS_MEM);
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

    _table.DrawTable(sizeAvail - topSize);

    return true;
}

/* ****************************************************************************************************************** */
