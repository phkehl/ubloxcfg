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

#ifndef __GUI_WIN_INPUT_H__
#define __GUI_WIN_INPUT_H__

#include <string>
#include <functional>
#include <memory>
#include <vector>

#include "ff_epoch.h"

#include "data.hpp"
#include "database.hpp"
#include "gui_widget.hpp"
#include "gui_win.hpp"
#include "gui_win_data.hpp"
#include "gui_win_filedialog.hpp"

/* ***** Data input window base class ******************************************************************************* */

class GuiWinInput : public GuiWin
{
    public:
        GuiWinInput(const std::string &name);
       ~GuiWinInput();

        void OpenPreviousDataWin();

        // enum Callback_e { DRAW_BUTTONS, CHANGE_TITLE, CLEAR_DATA };
        // void SetCallback(std::function<void(Callback_e)> callback);

        void Loop(const uint32_t &frame, const double &now) override;
        void DrawWindow() final;
        // virtual void ProcessData(const Data &data);

    protected:

        std::shared_ptr<Database>                  _database;
        // std::function<void(Callback_e)>            _callback;
        std::vector< std::unique_ptr<GuiWinData> > _dataWindows;
        GuiWidgetLog                               _logWidget;
        std::string                                _rxVerStr;

        std::shared_ptr<Ff::Epoch> _epoch;
        double                     _epochAge;
        const char                *_fixStr;
        //double                     _fixTime; TODO

        virtual void _ProcessData(const Data &data);
        virtual void _ClearData();
        virtual void _AddDataWindow(std::unique_ptr<GuiWinData> dataWin);

        virtual void _DrawDataWinButtons();
        virtual void _DrawActionButtons();
        virtual void _DrawNavStatusLeft(const EPOCH_t *epoch);
        virtual void _DrawNavStatusRight(const EPOCH_t *epoch);
        virtual void _DrawControls() = 0;
        virtual void _DrawLog();

        void _UpdateTitle();

    private:
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_INPUT_H__
