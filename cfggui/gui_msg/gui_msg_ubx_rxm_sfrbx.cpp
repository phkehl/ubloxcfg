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

#include "gui_msg_ubx_rxm_sfrbx.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxRxmSfrbx::GuiMsgUbxRxmSfrbx(std::shared_ptr<InputReceiver> receiver, std::shared_ptr<InputLogfile> logfile) :
    GuiMsg(receiver, logfile)
{
    _table.AddColumn("SV");
    _table.AddColumn("Signal");
    _table.AddColumn("Age");
    _table.AddColumn("Nav msg");
    _table.AddColumn("Words");
    _table.SetTableScrollFreeze(2, 1);
}

// ---------------------------------------------------------------------------------------------------------------------

GuiMsgUbxRxmSfrbx::SfrbxInfo::SfrbxInfo(
    const uint8_t _gnssId, const uint8_t _svId, const uint8_t _sigId, const uint8_t _freqId) :
    svStr{ ubxSvStr(_gnssId, _svId) }, sigStr{ ubxSigStr(_gnssId, _sigId) },
    gnssId{_gnssId}, svId{_svId}, sigId{_sigId}, freqId{_freqId}
{
    if (_gnssId == UBX_GNSSID_GLO)
    {
        sigStr += Ff::Sprintf(" (%+d)", (int)_freqId - 7);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxRxmSfrbx::Update(const std::shared_ptr<Ff::ParserMsg> &msg)
{
    if ( (msg->size >= UBX_RXM_SFRBX_V2_MIN_SIZE) && (UBX_RXM_SFRBX_VERSION_GET(msg->data) == UBX_RXM_SFRBX_V2_VERSION) )
    {
        UBX_RXM_SFRBX_V2_GROUP0_t sfrbx;
        std::memcpy(&sfrbx, &msg->data[UBX_HEAD_SIZE], sizeof(sfrbx));

        // UID for this entry, also the std::map sort
        const uint32_t uid =
            ((uint32_t)sfrbx.gnssId  << 24) |
            ((uint32_t)sfrbx.svId    << 16) |
            ((uint32_t)sfrbx.freqId  <<  8) |
             (uint32_t)sfrbx.sigId;

        SfrbxInfo *info = nullptr;
        auto entry = _sfrbxInfos.find(uid);
        if (entry != _sfrbxInfos.end())
        {
            info = &entry->second;
        }
        else
        {
            auto foo = _sfrbxInfos.insert({ uid, SfrbxInfo(sfrbx.gnssId, sfrbx.svId, sfrbx.sigId, sfrbx.freqId) });
            info = &foo.first->second;
            info->uid = uid;
        }

        const uint32_t *dwrds = (uint32_t *)&msg->data[UBX_HEAD_SIZE + sizeof(sfrbx)];
        info->dwrds.assign(dwrds, &dwrds[sfrbx.numWords]);

        info->dwrdsHex.clear();
        for (int ix = 0; ix < sfrbx.numWords; ix++)
        {
            info->dwrdsHex += Ff::Sprintf("%08x ", dwrds[ix]);
        }

        char str[200];
        ubxRxmSfrbxInfo(str, sizeof(str), msg->data, msg->size);
        info->navMsg = std::string(str);

        info->lastTs = msg->ts;
    }
    else
    {
        return;
    }

    _table.ClearRows();
    for (auto &entry: _sfrbxInfos)
    {
        auto &info = entry.second;
        _table.AddCellText(info.svStr + "##" + std::to_string(info.uid));
        _table.AddCellText(info.sigStr);
        if (_receiver)
        {
            _table.AddCellCb([](void *arg)
                {
                    const float dt = (TIME() - ((SfrbxInfo *)arg)->lastTs) * 1e-3f;
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
        _table.AddCellText(info.navMsg);
        _table.AddCellText(info.dwrdsHex);
        _table.SetRowUid(info.uid);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxRxmSfrbx::Clear()
{
    _sfrbxInfos.clear();
    _table.ClearRows();
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxRxmSfrbx::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail)
{
    UNUSED(msg);

    _table.DrawTable(sizeAvail);

    return true;
}

/* ****************************************************************************************************************** */
