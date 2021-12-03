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

#include "gui_inc.hpp"

#include "gui_win_data_log.hpp"

/* ****************************************************************************************************************** */

GuiWinDataLog::GuiWinDataLog(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database)
{
    _winSize = { 80, 25 };
    ClearData();

    _latestEpochEna = false;

    _log.SetSettings(GuiSettings::GetValue(GetName()));
}

GuiWinDataLog::~GuiWinDataLog()
{
    GuiSettings::SetValue(GetName(), _log.GetSettings());
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataLog::_ProcessData(const InputData &data)
{
    switch (data.type)
    {
        case InputData::DATA_MSG:
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
                    _log.AddLine(tmp, GUI_COLOUR(LOG_MSGUBX));
                    _nUbx++;
                    break;
                case Ff::ParserMsg::NMEA:
                    _log.AddLine(tmp, GUI_COLOUR(LOG_MSGNMEA));
                    _nNmea++;
                    break;
                case Ff::ParserMsg::RTCM3:
                _log.AddLine(tmp, GUI_COLOUR(LOG_MSGRTCM3));
                _nRtcm3++;
                    break;
                case Ff::ParserMsg::GARBAGE:
                    _log.AddLine(tmp, GUI_COLOUR(LOG_MSGGARBAGE));
                    _nGarbage++;
                    break;
            }
            _nMsg++;
            _sizeRx += data.msg->size;
            break;
        }
        case InputData::DATA_EPOCH:
        {
            char tmp[320];
            std::snprintf(tmp, sizeof(tmp), "@ %4u %s", data.epoch->seq, data.epoch->str.c_str());
            _log.AddLine(tmp, GUI_COLOUR(LOG_EPOCH));
            _nEpoch++;
            break;
        }
        default:
            break;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataLog::_ClearData()
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

void GuiWinDataLog::_DrawToolbar()
{
    Gui::VerticalSeparator();
    _log.DrawControls();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataLog::_DrawContent()
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));

    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_WHITE));
    ImGui::TextUnformatted("Msg: ");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTWHITE));
    ImGui::Text("%u", _nMsg);
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_WHITE));
    ImGui::TextUnformatted(", UBX: ");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(LOG_MSGUBX));
    ImGui::Text("%u", _nUbx);
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_WHITE));
    ImGui::TextUnformatted(", NMEA: ");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(LOG_MSGNMEA));
    ImGui::Text("%u", _nNmea);
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_WHITE));
    ImGui::TextUnformatted(", RTCM3: ");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(LOG_MSGRTCM3));
    ImGui::Text("%u", _nRtcm3);
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_WHITE));
    ImGui::TextUnformatted(", GARBAGE: ");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(LOG_MSGGARBAGE));
    ImGui::Text("%u", _nGarbage);
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_WHITE));
    ImGui::TextUnformatted(", Size: ");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTWHITE));
    ImGui::Text("%u", _sizeRx);
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_WHITE));
    ImGui::TextUnformatted(", Epoch: ");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(LOG_EPOCH));
    ImGui::Text("%u", _nEpoch);
    ImGui::PopStyleColor();
    //ImGui::SameLine();

    ImGui::PopStyleVar(); // ImGuiStyleVar_ItemSpacing

    ImGui::Separator();

    _log.DrawLog();
}

/* ****************************************************************************************************************** */
