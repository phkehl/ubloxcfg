// clang-format off
// flipflip's Allencheibs
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

#ifndef __FF_STUFF_H__
#define __FF_STUFF_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */

uint64_t TIME(void);
void SLEEP(uint32_t dur);

uint64_t timeOfDay(void);

//! Number of elements in array \hideinitializer
#define NUMOF(x) (int)(sizeof(x)/sizeof(*(x)))

//! Mark variable as unused to silence compiler warnings \hideinitializer
#ifndef UNUSED
#  define UNUSED(thing) (void)thing
#endif

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

#ifndef STRINGIFY
#  define STRINGIFY(x) _STRINGIFY(x) //!< preprocessor stringification  \hideinitializer
#  ifndef __DOXYGEN__
#    define _STRINGIFY(x) #x
#  endif
#endif
#ifndef CONCAT
#  define CONCAT(a, b)   _CONCAT(a, b) //!< preprocessor concatenation  \hideinitializer
#  ifndef __DOXYGEN__
#    define _CONCAT(a, b)  a ## b
#  endif
#endif


#define MIN(a, b)  ((b) < (a) ? (b) : (a)) //!< smaller value of a and b \hideinitializer
#define MAX(a, b)  ((b) > (a) ? (b) : (a)) //!< bigger value of a and b \hideinitializer
#define ABS(x) ( (x) < 0 ? -(x) : (x) ) //!< absolute value \hideinitializer

//! Bit \hideinitializer
#define BIT(bit) (1<<(bit))

//! Check if all bit(s) is (are) set \hideinitializer
#define CHKBITS(mask, bits)    (((mask) & (bits)) == (bits))

//! Check if any bit(s) is (are) set \hideinitializer
#define CHKBITS_ANY(mask, bits)    (((mask) & (bits)) != 0)

//! Sets the bits \hideinitializer
#define SETBITS(mask, bits)    ( (mask) |= (bits) )

//! Clears the bits \hideinitializer
#define CLRBITS(mask, bits)    ( (mask) &= ~(bits) )

//! Toggles the bits \hideinitializer
#define TOGBITS(mask, bits)    ( (mask) ^= (bits) )

#ifndef PRINTF_ATTR
#  define PRINTF_ATTR(n) __attribute__ ((format (printf, n, n + 1)))
#endif

#define CLIP(x, a, b) ((x) <= (a) ? (a) : ((x) >= (b) ? (b) : (x))) //!< Clip value in range [a:b] \hideinitializer

#ifdef __cplusplus
#  ifndef _Static_assert
#    define _Static_assert static_assert // static_assert only in c++11 and later
#  endif
#endif

//! Static (compile-time) assertion
#define STATIC_ASSERT(expr) _Static_assert((expr), #expr)

//! Size of struct member
#define SIZEOF_MEMBER(_type, _member) sizeof((((_type *)NULL)->_member))

/* ****************************************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_STUFF_H__
