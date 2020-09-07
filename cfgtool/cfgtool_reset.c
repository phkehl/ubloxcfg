// u-blox 9 positioning receivers configuration tool
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

#include <string.h>
#include <stddef.h>
#include <signal.h>

#include "cfgtool_util.h"

#include "ff_rx.h"
#include "ff_ubx.h"

#include "cfgtool_reset.h"

/* ****************************************************************************************************************** */

const char *resetHelp(void)
{
    return
// -----------------------------------------------------------------------------
"Command 'reset':\n"
"\n"
"    Usage: cfgtool reset -p <port> -r <reset>\n"
"\n"
"    This resets the receiver. Depending on the <reset> type different parts of\n"
"    the navigation data and the configuration are deleted.\n"
"\n"
"        <reset>   Navigation   Configuration   Reset\n"
"        ------------------------------------------------------------\n"
"        soft      -            - (update)      Software (controlled)\n"
"        hard      -            - (update)      Hardware (controlled)\n"
"        hot       -            -               Software (GNSS only)\n"
"        warm      Ephemerides  -               Software (GNSS only)\n"
"        cold      All          -               Software (GNSS only)\n"
"        default   -            All             Hardware (controlled)\n"
"        factory   All          All             Hardware (controlled)\n"
"        stop      -            -               -\n"
"        start     -            -               -\n"
"        gnss      -            -               -\n"
"\n"
"    Notes:\n"
"\n"
"        All hardware resets cause a USB disconnect, which triggers the the host\n"
"        to re-enumerate the device. For local serial <port>s (ser://<device>)\n"
"        this is taken into account and should be handled by this command.\n"
"\n"
"        The hot, warm, and cold resets use the same command as the corresponding\n"
"        buttons in u-center.\n"
"\n"
"        The soft and hard resets both trigger a full restart, which includes re-\n"
"        freshing the RAM configuration layer.\n"
"\n"
"        Note that for hardware resets the navigation and configuration data is\n"
"        only preserved with battery backup respectively configuration in the\n"
"        Flash layer.\n"
"\n"
"        The stop, start and gnss resets do not actually reset anything but\n"
"        instead stop, start and restart the GNSS operation.\n"
"\n";
}

/* ****************************************************************************************************************** */

bool resetTypeFromStr(const char *resetType, RX_RESET_t *reset)
{
    if (strcasecmp("soft", resetType) == 0)
    {
        *reset = RX_RESET_SOFT;
    }
    else if (strcasecmp("hard", resetType) == 0)
    {
        *reset = RX_RESET_HARD;
    }
    else if (strcasecmp("hot", resetType) == 0)
    {
        *reset = RX_RESET_HOT;
    }
    else if (strcasecmp("warm", resetType) == 0)
    {
        *reset = RX_RESET_WARM;
    }
    else if (strcasecmp("cold", resetType) == 0)
    {
        *reset = RX_RESET_COLD;
    }
    else if (strcasecmp("default", resetType) == 0)
    {
        *reset = RX_RESET_DEFAULT;
    }
    else if (strcasecmp("factory", resetType) == 0)
    {
        *reset = RX_RESET_FACTORY;
    }
    else if (strcasecmp("stop", resetType) == 0)
    {
        *reset = RX_RESET_GNSS_STOP;
    }
    else if (strcasecmp("start", resetType) == 0)
    {
        *reset = RX_RESET_GNSS_START;
    }
    else if (strcasecmp("gnss", resetType) == 0)
    {
        *reset = RX_RESET_GNSS_RESTART;
    }
    else
    {
        WARNING("Invalid argument '-r %s'!", resetType);
        return false;
    }
    return true;
}

int resetRun(const char *portArg, const char *resetArg)
{
    RX_RESET_t reset;
    if (!resetTypeFromStr(resetArg, &reset))
    {
        return EXIT_BADARGS;
    }

    RX_t *rx = rxInit(portArg, NULL);
    if ( (rx == NULL) || !rxOpen(rx) )
    {
        free(rx);
        return EXIT_RXFAIL;
    }

    const bool res = rxReset(rx, reset);

    rxClose(rx);
    free(rx);
    return res ? EXIT_SUCCESS : EXIT_RXFAIL;
}

/* ****************************************************************************************************************** */
// eof
