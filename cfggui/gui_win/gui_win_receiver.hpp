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

#ifndef __GUI_WIN_RECEIVER_H__
#define __GUI_WIN_RECEIVER_H__

#include <memory>
#include <map>
#include <string>
#include <functional>
#include <vector>

#include "imgui.h"

#include "ff_cpp.hpp"

#include "gui_win.hpp"
#include "gui_win_data.hpp"
#include "receiver.hpp"
#include "gui_widget.hpp"
#include "gui_win_filedialog.hpp"

/* ***** Receiver ******************************************************************************* */

class GuiWinData;

class GuiWinReceiver : public GuiWin
{
    public:
        GuiWinReceiver(const std::string &name, std::shared_ptr<Receiver> receiver, std::shared_ptr<Database> database);
       ~GuiWinReceiver();

        void                 DrawWindow() override;
        bool                 IsOpen() override;

        void                 Loop(const uint32_t &frame, const double &now);

        void                 ProcessData(const Data &data);

        void                 SetCallbacks(std::function<void()> dataWinButtonCb, std::function<void()> titleChangeCb,
                                 std::function<void()> clearCb);

    protected:
        std::string          _rxVerStr;

        std::shared_ptr<Receiver> _receiver;
        std::shared_ptr<Database> _database;
        std::function<void()> _dataWinButtonsCb;
        std::function<void()> _titleChangeCb;
        std::function<void()> _clearCb;

        std::string          _port;
        int                  _baudrate;
        GuiWidgetLog         _log;

        // Detected and recent ports
        static std::vector<std::string> _recentPorts;
        void                 _AddRecentPort(const std::string &port);
        void                 _InitRecentPorts();

        bool                 _stopSent;
        bool                 _triggerConnect;
        bool                 _focusPortInput;

        std::shared_ptr<Ff::Epoch> _epoch;
        double               _epochTs;
        double               _epochAge;
        const char          *_fixStr;
        double               _fixTime;

        std::unique_ptr<GuiWinFileDialog> _recordFileDialog;
        std::shared_ptr<std::string> _recordFileName;
        std::unique_ptr<std::ofstream> _recordHandle;
        uint32_t             _recordSize;
        std::string          _recordMessage;
        uint32_t             _recordLastSize;
        double               _recordLastMsgTime;
        double               _recordLastSizeTime;
        double               _recordKiBs;
        ImU32                _recordButtonColor;


        void                 _DrawConnectionWidget();
        void                 _DrawCommandButtons();
        void                 _DrawRecordButton();
        void                 _DrawRxStatus();
        void                 _DrawSvLevelHist(const EPOCH_t *epoch);
        void                 _UpdateTitle();
        void                 _ClearData();
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_RECEIVER_H__
