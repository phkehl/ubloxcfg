// flipflip's c++ stuff for flipflip's c stuff
//
// Copyright (c) 2020 Philippe Kehl (flipflip at oinkzwurgl dot org),
// https://oinkzwurgl.org/hacking/ubloxcfg
//
// Parts copyright by others. See source code.
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

#include <cstdarg>
#include <cerrno>
#include <vector>
#include <algorithm>
#include <time.h>

#include "ff_cpp.hpp"
#include "ff_debug.h"

/* ****************************************************************************************************************** */

// By Douglas Daseeco from https://stackoverflow.com/a/49812018/13355523 (CC BY-SA 4.0)
std::string Ff::Sprintf(const char * const zcFormat, ...)
{
    // initialize use of the variable argument array
    va_list vaArgs;
    va_start(vaArgs, zcFormat);

    // reliably acquire the size
    // from a copy of the variable argument array
    // and a functionally reliable call to mock the formatting
    va_list vaArgsCopy;
    va_copy(vaArgsCopy, vaArgs);
    const int iLen = std::vsnprintf(NULL, 0, zcFormat, vaArgsCopy);
    va_end(vaArgsCopy);

    // return a formatted string without risking memory mismanagement
    // and without assuming any compiler or platform specific behavior
    std::vector<char> zc(iLen + 1);
    std::vsnprintf(zc.data(), zc.size(), zcFormat, vaArgs);
    va_end(vaArgs);
    return std::string(zc.data(), iLen);
}

// ---------------------------------------------------------------------------------------------------------------------

std::string Ff::Strftime(const int64_t ts, const char * const fmt)
{
    std::vector<char> str(1000);
    struct tm now;
    int len = 0;
    const time_t t = (ts <= 0 ? time(NULL) : ts);
    if (localtime_r(&t, &now) == &now)
    {
        len = strftime(str.data(), str.size(), fmt, &now);
    }
    return std::string(str.data(), len);
}

// ---------------------------------------------------------------------------------------------------------------------

int Ff::StrReplace(std::string &str, const std::string &search, const std::string &replace)
{
    int count = 0;
    std::size_t pos = 0;
    while ( (pos = str.find(search, pos)) != std::string::npos )
    {
        str.replace(pos, search.size(), replace);
        pos += replace.size();
        count++;
    }
    return count;
}

// ---------------------------------------------------------------------------------------------------------------------

void Ff::StrTrimLeft(std::string &str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](const int c) { return std::isspace(c) == 0; }));
}

void Ff::StrTrimRight(std::string &str)
{
    str.erase(std::find_if(str.rbegin(), str.rend(), [](const int c) { return std::isspace(c) == 0; }).base(), str.end());
}

void Ff::StrTrim(std::string &str)
{
    StrTrimLeft(str);
    StrTrimRight(str);
}

// ---------------------------------------------------------------------------------------------------------------------

std::vector<std::string> Ff::StrSplit(const std::string &str, const std::string &sep)
{
    std::vector<std::string> out;
    std::string strCopy = str;
    std::size_t pos = 0;
    while ((pos = strCopy.find(sep)) != std::string::npos)
    {
        out.push_back( strCopy.substr(0, pos) );
        strCopy.erase(0, pos + sep.length());
    }
    out.push_back(strCopy);
    return out;
}

// ---------------------------------------------------------------------------------------------------------------------

std::string Ff::StrJoin(const std::vector<std::string> &strs, const std::string &sep)
{
    std::string res = "";
    for (auto &str: strs)
    {
        res += sep;
        res += str;
    }
    return res.empty() ? res : res.substr(1);
}

// ---------------------------------------------------------------------------------------------------------------------

Ff::ConfFile::ConfFile() :
    _keyVal{}, _lastLine{0}, _file{}
{
}

void Ff::ConfFile::Set(const std::string &key, const std::string &val)
{
    //DEBUG("Ff::ConfFile::Set() %s=%s", key.c_str(), val.c_str());
    _keyVal[key] = val;
}

void Ff::ConfFile::Set(const std::string &key, const int32_t val)
{
    Set(key, std::to_string(val));
}

void Ff::ConfFile::Set(const std::string &key, const uint32_t val, const bool hex)
{
    Set(key, hex ? Sprintf("0x%08x", val) : std::to_string(val));
}

void Ff::ConfFile::Set(const std::string &key, const double val)
{
    Set(key, std::to_string(val));
}

void Ff::ConfFile::Set(const std::string &key, const bool val)
{
    Set(key, val ? std::string("true") : std::string("false"));
}

bool Ff::ConfFile::Get(const std::string &key, std::string &val)
{
    const auto entry = _keyVal.find(key);
    if (entry != _keyVal.end())
    {
        val = entry->second;
        return true;
    }
    else
    {
        return false;
    }
    //return res;
}

bool Ff::ConfFile::Get(const std::string &key, int32_t &val)
{
    bool res = false;
    std::string str;
    if (Get(key, str))
    {
        std::size_t num;
        val = std::stol(str, &num, 0);
        res = num > 0;
    }
    return res;
}

bool Ff::ConfFile::Get(const std::string &key, uint32_t &val)
{
    bool res = false;
    std::string str;
    if (Get(key, str))
    {
        std::size_t num;
        val = std::stoul(str, &num, 0);
        res = num > 0;
    }
    return res;
}

bool Ff::ConfFile::Get(const std::string &key, float &val)
{
    bool res = false;
    std::string str;
    if (Get(key, str))
    {
        std::size_t num;
        val = std::stof(str, &num);
        res = num > 0;
    }
    return res;
}

bool Ff::ConfFile::Get(const std::string &key, double &val)
{
    bool res = false;
    std::string str;
    if (Get(key, str))
    {
        std::size_t num;
        val = std::stod(str, &num);
        res = num > 0;
    }
    return res;
}

bool Ff::ConfFile::Get(const std::string &key, bool &val)
{
    bool res = false;
    std::string str;
    if (Get(key, str))
    {
        val = (str == "true");
        res = true;
    }
    return res;
}

void Ff::ConfFile::Clear()
{
    _keyVal.clear();
}

bool Ff::ConfFile::Load(const std::string &file, const std::string &section)
{
    // Different file, start looking for section from the beginning
    if (file != _file)
    {
        _file = file;
        _lastLine = 0;
    }

    bool res = true;
    std::ifstream in;
    in.exceptions(/*std::ifstream::failbit |*/ std::ifstream::badbit);
    try
    {
        in.open(file.c_str());
        if (!in.good())
        {
            throw std::runtime_error("Failed reading");
        }
        int lineNo = 0;
        std::string line;
        while (lineNo < _lastLine)
        {
            std::getline(in, line);
            lineNo++;
        }
        // Find section begin
        {
            const std::string marker = "[" + section + "]";
            bool haveSection = false;
            while (res)
            {
                if (!std::getline(in, line))
                {
                    break;
                }
                lineNo++;
                if (line.substr(0, 8) == marker)
                {
                    haveSection = true;
                    break;
                }
            }
            if (!haveSection)
            {
                res = false;
            }
        }
        while (res)
        {
            if (!std::getline(in, line))
            {
                break;
            }
            lineNo++;
            if (line.empty() || line.substr(0, 1) == "#")
            {
                continue;
            }
            else if ( !section.empty() && (line.substr(0, 1) == "[") )
            {
                break;
            }
            std::string key;
            std::string val;
            std::size_t pos = line.find("=");
            if ( (pos != std::string::npos) && (pos > 0) && (pos < line.size()) )
            {
                key = line.substr(0, pos);
                val = line.substr(pos + 1);
                StrTrim(key);
                StrTrim(val);
                //DEBUG("%s:%d: key=[%s] val=[%s]", file.c_str(), lineNo, key.c_str(), val.c_str());
                if (!key.empty())
                {
                    _keyVal[key] = val;
                    continue;
                }
            }
            WARNING("Bad line %s:%d: %s", file.c_str(), lineNo, line.c_str());
        }
        in.close();
        _lastLine = lineNo - 1;
    }
    catch (std::ifstream::failure &e)
    {
        (void)e;
        WARNING("Failed reading %s: %s", file.c_str(), std::strerror(errno));
        res = false;
    }
    catch (std::runtime_error &e)
    {
        WARNING("%s (%s)!", e.what(), file.c_str());
        res = false;
    }
    return res;
}

bool Ff::ConfFile::Save(const std::string &file, const std::string &section, const bool append)
{
    bool res = true;
    std::ofstream out;
    out.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        std::ios_base::openmode mode = (append ? std::ofstream::app : (std::ios_base::openmode)0);
        out.open(file, mode);
        if (!section.empty())
        {
            out << "\n[" + section + "]\n";
        }
        for (const auto &t: _keyVal)
        {
            out << t.first << "=" << t.second << "\n";
        }
        out.close();
    }
    catch (...)
    {
        ERROR("Failed writing %s: %s", file.c_str(), std::strerror(errno));
        res = false;
    }
    return res;
}

/* ****************************************************************************************************************** */
