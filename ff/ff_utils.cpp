// flipflip's c++ stuff: utilities
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
#include <algorithm>
#include <cinttypes>
#include <cstring>

#include "ff_utils.hpp"

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

std::string Ff::Vsprintf(const char *fmt, va_list args)
{
    va_list argsCopy;
    va_copy(argsCopy, args);
    const int len = std::vsnprintf(NULL, 0, fmt, argsCopy);
    va_end(argsCopy);

    std::vector<char> buf(len + 1);
    std::vsnprintf(buf.data(), buf.size(), fmt, args);
    return std::string(buf.data(), len);
}

// ---------------------------------------------------------------------------------------------------------------------

std::string Ff::Strftime(const char * const fmt, const int64_t ts)
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

#include "ff_debug.h"
std::vector<std::string> Ff::StrSplit(const std::string &str, const std::string &sep, const int maxNum)
{
    std::vector<std::string> out;
    if (str.empty())
    {
        return out;
    }
    if (sep.empty())
    {
        out.push_back(str);
        return out;
    }

    std::string strCopy = str;
    std::size_t pos = 0;
    int num = 0;
    while ((pos = strCopy.find(sep)) != std::string::npos)
    {
        std::string part = strCopy.substr(0, pos);
        strCopy.erase(0, pos + sep.length());
        out.push_back(part);
        num++;
        if ( (maxNum > 0) && (num >= (maxNum - 1)) )
        {
            break;
        }
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

std::vector<std::string> Ff::HexDump(const std::vector<uint8_t> data)
{
    return HexDump(data.data(), (int)data.size());
}

std::vector<std::string> Ff::HexDump(const uint8_t *data, const int size)
{
    std::vector<std::string> hexdump;
    const char i2hex[] = "0123456789abcdef";
    const uint8_t *pData = data;
    for (int ix = 0; ix < size; )
    {
        char str[70];
        std::memset(str, ' ', sizeof(str));
        str[50] = '|';
        str[67] = '|';
        str[68] = '\0';
        for (int ix2 = 0; (ix2 < 16) && ((ix + ix2) < size); ix2++)
        {
            //           1         2         3         4         5         6
            // 012345678901234567890123456789012345678901234567890123456789012345678
            // xx xx xx xx xx xx xx xx  xx xx xx xx xx xx xx xx  |................|\0
            // 0  1  2  3  4  5  6  7   8  9  10 11 12 13 14 15
            const uint8_t c = pData[ix + ix2];
            int pos1 = 3 * ix2;
            int pos2 = 51 + ix2;
            if (ix2 > 7)
            {
                   pos1++;
            }
            str[pos1    ] = i2hex[ (c >> 4) & 0xf ];
            str[pos1 + 1] = i2hex[  c       & 0xf ];

            str[pos2] = isprint((int)c) ? c : '.';
        }
        char buf[1024];
        std::snprintf(buf, sizeof(buf), "0x%04" PRIx8 " %05d  %s", ix, ix, str);
        hexdump.push_back(buf);
        ix += 16;
    }
    return hexdump;
}

/* ****************************************************************************************************************** */
