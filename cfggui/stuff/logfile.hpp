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

#ifndef __LOGFILE_HPP__
#define __LOGFILE_HPP__

#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include <cstdint>

/* ****************************************************************************************************************** */

class Logfile
{
    public:

        Logfile();
       ~Logfile();

        bool     OpenRead(const std::string &path);
        uint64_t Read(uint8_t *data, const uint64_t size);
        uint64_t Size();
        void     Seek(const uint64_t pos);
        bool     CanSeek();

        bool     OpenWrite(const std::string &path);
        bool     Write(const uint8_t *data, const uint64_t size);
        bool     Write(const std::vector<uint8_t> data);

        bool     Close();

        bool     IsOpen();

        const std::string &Path();
        const std::string &GetError();

    private:

        bool           _isOpen;
        std::string    _path;
        std::unique_ptr<std::istream> _in;
        std::unique_ptr<std::ostream> _out;
        std::string    _errorStr;
        uint64_t       _size;
        bool           _isCompressed;

        bool _IsCompressed(const std::string &path);
        void _Clear();
};

/* ****************************************************************************************************************** */
#endif // __LOGFILE_HPP__
