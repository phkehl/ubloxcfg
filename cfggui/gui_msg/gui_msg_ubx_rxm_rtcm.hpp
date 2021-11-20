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

#ifndef __GUI_MSG_UBX_RXM_RTCM_HPP__
#define __GUI_MSG_UBX_RXM_RTCM_HPP__

#include <memory>
#include <map>
#include <cstdint>

#include "imgui.h"
#include "gui_msg.hpp"

/* ***** UBX-RXM-RTCM renderer ************************************************************************************** */

class GuiMsgUbxRxmRtcm : public GuiMsg
{
    public:
        GuiMsgUbxRxmRtcm(std::shared_ptr<Receiver> receiver = nullptr, std::shared_ptr<Logfile> logfile = nullptr);

        void Update(const std::shared_ptr<Ff::ParserMsg> &msg) final;
        bool Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2 &sizeAvail) final;
        void Clear() final;

    protected:

        struct RtcmInfo
        {
            RtcmInfo(const int _msgType, const int _subType, const int _refStation);
            std::string name;
            int         msgType;
            int         subType;
            int         refStation;
            uint32_t    nUsed;
            uint32_t    nUnused;
            uint32_t    nUnknown;
            uint32_t    nCrcFailed;
            uint32_t    lastTs;
            const char *tooltip;
        };

        std::map<uint32_t, RtcmInfo> _rtcmInfos;

    private:
};

/* ****************************************************************************************************************** */
#endif // __GUI_MSG_UBX_RXM_RTCM_HPP__
