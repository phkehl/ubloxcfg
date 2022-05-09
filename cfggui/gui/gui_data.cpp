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

#include <algorithm>

#include "ff_debug.h"

#include "gui_data.hpp"

/* ****************************************************************************************************************** */

/*static*/ uint32_t GuiData::serial = 0;

/*static*/ std::mutex GuiData::_receiversMutex {};
/*static*/ std::mutex GuiData::_logfilesMutex  {};
/*static*/ std::mutex GuiData::_databasesMutex {};

/*static*/ GuiData::ReceiverList GuiData::_receivers {};
/*static*/ GuiData::LogfileList  GuiData::_logfiles  {};
/*static*/ GuiData::DatabaseList GuiData::_databases {};

/*static*/ std::shared_ptr<bool> GuiData::_triggers {};

/*static*/ GuiData::ReceiverList GuiData::Receivers()
{
    std::unique_lock<std::mutex> lock(_receiversMutex);
    return _receivers;
}

/*static*/ GuiData::LogfileList GuiData::Logfiles()
{
    std::unique_lock<std::mutex> lock(_logfilesMutex);
    return _logfiles;
}

/*static*/ GuiData::DatabaseList GuiData::Databases()
{
    std::unique_lock<std::mutex> lock(_databasesMutex);
    return _databases;
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ void GuiData::AddReceiver(std::shared_ptr<InputReceiver> receiver)
{
    std::unique_lock<std::mutex> lock(_receiversMutex);
    auto entry = std::find(_receivers.begin(), _receivers.end(), receiver);
    if (entry == _receivers.end())
    {
        _receivers.push_back(receiver);
        _AddDatabase(receiver->GetDatabase());
    }
    std::sort(_receivers.begin(), _receivers.end(),
        [](const std::shared_ptr<InputReceiver> &a, const std::shared_ptr<InputReceiver> &b)
        {
            return a->GetName() < b->GetName();
        });
    serial++;
}

/*static*/ void GuiData::RemoveReceiver(std::shared_ptr<InputReceiver> receiver)
{
    std::unique_lock<std::mutex> lock(_receiversMutex);
    auto entry = std::find(_receivers.begin(), _receivers.end(), receiver);
    if (entry != _receivers.end())
    {
        _RemoveDatabase(entry->get()->GetDatabase());
        _receivers.erase(entry);
    }
    serial++;
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ void GuiData::AddLogfile(std::shared_ptr<InputLogfile> logfile)
{
    std::unique_lock<std::mutex> lock(_logfilesMutex);
    auto entry = std::find(_logfiles.begin(), _logfiles.end(), logfile);
    if (entry == _logfiles.end())
    {
        _logfiles.push_back(logfile);
        _AddDatabase(logfile->GetDatabase());
    }
    std::sort(_logfiles.begin(), _logfiles.end(),
        [](const std::shared_ptr<InputLogfile> &a, const std::shared_ptr<InputLogfile> &b)
        {
            return a->GetName() < b->GetName();
        });
    serial++;
}

/*static*/ void GuiData::RemoveLogfile(std::shared_ptr<InputLogfile> logfile)
{
    std::unique_lock<std::mutex> lock(_logfilesMutex);
    auto entry = std::find(_logfiles.begin(), _logfiles.end(), logfile);
    if (entry != _logfiles.end())
    {
        _RemoveDatabase(entry->get()->GetDatabase());
        _logfiles.erase(entry);
    }
    serial++;
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ void GuiData::_AddDatabase(std::shared_ptr<Database> database)
{
    std::unique_lock<std::mutex> lock(_databasesMutex);
    auto entry = std::find(_databases.begin(), _databases.end(), database);
    if (entry == _databases.end())
    {
        _databases.push_back(database);
    }
    std::sort(_databases.begin(), _databases.end(),
        [](const std::shared_ptr<Database> &a, const std::shared_ptr<Database> &b)
        {
            return a->GetName() < b->GetName();
        });
}

/*static*/ void GuiData::_RemoveDatabase(std::shared_ptr<Database> database)
{
    std::unique_lock<std::mutex> lock(_databasesMutex);
    auto entry = std::find(_databases.begin(), _databases.end(), database);
    if (entry != _databases.end())
    {
        _databases.erase(entry);
    }
}

/* ****************************************************************************************************************** */
// eof