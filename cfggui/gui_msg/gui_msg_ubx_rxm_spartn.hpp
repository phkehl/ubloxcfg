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

#ifndef __GUI_MSG_UBX_RXM_SPARTN_HPP__
#define __GUI_MSG_UBX_RXM_SPARTN_HPP__

#include <memory>
#include <map>
#include <cstdint>

#include "gui_inc.hpp"

#include "gui_widget_table.hpp"

#include "gui_msg.hpp"

/* ***** UBX-RXM-SPARTN renderer ************************************************************************************ */

class GuiMsgUbxRxmSpartn : public GuiMsg
{
    public:
        GuiMsgUbxRxmSpartn(std::shared_ptr<InputReceiver> receiver = nullptr, std::shared_ptr<InputLogfile> logfile = nullptr);

        void Update(const std::shared_ptr<Ff::ParserMsg> &msg) final;
        bool Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail) final;
        void Clear() final;

    private:

        struct SpartnInfo
        {
            SpartnInfo(const int _msgType, const int _subType);
            std::string name;
            int         msgType;
            int         subType;
            uint32_t    uid;
            uint32_t    nUsed;
            uint32_t    nUnused;
            uint32_t    nUnknown;
            uint32_t    lastTs;
            const char *tooltip;
        };

        std::map<uint32_t, SpartnInfo> _spartnInfos;
        GuiWidgetTable _table;
};

/* ****************************************************************************************************************** */
#endif // __GUI_MSG_UBX_RXM_SPARTN_HPP__
