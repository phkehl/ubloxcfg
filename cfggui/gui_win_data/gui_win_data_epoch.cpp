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

#include "ff_trafo.h"

#include "gui_inc.hpp"

#include "gui_win_data_epoch.hpp"

/* ****************************************************************************************************************** */

GuiWinDataEpoch::GuiWinDataEpoch(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database)
{
    _winSize = { 80, 25 };

    _table.AddColumn("Field");
    _table.AddColumn("Value");
}

// ---------------------------------------------------------------------------------------------------------------------


void GuiWinDataEpoch::_ProcessData(const InputData &data)
{
    // New epoch means a new database row is available
    if (data.type == InputData::DATA_EPOCH)
    {
        _table.ClearRows();

        const auto row = _database->LatestRow();
        for (const auto &def: _database->FIELDS) {
            if (def.label) {
                _table.AddCellText(def.label);
                _table.AddCellTextF(def.fmt ? def.fmt : "%g", row[def.field]);
                _table.SetRowUid((uint32_t)(uint64_t)(void *)def.label);
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataEpoch::_ClearData()
{
    _table.ClearRows();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataEpoch::_DrawContent()
{
    _table.DrawTable();
}

/* ****************************************************************************************************************** */
