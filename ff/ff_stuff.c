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

#include <unistd.h>
#include <time.h>
#ifdef _WIN32
#  define NOGDI
#  include <windows.h>
#endif

#include "ff_stuff.h"

/* ****************************************************************************************************************** */

uint64_t TIME(void)
{
    static uint64_t t0 = 0;
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    uint64_t t = (tp.tv_sec * 1000) + (tp.tv_nsec / 1000000);
    if (t0 == 0)
    {
        t0 = t;
        return 0;
    }

    return t - t0;
}

void SLEEP(uint32_t dur)
{
#ifdef _WIN32
    Sleep(dur);
#else
    usleep(dur * 1000);
#endif
}

uint64_t timeOfDay(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t t = ts.tv_sec % 86400;
    t *= 1000;
    t += ts.tv_nsec / 1000000;
    return t;
}

/* ****************************************************************************************************************** */
// eof
