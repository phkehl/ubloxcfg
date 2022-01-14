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

#ifndef __GUI_MSG_UBX_MON_HW2_HPP__
#define __GUI_MSG_UBX_MON_HW2_HPP__

#include <memory>
#include <deque>

#include "gui_inc.hpp"

#include "gui_msg.hpp"

/* ***** UBX-MON-HW2 renderer *************************************************************************************** */

class GuiMsgUbxMonHw2 : public GuiMsg
{
    public:
        GuiMsgUbxMonHw2(std::shared_ptr<InputReceiver> receiver = nullptr, std::shared_ptr<InputLogfile> logfile = nullptr);

        void Update(const std::shared_ptr<Ff::ParserMsg> &msg) final;
        bool Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail) final;
        void Clear() final;

        struct IQ
        {
            IQ(const float _magI, const float _magQ, const float _offsI, const float _offsQ) :
                magI{_magI}, magQ{_magQ}, offsI{_offsI}, offsQ{_offsQ}
            {}
            float magI;
            float magQ;
            float offsI;
            float offsQ;
        };

        static void DrawIQ(const ImVec2 &size, const std::deque<IQ> &iqs);

    private:

        static constexpr int NUM_IQS = 50;
        std::deque<IQ> _iqs;
};

/* ****************************************************************************************************************** */
#endif // __GUI_MSG_UBX_MON_HW2_HPP__
