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

#ifndef __GUI_WIN_EXPERIMENT_HPP__
#define __GUI_WIN_EXPERIMENT_HPP__

// #include <cinttypes>
#include <memory>
#include <string>

#include "imgui.h"

#include "ff_cpp.hpp"

#include "gui_win.hpp"
#include "gui_widget_opengl.hpp"
#include "gui_win_filedialog.hpp"
#include "gui_notify.hpp"

/* ***** Experiment ************************************************************************************************* */

class GuiWinExperiment : public GuiWin
{
    public:
        GuiWinExperiment();

        void DrawWindow() final;

    protected:

        // Experiment 1
        GuiWinFileDialog _openFileDialog;
        std::string      _openFilePath;
        GuiWinFileDialog _saveFileDialog;
        std::string      _saveFilePath;
        void _DrawGuiWinFileDialog();

        // Experiment 2
        bool _running;
        GuiWidgetOpenGl _gl;
        void _DrawGuiWidgetOpenGl();

        // Experiment 3
        void _DrawGuiNotify();
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_EXPERIMENT_HPP__
