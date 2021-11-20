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

#ifndef __GUI_MSG_UBX_RXM_RAWX_HPP__
#define __GUI_MSG_UBX_RXM_RAWX_HPP__

#include <memory>
#include <map>
#include <cstdint>

#include "imgui.h"
#include "gui_msg.hpp"

/* ***** UBX-RXM-RAWX renderer ************************************************************************************** */

class GuiMsgUbxRxmRawx : public GuiMsg
{
    public:
        GuiMsgUbxRxmRawx(std::shared_ptr<Receiver> receiver = nullptr, std::shared_ptr<Logfile> logfile = nullptr);

        void Update(const std::shared_ptr<Ff::ParserMsg> &msg) final;
        bool Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2 &sizeAvail) final;
        void Clear() final;

    protected:

        struct RawInfo
        {
            RawInfo(const uint8_t *groupData);
            uint32_t    uid;
            std::string sv;
            std::string signal;
            std::string cno;
            std::string pseudoRange;
            std::string carrierPhase;
            std::string doppler;
            std::string lockTime;
            bool        prValid;
            bool        cpValid;
            bool        halfCyc;
            bool        subHalfCyc;
        };

        bool   _valid;
        int    _week;
        double _rcvTow;
        int    _leapSec;
        bool   _leapSecValid;
        bool   _clkReset;
        std::vector<RawInfo> _rawInfos;

        uint32_t _selected;

    private:
};

/* ****************************************************************************************************************** */
#endif // __GUI_MSG_UBX_RXM_RAWX_HPP__
