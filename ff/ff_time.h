// clang-format off
// flipflip's time functions
//
// Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org) and contributors
// https://oinkzwurgl.org/projaeggd/ubloxcfg/
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

#ifndef __FF_TIME_H__
#define __FF_TIME_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */

double wnoTow2ts(const int wno, const double tow);

double ts2posix(const double ts, const int leapSec, const bool leapSecValid);

double posixNow();

/* ****************************************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_TIME_H__
