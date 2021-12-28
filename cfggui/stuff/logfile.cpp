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

#include <cstring>
#include <cerrno>
#include <fstream>

#include "zfstream.hpp"

#include "ff_stuff.h"
#include "ff_debug.h"

#include "platform.hpp"
#include "logfile.hpp"

/* ****************************************************************************************************************** */

Logfile::Logfile()
{
    _Clear();
}

Logfile::~Logfile()
{
    Close();
}

// ---------------------------------------------------------------------------------------------------------------------

void Logfile::_Clear()
{
    _in     = nullptr;
    _out    = nullptr;
    _isOpen = false;
    _size   = 0;
    _path.clear();
    _errorStr = "No file opened!";
}

// ---------------------------------------------------------------------------------------------------------------------

bool Logfile::Close()
{
    if (!_isOpen)
    {
        return false;
    }
    DEBUG("Logfile::close(%s)", _path.c_str());
    _Clear();
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::string &Logfile::GetError()
{
    return _errorStr;
}

// ---------------------------------------------------------------------------------------------------------------------

bool Logfile::IsOpen()
{
    return _isOpen;
}

// ---------------------------------------------------------------------------------------------------------------------

bool Logfile::_IsCompressed(const std::string &path)
{
    return (path.size() > 4) && ((path.substr(path.size() - 4) == ".ubz") || (path.substr(path.size() - 3) == ".gz"));
}

// ---------------------------------------------------------------------------------------------------------------------

bool Logfile::OpenRead(const std::string &path)
{
    if (_isOpen)
    {
        _errorStr = "Logfile::OpenRead(%s) already open: %s", path.c_str(), _path.c_str();
        return false;
    }

    _Clear();

    DEBUG("Logfile::OpenRead(%s)", path.c_str());

    _isCompressed = _IsCompressed(path);
    if (_isCompressed)
    {
        _in = std::make_unique<gzifstream>(path.c_str());
    }
    else
    {
        _in = std::make_unique<std::ifstream>(path, std::ios::binary);
    }

    if (_in->fail())
    {
        _Clear();
        _errorStr = std::strerror(errno);
        return false;
    }

    _isOpen = true;
    _path = path;

    _size = Platform::FileSize(path);

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

bool Logfile::OpenWrite(const std::string &path)
{
    if (_isOpen)
    {
        _errorStr = "Logfile::OpenWrite(%s) already open: %s", path.c_str(), _path.c_str();
        return false;
    }

    _Clear();

    DEBUG("Logfile::OpenWrite(%s)", path.c_str());

    _isCompressed = _IsCompressed(path);
    if (_isCompressed)
    {
        _out = std::make_unique<gzofstream>(path.c_str());
    }
    else
    {
        _out = std::make_unique<std::ofstream>(path, std::ios::binary);
    }

    if (_out->fail())
    {
        _Clear();
        _errorStr = std::strerror(errno);
        return false;
    }

    _isOpen = true;
    _path = path;

    return true;
}
// ---------------------------------------------------------------------------------------------------------------------

const std::string &Logfile::Path()
{
    return _path;
}

// ---------------------------------------------------------------------------------------------------------------------

bool Logfile::Write(const uint8_t *data, const uint64_t size)
{
    if (!_isOpen || !_out || !data)
    {
        return false;
    }

    _out->write((const char *)data, size);
    if (_out->fail())
    {
        _errorStr = std::strerror(errno);
        return false;
    }
    _size += size;
    return true;
}

bool Logfile::Write(const std::vector<uint8_t> data)
{
    return Write((const uint8_t *)data.data(), (int)data.size());
}

// ---------------------------------------------------------------------------------------------------------------------

uint64_t Logfile::Read(uint8_t *data, const uint64_t size)
{
    if (!_isOpen || !_in)
    {
        return 0;
    }
    else
    {
        _in->read((char *)data, size);
        return _in->gcount();
        //return _in->readsome((char *)data, size); // doesn't work for compressed logs
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool Logfile::CanSeek()
{
    return !_isCompressed;
}

// ---------------------------------------------------------------------------------------------------------------------

uint64_t Logfile::Size()
{
    return _size;
}
// ---------------------------------------------------------------------------------------------------------------------

void Logfile::Seek(const uint64_t pos)
{
    if ( !_isCompressed && _isOpen && _in && (pos < _size) )
    {
        _in->clear(); // seems to be neccessary or seekg() sometimes fails
        _in->seekg(pos, std::ios::beg);
        if (_in->fail())
        {
            WARNING("seek %lu fail: %s", pos, std::strerror(errno));
        }
    }
}

/* ****************************************************************************************************************** */
