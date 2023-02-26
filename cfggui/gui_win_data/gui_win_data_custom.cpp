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

#include <algorithm>
#include <cstring>

#include "ff_ubx.h"
#include "ff_nmea.h"
#include "ff_rtcm3.h"

#include "gui_inc.hpp"

#include "gui_win_data_custom.hpp"

/* ****************************************************************************************************************** */

GuiWinDataCustom::GuiWinDataCustom(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database),
    _currentTab            { nullptr },
    _tabbar                { WinName() },
    _dataIsStr             { false },
    _textLfToCrLf          { false },
    _ubxMessage            { nullptr },
    _ubxClsId              { 0 },
    _ubxMsgId              { 0 },
    _ubxLength             { 0 },
    _ubxLengthUser         { false },
    _ubxChecksum           { 0 },
    _ubxChecksumUser       { false },
    _ubxChecksumBad        { false },
    _nmeaTalkerChoiceIx    { 0 },
    _nmeaFormatterChoiceIx { 0 },
    _nmeaChecksumUser      { false },
    _nmeaChecksumBad       { false }
{
    _winSize = { 80, 20 };

    _latestEpochEna = false;
    _toolbarEna = false;

    // Make list of UBX messages
    int nMsgDefs = 0;
    const UBX_MSGDEF_t *msgDefs =ubxMessageDefs(&nMsgDefs);
    for (int ix = 0; ix < nMsgDefs; ix++)
    {
        _ubxMessages.emplace_back(msgDefs[ix].name, msgDefs[ix].clsId, msgDefs[ix].msgId);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataCustom::_SetData(const std::string &text)
{
    _dataIsStr = true;
    if (text.empty())
    {
        _data.clear();
        _hexDump.clear();
    }
    else
    {
        _data = std::vector<uint8_t>((const uint8_t *)text.data(), (const uint8_t *)text.data() + text.size());
        _hexDump = Ff::HexDump(_data);
    }
}

void GuiWinDataCustom::_SetData(const std::vector<uint8_t> bin)
{
    _dataIsStr = false;
    if (bin.empty())
    {
        _data.clear();
        _hexDump.clear();
    }
    else
    {
        _data = bin;
        _hexDump = Ff::HexDump(_data);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

std::vector<uint8_t> GuiWinDataCustom::_HexToBin(const std::string &hex)
{
    std::string copy = hex;
    Ff::StrReplace(copy, "\n", " ");

    const auto parts = Ff::StrSplit(copy, " ");
    std::vector<uint8_t> bin;
    bool okay = true;
    for (auto part: parts)
    {
        int n = 0;
        uint8_t byte;
        if ( (std::sscanf(part.c_str(), "%hhx%n", &byte, &n) == 1) && (n == 2) )
        {
            bin.push_back(byte);
        }
        else
        {
            okay = false;
            break;
        }
    }

    if (!okay)
    {
        bin.clear();
    }

    return bin;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataCustom::_DrawContent()
{
    const float  sepPadding   = (2 * GuiSettings::style->ItemSpacing.y) + 1;
    const ImVec2 sizeAvail    = ImGui::GetContentRegionAvail();
    const ImVec2 hexDumpSize  { 0, 10 * GuiSettings::charSize.y };
    const float  remHeight    = sizeAvail.y - hexDumpSize.y - sepPadding;
    const float  minHeight    = 10 * GuiSettings::charSize.y;
    const ImVec2 editSize     { 0, MAX(remHeight, minHeight) };

    if (ImGui::BeginChild("##Edit", editSize))
    {
        if (_tabbar.Begin())
        {
            const char *rawTab   = "Raw";
            const char *ubxTab   = "UBX";
            const char *nmeaTab  = "NMEA";
            // const char *rtcm3Tab = "RTCM3";
            const char *textTab  = "Text";
            const char *prevTab = _currentTab;

            _tabbar.Item(rawTab,   [&]() { _currentTab = rawTab;   _DrawEditRaw(prevTab != _currentTab); });
            _tabbar.Item(ubxTab,   [&]() { _currentTab = ubxTab;   _DrawEditUbx(prevTab != _currentTab); });
            _tabbar.Item(nmeaTab,  [&]() { _currentTab = nmeaTab;  _DrawEditNmea(prevTab != _currentTab); });
            //_tabbar.Item(rtcm3Tab, [&]() { _currentTab = rtcm3Tab; _DrawEditRtcm3(prevTab != _currentTab); });
            _tabbar.Item(textTab,  [&]() { _currentTab = textTab;  _DrawEditText(prevTab != _currentTab); });

            _tabbar.End();
        }
        // if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_FittingPolicyScroll))
        // {

        //     if (ImGui::BeginTabItem(rawTab))
        //     {
        //         _currentTab = rawTab;
        //         _DrawEditRaw(prevTab != _currentTab);
        //         ImGui::EndTabItem();
        //     }
        //     if (ImGui::BeginTabItem(ubxTab))
        //     {
        //         _currentTab = ubxTab;
        //         _DrawEditUbx(prevTab != _currentTab);
        //         ImGui::EndTabItem();
        //     }
        //     if (ImGui::BeginTabItem(nmeaTab))
        //     {
        //         _currentTab = nmeaTab;
        //         _DrawEditNmea(prevTab != _currentTab);
        //         ImGui::EndTabItem();
        //     }
        //     // if (ImGui::BeginTabItem(rtcm3Tab))
        //     // {
        //     //     _currentTab = rtcm3Tab;
        //     //     _DrawEditRtcm3(prevTab != _currentTab);
        //     //     ImGui::EndTabItem();
        //     // }
        //     if (ImGui::BeginTabItem(textTab))
        //     {
        //         _currentTab = textTab;
        //         _DrawEditText(prevTab != _currentTab);
        //         ImGui::EndTabItem();
        //     }
        //     ImGui::EndTabBar();
        // }
    }
    ImGui::EndChild();

    ImGui::Separator();

    if (ImGui::BeginChild("##Hexdump", hexDumpSize))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        for (const auto &line: _hexDump)
        {
            ImGui::TextUnformatted(line.c_str());
        }
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    if (ImGui::BeginPopupContextItem("Copy"))
    {
        const bool disabled = _data.empty();
        std::string text = "";

        ImGui::BeginDisabled(disabled);
        if (ImGui::MenuItem("Copy (full dump)"))
        {
            for (const auto &line: _hexDump)
            {
                text += line;
                text += "\n";
            }
        }
        ImGui::EndDisabled();

        ImGui::BeginDisabled(disabled);
        if (ImGui::MenuItem("Copy (hex only)"))
        {
            for (const auto &line: _hexDump)
            {
                text += line.substr(14, 48);
                text += "\n";
            }
        }
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!_dataIsStr || disabled);
        if (ImGui::MenuItem("Copy (string)"))
        {
            text.assign(_data.begin(), _data.end());
        }
        ImGui::EndDisabled();

        if (!text.empty())
        {
            ImGui::SetClipboardText(text.c_str());
        }
        ImGui::EndPopup();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

enum GuiWinDataCustom::Action_e GuiWinDataCustom::_DrawActionButtons()
{
    Action_e action = ACTION_NONE;

    if (ImGui::Button(ICON_FK_ERASER "##Clear", GuiSettings::iconSize))
    {
        action = ACTION_CLEAR;
    }
    Gui::ItemTooltip("Clear");

    ImGui::SameLine();

    const bool canSend = !_data.empty() && _receiver && _receiver->IsReady();
    ImGui::BeginDisabled(!canSend);
    if (ImGui::Button(ICON_FK_THUMBS_UP "##Send", GuiSettings::iconSize))
    {
        _receiver->Send(_data.data(), (int)_data.size());
    }
    ImGui::EndDisabled();

    return action;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataCustom::_DrawEditText(const bool refresh)
{
    bool dirty = refresh;

    // Buttons
    const auto action = _DrawActionButtons();
    ImGui::SameLine();
    if (Gui::ToggleButton(ICON_FK_PARAGRAPH "##Crlf", NULL, &_textLfToCrLf,
        "Replacing LF (0x0a) with CRLF (0x0a 0x0d)", "Not replacing LF (0x0a) with CRLF (0x0a 0x0d)", GuiSettings::iconSize))
    {
        dirty = true;
    }

    // Actions
    if (action == ACTION_CLEAR)
    {
        _textEdit.clear();
        dirty = true;
    }

    // Checks
    bool bad = false;

    // Inputs
    ImGui::Separator();
    ImGui::TextUnformatted("Text");
    if (bad) { ImGui::PushStyleColor(ImGuiCol_Border, GUI_COLOUR(TEXT_ERROR)); }
    if (ImGui::InputTextMultiline("##EditText", &_textEdit, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        dirty = true;
    }
    if (bad) { ImGui::PopStyleColor(); }

    // Update
    if (dirty)
    {
        if (bad)
        {
            _SetData();
        }
        else
        {
            std::string text = _textEdit;
            if (_textLfToCrLf)
            {
                Ff::StrReplace(text, "\n", "\r\n");
            }
            _SetData(text);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataCustom::_DrawEditRaw(const bool refresh)
{
    bool dirty = refresh;

    // Buttons
    const auto action = _DrawActionButtons();

    // Actions
    if (action == ACTION_CLEAR)
    {
        _rawHex.clear();
        dirty = true;
    }

    // Checks
    const bool bad = !_rawHex.empty() && _data.empty();

    // Inputs
    ImGui::Separator();
    ImGui::TextUnformatted("Data (hex)");
    if (bad) { ImGui::PushStyleColor(ImGuiCol_Border, GUI_COLOUR(TEXT_ERROR)); }
    if (ImGui::InputTextMultiline("##EditRaw", &_rawHex, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        dirty = true;
    }
    if (bad) { ImGui::PopStyleColor(); }

    // Update
    if (dirty)
    {
        _SetData( _HexToBin(_rawHex) );
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataCustom::_DrawEditUbx(const bool refresh)
{
    bool payloadDirty  = refresh;
    bool idsDirty      = refresh;
    bool lengthDirty   = refresh;
    bool messageDirty  = refresh;
    bool checksumDirty = refresh;

    // Buttons
    const auto action = _DrawActionButtons();

    // Actions
    if (action == ACTION_CLEAR)
    {
        payloadDirty  = true;
        idsDirty      = true;
        lengthDirty   = true;
        messageDirty  = true;
        checksumDirty = true;
        _ubxPayloadHex.clear();
        _ubxMessage = nullptr;
        _ubxPayload.clear();
        _ubxClsId = 0x00;
        _ubxMsgId = 0x00;
        _ubxLength = 0;
        _ubxChecksum = 0x0000;
        _ubxLengthUser = false;
        _ubxLengthBad = false;
        _ubxChecksumUser = false;
        _ubxChecksumBad = false;
    }

    // Checks
    const bool badPayload = !_ubxPayloadHex.empty() && _ubxPayload.empty();

    // Inputs
    ImGui::Separator();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Message");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(GuiSettings::charSize.x * 25);
    if (ImGui::BeginCombo("##msgName", _ubxMessage ? _ubxMessage : "", ImGuiComboFlags_HeightLarge))
    {
        if (ImGui::Selectable("##None", !_ubxMessage))
        {
            _ubxMessage    = nullptr;
            _ubxClsId      = 0x00;
            _ubxMsgId      = 0x00;
            messageDirty = true;
        }
        if (!_ubxMessage)
        {
            ImGui::SetItemDefaultFocus();
        }
        for (const auto &msg: _ubxMessages)
        {
            const bool selected = _ubxMessage && (_ubxMessage == msg.name);
            if (ImGui::Selectable(msg.name, selected))
            {
                _ubxMessage    = msg.name;
                _ubxClsId      = msg.clsId;
                _ubxMsgId      = msg.msgId;
                messageDirty = true;
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    Gui::VerticalSeparator();
    ImGui::TextUnformatted("Class ID");
    ImGui::SameLine();
    ImGui::PushItemWidth((GuiSettings::charSize.x * 2) + (GuiSettings::style->ItemInnerSpacing.x * 2));
    if (ImGui::InputScalar("##ClsId", ImGuiDataType_U8, &_ubxClsId, NULL, NULL, "%02x", ImGuiInputTextFlags_CharsHexadecimal))
    {
        idsDirty = true;
    }
    Gui::VerticalSeparator();
    ImGui::TextUnformatted("Messsage ID");
    ImGui::SameLine();
    ImGui::PushItemWidth((GuiSettings::charSize.x * 2) + (GuiSettings::style->ItemInnerSpacing.x * 2));
    if (ImGui::InputScalar("##MsgId", ImGuiDataType_U8, &_ubxMsgId, NULL, NULL, "%02x", ImGuiInputTextFlags_CharsHexadecimal))
    {
        idsDirty = true;
    }
    Gui::VerticalSeparator();
    ImGui::TextUnformatted("Length");
    ImGui::SameLine();
    ImGui::PushItemWidth((GuiSettings::charSize.x * 5) + (GuiSettings::style->ItemInnerSpacing.x * 2));
    if (_ubxLengthBad) { ImGui::PushStyleColor(ImGuiCol_Border, GUI_COLOUR(TEXT_ERROR)); }
    if (ImGui::InputScalar("##Length", ImGuiDataType_U16, &_ubxLength, NULL, NULL, "%u", ImGuiInputTextFlags_CharsDecimal))
    {
        lengthDirty = true;
        _ubxLengthUser = true;
    }
    if (_ubxLengthBad) { ImGui::PopStyleColor(); }
    Gui::VerticalSeparator();
    ImGui::TextUnformatted("Checksum");
    ImGui::SameLine();
    ImGui::PushItemWidth((GuiSettings::charSize.x * 4) + (GuiSettings::style->ItemInnerSpacing.x * 2));
    if (_ubxChecksumBad) { ImGui::PushStyleColor(ImGuiCol_Border, GUI_COLOUR(TEXT_ERROR)); }
    if (ImGui::InputScalar("##Checksum", ImGuiDataType_U16, &_ubxChecksum, NULL, NULL, "%04x", ImGuiInputTextFlags_CharsHexadecimal))
    {
        checksumDirty = true;
        _ubxChecksumUser = true;
    }
    if (_ubxChecksumBad) { ImGui::PopStyleColor(); }
    ImGui::Separator();
    ImGui::TextUnformatted("Payload (hex)");
    if (badPayload) { ImGui::PushStyleColor(ImGuiCol_Border, GUI_COLOUR(TEXT_ERROR)); }
    if (ImGui::InputTextMultiline("##EditUbx", &_ubxPayloadHex, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        payloadDirty = true;
        _ubxLengthUser = false;
        _ubxChecksumUser = false;
    }
    if (badPayload) { ImGui::PopStyleColor(); }

    // Update messages popup if message IDs were changed
    if (idsDirty)
    {
        auto entry = std::find_if(_ubxMessages.cbegin(), _ubxMessages.cend(),
           [this](const UbxMsg &m) { return (m.clsId == _ubxClsId) && (m.msgId == _ubxMsgId); });
        _ubxMessage = entry != _ubxMessages.end() ? entry->name : nullptr;
    }

    // Update
    if (payloadDirty || idsDirty || messageDirty || lengthDirty || checksumDirty)
    {
        _ubxPayload = _HexToBin(_ubxPayloadHex);

        std::vector<uint8_t> msg(_ubxPayload.size() + UBX_FRAME_SIZE);
        /*const int size = */ubxMakeMessage(_ubxClsId, _ubxMsgId, _ubxPayload.data(), _ubxPayload.size(), msg.data());

        if (_ubxChecksumUser)
        {
            const uint8_t c1 = (_ubxChecksum >> 8) & 0xff;
            const uint8_t c2 = _ubxChecksum & 0xff;
            _ubxChecksumBad = (msg[msg.size() - 2] != c1) || (msg[msg.size() - 1] != c2);
            msg[msg.size() - 2] = c1;
            msg[msg.size() - 1] = c2;
        }
        else
        {
            _ubxChecksum = ((uint16_t)msg[msg.size() - 2] << 8) | msg[msg.size() - 1];
            _ubxChecksumBad = false;
        }

        if (_ubxLengthUser)
        {
            const uint8_t l1 = _ubxLength & 0xff;
            const uint8_t l2 = (_ubxLength >> 8) & 0xff;
            _ubxLengthBad = (msg[4] != l1) || (msg[5] != l2);
            msg[4] = l1;
            msg[5] = l2;
        }
        else
        {
            _ubxLength = _ubxPayload.size();
            _ubxLengthBad = false;
        }

        _SetData(msg);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataCustom::_DrawEditNmea(const bool refresh)
{
    bool talkerDirty    = refresh;
    bool formatterDirty = refresh;
    bool payloadDirty   = refresh;
    bool checksumDirty  = refresh;

    // Buttons
    const auto action = _DrawActionButtons();

    // Actions
    if (action == ACTION_CLEAR)
    {
        _rawHex.clear();
        talkerDirty       = true;
        formatterDirty    = true;
        payloadDirty      = true;
        checksumDirty     = true;
        _nmeaChecksumUser = false;
        _nmeaChecksumBad  = false;
    }

    // Checks
    const bool badPayload = !_nmeaPayload.empty() && _data.empty();

    // Inputs
    ImGui::Separator();
    static const char * const kNmeaTalkers[] =
    {
        "", "GP", "GL", "GA", "GB", "GQ", "GN", "P",
    };
    static const char * const kNmeaFormatters[] =
    {
        "", "DTM", "GBS", "GGA", "GLL", "GNS", "GRS", "GSA", "GSV", "RLM", "RMC", "THS", "VLW", "VTG", "ZDA", "UBX",
    };

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Talker");
    ImGui::SameLine();
    ImGui::SetNextItemWidth((GuiSettings::charSize.x * 3) + (GuiSettings::style->ItemInnerSpacing.x * 2));
    if (ImGui::InputText("##Talker", &_nmeaTalker, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank))
    {
        talkerDirty = true;
        _nmeaTalkerChoiceIx = 0;
        for (int ix = 0; ix < NUMOF(kNmeaTalkers); ix++)
        {
            if (std::strcmp(kNmeaTalkers[ix], _nmeaTalker.c_str()) == 0)
            {
                _nmeaTalkerChoiceIx = ix;
                break;
            }
        }
    }
    ImGui::SameLine(0, 0);
    ImGui::SetNextItemWidth(ImGui::GetFrameHeight());
    if (ImGui::Combo("##TalkerChoice", &_nmeaTalkerChoiceIx, kNmeaTalkers, NUMOF(kNmeaTalkers)))
    {
        talkerDirty = true;
        if (_nmeaTalkerChoiceIx > 0)
        {
            _nmeaTalker = kNmeaTalkers[_nmeaTalkerChoiceIx];
        }
    }
    Gui::VerticalSeparator();
    ImGui::TextUnformatted("Formatter");
    ImGui::SameLine();
    ImGui::SetNextItemWidth((GuiSettings::charSize.x * 4) + (GuiSettings::style->ItemInnerSpacing.x * 2));
    if (ImGui::InputText("##Formatter", &_nmeaFormatter, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank))
    {
        formatterDirty = true;
        _nmeaFormatterChoiceIx = 0;
        for (int ix = 0; ix < NUMOF(kNmeaFormatters); ix++)
        {
            if (std::strcmp(kNmeaFormatters[ix], _nmeaFormatter.c_str()) == 0)
            {
                _nmeaFormatterChoiceIx = ix;
                break;
            }
        }
    }
    ImGui::SameLine(0, 0);
    ImGui::SetNextItemWidth(ImGui::GetFrameHeight());
    if (ImGui::Combo("##FormatterChoice", &_nmeaFormatterChoiceIx, kNmeaFormatters, NUMOF(kNmeaFormatters)))
    {
        talkerDirty = true;
        if (_nmeaFormatterChoiceIx > 0)
        {
            _nmeaFormatter = kNmeaFormatters[_nmeaFormatterChoiceIx];
        }
    }
    Gui::VerticalSeparator();
    ImGui::TextUnformatted("Checksum");
    ImGui::SameLine();
    ImGui::SetNextItemWidth((GuiSettings::charSize.x * 2) + (GuiSettings::style->ItemInnerSpacing.x * 2));
    if (_nmeaChecksumBad) { ImGui::PushStyleColor(ImGuiCol_Border, GUI_COLOUR(TEXT_ERROR)); }
    if (ImGui::InputText("##Checksum", &_nmeaChecksum, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsNoBlank))
    {
        checksumDirty = true;
        _nmeaChecksumUser = true;
    }
    if (_nmeaChecksumBad) { ImGui::PopStyleColor(); }
    ImGui::Separator();
    ImGui::TextUnformatted("Payload");
    if (badPayload) { ImGui::PushStyleColor(ImGuiCol_Border, GUI_COLOUR(TEXT_ERROR)); }
    if (ImGui::InputTextMultiline("##EditNmea", &_nmeaPayload, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        payloadDirty = true;
        _nmeaChecksumUser = false;
    }
    if (badPayload) { ImGui::PopStyleColor(); }

    // Update
    if (talkerDirty || formatterDirty || payloadDirty || checksumDirty)
    {
        // Check payload
        bool bad = false;
        for (char &c: _nmeaPayload)
        {
            if ( (c < 0x20) || (c > 0x7e) || // valid range
                 (c == '$') || (c == '\\') || (c == '!') || (c == '~') ) // reserved
            {
                bad = true;
                break;
            }
        }
        if (bad)
        {
            _SetData();
        }
        else
        {
            std::string msg;
            const int msgLen = _nmeaTalker.size() + _nmeaFormatter.size() + _nmeaPayload.size() + NMEA_FRAME_SIZE + 2 + 1;
            msg.resize(msgLen, 'X');
            const int size = nmeaMakeMessage(_nmeaTalker.c_str(), _nmeaFormatter.c_str(), _nmeaPayload.c_str(), msg.data());
            msg.resize(size, 'X');

            if (_nmeaChecksumUser)
            {
                const std::string ck = msg.substr(msg.size() - 4, 2);
                _nmeaChecksumBad = (ck != _nmeaChecksum);
                msg.replace(msg.size() - 4, 2, _nmeaChecksum);
            }
            else
            {
                _nmeaChecksum = msg.substr(msg.size() - 4, 2);
                _nmeaChecksumBad = false;
            }

            _SetData(msg);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataCustom::_DrawEditRtcm3(const bool refresh)
{
    ImGui::Text("Not implemented...");
    if (refresh)
    {
        _SetData();
    }
}

/* ****************************************************************************************************************** */
