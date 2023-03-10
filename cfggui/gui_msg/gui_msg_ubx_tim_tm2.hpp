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

#ifndef __GUI_MSG_UBX_TIM_TM2_HPP__
#define __GUI_MSG_UBX_TIM_TM2_HPP__

#include <memory>
#include <deque>

#include "gui_inc.hpp"

#include "gui_widget_table.hpp"

#include "gui_msg.hpp"

/* ***** UBX-TIM-TM2 renderer *************************************************************************************** */

class GuiMsgUbxTimTm2 : public GuiMsg
{
    public:
        GuiMsgUbxTimTm2(std::shared_ptr<InputReceiver> receiver = nullptr, std::shared_ptr<InputLogfile> logfile = nullptr);

        void Update(const std::shared_ptr<Ff::ParserMsg> &msg) final;
        bool Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail) final;
        void Clear() final;

    private:

        static constexpr int MAX_PLOT_DATA = 10000;

        struct PlotData
        {
            PlotData(const double tow, const bool signal, const float pRising, const float pFalling, const float dutyCycle) :
                tow_{tow}, signal_{signal}, pRising_{pRising}, pFalling_{pFalling}, dutyCycle_{dutyCycle} {}
            double tow_;
            bool   signal_;
            float  pRising_;
            float  pFalling_;
            float  dutyCycle_;
        };
        std::deque<PlotData> _plotData;
        double _lastRisingEdge;
        double _lastFallingEdge;
        double _periodRisingEdge;
        double _periodFallingEdge;
        double _dutyCycle;
};

/* ****************************************************************************************************************** */
#endif // __GUI_MSG_UBX_TIM_TM2_HPP__
