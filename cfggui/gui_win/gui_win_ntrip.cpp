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

#include <regex>

#include "base64.hpp"

#include "ff_trafo.h"
#include "ff_port.h"
#include "ff_nmea.h"
#include "ff_parser.h"

#include "config.h"

#include "gui_inc.hpp"

#include "gui_win_ntrip.hpp"

/* ****************************************************************************************************************** */

GuiWinNtrip::GuiWinNtrip(const std::string &name) :
    GuiWin(name),
    _guiDataSerial { 0 },
    _ggaAuto       { true },
    _ggaInt        { "5.0" },
    _ggaLastUpdate { 0.0 },
    _thread        { _winName, std::bind(&GuiWinNtrip::_Thread, this, std::placeholders::_1) }
{
    _winSize = { 70, 30 };
    DEBUG("GuiWinNtrip(%s)", _winName.c_str());

    auto &recent = GuiSettings::GetRecentItems(GuiSettings::RECENT_NTRIP_CASTERS);
    if (!recent.empty())
    {
        _SetCaster(recent[0]);
    }

    _UpdateGga();
}

GuiWinNtrip::~GuiWinNtrip()
{
    DEBUG("~GuiWinNtrip(%s)", _winName.c_str());
}

#define NTRIP_DEBUG(fmt, args...) DEBUG("%s: " fmt, _winName.c_str(), ## args)
//#define NTRIP_DEBUG(fmt, args...) /* nothing */

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinNtrip::Loop(const uint32_t &frame, const double &now)
{
    UNUSED(frame);

    // Update list of receivers
    if (_guiDataSerial != GuiData::serial)
    {
        _guiDataSerial = GuiData::serial;
        std::unique_lock<std::mutex> lock(_mutex);
        _receivers = GuiData::Receivers();

        // Source receiver still present?
        if (_srcReceiver)
        {
            auto entry = std::find(_receivers.begin(), _receivers.end(), _srcReceiver);
            if (entry == _receivers.end())
            {
                _srcReceiver = nullptr;
            }
        }
    }

    if ( (now - _ggaLastUpdate) > 0.25 )
    {
        _UpdateGga();
        _ggaLastUpdate = now;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinNtrip::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    // Connect/disconnect button
    {
        if (!_ntripClient.IsConnected())
        {
            ImGui::BeginDisabled( !_ntripClient.CanConnect() || (_casterInput.length() < 10) );
            if (ImGui::Button(ICON_FK_PLAY "###StartStop", GuiSettings::iconSize))
            {
                if (_SetCaster(_casterInput))
                {
                    _thread.Start();
                }
            }
            ImGui::EndDisabled();
            Gui::ItemTooltip("Connect to NTRIP caster");
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTGREEN));
            if (ImGui::Button(ICON_FK_STOP "###StartStop", GuiSettings::iconSize))
            {
                _thread.Stop();
            }
            ImGui::PopStyleColor();
            Gui::ItemTooltip("Disconnect from NTRIP caster");
        }
    }

    ImGui::SameLine();

    // Caster input
    {
        ImGui::PushItemWidth(-(GuiSettings::iconSize.x + GuiSettings::style->ItemSpacing.x));
        if (ImGui::InputTextWithHint("##NtripCaster", "[<user>:<pass>@]<host>:<port>/<mountpoint>",
            &_casterInput, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            DEBUG("_casterInput=%s", _casterInput.c_str());
            _SetCaster(_casterInput);
        }
        ImGui::PopItemWidth();
        Gui::ItemTooltip("NTRIP caster\n[<user>:<pass>@]<host>:<port>/<mountpoint>");
    }

    ImGui::SameLine();

    // Recent casters
    {
        if (ImGui::Button(ICON_FK_STAR "##Recent", GuiSettings::iconSize))
        {
            ImGui::OpenPopup("Recent");
        }
        Gui::ItemTooltip("Recent NTRIP casters");
        if (ImGui::BeginPopup("Recent"))
        {
            auto &recent = GuiSettings::GetRecentItems(GuiSettings::RECENT_NTRIP_CASTERS);
            for (auto &caster: recent)
            {
                if (ImGui::Selectable(caster.c_str()))
                {
                    _SetCaster(caster);
                }
            }
            if (!recent.empty())
            {
                ImGui::Separator();
                if (ImGui::Selectable("Clear recent casters"))
                {
                    GuiSettings::ClearRecentItems(GuiSettings::RECENT_NTRIP_CASTERS);
                }
            }
            ImGui::EndPopup();
        }
    }

    ImGui::TextUnformatted(_ntripClient.GetStatusStr().c_str());

    ImGui::Separator();

    // Position parameters
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("GGA interval");
        ImGui::SameLine();
        ImGui::PushItemWidth(GuiSettings::charSize.x * 5);
        if (ImGui::InputTextWithHint("##GgaInt", "Int", &_ggaInt, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsScientific))
        {
            _UpdateGga();
        }
        ImGui::PopItemWidth();
        Gui::ItemTooltip("Position update interval (0 = disable)");
        ImGui::SameLine(0, 0);
        if (ImGui::BeginCombo("##RecentLogs", NULL, ImGuiComboFlags_HeightLarge | ImGuiComboFlags_NoPreview))
        {
            if (ImGui::Selectable("off"))   { _ggaInt =  "0.0"; _UpdateGga(); }
            if (ImGui::Selectable( "1.0s")) { _ggaInt =  "1.0"; _UpdateGga(); }
            if (ImGui::Selectable( "2.0s")) { _ggaInt =  "2.0"; _UpdateGga(); }
            if (ImGui::Selectable( "5.0s")) { _ggaInt =  "5.0"; _UpdateGga(); }
            if (ImGui::Selectable("10.0s")) { _ggaInt = "10.0"; _UpdateGga(); }
            if (ImGui::Selectable("30.0s")) { _ggaInt = "30.0"; _UpdateGga(); }
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        ImGui::BeginDisabled(!_ntripClient.IsConnected());
        if (ImGui::Button(ICON_FK_THUMBS_UP "##ForceGga"))
        {
            _ntripClient.SendPos();
        }
        Gui::ItemTooltip("Force send GGA");
        ImGui::EndDisabled();

        Gui::VerticalSeparator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("GGA:");
        ImGui::SameLine();

        if (ImGui::Checkbox("##GgaAuto", &_ggaAuto))
        {
            DEBUG("update auto %s", _ggaAuto ? "true" : "false");
        }
        Gui::ItemTooltip("Automatic position update (from receiver position)");

        ImGui::SameLine();

        ImGui::BeginDisabled(_ggaAuto);
        ImGui::PushItemWidth(GuiSettings::charSize.x * 12);
        if (ImGui::InputTextWithHint("##GgaLat", "Lat", &_ggaLat, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsScientific))
        {
            _UpdateGga();
        }
        ImGui::PopItemWidth();
        Gui::ItemTooltip("Latitude [deg]");
        ImGui::SameLine();
        ImGui::PushItemWidth(GuiSettings::charSize.x * 12);
        if (ImGui::InputTextWithHint("##GgaLon", "Lon", &_ggaLon, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsScientific))
        {
            _UpdateGga();
        }
        ImGui::PopItemWidth();
        Gui::ItemTooltip("Longitude [deg]");
        ImGui::SameLine();
        ImGui::PushItemWidth(GuiSettings::charSize.x * 7);
        if (ImGui::InputTextWithHint("##GgaAlt", "Alt", &_ggaAlt, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsScientific))
        {
            _UpdateGga();
        }
        ImGui::PopItemWidth();
        Gui::ItemTooltip("Altitude [m]");
        ImGui::SameLine();
        ImGui::PushItemWidth(GuiSettings::charSize.x * 4);
        if (ImGui::InputTextWithHint("##GgaNumSv", "#SV", &_ggaNumSv, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsDecimal))
        {
            _UpdateGga();
        }
        Gui::ItemTooltip("Number of satellites used");
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Checkbox("##GgaFix", &_ggaFix))
        {
            _UpdateGga();
        }
        Gui::ItemTooltip("Valid position fix");
        ImGui::EndDisabled();
    }

    ImGui::Separator();

    // Receivers selection
    ImGui::Text("GGA source  Use NTRIP");
    //            1234567890123456789012345
    for (auto &receiver: _receivers)
    {
        ImGui::PushID(&receiver);
        const std::string &rxName = receiver->GetName();

        ImGui::NewLine();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0)); // smaller radio and checkbox
        {
            ImGui::SameLine(GuiSettings::charSize.x * 4);
            bool checked = _dstReceivers[rxName];

            if (ImGui::RadioButton("##input", receiver == _srcReceiver))
            {
                _srcReceiver = (_srcReceiver == receiver ? nullptr : receiver);
            }

            ImGui::SameLine(GuiSettings::charSize.x * 16);
            if (ImGui::Checkbox("##ntrip", &checked))
            {
                _dstReceivers[rxName] = !_dstReceivers[rxName];
            }
        }
        ImGui::PopStyleVar();

        ImGui::SameLine(GuiSettings::charSize.x * 25);
        const char *rxStatus = "not connected";
        if (receiver->IsReady())
        {
            rxStatus = receiver->GetDatabase()->LatestRow().fix_str;
        }
        ImGui::Text("%-10.10s %-15.15s %s", rxName.c_str(), rxStatus, receiver->GetRxVer().c_str());

        ImGui::PopID();
    }

    ImGui::Separator();

    // Messages status
    static constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
        ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("Status", 7, tableFlags))
    {
        ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Bytes", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Msgs/s", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Bytes/s", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Age", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Info");
        ImGui::TableHeadersRow();

        const uint32_t now = TIME();
        for (auto &infos: { _ntripClient.GetInMsgInfos(), _ntripClient.GetOutMsgInfos() })
        {
            uint32_t count = 0;
            uint32_t bytes = 0;
            float msgRate = 0.0f;
            float byteRate = 0.0f;
            for (auto &info: infos)
            {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Selectable(info.first.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%5u", info.second.count);
                count += info.second.count;

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%6u", info.second.bytes);
                bytes += info.second.bytes;

                if (info.second.ts != 0)
                {
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%6.1f", info.second.msgRate);
                    msgRate += info.second.msgRate;

                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%7.1f", info.second.byteRate);
                    byteRate += info.second.byteRate;
                }

                if ( _thread.IsRunning() && (info.second.ts != 0) )
                {
                    ImGui::TableSetColumnIndex(5);
                    ImGui::Text("%4.1f", (now - info.second.ts) * 1e-3);
                }

                ImGui::TableSetColumnIndex(6);
                ImGui::TextUnformatted(info.second.info.c_str());
            }

            if (infos.size() > 1)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Selectable("Total", false, ImGuiSelectableFlags_SpanAllColumns);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%5u", count);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%6u", bytes);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%6.1f", msgRate);
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%7.1f", byteRate);
            }
        }

        ImGui::EndTable();
    }

    ImGui::Separator();


    // Log
    _log.DrawLog();

    _DrawWindowEnd();
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWinNtrip::_SetCaster(const std::string &spec)
{
    NtripClient client;
    if (client.Init(spec))
    {
        _ntripClient = client;
        _casterInput = spec;
        GuiSettings::AddRecentItem(GuiSettings::RECENT_NTRIP_CASTERS, spec);
        return true;
    }
    else
    {
        return false;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinNtrip::_UpdateGga()
{
    std::unique_lock<std::mutex> lock(_mutex);

    float ggaInt = std::atof(_ggaInt.c_str());
    if ( (ggaInt != 0.0f) && ( (ggaInt < 1.0f) || (ggaInt > 60.0f) ) )
    {
        ggaInt = 1.0f;
    }

    _ntripPos.valid = false;

    // Automatic from source receiver
    if (_ggaAuto)
    {
        _ggaLat.clear();
        _ggaLon.clear();
        _ggaAlt.clear();
        _ggaNumSv.clear();
        if (_srcReceiver && _srcReceiver->IsReady())
        {
            const auto row = _srcReceiver->GetDatabase()->LatestRow();
            if (row.pos_avail && row.fix_ok && (row.fix_type >= EPOCH_FIX_S3D))
            {
                _ntripPos.lat   = row.pos_llh_lat;
                _ntripPos.lon   = row.pos_llh_lon;
                _ntripPos.alt   = row.pos_llh_height;
                _ntripPos.numSv = CLIP(row.sol_numsat_tot, 4, 12);
                _ntripPos.fix   = true;
                _ntripPos.valid = true;
            }
        }
    }
    // Manually set
    else
    {
        _ntripPos.lat   = std::atof(_ggaLat.c_str());
        _ntripPos.lat   = CLIP(_ntripPos.lat, NtripClient::Pos::MIN_LAT, NtripClient::Pos::MAX_LAT);
        _ntripPos.lon   = std::atof(_ggaLon.c_str());
        _ntripPos.lon   = CLIP(_ntripPos.lon, NtripClient::Pos::MIN_LON, NtripClient::Pos::MAX_LON);
        _ntripPos.alt   = std::atof(_ggaAlt.c_str());
        _ntripPos.alt   = CLIP(_ntripPos.alt, NtripClient::Pos::MIN_ALT, NtripClient::Pos::MAX_ALT);
        _ntripPos.numSv = std::atoi(_ggaNumSv.c_str());
        _ntripPos.numSv = CLIP(_ntripPos.numSv, NtripClient::Pos::MIN_NUMSV, NtripClient::Pos::MAX_NUMSV);
        _ntripPos.fix   = _ggaFix;
        _ntripPos.valid = true;
    }

    _ggaLat   = Ff::Sprintf("%.7f", _ntripPos.lat);
    _ggaLon   = Ff::Sprintf("%.7f", _ntripPos.lon);
    _ggaAlt   = Ff::Sprintf("%.1f", _ntripPos.alt);
    _ggaNumSv = Ff::Sprintf("%02d", _ntripPos.numSv);
    _ggaFix   = _ntripPos.fix;

    _ggaInt = Ff::Sprintf("%.1f", ggaInt);

    _ntripClient.SetPos(_ntripPos, ggaInt);
}

// ---------------------------------------------------------------------------------------------------------------------

#define LOG_NOTICE(str)  _log.AddLine(str, GUI_COLOUR(DEBUG_NOTICE));
#define LOG_WARNING(str) _log.AddLine(str, GUI_COLOUR(DEBUG_WARNING));
#define LOG_ERROR(str)   _log.AddLine(str, GUI_COLOUR(DEBUG_ERROR));

void GuiWinNtrip::_Thread(Ff::Thread *thread)
{
    // Connect
    if (!_ntripClient.Connect())
    {
        LOG_ERROR(_ntripClient.GetStatusStr());
        return;
    }

    LOG_NOTICE(_ntripClient.GetStatusStr());

    while ( !thread->ShouldAbort() && _ntripClient.IsConnected() )
    {
        PARSER_MSG_t msg;
        if (_ntripClient.GetData(&msg))
        {
            // Send RTCM3 messages to the receivers
            if (msg.type == PARSER_MSGTYPE_RTCM3)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                for (auto &receiver: _receivers)
                {
                    if (_dstReceivers[receiver->GetName()])
                    {
                        receiver->Send(msg.data, msg.size);
                    }
                }
            }
            else
            {
                DEBUG_HEXDUMP(msg.data, msg.size, "%s: %s", _winName.c_str(), msg.name);
                LOG_WARNING(std::string("Unexpected data from NTRIP caster (") + msg.name + ")");
            }
            continue;
        }

        thread->Sleep(100);
    }

    _ntripClient.Disconnect();
    LOG_NOTICE(_ntripClient.GetStatusStr());
}

/* ****************************************************************************************************************** */
