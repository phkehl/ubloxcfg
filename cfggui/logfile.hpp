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

#ifndef __LOGFILE_H__
#define __LOGFILE_H__

#include <memory>
#include <string>
#include <functional>

#include "data.hpp"
#include "database.hpp"

/* ****************************************************************************************************************** */

class Logfile
{
    public:
        Logfile(const std::string &name, std::shared_ptr<Database> database);
       ~Logfile();

        void                 SetDataCb(std::function<void(const Data &)> cb);

    protected:

    private:
        std::string          _name;
        std::shared_ptr<Database> _database;
};

/* ****************************************************************************************************************** */
#endif // __LOGFILE_H__
