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

#ifndef __GUI_MSG_USB_ESF_STATUS_HPP__
#define __GUI_MSG_USB_ESF_STATUS_HPP__

#include <memory>

#include "gui_inc.hpp"

#include "gui_widget_table.hpp"

#include "gui_msg.hpp"

/* ***** UBX-ESF-STATUS renderer ************************************************************************************ */

class GuiMsgUbxEsfStatus : public GuiMsg
{
    public:
        GuiMsgUbxEsfStatus(std::shared_ptr<InputReceiver> receiver = nullptr, std::shared_ptr<InputLogfile> logfile = nullptr);
        void Update(const std::shared_ptr<Ff::ParserMsg> &msg) final;
        bool Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail) final;
        void Clear() final;

    private:

        static const std::vector<StatusFlags> _fusionModeFlags;
        static const std::vector<StatusFlags> _statusFlags012;
        static const std::vector<StatusFlags> _statusFlags0123;

        struct Sensor
        {
            Sensor(const uint8_t *groupData);
            std::string type;
            bool        used;
            bool        ready;
            std::string calibStatus;
            bool        calibrated;
            std::string timeStatus;
            std::string freq;
            std::string faults;
        };
        bool                _valid;
        std::string         _iTow;
        int                 _wtInitStatus;
        int                 _mntAlgStatus;
        int                 _insInitStatus;
        int                 _imuInitStatus;
        int                 _fusionMode;
        std::vector<Sensor> _sensors;

        GuiWidgetTable _table;
};

/* ****************************************************************************************************************** */
#endif // __GUI_MSG_USB_ESF_STATUS_HPP__
