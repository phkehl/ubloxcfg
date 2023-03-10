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
#include "ff_nmea.h"
#include "ff_rtcm3.h"

#include "gui_inc.hpp"

#include "gui_win_data_messages.hpp"

/* ****************************************************************************************************************** */

GuiWinDataMessages::GuiWinDataMessages(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database),
    _showList       { true },
    _showSubSec     { false },
    _showHexDump    { true },
    _selectedEntry  {_messages.end() },
    _displayedEntry {_messages.end() }
{
    _winSize = { 130, 25 };

    _latestEpochEna = false;
    _toolbarEna = false;

    _selectedName = GuiSettings::GetValue(_winName + ".selectedMessage");
    GuiSettings::GetValue(_winName + ".showHexDump", _showHexDump, true);
    GuiSettings::GetValue(_winName + ".showList",    _showList,    true);

    _InitMsgRatesAndPolls();
    ClearData();
}

GuiWinDataMessages::~GuiWinDataMessages()
{
    const std::string selName = _selectedEntry != _messages.end() ? _selectedEntry->second.msg->name : "";
    GuiSettings::SetValue(_winName + ".selectedMessage", selName);
    GuiSettings::SetValue(_winName + ".showHexDump", _showHexDump);
    GuiSettings::SetValue(_winName + ".showList", _showList);
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataMessages::MsgRate::MsgRate(const UBLOXCFG_MSGRATE_t *_rate) :
    msgName{_rate->msgName}, rate{_rate}
{
    haveIds = ubxMessageClsId(rate->msgName, &clsId, &msgId) /* ||
              nmea3MessageClsId(rate->msgName, &clsId, &msgId) ||   // FIXME: poll NMEA or RTCM
              rtcm3MessageClsId(rate->msgName, &clsId, &msgId)*/;   //        doesn't work this way.. :-(

    // They use "RTCM-3X-TYPE1234", we use "RTCM3-TYPE1234"
    if (msgName.substr(0, 8) == "RTCM-3X-")
    {
        msgName = "RTCM3-" + msgName.substr(8);
    }

    // "(#) UBX-CLASS-MESSAGE##UBX-CLASS-MESSAGE"
    menuNameEnabled = ICON_FK_CIRCLE " ";
    menuNameEnabled += msgName + "##" + msgName;
    // "( ) UBX-CLASS-MESSAGE##UBX-CLASS-MESSAGE"
    menuNameDisabled = ICON_FK_CIRCLE_THIN " ";
    menuNameDisabled += msgName + "##" + msgName;
}

GuiWinDataMessages::MsgPoll::MsgPoll(const uint8_t _clsId, const uint8_t _msgId) :
    clsId{_clsId}, msgId{_msgId}
{
}

void GuiWinDataMessages::_InitMsgRatesAndPolls()
{
    if (!_msgRates.empty())
    {
        return;
    }

    int numMsgrates = 0;
    const UBLOXCFG_MSGRATE_t **msgrates = ubloxcfg_getAllMsgRateCfgs(&numMsgrates);
    std::string prevName = "";
    std::vector<MsgRate> *rates = nullptr;
    for (int ix = 0; ix < numMsgrates; ix++)
    {
        const UBLOXCFG_MSGRATE_t *rate = msgrates[ix];
        std::string msgName = rate->msgName;
        //DEBUG("add msg %s", msgName.c_str());

        // Create a list of rates and other info per message class
        const std::string className = msgName.substr(0, msgName.rfind('-'));
        if (!className.empty() && (className != prevName))
        {
            auto foo = _msgRates.insert({ className, std::vector<MsgRate>() });
            rates = &foo.first->second; // msg rates in this msg class
            _classNames.push_back(className);
            //DEBUG("add class %s", className.c_str());
            prevName = className;
        }

        // Add rate info
        if (rates)
        {
            rates->emplace_back(rate);
        }

        // Add poll info (some cannot be polled)
        if ( (msgName != "UBX-RXM-SFRBX") && (msgName != "UBX-RXM-RTCM") )
        {
            uint8_t clsId;
            uint8_t msgId;
            if ( ubxMessageClsId(rate->msgName, &clsId, &msgId) /* ||
                nmea3MessageClsId(rate->msgName, &clsId, &msgId) ||   // FIXME: poll NMEA or RTCM
                rtcm3MessageClsId(rate->msgName, &clsId, &msgId)*/ )  //        doesn't work this way.. :-(
            {
                _msgPolls.insert({ msgName, MsgPoll(clsId, msgId) });
            }
        }
    }

    // Pollable non-periodic messages
    _msgPolls.insert({ "UBX-MON-VER", MsgPoll(UBX_MON_CLSID, UBX_MON_VER_MSGID) });
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::_ProcessData(const InputData &data)
{
    switch (data.type)
    {
        case InputData::DATA_MSG:
        {
            // Get message info entry, create new as necessary
            MsgInfo *info = nullptr;
            auto entry = _messages.find(data.msg->name);
            if (entry == _messages.end())
            {
                auto foo = _messages.insert(
                    { data.msg->name, MsgInfo(data.msg->name, GuiMsg::GetRenderer(data.msg->name, _receiver, _logfile)) });
                info = &foo.first->second;
            }
            else
            {
                info = &entry->second;
            }

            if (info)
            {
                info->Update(data.msg);
            }

            break;
        }
        default:
            break;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataMessages::MsgInfo::MsgInfo(const std::string &_name, std::unique_ptr<GuiMsg> _renderer) :
    name{_name}, msg{nullptr}, count{0}, dt{}, dtIx{0}, rate{0.0}, age{0.0}, flag{false},
    hexdump{}, renderer{std::move(_renderer)}
{
    const auto offs = name.rfind("-");
    group = offs != std::string::npos ? name.substr(0, offs) : std::string("Other");
    groupId = ImGui::GetID(group.c_str());
}

void GuiWinDataMessages::MsgInfo::Clear()
{
    count = 0;
    msg   = nullptr;
    rate  = 0.0;
    hexdump.clear();
    renderer->Clear();
}

void GuiWinDataMessages::MsgInfo::Update(const std::shared_ptr<Ff::ParserMsg> &_msg)
{
    // Store time since last message [ms]
    if (msg)
    {
        dt[dtIx] = _msg->ts - msg->ts;
        dtIx++;
        dtIx %= NUMOF(dt);
    }

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

    // Update hexdump
    hexdump = Ff::HexDump(msg->data, msg->size);

    // Update renderer
    if (msg)
    {
        renderer->Update(msg);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::_ClearData()
{
    _messages.clear();

    if (_selectedEntry != _messages.end())
    {
        _selectedName = _selectedEntry->first;
    }

    _selectedEntry  = _messages.end();
    _displayedEntry = _messages.end();
    for (auto &entry: _messages)
    {
        entry.second.Clear();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::_UpdateInfo()
{
    _nowTs = TIME();
    for (auto entry = _messages.begin(); entry != _messages.end(); entry++)
    {
        auto &info = entry->second;

        if (info.msg)
        {
            info.age = (float)(_nowTs - info.msg->ts) * 1e-3f;
        }
        else
        {
            info.age = 9999;
        }

        // Stop displaying rate?
        if ( (info.age > 10.0f) || (info.rate < 0.05f) || (info.rate > 99.5f) )
        {
            info.rate = 0.0;
        }

        info.flag = info.age < 0.2f;

        // Select message from saved config, once it appears
        if (!_selectedName.empty())
        {
            // User meanwhile selected another message
            if (_selectedEntry != _messages.end())
            {
                _selectedName.clear();
            }
            else if (_selectedName == entry->first)
            {
                _selectedEntry = entry;
                _displayedEntry = _selectedEntry;
                _selectedName.clear();
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::_DrawContent()
{
    _UpdateInfo();

    const float listWidth = 40 * GuiSettings::charSize.x;

    const bool showList = _showList || (_displayedEntry == _messages.end());

    // Controls
    _DrawListButtons();
    Gui::VerticalSeparator(showList ? listWidth + (2 * GuiSettings::style->ItemSpacing.x) : 0);
    _DrawMessageButtons();

    ImGui::Separator();

    // Messsages list (left side of window)
    if (showList)
    {
        if (ImGui::BeginChild("##List", ImVec2(listWidth, 0.0f)))
        {
            _DrawList();
        }
        ImGui::EndChild(); // ##List

        Gui::VerticalSeparator();
    }

    // Message (right side of window)
    if (_displayedEntry != _messages.end())
    {
        if (ImGui::BeginChild("##Message", ImVec2(0.0f, 0.0f)))
        {
            _DrawMessage();
        }
        ImGui::EndChild(); // ##Message
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::_DrawListButtons()
{
    Gui::ToggleButton(ICON_FK_LIST "###ShowList", NULL, &_showList,
        "Showing message list", "Not showing message list", GuiSettings::iconSize);

    if (!_showList)
    {
        return;
    }

    ImGui::SameLine();

    Gui::ToggleButton(ICON_FK_DOT_CIRCLE_O "###ShowSubSec", ICON_FK_CIRCLE_O " ###ShowSubSec", &_showSubSec,
        "Showing sub-second delta times", "Not showing sub-second delta times", GuiSettings::iconSize);

    ImGui::SameLine();

    const bool ctrlEnabled = _receiver && _receiver->IsReady();

    // Quick message enable/disable
    {
        ImGui::BeginDisabled(!ctrlEnabled);
        if (ImGui::Button(ICON_FK_BARS "##Quick", GuiSettings::iconSize))
        {
            ImGui::OpenPopup("Quick");
        }
        if (ImGui::IsPopupOpen("Quick"))
        {
            if (ImGui::BeginPopup("Quick"))
            {
                _DrawMessagesMenu();
                ImGui::EndPopup();
            }
        }

        Gui::ItemTooltip("Enable/disable output messages");
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    // Clear
    {
        if (ImGui::Button(ICON_FK_ERASER "##Clear", GuiSettings::iconSize))
        {
            ClearData();
        }
        Gui::ItemTooltip("Clear all data");
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::_DrawMessagesMenu()
{
    Gui::BeginMenuPersist();

    for (const auto &classEntry: _msgRates)
    {
        // UBX-CLASS menu entry
        if (ImGui::BeginMenu(classEntry.first.c_str()))
        {
            for (auto &rate: classEntry.second)
            {
                // Check if message is enabled  FIXME: this could be nicer...
                bool enabled = false;

                // NMEA 0183 messages are named NMEA-<talker>-<formatter>, but in the configuration
                // library we use NMEA-*S*TANDAR*D*-<formatter>. Look for formatter
                if ( (rate.msgName[5] == 'S') && (rate.msgName[12] == 'D') ) //
                {
                    const std::string search = rate.msgName.substr(14);
                    for (const auto &entry: _messages)
                    {
                        if ( (entry.first.size() > 8) && (entry.first.substr(8) == search) )
                        {
                            if ( (_nowTs - entry.second.msg->ts) < 1500 )
                            {
                                enabled = true;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    const auto entry = _messages.find(rate.msgName);
                    enabled = !( (entry == _messages.end()) || ((_nowTs - entry->second.msg->ts) > 1500) );
                }

                // UBX-CLASS-MESSAGE menu entry
                if (ImGui::BeginMenu(enabled ? rate.menuNameEnabled.c_str() : rate.menuNameDisabled.c_str()))
                {
                    ImGui::BeginDisabled(enabled);
                    if (ImGui::MenuItem("Enable"))
                    {
                        _SetRate(rate, 1);
                    }
                    ImGui::EndDisabled();
                    ImGui::BeginDisabled(!enabled);
                    if (ImGui::MenuItem("Disable"))
                    {
                        _SetRate(rate, 0);
                    }
                    ImGui::EndDisabled();
                    ImGui::BeginDisabled(enabled || (_msgPolls.find(rate.msgName) == _msgPolls.end()) );
                    if (ImGui::MenuItem("Poll"))
                    {
                        _PollMsg(_msgPolls.find(rate.msgName)->second);
                    }
                    ImGui::EndDisabled();

                    ImGui::EndMenu();
                }
            }

            ImGui::EndMenu();
        }
    }
    Gui::EndMenuPersist();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::_SetRate(const GuiWinDataMessages::MsgRate &def, const int rate)
{
    // Enable/disable
    if (rate >= 0)
    {
        std::vector<UBLOXCFG_KEYVAL_t> keyVal;
        if (def.rate->itemUart1 != NULL) { keyVal.push_back({ .id = def.rate->itemUart1->id, .val = { .U1 = (uint8_t)MIN(rate, 0xff) } }); }
        if (def.rate->itemUart2 != NULL) { keyVal.push_back({ .id = def.rate->itemUart2->id, .val = { .U1 = (uint8_t)MIN(rate, 0xff) } }); }
        if (def.rate->itemSpi   != NULL) { keyVal.push_back({ .id = def.rate->itemSpi->id,   .val = { .U1 = (uint8_t)MIN(rate, 0xff) } }); }
        if (def.rate->itemI2c   != NULL) { keyVal.push_back({ .id = def.rate->itemI2c->id,   .val = { .U1 = (uint8_t)MIN(rate, 0xff) } }); }
        if (def.rate->itemUsb   != NULL) { keyVal.push_back({ .id = def.rate->itemUsb->id,   .val = { .U1 = (uint8_t)MIN(rate, 0xff) } }); }
        if (!keyVal.empty())
        {
            _receiver->SetConfig(true, false, false, false, keyVal);
        }
    }
}

void GuiWinDataMessages::_PollMsg(const MsgPoll &def)
{
    uint8_t poll[UBX_FRAME_SIZE] = { 0 };
    const int size = ubxMakeMessage(def.clsId, def.msgId, poll, 0, poll);
    if (size > 0)
    {
        _receiver->Send(poll, size);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::_DrawMessageButtons()
{
    const bool haveDispEntry = (_displayedEntry != _messages.end());

    Gui::ToggleButton(ICON_FK_FILE_CODE_O "###ShowHexdump", NULL, &_showHexDump,
        "Showing hexdump", "Not showing hexdump", GuiSettings::iconSize);

    ImGui::SameLine();

    // Clear
    {
        ImGui::BeginDisabled(!haveDispEntry);
        if (ImGui::Button(ICON_FK_ERASER "##ClearMsg", GuiSettings::iconSize))
        {
            _displayedEntry->second.Clear();
        }
        ImGui::EndDisabled();
        Gui::ItemTooltip("Clear message data");
    }

    ImGui::SameLine();

    // Poll
    {
        ImGui::BeginDisabled( !haveDispEntry || !_receiver || !_receiver->IsReady() ||
            (_msgPolls.find(_displayedEntry->first) == _msgPolls.end()) );
        if (ImGui::Button(ICON_FK_REFRESH "##PollMsg", GuiSettings::iconSize))
        {
            _PollMsg(_msgPolls.find(_displayedEntry->first)->second);
        }
        ImGui::EndDisabled();
        Gui::ItemTooltip("Poll message");
    }

    ImGui::SameLine();

    // Step
    {
        ImGui::BeginDisabled( !haveDispEntry || !_logfile || !_logfile->CanStep() );
        if (ImGui::Button(ICON_FK_STEP_FORWARD "##StepMsg", GuiSettings::iconSize))
        {
            _logfile->StepMsg(_displayedEntry->first);
        }
        ImGui::EndDisabled();
        Gui::ItemTooltip("Step message");
    }

    // Optional message view buttons
    if (haveDispEntry)
    {
        ImGui::PushID(std::addressof(_displayedEntry->second.renderer));
        _displayedEntry->second.renderer->Buttons();
        ImGui::PopID();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::_DrawList()
{
    const bool showDeltaTime = _receiver && !_receiver->IsIdle();

    Gui::TextDim("      Count: Age: Rate: Name:");

    _displayedEntry = _selectedEntry;
    uint64_t prevGroupId = 0;
    bool drawNodes = false;
    for (auto entry = _messages.begin(); entry != _messages.end(); entry++)
    {
        auto &name = entry->first;
        auto &info = entry->second;

        if (prevGroupId != info.groupId)
        {
            if (drawNodes && (prevGroupId != 0))
            {
                ImGui::TreePop();
            }
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            drawNodes = ImGui::TreeNode(info.group.c_str());

            prevGroupId = info.groupId;
        }

        if (!drawNodes)
        {
            continue;
        }

        char str[100];
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
                info.flag ? '*' : ' ', info.count, dtStr, rateStr, name.c_str(), name.c_str());
        }
        else
        {
            std::snprintf(str, sizeof(str), "%c%8d    -    - %s###%s",
                info.flag ? '*' : ' ', info.count, name.c_str(), name.c_str());
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
                _displayedEntry = _selectedEntry;
            }
        }
        // Hover message
        if ( (_selectedEntry == _messages.end()) && (ImGui::IsItemActive() || ImGui::IsItemHovered()) )
        {
            _displayedEntry = entry;
        }
    }
    if (drawNodes && (prevGroupId != 0))
    {
        ImGui::TreePop();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::_DrawMessage()
{
    if (!_displayedEntry->second.msg)
    {
        return;
    }

    const auto &info = _displayedEntry->second;
    const auto &msg  = _displayedEntry->second.msg;

    const float sepPadding     = (2 * GuiSettings::style->ItemSpacing.y) + 1;
    const ImVec2 sizeAvail     = ImGui::GetContentRegionAvail();
    const ImVec2 msgInfoSize   { 0, (2 * GuiSettings::charSize.y) + (1 * GuiSettings::style->ItemSpacing.y) };
    const ImVec2 hexDumpSize   { 0, _showHexDump ? (info.hexdump.size(), 10) * GuiSettings::charSize.y : 0 };
    const float remHeight      = sizeAvail.y - msgInfoSize.y - hexDumpSize.y - sepPadding - (_showHexDump ? sepPadding : 0);
    const float minHeight      = 10 * GuiSettings::charSize.y;
    const ImVec2 msgDetailSize { 0, MAX(remHeight, minHeight) };

    // Message info
    if (ImGui::BeginChild("##MsgInfo", msgInfoSize))
    {
        //            12345678901234567890 12345678 12345678 12345 12345 12345 12345 12345
        Gui::TextDim("Name:                  Type:    Source:  Seq:  Cnt:  Age:  Rate: Size:");
        if (_receiver && !_receiver->IsIdle())
        {
            ImGui::Text("%-20s %c %-8s %-8s %5u %5u %5.1f %5.1f %5d",
                msg->name.c_str(), info.flag ? '*' : ' ', msg->typeStr.c_str(), msg->srcStr.c_str(),
                msg->seq, info.count, info.age, info.rate, msg->size);
        }
        else
        {
            ImGui::Text("%-20s %c %-8s %-8s %5u %5u     -     - %5d",
                msg->name.c_str(), info.flag ? '*' : ' ', msg->typeStr.c_str(), msg->srcStr.c_str(),
                msg->seq, info.count, msg->size);
        }
    }
    ImGui::EndChild();

    ImGui::Separator();

    // Message details
    if (ImGui::BeginChild("##MsgDetails", msgDetailSize, false, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse))
    {
        if (!info.renderer->Render(msg, ImGui::GetContentRegionAvail()))
        {
            if (!msg->info.empty())
            {
                Gui::TextDim("Info:");
                ImGui::SameLine();
                ImGui::TextWrapped("%s", msg->info.c_str());
            }
        }
    }
    ImGui::EndChild();

    // Hexdump
    if (_showHexDump)
    {
        ImGui::Separator();
        if (ImGui::BeginChild("##HexDump", hexDumpSize))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            for (const auto &line: info.hexdump)
            {
                ImGui::TextUnformatted(line.c_str());
            }
            ImGui::PopStyleVar();
        }
        ImGui::EndChild();
        if (ImGui::BeginPopupContextItem("Copy"))
        {
            std::string text = "";
            if (ImGui::MenuItem("Copy (full dump)"))
            {
                for (const auto &line: info.hexdump)
                {
                    text += line;
                    text += "\n";
                }
            }
            if (ImGui::MenuItem("Copy (hex only)"))
            {
                for (const auto &line: info.hexdump)
                {
                    text += line.substr(14, 48);
                    text += "\n";
                }
            }

            const bool isStr = msg->type == Ff::ParserMsg::Type_e::NMEA;

            ImGui::BeginDisabled(!isStr);
            if (ImGui::MenuItem("Copy (string)"))
            {
                text.assign(msg->data, msg->data + msg->size);
            }
            ImGui::EndDisabled();

            if (!text.empty())
            {
                ImGui::SetClipboardText(text.c_str());
            }
            ImGui::EndPopup();
        }
    }
}

/* ****************************************************************************************************************** */
