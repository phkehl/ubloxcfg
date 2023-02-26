// flipflip's CRC stuff
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

#ifndef __FF_CRC_H__
#define __FF_CRC_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */

uint32_t crcRtcm3(const uint8_t *data, const int len);
uint32_t crcSpartn4(const uint8_t *data, const int len);   // frame CRC
uint32_t crcSpartn8(const uint8_t *data, const int len);   // type 0
uint32_t crcSpartn16(const uint8_t *data, const int len);  // type 1
uint32_t crcSpartn24(const uint8_t *data, const int len);  // type 2, same as crcRtcm3()
uint32_t crcSpartn32(const uint8_t *data, const int len);  // type 3
uint32_t crcNovatel32(const uint8_t *data, const int len);

/* ****************************************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_CRC_H__
