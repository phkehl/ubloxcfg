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

#ifndef __GUI_WIN_DATA_FWUPDATE_H__
#define __GUI_WIN_DATA_FWUPDATE_H__

#include "gui_win_data.hpp"

/* ***** Firmware update ******************************************************************************************** */

class GuiWinDataFwupdate : public GuiWinData
{
    public:
        GuiWinDataFwupdate(const std::string &name, std::shared_ptr<Database> database);

      //void                 Loop(const uint32_t &frame, const double &now) final;
      //void                 ProcessData(const Data &data) final;
        void                 ClearData() final;
        void                 DrawWindow() final;

    protected:
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_FWUPDATE_H__
