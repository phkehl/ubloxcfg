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

#ifndef __GUI_MSG_UBX_RXM_SFRBX_HPP__
#define __GUI_MSG_UBX_RXM_SFRBX_HPP__

#include <memory>
#include <map>
#include <vector>
#include <cstdint>

#include "gui_inc.hpp"

#include "gui_widget_table.hpp"

#include "gui_msg.hpp"

/* ***** UBX-RXM-SFRBX renderer ************************************************************************************* */

class GuiMsgUbxRxmSfrbx : public GuiMsg
{
    public:
        GuiMsgUbxRxmSfrbx(std::shared_ptr<InputReceiver> receiver = nullptr, std::shared_ptr<InputLogfile> logfile = nullptr);

        void Update(const std::shared_ptr<Ff::ParserMsg> &msg) final;
        bool Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail) final;
        void Clear() final;

    private:

        struct SfrbxInfo
        {
            SfrbxInfo(const uint8_t _gnssId, const uint8_t _svId, const uint8_t _sigId, const uint8_t _freqId);
            std::string           svStr;
            std::string           sigStr;
            uint32_t              lastTs;
            uint8_t               gnssId;
            uint8_t               svId;
            uint32_t              uid;
            uint8_t               sigId;
            uint8_t               freqId;
            std::vector<uint32_t> dwrds;
            std::string           dwrdsHex;
            std::string           navMsg;
        };

        std::map<uint32_t, SfrbxInfo> _sfrbxInfos;
        GuiWidgetTable _table;
};

/* ****************************************************************************************************************** */
#endif // __GUI_MSG_UBX_RXM_SFRBX_HPP__
