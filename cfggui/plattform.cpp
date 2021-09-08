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

#include <filesystem>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <cerrno>

#ifdef _WIN32
#  ifndef NOGDI
#    define NOGDI
#  endif
#endif

#include "platform_folders.hpp"

#ifndef _WIN32
#  include <sys/prctl.h>
#endif

#include "ff_debug.h"
#include "platform.hpp"

namespace Platform {
/* ****************************************************************************************************************** */

static bool _checkDir(std::string &dir)
{
    std::filesystem::path path(dir);
    std::error_code ec;
    if (!std::filesystem::exists(path) && !std::filesystem::create_directories(path, ec))
    {
        WARNING("Failed creating %s: %s", dir.c_str(), ec.message().c_str());
        return false;
    }
    if (!std::filesystem::is_directory(path))
    {
        WARNING("Not a directory: %s", dir.c_str());
        return false;
    }
    // FIXME: check that WE can write
    // auto perms = std::filesystem::status(path, ec).permissions();
    // if ( (perms & std::filesystem::perms::owner_write) != std::filesystem::perms::owner_write)
    // {
    //     WARNING("Read-only: %s", dir.c_str());
    //     return false;
    // }
    return true;
}

bool Init()
{
    bool res = true;
    auto configDir = ConfigBaseDir();
    DEBUG("Platform::ConfigDir() = %s", configDir.c_str());
    if (!_checkDir(configDir))
    {
        res = false;
    }
    auto cacheDir = CacheBaseDir();
    DEBUG("Platform::CacheDir() = %s", cacheDir.c_str());
    if (!_checkDir(cacheDir))
    {
        res = false;
    }
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::string &ConfigBaseDir()
{
    static std::string configDir = "";
    if (configDir.empty())
    {
        std::filesystem::path dir = sago::getConfigHome();
        dir /= "cfggui";
        configDir = dir.string();
    }
    return configDir;
}

const std::string &CacheBaseDir()
{
    static std::string cacheDir = "";
    if (cacheDir.empty())
    {
        std::filesystem::path dir = sago::getCacheDir();
        dir /= "cfggui";
        cacheDir = dir.string();
    }
    return cacheDir;
}

std::string ConfigFile(const std::string &name)
{
    std::filesystem::path path( ConfigBaseDir() );
    path /= name;
    return path.string();
}

std::string CacheDir(const std::string &name)
{
    std::filesystem::path path( CacheBaseDir() );
    path /= name;
    return path.string();
}

bool FileExists(const std::string &file)
{
   std::filesystem::path path(file);
   return std::filesystem::exists(path);
}

bool FileSpew(const std::string &file, const uint8_t *data, const int size, const bool debug)
{
    bool res = true;
    std::ofstream out;
    out.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        if (debug)
        {
            DEBUG("FileSpew() %s (%d)", file.c_str(), size);
        }
        out.open(file, std::ofstream::binary);
        out.write((const char*)data, size);
    }
    catch (...)
    {
        WARNING("Failed writing %s: %s", file.c_str(), std::strerror(errno));
        res = false;
    }
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::vector<Port> &EnumeratePorts(const bool enumerate)
{
    static std::vector<Port> ports;

    if (!enumerate && !ports.empty())
    {
        return ports;
    }

    ports.clear();

#ifdef _WIN32
// TODO...
#else
    const std::filesystem::path devSerialByIdDir("/dev/serial/by-id");

    if (!std::filesystem::exists(devSerialByIdDir))
    {
        return ports;
    }
    for (auto &entry: std::filesystem::directory_iterator(devSerialByIdDir))
    {
        if (entry.is_symlink())
        {
            //auto source = entry.path();
            auto target = std::filesystem::weakly_canonical( devSerialByIdDir / std::filesystem::read_symlink(entry) );

            ports.push_back( Port(target, entry.path().filename().string()) );
        }
    }
    std::sort(ports.begin(), ports.end(),
        [](const Port &a, const Port &b)
        {
            return a.port < b.port;
        });
#endif

    return ports;
}


/* ****************************************************************************************************************** */
} // namespace Platform
