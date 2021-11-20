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

#ifndef __GUI_MSG_USB_ESF_MEAS_HPP__
#define __GUI_MSG_USB_ESF_MEAS_HPP__

#include <memory>

#include "imgui.h"
#include "gui_msg.hpp"

/* ***** UBX-ESF-MEAS renderer ************************************************************************************** */

class GuiMsgUbxEsfMeas : public GuiMsg
{
    public:
        GuiMsgUbxEsfMeas(std::shared_ptr<Receiver> receiver = nullptr, std::shared_ptr<Logfile> logfile = nullptr);
        void Update(const std::shared_ptr<Ff::ParserMsg> &msg) final;
        bool Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2 &sizeAvail) final;
        void Clear() final;

        struct MeasDef
        {
            const char    *name;
            enum Type_e    { UNSIGNED, SIGNED, TICK };
            enum Type_e    type;
            const char    *fmt;
            double         scale;
            const char    *unit;
        };

        static const std::vector<MeasDef> MEAS_DEFS;

    private:

        struct MeasInfo
        {
            MeasInfo();
            uint32_t    lastTs;
            uint32_t    nMeas;
            int         type;
            std::string name;
            std::string rawHex;
            std::string value;
            std::string ttagSens;
            std::string ttagRx;
            // std::string source;
            std::string provider;
        };

        std::map<std::string, MeasInfo> _measInfos;
        std::string _selected;
};

/* ****************************************************************************************************************** */
#endif // __GUI_MSG_USB_ESF_MEAS_HPP__
