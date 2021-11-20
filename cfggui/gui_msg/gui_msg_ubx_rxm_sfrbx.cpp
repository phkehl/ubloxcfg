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

#include "gui_inc.hpp"

#include "gui_msg_ubx_rxm_sfrbx.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxRxmSfrbx::GuiMsgUbxRxmSfrbx(std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile) :
    GuiMsg(receiver, logfile),
    _selected{0}
{
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
        }

        const uint32_t *dwrds = (uint32_t *)&msg->data[UBX_HEAD_SIZE + sizeof(sfrbx)];
        info->dwrds.assign(dwrds, &dwrds[sfrbx.numWords]);

        info->dwrdsHex.clear();
        for (int ix = 0; ix < sfrbx.numWords; ix++)
        {
            info->dwrdsHex += Ff::Sprintf("%08x ", dwrds[ix]);
        }

        info->navMsg = GetNavMsg(*info);

        info->lastTs = msg->ts;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxRxmSfrbx::Clear()
{
    _sfrbxInfos.clear();
    _selected = 0;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxRxmSfrbx::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2 &sizeAvail)
{
    UNUSED(msg);
    const uint32_t now = TIME();

    const struct { const char *label; ImGuiTableColumnFlags flags; } columns[] =
    {
        { .label = "SV  ",    .flags = ImGuiTableColumnFlags_NoReorder },
        { .label = "Signal",  .flags = ImGuiTableColumnFlags_NoReorder },
        { .label = "Age ",    .flags = 0 },
        { .label = "Nav msg", .flags = 0 },
        { .label = "Words",   .flags = ImGuiTableColumnFlags_NoHeaderWidth },
    };

    if (ImGui::BeginTable("stats", NUMOF(columns), TABLE_FLAGS, sizeAvail))
    {
        ImGui::TableSetupScrollFreeze(4, 1);
        for (int ix = 0; ix < NUMOF(columns); ix++)
        {
            ImGui::TableSetupColumn(columns[ix].label, columns[ix].flags);
        }
        ImGui::TableHeadersRow();

        for (auto &entry: _sfrbxInfos)
        {
            int ix = 0;
            auto &info = entry.second;
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(ix++);
            if (ImGui::Selectable(info.svStr.c_str(), _selected == entry.first, ImGuiSelectableFlags_SpanAllColumns))
            {
                _selected = (_selected == entry.first ? 0 : entry.first);
            }

            ImGui::TableSetColumnIndex(ix++);
            ImGui::Text("%s", info.sigStr.c_str());

            ImGui::TableSetColumnIndex(ix++);
            if (_receiver)
            {
                ImGui::Text("%.1f", (now - info.lastTs) * 1e-3);
            }

            ImGui::TableSetColumnIndex(ix++);
            ImGui::Text("%s", info.navMsg.c_str());

            ImGui::TableSetColumnIndex(ix++);
            ImGui::TextUnformatted(info.dwrdsHex.c_str());
        }

        ImGui::EndTable();
    }

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

/* static */ std::string GuiMsgUbxRxmSfrbx::GetNavMsg(const SfrbxInfo &info)
{
    // - ZED-F9P Integration Manual, 3.13.1 Broadcast navigation data
    // - IS-GPS-200M
    // - Galileo OS SIS ICD
    // - Navipedia

    // GPS L1 C/A LNAV message
    if ( (info.gnssId == UBX_GNSSID_GPS) && (info.sigId == UBX_SIGID_GPS_L1CA) && (info.dwrds.size() == 10) )
    {
        //const uint16_t towCount = (info.dwrds[1] & 0x3fffe000) >> 13;
        const uint8_t  subFrame = (info.dwrds[1] & 0x00000700) >>  8;
        if ( (1 <= subFrame) && (subFrame <= 3 ) )
        {
            return Ff::Sprintf("LNAV eph %u/3", subFrame);
        }
        else
        {
            const uint8_t dataId = (info.dwrds[2] & 0x30000000) >> 28;
            if (dataId == 1)
            {
                const uint8_t svId = (info.dwrds[2] & 0x0fc00000) >> 22; // 20.3.3.5.1, 20.3.3.5.1.1 (Table 20-V)
                if ( (1 <= svId) && (svId <= 32) ) // subFrame 5 pages 1-24, subFrame 4 pages 2-5 and 7-10
                {
                    return Ff::Sprintf("LNAV alm %0u", svId);
                }
                else if (svId == 51) // subFrame 5, page 25
                {
                    return "LNAV toa, health"; // health 1-24, ALM reference time
                }
                else if (svId == 52) // subFrame 4, page 13
                {
                    return "LNAV NCMT";
                }
                else if (svId == 55) // subFrame 4, page 17
                {
                    return "LNAV special";
                }
                else if (svId == 56) // subFrame 4, page 18
                {
                    return "LNAV iono, UTC";
                }
                else if (svId == 63) // subFrame 4, page 25
                {
                    return "LNAV AS, health"; // anti-spoofing flags, health 25-32
                }
                else
                {
                    return "LNAV reserved";
                }
            }
        }
        return "bad?"; // Hmm.. should we check the parity first?
    }

    // GPS CNAV message (12 seconds, 300 bits)
    else if ( (info.gnssId == UBX_GNSSID_GPS) && ( (info.sigId == UBX_SIGID_GPS_L2CL) || (info.sigId == UBX_SIGID_GPS_L2CM) ) && (info.dwrds.size() == 10) )
    {
        const uint8_t msgTypeId = (info.dwrds[0] & 0x0003f000) >> 12;
        const char * const msgTypeToDesc[] =
        {
            nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr,
            /* 10 */ "eph 1", /* 11 */ "eph 2", /* 12 */ "red alm", /* 13 */ "clock diff", /* 14 */ "eph diff",
            /* 15 */ "text", nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr,
            /* 30 */ "iono, clk, tgd", /* 31 */ "clock, red. alm", /* 32 */ "clock, EOP", /* 33 */ "clock, UTC", /* 34 */ "clock, diff",
            /* 35 */ "clock, GGTO", /* 36 */ "clock, text", /* 37 */ "clock, mid alm", nullptr, nullptr,
            /* 40 */ "integrity",
        };
        return Ff::Sprintf("CNAV msg %u %s", msgTypeId,
            (msgTypeId < NUMOF(msgTypeToDesc)) && msgTypeToDesc[msgTypeId] ? msgTypeToDesc[msgTypeId] : "");
    }

    // GLONASS strings
    else if (info.gnssId == UBX_GNSSID_GLO)
    {
        const uint8_t  string     = (info.dwrds[0] & 0x78000000) >> 27;
        const uint16_t superFrame = (info.dwrds[3] & 0xffff0000) >> 16;
        const uint16_t frame      = (info.dwrds[3] & 0x0000000f);
        return Ff::Sprintf("sup %u fr %u str %u", superFrame, frame, string);
    }

    // Galileo I/NAV
    else if ( (info.gnssId == UBX_GNSSID_GAL) && ( (info.sigId == UBX_SIGID_GAL_E1B) || (info.sigId == UBX_SIGID_GAL_E5BI) ) )
    {
        // FIXME: to check.. the u-blox manual is weird here
        const char    evenOdd1  = (info.dwrds[0] & 0x80000000) == 0x80000000 ? 'o' : 'e'; // even (0) or odd (1)
        const char    pageType1 = (info.dwrds[0] & 0x40000000) == 0x40000000 ? 'a' : 'n'; // nominal (0) or alert (1)
        const uint8_t wordType1 = (info.dwrds[0] & 0x3f000000) >> 24;
        const char    evenOdd2  = (info.dwrds[4] & 0x80000000) == 0x80000000 ? 'o' : 'e'; // even (0) or odd (1)
        const char    pageType2 = (info.dwrds[4] & 0x40000000) == 0x40000000 ? 'a' : 'n'; // nominal (0) or alert (1)
        const uint8_t wordType2 = (info.dwrds[4] & 0x3f000000) >> 24;
        return Ff::Sprintf("I/NAV %c %c %u, %c %c %u", pageType1, evenOdd1, wordType1, pageType2, evenOdd2, wordType2);
    }

    return "?";
}

/* ****************************************************************************************************************** */
