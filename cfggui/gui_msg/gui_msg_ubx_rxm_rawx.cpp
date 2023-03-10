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

#include "ff_ubx.h"

#include "gui_inc.hpp"

#include "gui_msg_ubx_rxm_rawx.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxRxmRawx::GuiMsgUbxRxmRawx(std::shared_ptr<InputReceiver> receiver, std::shared_ptr<InputLogfile> logfile) :
    GuiMsg(receiver, logfile)

{
    _table.AddColumn("SV");
    _table.AddColumn("Signal");
    _table.AddColumn("CNo", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("Pseudo range [m]", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("Carrier phase [cycles]", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("Doppler [Hz]", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("Lock [s]", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxRxmRawx::Clear()
{
    _rawInfos.clear();
    _valid = false;
    _table.ClearRows();
}

// ---------------------------------------------------------------------------------------------------------------------

GuiMsgUbxRxmRawx::RawInfo::RawInfo(const uint8_t *groupData)
{
    UBX_RXM_RAWX_V1_GROUP1_t meas;
    std::memcpy(&meas, groupData, sizeof(meas));

    uid = ((uint32_t)meas.gnssId << 24) |
          ((uint32_t)meas.svId   << 16) |
          ((uint32_t)meas.freqId <<  8) |
           (uint32_t)meas.sigId;

    prValid    = CHKBITS(meas.trkStat, UBX_RXM_RAWX_V1_TRKSTAT_PRVALID);
    cpValid    = CHKBITS(meas.trkStat, UBX_RXM_RAWX_V1_TRKSTAT_CPVALID);
    halfCyc    = CHKBITS(meas.trkStat, UBX_RXM_RAWX_V1_TRKSTAT_HALFCYC);
    subHalfCyc = CHKBITS(meas.trkStat, UBX_RXM_RAWX_V1_TRKSTAT_SUBHALFCYC);

    sv = Ff::Sprintf("%s##%p", ubxSvStr(meas.gnssId, meas.svId), groupData);

    if (meas.gnssId == UBX_GNSSID_GLO)
    {
        signal = Ff::Sprintf("%s (%+d)", ubxSigStr(meas.gnssId, meas.sigId), (int)meas.freqId - 7);
    }
    else
    {
        signal = ubxSigStr(meas.gnssId, meas.sigId);
    }

    cno = Ff::Sprintf("%u", meas.cno);
    pseudoRange = Ff::Sprintf("%.2f %6.2f", meas.prMeas, UBX_RXM_RAWX_V1_PRSTD_SCALE(UBX_RXM_RAWX_V1_PRSTDEV_PRSTD_GET(meas.prStdev)));
    carrierPhase = Ff::Sprintf("%.2f %6.3f", meas.cpMeas, UBX_RXM_RAWX_V1_CPSTD_SCALE(UBX_RXM_RAWX_V1_CPSTDEV_CPSTD_GET(meas.cpStdev)));
    doppler = Ff::Sprintf("%.1f %6.3f", meas.doMeas, UBX_RXM_RAWX_V1_DOSTD_SCALE(UBX_RXM_RAWX_V1_DOSTDEV_DOSTD_GET(meas.doStdev)));
    lockTime = Ff::Sprintf("%.3f", (double)meas.locktime * UBX_RXM_RAWX_V1_LOCKTIME_SCALE);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxRxmRawx::Update(const std::shared_ptr<Ff::ParserMsg> &msg)
{
    Clear();
    if ( (UBX_RXM_RAWX_VERSION_GET(msg->data) != UBX_RXM_RAWX_V1_VERSION) || (UBX_RXM_RAWX_V1_SIZE(msg->data) != msg->size) )
    {
        return;
    }

    UBX_RXM_RAWX_V1_GROUP0_t rawx;
    std::memcpy(&rawx, &msg->data[UBX_HEAD_SIZE], sizeof(rawx));

    _valid  = true;

    _week         = rawx.week;
    _rcvTow       = rawx.rcvTow;
    _leapSec      = rawx.leapS;
    _leapSecValid = CHKBITS(rawx.recStat, UBX_RXM_RAWX_V1_RECSTAT_LEAPSEC);
    _clkReset     = CHKBITS(rawx.recStat, UBX_RXM_RAWX_V1_RECSTAT_CLKRESET);

    for (int measIx = 0, offs = UBX_HEAD_SIZE + (int)sizeof(UBX_RXM_RAWX_V1_GROUP0_t); measIx < (int)rawx.numMeas;
            measIx++, offs += (int)sizeof(UBX_RXM_RAWX_V1_GROUP1_t))
    {
        _rawInfos.emplace_back(RawInfo(&msg->data[offs]));
    }

    std::sort(_rawInfos.begin(), _rawInfos.end(), [](const RawInfo &a, const RawInfo &b) { return a.uid < b.uid; });

    _table.ClearRows();
    for (const auto &info: _rawInfos)
    {
        _table.AddCellText(info.sv + "##" + std::to_string(info.uid));
        _table.AddCellText(info.signal);
        _table.AddCellText(info.cno);
        _table.AddCellText(info.pseudoRange);
        if (!info.prValid) { _table.SetCellColour(GUI_COLOUR(TEXT_DIM)); }
        _table.AddCellText(info.carrierPhase);
        if (!info.cpValid) { _table.SetCellColour(GUI_COLOUR(TEXT_DIM)); }
        _table.AddCellText(info.doppler);
        _table.AddCellText(info.lockTime);
        _table.SetRowUid(info.uid);
        // TODO: info.halfCyc, info.subHalfCyc
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxRxmRawx::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail)
{
    UNUSED(msg);

    if (!_valid)
    {
        return false;
    }

    const ImVec2 topSize = _CalcTopSize(3);
    const float dataOffs = 25 * GuiSettings::charSize.x;

    if (ImGui::BeginChild("##Status", topSize))
    {
        ImGui::TextUnformatted("Receiver local time");
        ImGui::SameLine(dataOffs);
        ImGui::Text("%04d:%010.3f %s", _week, _rcvTow, _clkReset ? "clock reset" : "");
        ImGui::TextUnformatted("Leap second:");
        ImGui::SameLine(dataOffs);
        ImGui::Text("%d (%s)", _leapSec, _leapSecValid ? "valid" : " invalid");
        ImGui::TextUnformatted("Measurements:");
        ImGui::SameLine(dataOffs);
        ImGui::Text("%d", (int)_rawInfos.size());
    }
    ImGui::EndChild();

    _table.DrawTable(sizeAvail - topSize);

    return true;

}

/* ****************************************************************************************************************** */
