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

#ifndef __GUI_WIN_DATA_LOG_H__
#define __GUI_WIN_DATA_LOG_H__

#include "gui_win_data.hpp"

/* ***** Receiver log *********************************************************************************************** */

class GuiWinDataLog : public GuiWinData
{
    public:
        GuiWinDataLog(const std::string &name, std::shared_ptr<Database> database);
       ~GuiWinDataLog();

      //void                 Loop(const uint32_t &frame, const double &now) final;
        void                 ProcessData(const Data &data) final;
        void                 ClearData() final;
        void                 DrawWindow() final;

    protected:
        GuiWidgetLog         _log;

        uint32_t             _sizeRx;
        uint32_t             _nMsg;
        uint32_t             _nUbx;
        uint32_t             _nNmea;
        uint32_t             _nRtcm3;
        uint32_t             _nGarbage;
        uint32_t             _nEpoch;
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_LOG_H__
