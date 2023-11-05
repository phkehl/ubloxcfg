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

#ifndef __GUI_WIN_INPUT_RECEIVER_HPP__
#define __GUI_WIN_INPUT_RECEIVER_HPP__

#include <memory>
#include <map>
#include <vector>

#include "imgui.h"

#include "ff_cpp.hpp"

#include "logfile.hpp"

#include "input_receiver.hpp"
#include "gui_win_filedialog.hpp"
#include "gui_win_input.hpp"

/* ***** Receiver data input **************************************************************************************** */

class GuiWinInputReceiver : public GuiWinInput
{
    public:
        GuiWinInputReceiver(const std::string &name);
       ~GuiWinInputReceiver();

        void Loop(const uint32_t &frame, const double &now) final;

        bool WinIsOpen() final;

        std::shared_ptr<InputReceiver> GetReceiver();

    protected:

        std::shared_ptr<InputReceiver> _receiver;

        std::string          _port;
        RX_OPTS_t            _rxOpts;
        std::string          _rxOptsTooltip;

        bool                 _stopSent;
        bool                 _triggerConnect;
        bool                 _focusPortInput;

        double               _epochTs;

        GuiWinFileDialog     _recordFileDialog;
        std::string          _recordFilePath;
        Logfile              _recordLog;
        uint32_t             _recordSize;
        std::string          _recordMessage;
        uint32_t             _recordLastSize;
        double               _recordLastMsgTime;
        double               _recordLastSizeTime;
        double               _recordKiBs;
        ImU32                _recordButtonColor;

        void _DrawActionButtons() final;
        void _DrawControls() final;
        void _ProcessData(const InputData &data) final;

        void _ClearData() final;
        void _AddDataWindow(std::unique_ptr<GuiWinData> dataWin) final;
        void _LogOpen(const std::string &path);
        void _LogClose();
        void _UpdateRxOptsTooltip();
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_INPUT_RECEIVER_HPP__
