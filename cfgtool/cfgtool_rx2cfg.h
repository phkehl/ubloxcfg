/* ************************************************************************************************/ // clang-format off
// u-blox 9 positioning receivers configuration tool
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

#include <stdint.h>
#include <stdbool.h>

#ifndef __CFGTOOL_RX2CFG_H__
#define __CFGTOOL_RX2CFG_H__

/* ****************************************************************************************************************** */

const char *rx2cfgHelp(void);
int rx2cfgRun(const char *portArg, const char *layerArg, const bool useUnknownItems, const bool extraInfo);

const char *rx2listHelp(void);
int rx2listRun(const char *portArg, const char *layerArg, const bool useUnknownItems, const bool extraInfo);

/* ****************************************************************************************************************** */
#endif // __CFGTOOL_RX2CFG_H__
