// flipflip's time functions
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

#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "ff_debug.h"

#include "ff_time.h"

/* ****************************************************************************************************************** */

#define GPS_POSIX_OFFS    (3657*86400)    // Offset [s] from POSIX epoch to GPS epoch
#define WEEK              (7*86400)       // Number of seconds in a week
#define GPS_TAI_OFFSET    19              // TAI (International Atomic Time) - GPS time

// ---------------------------------------------------------------------------------------------------------------------

static int sLeapSecAtTs(const double ts)
{
    // See IERS bulletin C (https://hpiers.obspm.fr/iers/bul/bulc/bulletinc.dat)
    if (ts > 1356393618.0) // 2022-12-30 2242:432018 (see IERS "Bulletin C" #63 January 2022)
    {
        static bool warned = false;
        if (!warned)
        {
            WARNING("No leap second information!");
            warned = true;
        }
        return 37 - GPS_TAI_OFFSET; // = 37 - 19 = 18
    }
    else if (ts >= 1167264000.0) { return 18; } // 2017-01-01 1930:000000 (see IERS "Bulletin C" #52 January 2016)
    else if (ts >= 1119744000.0) { return 17; } // 2015-07-01 1851:259200
    else if (ts >= 1025136000.0) { return 16; } // 2012-07-01 1695:000000
    else if (ts >=  914803200.0) { return 15; } // 2009-01-01 1512:345600
    else if (ts >=  820108800.0) { return 14; } // 2006-01-01 1356:000000
    else if (ts >=  598795200.0) { return 13; } // 1999-01-01 0990:043200
    else if (ts >=  551750400.0) { return 12; } // 1997-07-01 0912:172800
    else if (ts >=  504489600.0) { return 11; } // 1996-01-01 0834:086400
    else if (ts >=  457056000.0) { return 10; } // 1994-07-01 0755:432000
    else if (ts >=  425520000.0) { return  9; } // 1993-07-01 0703:345600
    else if (ts >=  393984000.0) { return  8; } // 1992-07-01 0651:259200
    else if (ts >=  346723200.0) { return  7; } // 1991-01-01 0573:172800
    else if (ts >=  315187200.0) { return  6; } // 1990-01-01 0512:086400
    else if (ts >=  251640000.0) { return  5; } // 1988-01-01 0416:043200
    else if (ts >=  173059200.0) { return  4; } // 1985-07-01 0286:086400
    else if (ts >=  109900800.0) { return  3; } // 1983-07-01 0181:432000
    else if (ts >=   62726400.0) { return  2; } // 1982-01-01 0103:432000
    else if (ts >=   31190400.0) { return  1; } // 1981-01-01 0051:345600
    else if (ts >=          0.0) { return  0; } // 1980-01-06 0000:000000
    else                         { return  0; } // FIXME: warn?
}

// ---------------------------------------------------------------------------------------------------------------------

double wnoTow2ts(const int wno, const double tow)
{
    return (double)(wno * WEEK) + tow;
}

// ---------------------------------------------------------------------------------------------------------------------

double ts2posix(const double ts, const int leapSec, const bool leapSecValid)
{
    if (leapSecValid)
    {
        return ts + (double)(GPS_POSIX_OFFS - leapSec);
    }
    else
    {
        return ts + (double)(GPS_POSIX_OFFS - sLeapSecAtTs(ts));
    }
}


// ---------------------------------------------------------------------------------------------------------------------

double posixNow()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_sec + ((double)ts.tv_nsec * 1e-9);
}

/* ****************************************************************************************************************** */
// eof
