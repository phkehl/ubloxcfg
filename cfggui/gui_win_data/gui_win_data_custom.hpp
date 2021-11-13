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

#ifndef __GUI_WIN_DATA_CUSTOM_H__
#define __GUI_WIN_DATA_CUSTOM_H__

#include <vector>
#include <cinttypes>

#include "gui_win_data.hpp"

/* ***** Custom message ********************************************************************************************* */

class GuiWinDataCustom : public GuiWinData
{
    public:
        GuiWinDataCustom(const std::string &name, std::shared_ptr<Database> database);

      //void Loop(const uint32_t &frame, const double &now) final;
      //void ProcessData(const Data &data) final;
      //void ClearData() final;
      void DrawWindow() final;

    protected:

        // Global state
        const char *_currentTab;

        // Final data
        std::vector<uint8_t>     _data;
        bool                     _dataIsStr;
        std::vector<std::string> _hexDump;
        void _SetData(const std::string &text = "");
        void _SetData(const std::vector<uint8_t> bin);

        // Common buttons / actions
        enum Action_e { ACTION_NONE, ACTION_CLEAR };
        enum Action_e _DrawButtons();

        // Text editor
        std::string _textEdit;
        bool        _textLfToCrLf;
        void _DrawEditText(const bool refresh);

        // Raw editor
        std::string _rawHex;
        void _DrawEditRaw(const bool refresh);

        // UBX editor
        struct UbxMsg
        {
            UbxMsg(const char *_name, const uint8_t _clsId, const uint8_t _msgId) : name{_name}, clsId{_clsId}, msgId{_msgId} { }
            const char *name;
            uint8_t     clsId;
            uint8_t     msgId;
        };
        std::vector<UbxMsg>  _ubxMessages;
        const char          *_ubxMessage;
        uint8_t              _ubxClsId;
        uint8_t              _ubxMsgId;
        uint16_t             _ubxLength;
        bool                 _ubxLengthUser;
        bool                 _ubxLengthBad;
        std::string          _ubxPayloadHex;
        std::vector<uint8_t> _ubxPayload;
        uint16_t             _ubxChecksum;
        bool                 _ubxChecksumUser;
        bool                 _ubxChecksumBad;
        void _DrawEditUbx(const bool refresh);

        // NMEA editor
        std::string _nmeaTalker;
        int         _nmeaTalkerChoiceIx;
        std::string _nmeaFormatter;
        int         _nmeaFormatterChoiceIx;
        std::string _nmeaChecksum;
        bool        _nmeaChecksumUser;
        bool        _nmeaChecksumBad;
        std::string _nmeaPayload;
        void _DrawEditNmea(const bool refresh);

        // RTCM3 editor
        void _DrawEditRtcm3(const bool refresh);

        // Helpers
        std::vector<uint8_t> _HexToBin(const std::string &hex);
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_CUSTOM_H__
