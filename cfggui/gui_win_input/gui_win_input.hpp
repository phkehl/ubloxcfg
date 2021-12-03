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

#ifndef __GUI_WIN_INPUT_HPP__
#define __GUI_WIN_INPUT_HPP__

#include <string>
#include <functional>
#include <memory>
#include <vector>

#include "ff_epoch.h"

#include "input_data.hpp"
#include "database.hpp"

#include "gui_widget_log.hpp"
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

        void Loop(const uint32_t &frame, const double &now) override; // children can override but must call base method, too!

        void DrawWindow() final;

        void DrawDataWindows();
        void DataWinMenu();

    protected:

        std::shared_ptr<Database>                  _database;
        // std::function<void(Callback_e)>            _callback;
        std::vector< std::unique_ptr<GuiWinData> > _dataWindows;
        static constexpr int MAX_SAVED_WINDOWS = 20;
        GuiWidgetLog                               _logWidget;
        std::string                                _rxVerStr;

        std::shared_ptr<Ff::Epoch> _epoch;
        double                     _epochAge;
        const char                *_fixStr;
        //double                     _fixTime; TODO

        virtual void _ProcessData(const InputData &data);
        virtual void _ClearData();
        virtual void _AddDataWindow(std::unique_ptr<GuiWinData> dataWin);

        using DataWinCreateFn_t = std::function< std::unique_ptr<GuiWinData>(const std::string &, std::shared_ptr<Database>) >;
        struct DataWinDef
        {
            const char   *name;
            const char   *title;
            const char   *button;
            enum Cap_e { ACTIVE = BIT(0), PASSIVE = BIT(1), ALL = BIT(0) | BIT(1) };
            enum Cap_e    reqs;
            DataWinCreateFn_t create;
        };
        static const std::vector<DataWinDef> _dataWinDefs;
        enum DataWinDef::Cap_e               _dataWinCaps;

        bool _autoHideDatawin;

        void _DrawDataWinButtons();
        virtual void _DrawActionButtons();                     // children must call base method, too!
        void _DrawNavStatusLeft(const EPOCH_t *epoch);
        void _DrawNavStatusRight(const EPOCH_t *epoch);
        virtual void _DrawControls() = 0;
        virtual void _DrawLog();

        void _UpdateTitle();

    private:
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_INPUT_HPP__
