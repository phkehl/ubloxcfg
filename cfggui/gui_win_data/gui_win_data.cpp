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

#include "gui_win_data.hpp"

/* ****************************************************************************************************************** */

GuiWinData::GuiWinData(const std::string &name,
            std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile, std::shared_ptr<Database> database) :
    _receiver{receiver},
    _logfile{logfile},
    _database{database}
{
    _winTitle = name;
    _winName  = name;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinData::Loop(const uint32_t &frame, const double &now)
{
    (void)frame;
    (void)now;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinData::ProcessData(const Data &data)
{
    (void)data;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinData::ClearData()
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinData::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    _DrawWindowEnd();
}

/* ****************************************************************************************************************** */
