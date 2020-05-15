// flipflip's RTCM3 protocol stuff
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

#include <stdint.h>
#include <stdbool.h>

#ifndef __FF_RTCM3_H__
#define __FF_RTCM3_H__

/* ********************************************************************************************** */

#define RTCM3_PREAMBLE 0xd3
#define RTCM3_HEAD_SIZE 3
#define RTCM3_FRAME_SIZE (RTCM3_HEAD_SIZE + 3)

bool rtcm3MessageName(char *name, const int size, const uint8_t *msg, const int msgSize);

/* ********************************************************************************************** */
#endif // __FF_RTCM3_H__
