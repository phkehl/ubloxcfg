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

#ifndef __GUI_WIN_INPUT_RECEIVER_HPP__
#define __GUI_WIN_INPUT_RECEIVER_HPP__

#include <memory>
#include <map>
#include <vector>

#include "imgui.h"

#include "ff_cpp.hpp"

#include "receiver.hpp"
#include "gui_win_input.hpp"

/* ***** Receiver data input **************************************************************************************** */

class GuiWinInputReceiver : public GuiWinInput
{
    public:
        GuiWinInputReceiver(const std::string &name);
       ~GuiWinInputReceiver();

        void Loop(const uint32_t &frame, const double &now) final;

        bool IsOpen() final;

    protected:

        std::shared_ptr<Receiver> _receiver;

        std::string          _port;
        int                  _baudrate;

        // Detected and recent ports
        static std::vector<std::string> _recentPorts;
        static constexpr int  MAX_RECENT_PORTS = 20;
        void                 _AddRecentPort(const std::string &port);
        void                 _InitRecentPorts();

        bool                 _stopSent;
        bool                 _triggerConnect;
        bool                 _focusPortInput;

        double               _epochTs;

        GuiWinFileDialog     _recordFileDialog;
        std::string          _recordFilePath;
        std::unique_ptr<std::ofstream> _recordHandle;
        uint32_t             _recordSize;
        std::string          _recordMessage;
        uint32_t             _recordLastSize;
        double               _recordLastMsgTime;
        double               _recordLastSizeTime;
        double               _recordKiBs;
        ImU32                _recordButtonColor;

        void _DrawActionButtons() final;
        void _DrawControls() final;
        void _ProcessData(const Data &data) final;
        void _ClearData() final;
        void _AddDataWindow(std::unique_ptr<GuiWinData> dataWin) final;
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_INPUT_RECEIVER_HPP__
