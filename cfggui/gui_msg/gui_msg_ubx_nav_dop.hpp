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

#ifndef __GUI_MSG_UBX_NAV_DOP_HPP__
#define __GUI_MSG_UBX_NAV_DOP_HPP__

#include <memory>

#include "gui_inc.hpp"

#include "gui_msg.hpp"

/* ***** UBX-NAV-DOP renderer *************************************************************************************** */

class GuiMsgUbxNavDop : public GuiMsg
{
    public:
        GuiMsgUbxNavDop(std::shared_ptr<InputReceiver> receiver = nullptr, std::shared_ptr<InputLogfile> logfile = nullptr);

        void Update(const std::shared_ptr<Ff::ParserMsg> &msg) final;
        bool Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2 &sizeAvail) final;

    private:

        std::string _gDopStr;
        float       _gDopFrac;
        std::string _pDopStr;
        float       _pDopFrac;
        std::string _tDopStr;
        float       _tDopFrac;
        std::string _vDopStr;
        float       _vDopFrac;
        std::string _hDopStr;
        float       _hDopFrac;
        std::string _nDopStr;
        float       _nDopFrac;
        std::string _eDopStr;
        float       _eDopFrac;

        static constexpr float MAX_DOP   = 100.0f;
        static constexpr float LOG_SCALE = 0.1f;
        static constexpr float MAX_LOG   = std::log10(MAX_DOP * (1.0/LOG_SCALE));

};

/* ****************************************************************************************************************** */
#endif // __GUI_MSG_UBX_NAV_DOP_HPP__
