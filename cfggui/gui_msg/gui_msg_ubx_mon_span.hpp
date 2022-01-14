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

#ifndef __GUI_MSG_UBX_MON_SPAN_HPP__
#define __GUI_MSG_UBX_MON_SPAN_HPP__

#include <memory>

#include "gui_inc.hpp"

#include "gui_msg.hpp"

/* ***** UBX-MON-SPAN renderer ************************************************************************************** */

class GuiMsgUbxMonSpan : public GuiMsg
{
    public:
        GuiMsgUbxMonSpan(std::shared_ptr<InputReceiver> receiver = nullptr, std::shared_ptr<InputLogfile> logfile = nullptr);

        void Update(const std::shared_ptr<Ff::ParserMsg> &msg) final;
        void Buttons() final;
        bool Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail) final;
        void Clear() final;

    private:

        struct SpectData
        {
            SpectData();
            bool valid;
            std::vector<double> freq;
            std::vector<double> ampl;
            std::vector<double> min;
            std::vector<double> max;
            std::vector<double> mean;
            double center;
            double span;
            double res;
            double pga;
            double count;
            std::string tab;
            std::string title;
            std::string xlabel;
        };
        struct Label
        {
            double      freq;
            std::string id;
            std::string title;
        };
        static const std::vector<Label> FREQ_LABELS;

        std::vector<SpectData> _spects;;
        bool _resetPlotRange;

        void _DrawSpect(const SpectData &spect, const ImVec2 size);
};

/* ****************************************************************************************************************** */
#endif // __GUI_MSG_UBX_MON_SPAN_HPP__
