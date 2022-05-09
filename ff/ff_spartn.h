// flipflip's SPARTN protocol stuff
//
// Copyright (c) 2022 Philippe Kehl (flipflip at oinkzwurgl dot org),
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

#ifndef __FF_SPARTN_H__
#define __FF_SPARTN_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */

#define SPARTN_PREAMBLE 0x73
#define SPARTN_MIN_HEAD_SIZE 8

bool spartnMessageName(char *name, const int size, const uint8_t *msg, const int msgSize);

bool spartnMessageInfo(char *info, const int size, const uint8_t *msg, const int msgSize);

const char *spartnTypeDesc(const int msgType, const int subType);

/* ****************************************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_SPARTN_H__
