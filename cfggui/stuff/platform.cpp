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

#include <filesystem>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <exception>
#include <chrono>

#include <unistd.h>
#include <sys/prctl.h>

#include "ff_debug.h"

#include "platform_folders.hpp"

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

const std::string &HomeDir()
{
    static std::string homeDir = "";
    if (homeDir.empty())
    {
        homeDir = sago::getUserHome();
    }
    return homeDir;
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
    if (!name.empty())
    {
        path /= name;
    }
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

uint64_t FileSize(const std::string &file)
{
    std::filesystem::path path { file };
    std::error_code err;
    const uint64_t size = std::filesystem::file_size(path, err);
    if (err)
    {
        return 0;
    }
    else
    {
        return size;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

std::string FileName(const std::string &path) {
    return std::filesystem::path(path).filename();
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

    return ports;
}

// ---------------------------------------------------------------------------------------------------------------------

void SetThreadName(const std::string &name)
{
    // FIXME: refactor...
    char currName[16];
    char threadName[32]; // max 16 cf. prctl(2)
    currName[0] = '\0';
    if (prctl(PR_GET_NAME, currName, 0, 0, 0) == 0)
    {
        currName[7] = '\0';
        std::snprintf(threadName, sizeof(threadName), "%s:%s", currName, name.c_str());
        prctl(PR_SET_NAME, threadName, 0, 0, 0);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

class CacheWiper
{
    public:
        CacheWiper(const std::string &path, const double maxAge) :
            _basePath{path}, _maxAge{maxAge},
            _numFilesTotal{0}, _numDirsTotal{0}, _numFilesWiped{0}, _numDirsWiped{0}, _sizeTotal{0}, _sizeWiped{0}

        {
            if (path.empty() || (maxAge < 0.0) )
            {
                throw std::runtime_error("Bad parameters!");
            }
        }

        void Wipe()
        {
            _refTime = std::filesystem::file_time_type::clock::now();
            _Wipe(_basePath);
            DEBUG("CacheWiper: wiped %d/%d files and %d/%d dirs (%.1f/%.1fMiB) in %s",
                _numFilesWiped, _numFilesTotal, _numDirsWiped, _numDirsTotal,
                (double)_sizeWiped / 1024.0 / 1024.0, (double)_sizeTotal / 1024.0 / 1024.0,
                _basePath.c_str());
        }

    private:
        std::filesystem::path _basePath;
        double      _maxAge;
        std::filesystem::file_time_type _refTime;
        int         _numFilesTotal;
        int         _numDirsTotal;
        int         _numFilesWiped;
        int         _numDirsWiped;
        std::size_t _sizeTotal;
        std::size_t _sizeWiped;

        bool _Wipe(const std::filesystem::path &path)
        {
            bool dirEmpty = true;
            for (const auto &entry: std::filesystem::directory_iterator(path))
            {
                // Check files
                if (entry.is_regular_file())
                {
                    _numFilesTotal++;
                    const auto size = entry.file_size();
                    _sizeTotal += size;
                    if ( (_Age(entry) > _maxAge) && _Remove(entry.path()) )
                    {
                        _numFilesWiped++;
                        _sizeWiped += size;
                    }
                    else
                    {
                        dirEmpty = false;
                    }
                }
                // Iterate directories
                else if (entry.is_directory())
                {
                    _numDirsTotal++;
                    if (!_Wipe(entry.path()))
                    {
                        dirEmpty = false;
                    }
                }
                // There should be no special files here (hmmm...?)
                else
                {
                    WARNING("CacheWiper: Ignoring '%s'!", entry.path().string().c_str());
                }
            }
            // Remove this directory if it's empty now, and if it's not the base dir
            if ( dirEmpty && (path != _basePath) )
            {
                if (_Remove(path))
                {
                    _numDirsWiped++;
                }
            }

            return dirEmpty;
        }

        double _Age(const std::filesystem::directory_entry &entry)
        {
            return std::chrono::duration<double, std::ratio<86400> /*std::chrono::hours*/>(
                _refTime - entry.last_write_time() ).count();
        }

        bool _Remove(const std::filesystem::path &path)
        {
            std::error_code err;
            std::filesystem::remove(path, err);
            if (err)
            {
                WARNING("Failed wiping %s: %s", path.c_str(), err.message().c_str());
                return false;
            }
            else
            {
                return true;
            }
        }
};

// static void _WipeCache(const std::string &path, const double maxAge, const int maxDepth, WipeCacheState &state)
// {
//     for (const auto &entry: std::filesystem::directory_iterator{path})
//     {
//         UNUSED(entry);
//     }

//     UNUSED(maxAge);
//     UNUSED(maxDepth);
//     UNUSED(state);

// }

void WipeCache(const std::string &path, const double maxAge)
{
    if (path.empty())
    {
        ERROR("Won't wipe empty path!");
        return;
    }

    CacheWiper wiper(path, maxAge);
    wiper.Wipe();
}

// ---------------------------------------------------------------------------------------------------------------------

MemUsage::MemUsage() :
    size     { 0.0f },
    resident { 0.0f },
    shared   { 0.0f },
    text     { 0.0f },
    data     { 0.0f }
{
}

MemUsage GetMemUsage()
{
    MemUsage memUsage;
    FILE *fh = fopen( "/proc/self/statm", "r");
    if (fh != NULL)
    {
        float size, resident, shared, text, data;
        if (fscanf(fh, "%f %f %f %f %*s %f", &size, &resident, &shared, &text, &data) == 5)
        {
            const float pageSize = (float)sysconf(_SC_PAGESIZE) * (1.0f/1024.0f/1024.0f);
            memUsage.size     = size     * pageSize;
            memUsage.resident = resident * pageSize;
            memUsage.shared   = shared   * pageSize;
            memUsage.text     = text     * pageSize;
            memUsage.data     = data     * pageSize;
        }
        fclose(fh);
    }
    return memUsage;
}


/* ****************************************************************************************************************** */
} // namespace Platform
