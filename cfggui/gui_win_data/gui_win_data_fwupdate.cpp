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

#include "gui_win_data_fwupdate.hpp"

#include "gui_win_data_inc.hpp"

/* ****************************************************************************************************************** */

GuiWinDataFwupdate::GuiWinDataFwupdate(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database)
{
    _winSize = { 80, 25 };
    ClearData();
}

// ---------------------------------------------------------------------------------------------------------------------

//void GuiWinDataFwupdate::Loop(const std::unique_ptr<Receiver> &receiver)
//{
//}

// ---------------------------------------------------------------------------------------------------------------------

//void GuiWinDataFwupdate::ProcessData(const Data &data)
//{
//}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataFwupdate::ClearData()
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataFwupdate::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    ImGui::TextUnformatted("Not implemented");

    _DrawWindowEnd();
}

/* ****************************************************************************************************************** */
