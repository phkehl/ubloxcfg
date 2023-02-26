// flipflip's c++ stuff for flipflip's c stuff
//
// Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org),
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

#include <stdexcept>
#include <fstream>
#include <cerrno>
#include <cstring>

#include "ff_debug.h"
#include "ff_utils.hpp"

#include "ff_conffile.hpp"

/* ****************************************************************************************************************** */

Ff::ConfFile::ConfFile() :
    _keyVal{}, _lastLine{0}, _file{}, _sectionBeginLine{0}
{
}

// ---------------------------------------------------------------------------------------------------------------------

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
    //Set(key, std::to_string(val));
    Set(key, Ff::Sprintf("%.12g", val));
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
        std::size_t num = 0;
        try { val = std::stol(str, &num, 0); } catch (...) { }
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
        std::size_t num = 0;
        try { val = std::stoul(str, &num, 0); } catch (...) { }
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
        std::size_t num = 0;
        try { val = std::stof(str, &num); } catch (...) { }
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
        std::size_t num = 0;
        try { val = std::stod(str, &num); } catch (...) { }
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

// ---------------------------------------------------------------------------------------------------------------------

const std::map<std::string, std::string> &Ff::ConfFile::GetKeyValList()
{
    return _keyVal;
}

// ---------------------------------------------------------------------------------------------------------------------

int Ff::ConfFile::GetSectionBeginLine()
{
    return _sectionBeginLine;
}

// ---------------------------------------------------------------------------------------------------------------------

void Ff::ConfFile::Clear()
{
    _keyVal.clear();
}

// ---------------------------------------------------------------------------------------------------------------------

bool Ff::ConfFile::Load(const std::string &file, const std::string &section)
{
    // Different file, start looking for section from the beginning
    if (file != _file)
    {
        _file = file;
        _lastLine = 0;
        _sectionBeginLine = 0;
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
                    _sectionBeginLine = lineNo;
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
            if (!t.second.empty())
            {
                out << t.first << "=" << t.second << "\n";
            }
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
