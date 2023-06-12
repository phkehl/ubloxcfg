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

#ifndef __PLATFORM_HPP__
#define __PLATFORM_HPP__

#include <vector>
#include <string>
#include <cstdint>

namespace Platform {
/* ****************************************************************************************************************** */

bool Init();

const std::string &ConfigBaseDir();
const std::string &CacheBaseDir();
const std::string &HomeDir();

std::string ConfigFile(const std::string &name);
std::string CacheDir(const std::string &name = "");


bool FileExists(const std::string &file);

bool FileSpew(const std::string &file, const uint8_t *data, const int size, bool debug = true);

uint64_t FileSize(const std::string &file);

std::string FileName(const std::string &path);

struct Port
{
    Port(const std::string &_port, const std::string &_desc) : port{_port}, desc{_desc} {}
    Port(const char *_port, const char *_desc) : port{_port}, desc{_desc} {}
    std::string port;
    std::string desc;
};
const std::vector<Port> &EnumeratePorts(const bool enumerate = true);

void SetThreadName(const std::string &name);

void WipeCache(const std::string &path, const double maxAge);

struct MemUsage
{
    MemUsage();
    // all in  [MiB]
    float size;     // total size
    float resident; // resident set size
    float shared;   // resident shared
    float text;     // text (code)
    float data;     // data + stack
};

MemUsage GetMemUsage();

/* ****************************************************************************************************************** */
} // namespace Platform
#endif // __PLATFORM_HPP__
