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

#include <cstring>
#include <cmath>

#include "ff_stuff.h"

#include "gui_inc.hpp"

#include "gui_win_experiment.hpp"

/* ****************************************************************************************************************** */

GuiWinExperiment::GuiWinExperiment() :
    GuiWin("Experiments"),
    _openFileDialog{_winName + "OpenFileDialog"},
    _saveFileDialog{_winName + "SaveFileDialog"}
{
    _winSize = { 100, 50 };
    _winFlags |= ImGuiWindowFlags_NoDocking;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinExperiment::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    if (ImGui::Button("Open a file..."))
    {
        if (!_openFileDialog.IsInit())
        {
            _openFileDialog.InitDialog(GuiWinFileDialog::FILE_OPEN);
            _openFileDialog.SetDirectory("/usr/share/doc");
        }
        else
        {
            _openFileDialog.Focus();
        }
    }
    ImGui::SameLine();
    ImGui::Text("--> %s", _openFilePath.c_str());


    if (ImGui::Button("Save a file..."))
    {
        if (!_saveFileDialog.IsInit())
        {
            _saveFileDialog.InitDialog(GuiWinFileDialog::FILE_SAVE);
            _saveFileDialog.SetFilename("saveme.txt");
            _saveFileDialog.SetTitle("blablabla...");
            //_saveFileDialog->ConfirmOverwrite(false);
        }
        else
        {
            _saveFileDialog.Focus();
        }
    }
    ImGui::SameLine();
    ImGui::Text("--> %s", _saveFilePath.c_str());


    if (_openFileDialog.IsInit())
    {
        if (_openFileDialog.DrawDialog())
        {
            _openFilePath = _openFileDialog.GetPath();
            DEBUG("open file done");
        }
    }
    if (_saveFileDialog.IsInit())
    {
        if (_saveFileDialog.DrawDialog())
        {
            _saveFilePath = _saveFileDialog.GetPath();
            DEBUG("save file done");
        }
    }

    _DrawWindowEnd();
}

/* ****************************************************************************************************************** */
