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

#include "ff_ubx.h"
#include "ff_nmea.h"
#include "ff_rtcm3.h"

#include "gui_win_data_inc.hpp"

#include "gui_win_data_messages.hpp"

/* ****************************************************************************************************************** */

GuiWinDataMessages::GuiWinDataMessages(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database),
    _showList{true}, _showSubSec{false}, _showHexDump{true},
    _selectedEntry{_messages.end()}, _displayedEntry{_messages.end()}, _classNames{}
{
    _winSize = { 130, 25 };

    _selectedName = _winSettings->GetValue(_winName + ".selectedMessage");
    _winSettings->GetValue(_winName + ".showHexDump", _showHexDump, false);
    _winSettings->GetValue(_winName + ".showList",    _showList,    false);

    _InitMsgRates();
    ClearData();
}

GuiWinDataMessages::~GuiWinDataMessages()
{
    const std::string selName = _selectedEntry != _messages.end() ? _selectedEntry->second.msg->name : "";
    _winSettings->SetValue(_winName + ".selectedMessage", selName);
    _winSettings->SetValue(_winName + ".showHexDump", _showHexDump);
    _winSettings->SetValue(_winName + ".showList", _showList);
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

void GuiWinDataMessages::_InitMsgRates()
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

        // Create a list of rates and other info per message class
        const std::string className = msgName.substr(0, msgName.rfind('-'));
        if (!className.empty() && (className != prevName))
        {
            auto foo = _msgRates.insert({ className, std::vector<MsgRate>() });
            rates = &foo.first->second;
            _classNames.push_back(className);
            prevName = className;
        }

        // Add rate info
        rates->emplace_back(rate);
    }
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
            // Get message info entry, create new as necessary
            MsgInfo *info = nullptr;
            auto entry = _messages.find(data.msg->name);
            if (entry == _messages.end())
            {
                auto foo = _messages.insert(
                    { data.msg->name, GuiMsg::GetRenderer(data.msg->name, _receiver, _logfile) });
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

void GuiWinDataMessages::Loop(const uint32_t &frame, const double &now)
{
    UNUSED(frame);
    _nowIm = now;
    _nowTs = TIME();
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataMessages::MsgInfo::MsgInfo(std::unique_ptr<GuiMsg> _renderer) :
    msg{nullptr}, count{0}, dt{}, dtIx{0}, rate{0.0}, age{0.0}, flag{false},
    hexdump{}, renderer{std::move(_renderer)}
{
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
    const uint8_t *data = msg->data;
    const int size = msg->size;

    hexdump.clear();
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
        hexdump.push_back(buf);
        ix += 16;
    }

    // Update renderer
    if (msg)
    {
        renderer->Update(msg);
    }
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
    for (auto &entry: _messages)
    {
        entry.second.Clear();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::_UpdateInfo()
{
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

void GuiWinDataMessages::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    _UpdateInfo();

    const float listWidth = 40 * _winSettings->charSize.x;

    // Controls
    _DrawListButtons();
    Gui::VerticalSeparator(_showList ? listWidth + (2 * _winSettings->style.ItemSpacing.x) : 0);
    _DrawMessageButtons();

    ImGui::Separator();

    // Messsages list (left side of window)
    if (_showList)
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

    _DrawWindowEnd();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::_DrawListButtons()
{
    ToggleButton(ICON_FK_LIST "###ShowList", NULL, &_showList,
        "Showing message list", "Not showing message list");

    if (!_showList)
    {
        return;
    }

    ImGui::SameLine();

    ToggleButton(ICON_FK_DOT_CIRCLE_O "###ShowSubSec", ICON_FK_CIRCLE_O " ###ShowSubSec", &_showSubSec,
        "Showing sub-second delta times", "Not showing sub-second delta times");

    ImGui::SameLine();

    const bool ctrlEnabled = _receiver && _receiver->IsReady();

    // Quick message enable/disable
    if (!ctrlEnabled) { Gui::BeginDisabled(); }
    if (ImGui::Button(ICON_FK_BARS "##Quick", _winSettings->iconButtonSize))
    {
        ImGui::OpenPopup("Quick");
    }

    if (ImGui::IsPopupOpen("Quick"))
    {
        ImGui::PushStyleColor(ImGuiCol_PopupBg, _winSettings->style.Colors[ImGuiCol_MenuBarBg]);
        if (ImGui::BeginPopup("Quick"))
        {
            _DrawMessagesMenu();
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor();
    }

    Gui::ItemTooltip("Enable/disable output messages");
    if (!ctrlEnabled) { Gui::EndDisabled(); }

    ImGui::SameLine();

    // Clear
    if (ImGui::Button(ICON_FK_ERASER "##Clear", _winSettings->iconButtonSize))
    {
        ClearData();
    }
    Gui::ItemTooltip("Clear all data");

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
                    if (enabled) { Gui::BeginDisabled(); }
                    if (ImGui::MenuItem("Enable"))
                    {
                        _SetRate(rate, 1);
                    }
                    if (enabled) { Gui::EndDisabled(); }
                    if (!enabled) { Gui::BeginDisabled(); }
                    if (ImGui::MenuItem("Disable"))
                    {
                        _SetRate(rate, 0);
                    }
                    if (!enabled) { Gui::EndDisabled(); }
                    if (enabled || !rate.haveIds) { Gui::BeginDisabled(); }
                    if (ImGui::MenuItem("Poll"))
                    {
                        _SetRate(rate, -1);
                    }
                    if (enabled || !rate.haveIds) { Gui::EndDisabled(); }

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
    // Poll
    if (rate < 0)
    {
        uint8_t poll[UBX_FRAME_SIZE];
        const int size = ubxMakeMessage(def.clsId, def.msgId, poll, 0, poll);
        if (size > 0)
        {
            _receiver->Send(poll, size, _winUid);
        }
    }
    // Enable/disable
    else
    {
        std::vector<UBLOXCFG_KEYVAL_t> keyVal;
        if (def.rate->itemUart1 != NULL) { keyVal.push_back({ .id = def.rate->itemUart1->id, .val = { .U1 = (uint8_t)MIN(rate, 0xff) } }); }
        if (def.rate->itemUart2 != NULL) { keyVal.push_back({ .id = def.rate->itemUart2->id, .val = { .U1 = (uint8_t)MIN(rate, 0xff) } }); }
        if (def.rate->itemSpi   != NULL) { keyVal.push_back({ .id = def.rate->itemSpi->id,   .val = { .U1 = (uint8_t)MIN(rate, 0xff) } }); }
        if (def.rate->itemI2c   != NULL) { keyVal.push_back({ .id = def.rate->itemI2c->id,   .val = { .U1 = (uint8_t)MIN(rate, 0xff) } }); }
        if (def.rate->itemUsb   != NULL) { keyVal.push_back({ .id = def.rate->itemUsb->id,   .val = { .U1 = (uint8_t)MIN(rate, 0xff) } }); }
        if (!keyVal.empty())
        {
            _receiver->SetConfig(true, false, false, false, keyVal, _winUid);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::_DrawMessageButtons()
{
    const bool haveDispEntry = (_displayedEntry != _messages.end());

    ToggleButton(ICON_FK_FILE_CODE_O "###ShowHexdump", NULL, &_showHexDump,
        "Showing hexdump", "Not showing hexdump");

    ImGui::SameLine();

    // Clear
    if (!haveDispEntry) { Gui::BeginDisabled(); }
    if (ImGui::Button(ICON_FK_ERASER "##ClearMsg", _winSettings->iconButtonSize))
    {
        _displayedEntry->second.Clear();
    }
    Gui::ItemTooltip("Clear message data");
    if (!haveDispEntry) { Gui::EndDisabled(); }

    if (haveDispEntry)
    {
        _displayedEntry->second.renderer->Buttons();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataMessages::_DrawList()
{
    const bool showDeltaTime = _receiver && !_receiver->IsIdle();

    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
    ImGui::TextUnformatted("  Count: Age: Rate: Name:");
    ImGui::PopStyleColor();

    _displayedEntry = _selectedEntry;
    for (auto entry = _messages.begin(); entry != _messages.end(); entry++)
    {
        auto &name = entry->first;
        auto &info = entry->second;
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

    const float sepPadding     = (2 * _winSettings->style.ItemSpacing.y) + 1;
    const ImVec2 sizeAvail     = ImGui::GetContentRegionAvail();
    const ImVec2 msgInfoSize   { 0, (2 * _winSettings->charSize.y) + (1 * _winSettings->style.ItemSpacing.y) };
    const ImVec2 hexDumpSize   { 0, _showHexDump ? (info.hexdump.size(), 10) * _winSettings->charSize.y : 0 };
    const float remHeight      = sizeAvail.y - msgInfoSize.y - hexDumpSize.y - sepPadding - (_showHexDump ? sepPadding : 0);
    const float minHeight      = 10 * _winSettings->charSize.y;
    const ImVec2 msgDetailSize { 0, MAX(remHeight, minHeight) };

    // Message info
    if (ImGui::BeginChild("##MsgInfo", msgInfoSize))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
        //                      12345678901234567890 12345678 12345678 12345 12345 12345 12345 12345
        ImGui::TextUnformatted("Name:                  Type:    Source:  Seq:  Cnt:  Age:  Rate: Size:");
        ImGui::PopStyleColor();
        ImGui::Text("%-20s %c %-8s %-8s %5u %5u %5.1f %5.1f %5d",
            msg->name.c_str(), info.flag ? '*' : ' ', msg->typeStr.c_str(), msg->srcStr.c_str(),
            msg->seq, info.count, info.age, info.rate, msg->size);
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
                ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
                ImGui::TextUnformatted("Info:");
                ImGui::PopStyleColor();
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
        if (ImGui::BeginPopupContextItem("Constants"))
        {
            std::string text = "";
            if (ImGui::MenuItem("Copy (all)"))
            {
                for (const auto &line: info.hexdump)
                {
                    text += line;
                    text += "\n";
                }
            }
            if (ImGui::MenuItem("Copy (data)"))
            {
                for (const auto &line: info.hexdump)
                {
                    text += line.substr(14, 48);
                    text += "\n";
                }
            }
            if (!text.empty())
            {
                ImGui::SetClipboardText(text.c_str());
            }
            ImGui::EndPopup();
        }
    }
}

/* ****************************************************************************************************************** */
