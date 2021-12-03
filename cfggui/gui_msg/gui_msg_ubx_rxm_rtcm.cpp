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
#include "ff_rtcm3.h"

#include "gui_inc.hpp"

#include "gui_msg_ubx_rxm_rtcm.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxRxmRtcm::GuiMsgUbxRxmRtcm(std::shared_ptr<InputReceiver> receiver, std::shared_ptr<InputLogfile> logfile) :
    GuiMsg(receiver, logfile)
{
    _table.AddColumn("Message");
    _table.AddColumn("Ref");
    _table.AddColumn("#Used", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("#Unused", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("#Unknown", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("#CrcFail", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("Age");
    _table.AddColumn("Desc");
}

// ---------------------------------------------------------------------------------------------------------------------

GuiMsgUbxRxmRtcm::RtcmInfo::RtcmInfo(const int _msgType, const int _subType, const int _refStation) :
    msgType{_msgType}, subType{_subType}, refStation{_refStation == 0xffff ? -1 : _refStation},
    nUsed{0}, nUnused{0}, nUnknown{0}, nCrcFailed{0}
{
    name = std::to_string(msgType) + (msgType == 4072 ? "." + std::to_string(subType) : "");
    tooltip = rtcm3TypeDesc(msgType, subType);
    if (tooltip == NULL)
    {
        tooltip = "Unknown RTCM3 message";
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxRxmRtcm::Clear()
{
    _rtcmInfos.clear();
    _table.ClearRows();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxRxmRtcm::Update(const std::shared_ptr<Ff::ParserMsg> &msg)
{
    if ( (msg->size == UBX_RXM_RTCM_V2_SIZE) && (UBX_RXM_RTCM_VERSION_GET(msg->data) == UBX_RXM_RTCM_V2_VERSION) )
    {
        UBX_RXM_RTCM_V2_GROUP0_t rtcm;
        std::memcpy(&rtcm, &msg->data[UBX_HEAD_SIZE], sizeof(rtcm));

        // UID for this entry, also the std::map sort
        const uint32_t uid =
            ((uint32_t)MAX(rtcm.refStation, 0) << 24) |
            ((uint32_t)rtcm.subType            << 12) |
             (uint32_t)rtcm.msgType;

        RtcmInfo *info = nullptr;
        auto entry = _rtcmInfos.find(uid);
        if (entry != _rtcmInfos.end())
        {
            info = &entry->second;
        }
        else
        {
            auto foo = _rtcmInfos.insert({ uid, RtcmInfo(rtcm.msgType, rtcm.subType, rtcm.refStation) });
            info = &foo.first->second;
            info->uid = uid;
        }

        // Update statistics
        const bool crcFailed  = CHKBITS(rtcm.flags, UBX_RXM_RTCM_V2_FLAGS_CRCFAILED);
        if (crcFailed)
        {
            info->nCrcFailed++;
        }
        else
        {
            switch (UBX_RXM_RTCM_V2_FLAGS_MSGUSED_GET(rtcm.flags))
            {
                case UBX_RXM_RTCM_V2_FLAGS_MSGUSED_UNUSED:  info->nUnused++;  break;
                case UBX_RXM_RTCM_V2_FLAGS_MSGUSED_USED:    info->nUsed++;    break;
                case UBX_RXM_RTCM_V2_FLAGS_MSGUSED_UNKNOWN:
                default:                                    info->nUnknown++; break;
            }
        }
        info->lastTs = msg->ts;
    }
    else
    {
        return;
    }

    _table.ClearRows();
    for (auto &entry: _rtcmInfos)
    {
        auto &info = entry.second;

        _table.AddCellText(info.name + "##" + std::to_string(info.uid));
        _table.AddCellTextF("%d", info.refStation);
        _table.AddCellTextF("%d", info.nUsed);
        _table.AddCellTextF("%u", info.nUnused);
        _table.AddCellTextF("%u", info.nUnknown);
        _table.AddCellTextF("%u", info.nCrcFailed);
        if (_receiver)
        {
            _table.AddCellCb([](void *arg)
                {
                    const float dt = (TIME() - ((RtcmInfo *)arg)->lastTs) * 1e-3f;
                    if (dt < 1000.0f)
                    {
                        ImGui::Text("%5.1f", dt);
                    }
                    else
                    {
                        ImGui::TextUnformatted("oo");
                    }
                }, &info);
        }
        else
        {
            _table.AddCellEmpty();
        }
        _table.AddCellText(info.tooltip);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxRxmRtcm::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2 &sizeAvail)
{
    UNUSED(msg);

    _table.DrawTable(sizeAvail);

    return true;
}

/* ****************************************************************************************************************** */
