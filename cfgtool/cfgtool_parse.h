// clang-format off
/* ****************************************************************************************************************** */
// u-blox positioning receivers configuration tool
//
// Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org) and contributors
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

#include <stdint.h>
#include <stdbool.h>

#ifndef __CFGTOOL_PARSE_H__
#define __CFGTOOL_PARSE_H__

/* ****************************************************************************************************************** */

const char *parseHelp(void);

int parseRun(const bool extraInfo, const bool doEpoch);

/* ****************************************************************************************************************** */
#endif // __CFGTOOL_PARSE_H__
