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

// #include <cstdint>
// #include <cstring>
// #include <functional>

// #include "ff_stuff.h"
// #include "ff_ubx.h"
// #include "ff_port.h"
// #include "ff_trafo.h"
// #include "ff_cpp.hpp"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "IconsForkAwesome.h"

// #include "platform.hpp"

#include "gui_win_input_logfile.hpp"

/* ****************************************************************************************************************** */

GuiWinInputLogfile::GuiWinInputLogfile(const std::string &name) :
    GuiWinInput(name)
{
    DEBUG("GuiWinInputLogfile(%s)", _winName.c_str());

    _winIniPos = POS_NW;
    _winOpen   = true;
    _winSize   = { 80, 25 };
    SetTitle("Logfile X");

    _logfile = std::make_shared<Logfile>(name, _database);
    _logfile->SetDataCb( std::bind(&GuiWinInputLogfile::_ProcessData, this, std::placeholders::_1) );

    _ClearData();
};

// ---------------------------------------------------------------------------------------------------------------------

GuiWinInputLogfile::~GuiWinInputLogfile()
{
    DEBUG("~GuiWinInputLogfile(%s)", _winName.c_str());
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputLogfile::Loop(const uint32_t &frame, const double &now)
{
    UNUSED(frame);
    UNUSED(now);

    // // Pump receiver events and dispatch
    // _receiver->Loop(now);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputLogfile::_ProcessData(const Data &data)
{
    GuiWinInput::_ProcessData(data);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputLogfile::_ClearData()
{
    GuiWinInput::_ClearData();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputLogfile::_AddDataWindow(std::unique_ptr<GuiWinData> dataWin)
{
    dataWin->SetLogfile(_logfile);
    _dataWindows.push_back(std::move(dataWin));
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputLogfile::_DrawActionButtons()
{
    GuiWinInput::_DrawActionButtons();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputLogfile::_DrawControls()
{
    ImGui::TextUnformatted("to be implemented...");
}

/* ****************************************************************************************************************** */
