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

#include "gui_win_data_messages.hpp"

#include "gui_win_data_inc.hpp"

/* ****************************************************************************************************************** */

GuiWinDataMessages::GuiWinDataMessages(const std::string &name,
            std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile, std::shared_ptr<Database> database) :
    _showSubSec{false}, _selectedEntry{}, _displayedEntry{}, _hexDump{}, _classNames{}
{
    _winSize  = { 130, 25 };
    _receiver = receiver;
    _logfile  = logfile;
    _database = database;
    _winTitle = name;
    _winName  = name;

    int numMsgrates = 0;
    const UBLOXCFG_MSGRATE_t **msgrates = ubloxcfg_getAllMsgRateCfgs(&numMsgrates);
    std::string prevName = "";
    for (int ix = 0; ix < numMsgrates; ix++)
    {
        const UBLOXCFG_MSGRATE_t *rate = msgrates[ix];
        std::string msgName = rate->msgName;
        const std::string className = msgName.substr(0, msgName.rfind('-'));
        if (!className.empty() && (className != prevName))
        {
            _classNames.push_back(className);
            prevName = className;
        }
    }

    ClearData();
}

// ---------------------------------------------------------------------------------------------------------------------

//void GuiWinDataMessages::Loop(const uint32_t &frame, const double &now)
//{
//    (void)frame;
//    (void)now;
//}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::ProcessData(const Data &data)
{
    switch (data.type)
    {
        case Data::Type::DATA_MSG:
        {
            auto entry = _messages.find(data.msg->name);
            if (entry != _messages.end())
            {
                auto &info = entry->second;
                info.Update(data.msg);
            }
            else
            {
                _messages.insert({ data.msg->name, MsgInfo(data.msg) });
            }
            _displayedEntry = _messages.end();
            break;
        }
        default:
            break;
    }
}

void GuiWinDataMessages::MsgInfo::Update(const std::shared_ptr<Ff::ParserMsg> &_msg)
{
    // store time since last message [ms]
    dt[dtIx] = _msg->ts - msg->ts;
    dtIx++;
    dtIx %= NUMOF(dt);

    count++;
    msg = _msg;

    // calculate average message rate
    uint32_t sum = 0;
    int num = 0;
    for (int ix = 0; ix < NUMOF(dt); ix++)
    {
        if (dt[ix] != 0)
        {
            sum += dt[ix];
            num++;
        }
    }
    rate = 1e3f / ((float)sum / (float)num);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::ClearData()
{
    _messages.clear();
    _selectedEntry  = _messages.end();
    _displayedEntry = _messages.end();
    _UpdateHexdump();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    uint32_t now = TIME();

    const bool ctrlEnabled = _receiver && _receiver->IsReady();

    // Controls
    {
        // Show sub-second dt?
        if (_showSubSec)
        {
            if (ImGui::Button(ICON_FK_DOT_CIRCLE_O "###ShowSubSecDt", _winSettings->iconButtonSize))
            {
                _showSubSec = false;
            }
            Gui::ItemTooltip("Showing sub-second delta times");
        }
        else
        {
            if (ImGui::Button(ICON_FK_CIRCLE_O "###ShowSubSecDt", _winSettings->iconButtonSize))
            {
                _showSubSec = true;
            }
            Gui::ItemTooltip("Not showing sub-second delta times");
        }

        Gui::VerticalSeparator();

        // Quick message enable/disable
        if (!ctrlEnabled) { Gui::BeginDisabled(); }
        if (ImGui::Button(ICON_FK_BARS "##Quick", _winSettings->iconButtonSize))
        {
            ImGui::OpenPopup("Quick");
        }
        Gui::ItemTooltip("Enable/disable output messages");
        if (ImGui::BeginPopup("Quick"))
        {
            Gui::BeginMenuPersist();
            for (const auto &className: _classNames)
            {
                if (ImGui::BeginMenu(className.c_str()))
                {
                    int numMsgrates = 0;
                    const UBLOXCFG_MSGRATE_t **msgrates = ubloxcfg_getAllMsgRateCfgs(&numMsgrates);
                    for (int ix = 0; ix < numMsgrates; ix++)
                    {
                        const UBLOXCFG_MSGRATE_t *rate = msgrates[ix];
                        if (std::strncmp(className.c_str(), rate->msgName, className.size()) == 0)
                        {
                            bool enabled = false;

                            // NMEA 0183 messages are named NMEA-<talker>-<formatter>, but in the configuration
                            // library we use NMEA-STANDARD-<formatter>. Look for formatter
                            if ( (rate->msgName[5] == 'S') && (rate->msgName[12] == 'D') )
                            {
                                const std::string search = &rate->msgName[14];
                                for (const auto &entry: _messages)
                                {
                                    if (entry.first.substr(8) == search)
                                    {
                                        if ( (now - entry.second.msg->ts) < 1500 )
                                        {
                                            enabled = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            // Otherwise we can just look for the name
                            else
                            {
                                const auto entry = _messages.find(rate->msgName);
                                enabled = !( (entry == _messages.end()) || ((now - entry->second.msg->ts) > 1500) );
                            }
                            if (ImGui::MenuItem(rate->msgName, NULL, enabled))
                            {
                                _SetRate(rate, enabled ? 0 : 1);
                            }
                        }
                    }
                    ImGui::EndMenu();
                }
            }
            Gui::EndMenuPersist();
            ImGui::EndPopup();
        }
        if (!ctrlEnabled) { Gui::EndDisabled(); }

        Gui::VerticalSeparator();

        // Clear
        if (ImGui::Button(ICON_FK_ERASER "##Clear", _winSettings->iconButtonSize))
        {
            ClearData();
        }
        Gui::ItemTooltip("Clear all data");
    }

    ImGui::Separator();

    const float currWidth = ImGui::GetWindowContentRegionWidth();
    const float listWidth = 40 * _winSettings->charSize.x;
    const float dumpWidthMin = 40 * _winSettings->charSize.x;
    const float dumpWidth = (currWidth < (listWidth + dumpWidthMin)) ? dumpWidthMin : (currWidth - listWidth);
    const float scrollWidth = listWidth + dumpWidth;
    ImGui::SetNextWindowContentSize(ImVec2(scrollWidth, 0.0f));
    ImGui::BeginChild("##MessagesAndPayload", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

    auto dispEntry = _selectedEntry;

    // Messages
    {
        ImGui::BeginChild("##Messages", ImVec2(listWidth, 0.0f));

        ImGui::PushStyleColor(ImGuiCol_Text, Gui::BrightCyan);
        ImGui::TextUnformatted("Messages");
        ImGui::PopStyleColor();
        ImGui::Separator();

        const bool showDeltaTime = _receiver && !_receiver->IsIdle();

        for (auto entry = _messages.begin(); entry != _messages.end(); entry++)
        {
            auto name = entry->first;
            auto info = entry->second;
            char str[100];
            const float dt = (float)(now - info.msg->ts) * 1e-3f;
            const char flag = dt < 0.2f ? '*' : ' ';

            if (showDeltaTime)
            {
                char dtStr[10];
                if (dt < 99.9)
                {
                    std::snprintf(dtStr, sizeof(dtStr), _showSubSec ? "%4.1f" : "%4.0f", dt);
                }
                else
                {
                    std::strcpy(dtStr, "   -");
                }
                char rateStr[10];
                if ( (dt < 10.0f) && (info.rate > 0.05f) && (info.rate < 99.5f) )
                {
                    std::snprintf(rateStr, sizeof(rateStr), "%4.1f", info.rate);
                }
                else
                {
                    std::strcpy(rateStr, "   ?");
                }
                std::snprintf(str, sizeof(str), "%c%8d %s %s %s###%s",
                    flag, info.count, dtStr, rateStr, name.c_str(), name.c_str());
            }
            else
            {
                std::snprintf(str, sizeof(str), "%c%8d    -    - %s###%s",
                    flag, info.count, name.c_str(), name.c_str());
            }

            const bool selected = _selectedEntry == entry;
            if (ImGui::Selectable(str, selected))
            {
                if (selected)
                {
                    _selectedEntry = _messages.end();
                }
                else
                {
                    _selectedEntry = entry;
                }
            }
            if (ImGui::IsItemActive() || ImGui::IsItemHovered())
            {
                dispEntry = entry;
            }
        }

        ImGui::EndChild();
    }

    if (_displayedEntry != dispEntry)
    {
        _displayedEntry = dispEntry;
        _UpdateHexdump();
    }

    //ImGui::SameLine();
    Gui::VerticalSeparator();

    // Payload
    {
        ImGui::BeginChild("##Payload", ImVec2(0.0f, 0.0f));

        ImGui::PushStyleColor(ImGuiCol_Text, Gui::BrightCyan);
        ImGui::TextUnformatted("Payload");
        ImGui::PopStyleColor();
        ImGui::Separator();

        if (dispEntry != _messages.end())
        {
            const auto &msg = dispEntry->second.msg;
            ImGui::Text("Name:     %s", msg->name.c_str());
            ImGui::Text("Type:     %s", msg->typeStr.c_str());
            ImGui::Text("Source:   %s", msg->srcStr.c_str());
            ImGui::Text("Sequence: %u", msg->seq);
            ImGui::Text("Age:      %.1f s", (float)(now - msg->ts) * 1e-3f);
            ImGui::Text("Rate:     %.1f Hz", dispEntry->second.rate);
            ImGui::Text("Size:     %d", msg->size);

            ImGui::Separator();

            ImGui::TextWrapped("%s", msg->info.c_str());

            ImGui::Separator();

            ImGui::TextUnformatted(_hexDump.c_str());
        }

        ImGui::EndChild();
    }

    ImGui::EndChild(); // MessagesAndPayload

    _DrawWindowEnd();
}

void GuiWinDataMessages::_SetRate(const UBLOXCFG_MSGRATE_t *def, const uint8_t rate)
{
    std::vector<UBLOXCFG_KEYVAL_t> keyVal;
    if (def->itemUart1 != NULL) { keyVal.push_back({ .id = def->itemUart1->id, .val = { .U1 = rate } }); }
    if (def->itemUart2 != NULL) { keyVal.push_back({ .id = def->itemUart2->id, .val = { .U1 = rate } }); }
    if (def->itemSpi   != NULL) { keyVal.push_back({ .id = def->itemSpi->id,   .val = { .U1 = rate } }); }
    if (def->itemI2c   != NULL) { keyVal.push_back({ .id = def->itemI2c->id,   .val = { .U1 = rate } }); }
    if (def->itemUsb   != NULL) { keyVal.push_back({ .id = def->itemUsb->id,   .val = { .U1 = rate } }); }
    if (!keyVal.empty())
    {
        _receiver->SetConfig(true, false, false, false, keyVal, _winUid);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::_UpdateHexdump()
{
    _hexDump.clear();
    if (_displayedEntry == _messages.end())
    {
        return;
    }
    const auto &msg = _displayedEntry->second.msg;

    const uint8_t *data = msg->data;
    const int size = msg->size;

    const char i2hex[] = "0123456789abcdef";
    const uint8_t *pData = data;
    for (int ix = 0; ix < size; )
    {
        char str[70];
        memset(str, ' ', sizeof(str));
        str[50] = '|';
        str[67] = '|';
        str[68] = '\0';
        for (int ix2 = 0; (ix2 < 16) && ((ix + ix2) < size); ix2++)
        {
            //           1         2         3         4         5         6
            // 012345678901234567890123456789012345678901234567890123456789012345678
            // xx xx xx xx xx xx xx xx  xx xx xx xx xx xx xx xx  |................|\0
            // 0  1  2  3  4  5  6  7   8  9  10 11 12 13 14 15
            const uint8_t c = pData[ix + ix2];
            int pos1 = 3 * ix2;
            int pos2 = 51 + ix2;
            if (ix2 > 7)
            {
                   pos1++;
            }
            str[pos1    ] = i2hex[ (c >> 4) & 0xf ];
            str[pos1 + 1] = i2hex[  c       & 0xf ];

            str[pos2] = isprint((int)c) ? c : '.';
        }
        char buf[1024];
        std::snprintf(buf, sizeof(buf), "0x%04" PRIx8 " %05d  %s\n", ix, ix, str);
        _hexDump += std::string { buf };
        ix += 16;
    }
}

/* ****************************************************************************************************************** */
