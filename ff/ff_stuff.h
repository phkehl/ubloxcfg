// flipflip's Allencheibs
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

#ifndef __FF_STUFF_H__
#define __FF_STUFF_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */

uint32_t TIME(void);
void SLEEP(uint32_t dur);

uint32_t timeOfDay(void);

#define NUMOF(x) (int)(sizeof(x)/sizeof(*(x)))

#ifdef _WIN32
#  define IF_WIN(x) x
#  define NOT_WIN(x) /* nothing */
#else
#  define IF_WIN(x) /* nothing */
#  define NOT_WIN(x) x
#endif

#if (defined(DEBUG) || defined(_DEBUG) || defined(DBG)) && !defined(NDEBUG)
#  define FF_DEBUG 1
#else
#  define FF_DEBUG 0
#endif

#define STRINGIFY(x) _STRINGIFY(x) //!< preprocessor stringification  \hideinitializer
#define CONCAT(a, b)   _CONCAT(a, b) //!< preprocessor concatenation  \hideinitializer

#ifndef __DOXYGEN__
#  define _STRINGIFY(x) #x
#  define _CONCAT(a, b)  a ## b
#endif

#define MIN(a, b)  ((b) < (a) ? (b) : (a)) //!< smaller value of a and b \hideinitializer
#define MAX(a, b)  ((b) > (a) ? (b) : (a)) //!< bigger value of a and b \hideinitializer

/* ****************************************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_STUFF_H__
