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

#include "ff_debug.h"

#include "logfile.hpp"

/* ****************************************************************************************************************** */

Logfile::Logfile(const std::string &name, std::shared_ptr<Database> database) :
    _name{name}, _database{database}

{
    DEBUG("Logfile(%s)", _name.c_str());
    //throw std::runtime_error("hmmm");
}

// ---------------------------------------------------------------------------------------------------------------------

Logfile::~Logfile()
{
    DEBUG("~Logfile(%s)", _name.c_str());

}

/* ****************************************************************************************************************** */
