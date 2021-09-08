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

#include "gui_win_data_log.hpp"

#include "gui_win_data_inc.hpp"

/* ****************************************************************************************************************** */

GuiWinDataLog::GuiWinDataLog(const std::string &name,
            std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile, std::shared_ptr<Database> database)
{
    _winSize  = { 80, 25 };
    _receiver = receiver;
    _logfile  = logfile;
    _database = database;
    _winTitle = name;
    _winName  = name;
    ClearData();
}

// ---------------------------------------------------------------------------------------------------------------------

//void GuiWinDataLog::Loop(const std::unique_ptr<Receiver> &receiver)
//{
//}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataLog::ProcessData(const Data &data)
{
    switch (data.type)
    {
        case Data::Type::DATA_MSG:
        {
            const char srcChar[] =
            {
                [Ff::ParserMsg::UNKN]    = '?',
                [Ff::ParserMsg::FROM_RX] = '<',
                [Ff::ParserMsg::TO_RX]   = '>',
                [Ff::ParserMsg::VIRTUAL] = 'V',
                [Ff::ParserMsg::USER]    = 'U',
                [Ff::ParserMsg::LOG]     = 'L'
            };
            char tmp[256];
            std::snprintf(tmp, sizeof(tmp), "%c %4u, size %4d, %-20s %s\n",
                data.msg->src < NUMOF(srcChar) ? srcChar[data.msg->src] : srcChar[0],
                data.msg->seq, data.msg->size, data.msg->name.c_str(), data.msg->info.size() > 0 ? data.msg->info.c_str() : "n/a");
            switch (data.msg->type)
            {
                case Ff::ParserMsg::UBX:
                    _log.AddLine(tmp, Gui::MsgUbx);
                    _nUbx++;
                    break;
                case Ff::ParserMsg::NMEA:
                    _log.AddLine(tmp, Gui::MsgNmea);
                    _nNmea++;
                    break;
                case Ff::ParserMsg::RTCM3:
                _log.AddLine(tmp, Gui::MsgRtcm3);
                _nRtcm3++;
                    break;
                case Ff::ParserMsg::GARBAGE:
                    _log.AddLine(tmp, Gui::MsgGarbage);
                    _nGarbage++;
                    break;
            }
            _nMsg++;
            _sizeRx += data.msg->size;
            break;
        }
        case Data::Type::DATA_EPOCH:
        {
            char tmp[320];
            std::snprintf(tmp, sizeof(tmp), "@ %4u %s", data.epoch->seq, data.epoch->str.c_str());
            _log.AddLine(tmp, Gui::Epoch);
            _nEpoch++;
            break;
        }
        default:
            break;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataLog::ClearData()
{
    _log.Clear();
    _sizeRx   = 0;
    _nMsg     = 0;
    _nUbx     = 0;
    _nNmea    = 0;
    _nRtcm3   = 0;
    _nGarbage = 0;
    _nEpoch   = 0;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataLog::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));

        ImGui::PushStyleColor(ImGuiCol_Text, Gui::White);
        ImGui::TextUnformatted("Msg: ");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, Gui::BrightWhite);
        ImGui::Text("%u", _nMsg);
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, Gui::White);
        ImGui::TextUnformatted(", UBX: ");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, Gui::MsgUbx);
        ImGui::Text("%u", _nUbx);
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, Gui::White);
        ImGui::TextUnformatted(", NMEA: ");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, Gui::MsgNmea);
        ImGui::Text("%u", _nNmea);
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, Gui::White);
        ImGui::TextUnformatted(", RTCM3: ");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, Gui::MsgRtcm3);
        ImGui::Text("%u", _nRtcm3);
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, Gui::White);
        ImGui::TextUnformatted(", GARBAGE: ");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, Gui::MsgGarbage);
        ImGui::Text("%u", _nGarbage);
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, Gui::White);
        ImGui::TextUnformatted(", rx: ");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, Gui::BrightWhite);
        ImGui::Text("%u", _sizeRx);
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, Gui::White);
        ImGui::TextUnformatted(", Epoch: ");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, Gui::Epoch);
        ImGui::Text("%u", _nEpoch);
        ImGui::PopStyleColor();
        //ImGui::SameLine();

        ImGui::PopStyleVar(); // ImGuiStyleVar_ItemSpacing
    }
    ImGui::Separator();

    _log.DrawWidget();

    _DrawWindowEnd();
}

/* ****************************************************************************************************************** */
