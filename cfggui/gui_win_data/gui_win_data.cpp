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

#include "gui_inc.hpp"

#include "gui_win_data.hpp"

/* ****************************************************************************************************************** */

GuiWinData::GuiWinData(const std::string &name, std::shared_ptr<Database> database) :
    GuiWin(name),
    _database{database}, _toolbarEna{true}, _latestEpochEna{true}
{
    DEBUG("GuiWinData(%s)", _winName.c_str());

    //_winClass.ClassId = 2;

    ClearData();
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinData::~GuiWinData()
{
    DEBUG("~GuiWinData(%s)", _winName.c_str());
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinData::SetReceiver(std::shared_ptr<InputReceiver> receiver)
{
    _receiver = receiver;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinData::SetLogfile(std::shared_ptr<InputLogfile> logfile)
{
    _logfile = logfile;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinData::Loop(const uint32_t &frame, const double &now)
{
    // Expire data
    if (_latestEpochEna && _latestEpoch && _receiver && !_receiver->IsIdle())
    {
        _latestEpochAge = now - _latestEpochTs;
        if (_latestEpochAge > _latestEpochExpire)
        {
            ClearData();
        }
    }

    _Loop(frame, now);
}

void GuiWinData::_Loop(const uint32_t &frame, const double &now)
{
    UNUSED(frame);
    UNUSED(now);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinData::ProcessData(const InputData &data)
{
    if ( _latestEpochEna && (data.type == InputData::DATA_EPOCH) )
    {
        _latestEpoch = data.epoch;
        _latestEpochTs = (double)data.epoch->epoch.ts * 1e-3;
    }
    _ProcessData(data);
}

void GuiWinData::_ProcessData(const InputData &data)
{
    UNUSED(data);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinData::ClearData()
{
    _latestEpoch = nullptr;
    _ClearData();
}

void GuiWinData::_ClearData()
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinData::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    // Common buttons toolbar
    if (_toolbarEna)
    {
        // Clear
        if (ImGui::Button(ICON_FK_ERASER "##Clear", GuiSettings::iconSize))
        {
            ClearData();
        }
        Gui::ItemTooltip("Clear all data");

        _DrawToolbar();

        // Age of latest epoch
        if (_latestEpochEna && _receiver && !_receiver->IsIdle())
        {
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - (4 * GuiSettings::charSize.x));
            ImGui::AlignTextToFramePadding();
            if (_latestEpoch)
            {
                ImGui::Text("%4.1f", _latestEpochAge);
            }
            else
            {
                ImGui::TextUnformatted(" n/a");
            }
        }

        ImGui::Separator();
    }

    _DrawContent();

    _DrawWindowEnd();
}

void GuiWinData::_DrawToolbar()
{
}

void GuiWinData::_DrawContent()
{
    ImGui::TextUnformatted("Not implemented...");
}

/* ****************************************************************************************************************** */
