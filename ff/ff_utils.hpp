// flipflip's c++ stuff: utilities
//
// Copyright (c) 2020 Philippe Kehl (flipflip at oinkzwurgl dot org),
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

#ifndef __FF_UTILS_HPP__
#define __FF_UTILS_HPP__

#include <vector>
#include <string>
#include <cstdarg>

#include "ff_stuff.h"

/* ****************************************************************************************************************** */

namespace Ff
{
    // sprintf()
    std::string Sprintf(const char * const zcFormat, ...) PRINTF_ATTR(1);

    // vsprintf()
    std::string Vsprintf(const char *fmt, va_list args);

    // strftime()
    std::string Strftime(const char * const fmt, const int64_t ts = 0);

    // String search & replace
    int StrReplace(std::string &str, const std::string &search, const std::string &replace);

    // Trim string
    void StrTrimLeft(std::string &str);
    void StrTrimRight(std::string &str);
    void StrTrim(std::string &str);

    // Split string
    std::vector<std::string> StrSplit(const std::string &str, const std::string &sep, const int maxNum = 0);

    // Join string
    std::string StrJoin(const std::vector<std::string> &strs, const std::string &sep);

    // Remove duplicates
    void MakeUnique(std::vector<std::string> &vec);

    // Hexdump
    std::vector<std::string> HexDump(const std::vector<uint8_t> data);
    std::vector<std::string> HexDump(const uint8_t *data, const int size);

    // 2d Vector
    template<typename T>
    struct Vec2
    {
        T x, y;
        Vec2() : x{0.0}, y{0.0} { }
        Vec2(const T _x, const T _y) : x{_x}, y{_y} { }
        inline Vec2  operator *  (const T f)     const { return Vec2(x * f,   y * f);   }   // Vec2 * x
        inline Vec2  operator /  (const T f)     const { return Vec2(x / f,   y / f);   }   // Vec2 / x
        inline Vec2  operator *  (const Vec2 &v) const { return Vec2(x * v.x, y * v.y); }   // Vec2 * Vec2
        inline Vec2  operator /  (const Vec2 &v) const { return Vec2(x / v.x, y / v.y); }   // Vec2 / Vec2
        inline Vec2  operator +  (const Vec2 &v) const { return Vec2(x + v.x, y + v.y); }   // Vec2 + Vec2
        inline Vec2  operator -  (const Vec2 &v) const { return Vec2(x - v.x, y - v.y); }   // Vec2 - Vec2
        inline Vec2 &operator *= (const T f)           { x *= f;   y *= f; }                // Vec2 *= x;
        inline Vec2 &operator /= (const T f)           { x /= f;   y /= f; }                // Vec2 /= x;
        inline Vec2 &operator *= (const Vec2 &v)       { x *= v.x; y *= v.y; }              // Vec2 *= Vec2;
        inline Vec2 &operator /= (const Vec2 &v)       { x /= v.x; y /= v.y; }              // Vec2 /= Vec2;
        inline Vec2 &operator += (const Vec2 &v)       { x += v.x; y += v.y; }              // Vec2 += Vec2;
        inline Vec2 &operator -= (const Vec2 &v)       { x -= v.x; y -= v.y; }              // Vec2 -= Vec2;
    };

    template <typename T>
    inline Vec2<T> operator*(const T f, Vec2<T> const & v) { return v * f; } // x * Vec2
};

/* ****************************************************************************************************************** */

#endif // __FF_UTILS_HPP__
