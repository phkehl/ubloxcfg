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
    _showSubSec{false}, _selectedEntry{_messages.end()}, _displayedEntry{_messages.end()}, _hexDump{}, _classNames{}
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

    _selectedName = _winSettings->GetValue(_winName + ".selectedMessage");

    ClearData();
}

GuiWinDataMessages::~GuiWinDataMessages()
{
    const std::string selName = _selectedEntry != _messages.end() ? _selectedEntry->second.msg->name : "";
    _winSettings->SetValue(_winName + ".selectedMessage", selName);
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
                _messages.insert({ data.msg->name, MsgInfo(data.msg,
                    GuiMsg::GetRenderer(data.msg->name, _receiver, _logfile) ) });
            }
            _displayedEntry = _messages.end();
            break;
        }
        default:
            break;
    }
}

GuiWinDataMessages::MsgInfo::MsgInfo(const std::shared_ptr<Ff::ParserMsg> &_msg, std::unique_ptr<GuiMsg> _renderer) :
    msg{_msg}, count{1}, dt{}, dtIx{0}, rate{0.0}, age{0.0}, renderer{std::move(_renderer)}
{
    renderer->Update(_msg);
}

void GuiWinDataMessages::MsgInfo::Update(const std::shared_ptr<Ff::ParserMsg> &_msg)
{
    // Store time since last message [ms]
    dt[dtIx] = _msg->ts - msg->ts;
    dtIx++;
    dtIx %= NUMOF(dt);

    // Store new message
    count++;
    msg = _msg;

    // Calculate average message rate
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

    renderer->Update(_msg);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::ClearData()
{
    _messages.clear();

    if (_selectedEntry != _messages.end())
    {
        _selectedName = _selectedEntry->first;
    }

    _selectedEntry  = _messages.end();
    _displayedEntry = _messages.end();
    _UpdateHexdump();
    for (const auto &entry: _messages)
    {
        entry.second.renderer->Clear();
    }
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
    const float currWidth = ImGui::GetWindowContentRegionWidth();
    const float listWidth = 40 * _winSettings->charSize.x;
    const float dumpWidthMin = 40 * _winSettings->charSize.x;
    const float dumpWidth = (currWidth < (listWidth + dumpWidthMin)) ? dumpWidthMin : (currWidth - listWidth);
    const float scrollWidth = listWidth + dumpWidth;
    ImGui::SetNextWindowContentSize(ImVec2(scrollWidth, 0.0f));
    ImGui::BeginChild("##MessagesAndPayload", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

    auto dispEntry = _selectedEntry;

    // Messages (left side of window)
    {
        ImGui::BeginChild("##Messages", ImVec2(listWidth, 0.0f));

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

            ImGui::SameLine();

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

            ImGui::SameLine();

            // Clear
            if (ImGui::Button(ICON_FK_ERASER "##Clear", _winSettings->iconButtonSize))
            {
                ClearData();
            }
            Gui::ItemTooltip("Clear all data");
        }

        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTCYAN));
        ImGui::TextUnformatted("Messages");
        ImGui::PopStyleColor();

        ImGui::Separator();

        const bool showDeltaTime = _receiver && !_receiver->IsIdle();

        for (auto entry = _messages.begin(); entry != _messages.end(); entry++)
        {
            auto &name = entry->first;
            auto &info = entry->second;
            char str[100];
            info.age = (float)(now - info.msg->ts) * 1e-3f;
            const char flag = info.age < 0.2f ? '*' : ' ';

            // Stop displaying rate?
            if ( (info.age > 10.0f) || (info.rate < 0.05f) || (info.rate > 99.5f) )
            {
                info.rate = 0.0;
            }

            // Select message from saved config, once it appears
            if (!_selectedName.empty())
            {
                // User meanwhile selected another message
                if (_selectedEntry != _messages.end())
                {
                    _selectedName.clear();
                }
                else if (_selectedName == name)
                {
                    _selectedEntry = entry;
                }
            }


            if (showDeltaTime)
            {
                char dtStr[10];
                if (info.age < 99.9)
                {
                    std::snprintf(dtStr, sizeof(dtStr), _showSubSec ? "%4.1f" : "%4.0f", info.age);
                }
                else
                {
                    std::strcpy(dtStr, "   -");
                }
                char rateStr[10];
                if (info.rate > 0.0)
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
            // Click on message
            if (ImGui::Selectable(str, selected))
            {
                // Unselect currently selected message
                if (selected)
                {
                    _selectedEntry = _messages.end();
                }
                // Select new message
                else
                {
                    _selectedEntry = entry;
                }
            }
            // Hover message
            if ( (_selectedEntry == _messages.end()) && (ImGui::IsItemActive() || ImGui::IsItemHovered()) )
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

    // Payload (right side of window)
    {
        ImGui::BeginChild("##Payload", ImVec2(0.0f, 0.0f));

        const bool haveDispEntry = (dispEntry != _messages.end());

        // Clear
        if (!haveDispEntry) { Gui::BeginDisabled(); }
        if (ImGui::Button(ICON_FK_ERASER "##Clear", _winSettings->iconButtonSize))
        {
            dispEntry->second.renderer->Clear();
            //dispEntry->second.msg = nullptr; // TODO
        }
        Gui::ItemTooltip("Clear data");
        if (!haveDispEntry) { Gui::EndDisabled(); }

        if (haveDispEntry)
        {
            dispEntry->second.renderer->Buttons();
        }

        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
        ImGui::TextUnformatted("Payload");
        ImGui::PopStyleColor();

        ImGui::Separator();

        if (haveDispEntry)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
            //                      12345678901234567890 12345678 12345678 12345 12345 12345 12345 12345
            ImGui::TextUnformatted("Name:                Type:    Source:  Seq:  Cnt:  Age:  Rate: Size:");
            ImGui::PopStyleColor();

            const auto &info = dispEntry->second;
            const auto &msg = dispEntry->second.msg;
            ImGui::Text("%-20s %-8s %-8s %5u %5u %5.1f %5.1f %5d",
                msg->name.c_str(), msg->typeStr.c_str(), msg->srcStr.c_str(), msg->seq, info.count, info.age, info.rate, msg->size);
            if (!msg->info.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
                ImGui::TextUnformatted("Info:");
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::TextWrapped("%s", msg->info.c_str());
            }

            ImGui::Separator();

            const float numLines = MIN(_hexDump.size(), 10);
            const ImVec2 sizeAvail = ImGui::GetContentRegionAvail() -
                ImVec2(0, (numLines * _winSettings->charSize.y) + (2 * _winSettings->style.ItemSpacing.y) + 1);
            if (info.renderer->Render(msg, sizeAvail))
            {
                ImGui::Separator();
            }

            ImGui::BeginChild("##hexdump");
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            for (const auto &line: _hexDump)
            {
                ImGui::TextUnformatted(line.c_str());
            }
            ImGui::PopStyleVar();
            ImGui::EndChild();
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
        std::snprintf(buf, sizeof(buf), "0x%04" PRIx8 " %05d  %s", ix, ix, str);
        _hexDump.push_back(buf);
        ix += 16;
    }
}

/* ****************************************************************************************************************** */
