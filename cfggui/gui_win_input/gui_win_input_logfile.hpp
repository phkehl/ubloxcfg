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

#ifndef __GUI_WIN_INPUT_LOGFILE_HPP__
#define __GUI_WIN_INPUT_LOGFILE_HPP__

#include <memory>
#include <map>
#include <vector>

#include "input_logfile.hpp"
#include "gui_win_filedialog.hpp"
#include "gui_win_input.hpp"

/* ***** Logfile input ********************************************************************************************** */

class GuiWinInputLogfile : public GuiWinInput
{
    public:
        GuiWinInputLogfile(const std::string &name);
       ~GuiWinInputLogfile();

        void Loop(const uint32_t &frame, const double &now) final;

        std::shared_ptr<InputLogfile> GetLogfile();

    protected:

        std::shared_ptr<InputLogfile>  _logfile;
        GuiWinFileDialog  _fileDialog;
        float             _seekProgress;
        float             _playSpeed;
        bool              _limitPlaySpeed;
        std::string       _progressBarFmt;

        void _DrawActionButtons() final;
        void _DrawControls() final;
        void _ProcessData(const InputData &data) final;
        void _ClearData() final;
        void _AddDataWindow(std::unique_ptr<GuiWinData> dataWin) final;
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_INPUT_LOGFILE_HPP__
