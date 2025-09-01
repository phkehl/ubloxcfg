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

#include <string.h>
#include <stddef.h>
#include <signal.h>

#include "ubloxcfg.h"

#include "cfgtool_util.h"

#include "cfgtool_cfginfo.h"

/* ****************************************************************************************************************** */

const char *cfginfoHelp(void)
{
    return
// -----------------------------------------------------------------------------
"Command 'cfginfo':\n"
"\n"
"    Usage: cfgtool cfginfo [-o <outfile>] [-y]\n"
"\n"
"    This generates a list with information on all configuration items and\n"
"    messages known to this tool.\n"
"\n";
}

/* ****************************************************************************************************************** */

int cfginfoRun(void)
{
    int numItems = 0;
    const UBLOXCFG_ITEM_t **items = ubloxcfg_getAllItems(&numItems);
    ioOutputStr("#     Item/constant                          Type  Scale            Unit         Description\n");
    ioOutputStr("#---------------------------------------------------------------------------------------------------------------------\n");
    for (int ix = 0; ix < numItems; ix++)
    {
        const UBLOXCFG_ITEM_t *it = items[ix];
        ioOutputStr("item %-40s %-2s  %-15s  %-10s # %s\n",
            it->name,
            ubloxcfg_typeStr(it->type),
            it->scale != NULL ? it->scale : "-",
            it->unit  != NULL ? it->unit  : "-",
            it->title);
        for (int ix2 = 0; ix2 < it->nConsts; ix2++)
        {
            const UBLOXCFG_CONST_t *co = &it->consts[ix2];
            ioOutputStr("const    %-20s  %-18s                             #    %s\n",
                co->name,
                co->value,
                co->title);
        }
    }

    ioOutputStr("\n");
    ioOutputStr("#       Message                         UART1                                UART2                                SPI                                  I2C                                  USB\n");
    ioOutputStr("#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    int numRates = 0;
    const UBLOXCFG_MSGRATE_t **rates = ubloxcfg_getAllMsgRateCfgs(&numRates);
    for (int ix = 0; ix < numRates; ix++)
    {
        const UBLOXCFG_MSGRATE_t *ra = rates[ix];
        ioOutputStr("message %-30s  %-35s  %-35s  %-35s  %-35s  %-35s\n",
            ra->msgName,
            ra->itemUart1 != NULL ? ra->itemUart1->name : "-",
            ra->itemUart2 != NULL ? ra->itemUart2->name : "-",
            ra->itemSpi   != NULL ? ra->itemSpi->name   : "-",
            ra->itemI2c   != NULL ? ra->itemI2c->name   : "-",
            ra->itemUsb   != NULL ? ra->itemUsb->name   : "-");
    }

    return ioWriteOutput(false) ? EXIT_SUCCESS : EXIT_OTHERFAIL;
}

/* ****************************************************************************************************************** */
// eof
