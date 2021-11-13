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

#ifndef __GUI_WIN_INPUT_LOGFILE_H__
#define __GUI_WIN_INPUT_LOGFILE_H__

#include <memory>
#include <map>
#include <vector>
#include <mutex>

#include "logfile.hpp"
#include "gui_win_filedialog.hpp"
#include "gui_win_input.hpp"

/* ***** Logfile input ********************************************************************************************** */

class GuiWinInputLogfile : public GuiWinInput
{
    public:
        GuiWinInputLogfile(const std::string &name);
       ~GuiWinInputLogfile();

        void Loop(const uint32_t &frame, const double &now);

        void ProcessData(const Data &data);

    protected:

        // Shared across multiple instances of this window
        static std::vector<std::string> _recentLogs;
        static std::mutex _recentLogsMutex;
        static constexpr int MAX_RECENT_LOGS = 20;
        static void _AddRecentLog(const std::string &path);

        std::shared_ptr<Logfile>  _logfile;
        GuiWinFileDialog          _fileDialog;
        float                     _seekProgress;
        float                     _playSpeed;
        bool                      _limitPlaySpeed;

        void _DrawActionButtons() final;
        void _DrawControls() final;
        void _ProcessData(const Data &data) final;
        void _ClearData() final;
        void _AddDataWindow(std::unique_ptr<GuiWinData> dataWin) final;
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_INPUT_LOGFILE_H__
