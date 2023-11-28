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

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <inttypes.h>

#include "ubloxcfg.h"

#include "cfgtool_util.h"

#include "ff_rx.h"
#include "ff_ubx.h"
#include "ff_parser.h"

#include "cfgtool_reset.h"

#include "cfgtool_cfg2rx.h"

/* ****************************************************************************************************************** */

const char *cfg2rxHelp(void)
{
    return
// -----------------------------------------------------------------------------
"Command 'cfg2rx':\n"
"\n"
"    Usage: cfgtool cfg2rx [-i <infile>] -p <port> -l <layers> [-r <reset>] [-a] [-U] [-R]\n"
"\n"
"    This configures a receiver from a configuration file. The configuration is\n"
"    stored to one or more (comma-separated) of the following <layers>: 'RAM',\n"
"    'BBR' or 'Flash'. See the notes on using the RAM layer below.\n"
"\n"
"    Optionally the receiver can be reset to default configuration before storing\n"
"    the configuration. See the 'reset' command for details.\n"
"\n"
"    The -a switch soft resets the receiver after storing the configuration in\n"
"    order to activate the changed configuration. See the notes below and the\n"
"    'reset' command description.\n"
"\n"
"    The -U switch makes the command only update the receiver configuration if\n"
"    necessary. If the current configuration of the receiver already contains all\n"
"    the configuration from <infile>, no action is taken and the command finishes\n"
"    early. Additionally, with '-r factory', the items not covered by <infile>\n"
"    are checked against the default configuration.\n"
"\n"
"    A configuration file consists of one or more lines of configuration\n"
"    parameters. Leading and trailing whitespace, empty lines as well as comments\n"
"    (# style) are ignored. Acceptable separators for the tokens are (any number\n"
"    of) spaces and/or tabs. Each item should be listed only once. The '-R' flag\n"
"    can be used to allow listing items more than once. In this case the last\n"
"    occurrence of the item is used.\n"
"\n"
"    The following types of configuration can be used:\n"
"\n"
// -----------------------------------------------------------------------------
"        <key> <value>\n"
"            A key-value pair to set a particular configuration parameter (item)\n"
"            to a value. The key can be the item name (e.g. 'CFG-NAVSPG-FIXMODE')\n"
"            or its ID as hexadecimal number (e.g. '0x20110011').\n"
"            The acceptable strings for values depend on the item type: Type L\n"
"            values can be converted from: 'true', 'false' or any decimal,\n"
"            hexadecimal or octal string that translate to the value 1 or 0.\n"
"            Type U and X values can be converted from decimal, hexadecimal or\n"
"            octal strings. Type I and E values can be converted from decimal or\n"
"            hexadecimal strings. For known type E and X items converting\n"
"            constant names is attempted first. Type E constants are single\n"
"            words such as 'AUTO'. Type X constants are one or more words or\n"
"            hexadecimal strings separated by a '|', e.g. 'FIRST|0x02'.\n"
"\n"
"         <messagename> <uart1> <uart2> <spi> <i2c> <usb>\n"
"            A message name (e.g. 'UBX-NAV-PVT', 'NMEA-STANDARD-GGA', etc.) and\n"
"            output rates for the listed ports. For each port a value '0'...'255'\n"
"            or the '-' character must be given to set the given rate resp.\n"
"            to not configure the rate for that port.\n"
"            This is a shortcut to using the CFG-MSGOUT-... <key> <value> pairs.\n"
"\n"
"         <port> <baudrate> <inprot> <outprot>\n"
"            To configure a communication port its name ('UART1', 'UART2', 'SPI',\n"
"            'I2C' or 'USB'), its baudrate (for UARTs only, use '-' otherwise)\n"
"            and the input and output protocol filters must be specified. The\n"
"            protocol filters are space-separated words for the different\n"
"            protocols ('UBX', 'NMEA', 'RTCM3') or '-' to not configure the\n"
"            filter. Prepending a '!' to the protocol disables it.\n"
"            This is a shortcut to using some of the various <key> <value> pairs\n"
"            that configure the communication ports (e.g. CFG-UART1-..., etc.).\n"
"            See the note on changing baudrate below.\n"
"\n"
"    Example configuration file:\n"
"\n"
"        # Set some navigation parameters:\n"
"        CFG-NAVSPG-INIFIX3D        true    # Want initial fix to be a 3D one\n"
"        CFG-NAVSPG-FIXMODE         AUTO    # Use automatic fixmode\n"
"        CFG-NAVSPG-WKNROLLOVER     2099    # Set GPS weeknumber rollover value\n"
"        0x20110021                 6       # = CFG-NAVSPG-DYNMODEL AIR1\n"
"        CFG-NAVSPG-INFIL_CNOTHRS   0x23    # Require C/N0 >= 35\n"
"        # Configure UART1 baudrate, don't change in/out filters\n"
"        UART1 115200   -        -\n"
"        # Disable NMEA and RTCM3 output on USB and SPI\n"
"        USB            -        !NMEA,!RTCM3\n"
"        SPI            -        !NMEA,!RTCM3\n"
"        # Enable some UBX messages on UART1 and USB\n"
"        UBX-NAV-PVT        1 - - - 1\n"
"        UBX-NAV-SIG        1 - - - 1\n"
"        UBX-NAV-SAT        1 - - - 1\n"
"        UBX-MON-COMMS      5 - - - 5 # Every 5 seconds\n"
"        # Disable some NMEA messages on all ports\n"
"        NMEA-STANDARD-GGA  0 0 0 0 0\n"
"        NMEA-STANDARD-RMC  0 0 0 0 0\n"
"        NMEA-STANDARD-GSV  0 0 0 0 0\n"
"\n"
"    Example usage:\n"
"\n"
#ifdef _WIN32
"        cfgtool cfg2rx -p COM12 -r factory -l Flash -a -i some.cfg\n"
#else
"        cfgtool cfg2rx -p /dev/ttyUSB0 -r factory -l Flash -a -i some.cfg\n"
#endif
"\n"
"    Notes:\n"
"\n"
"        When storing configuration to the RAM layer (that is, the 'current\n"
"        configuration') changes become effective immediately. This is often\n"
"        not desired. The 'correct' way to configure the receiver is changing\n"
"        the configuration in the BBR and Flash layers and then resetting the\n"
"        receiver in order to activate the changed configuration.\n"
"\n"
"        This command likely fails when changing the baudrate of the port used\n"
"        for the connection in the RAM layer. This is because the receiver sends\n"
"        the success/failure result at the new baudrate. The command is therefore\n"
"        not be able to determine the result and fails. The correct way of doing\n"
"        this is outlined in the note above. A ugly and unreliable work-around is\n"
"        running the command twice.\n"
"\n";
}

/* ****************************************************************************************************************** */

#define WARNING_INFILE(fmt, args...) WARNING("%s:%d: " fmt, line->file, line->lineNr, ## args)
#define DEBUG_INFILE(fmt, args...)   DEBUG("%s:%d: " fmt, line->file, line->lineNr, ## args)
#define TRACE_INFILE(fmt, args...)   TRACE("%s:%d: " fmt, line->file, line->lineNr, ## args)
#define CFG_SET_MAX_MSGS 20
#define CFG_SET_MAX_KV   (UBX_CFG_VALSET_V1_MAX_KV * CFG_SET_MAX_MSGS)

/* ****************************************************************************************************************** */

int cfg2rxRun(const char *portArg, const char *layerArg, const char *resetArg, const bool applyConfig, const bool updateOnly, const bool allowReplace)
{
    bool ram = false;
    bool bbr = false;
    bool flash = false;
    if (!layersStringToFlags(layerArg, &ram, &bbr, &flash, NULL) || !(ram || bbr || flash))
    {
        return EXIT_BADARGS;
    }

    RX_RESET_t reset;
    if ((resetArg != NULL) && !resetTypeFromStr(resetArg, &reset))
    {
        return EXIT_BADARGS;
    }

    PRINT("Loading configuration");
    int nAllKvCfg = 0;
    UBLOXCFG_KEYVAL_t *allKvCfg = cfgToKeyVal(&nAllKvCfg, allowReplace);
    if (allKvCfg == NULL)
    {
        return EXIT_OTHERFAIL;
    }

    RX_t *rx = rxInit(portArg, NULL);
    if ( (rx == NULL) || !rxOpen(rx) )
    {
        free(allKvCfg);
        free(rx);
        return EXIT_RXFAIL;
    }

    // Check current config
    if (updateOnly)
    {
        bool configNeedsUpdate = false;
        // Get current and default config
        const uint32_t keys[] = { UBX_CFG_VALGET_V0_ALL_WILDCARD };
        UBLOXCFG_KEYVAL_t allKvRam[3000];
        const int nAllKvRam = rxGetConfig(rx, UBLOXCFG_LAYER_RAM, keys, NUMOF(keys), allKvRam, NUMOF(allKvRam));
        UBLOXCFG_KEYVAL_t allKvDef[NUMOF(allKvRam)];
        const int nAllKvDef = reset == RX_RESET_FACTORY ?
            rxGetConfig(rx, UBLOXCFG_LAYER_DEFAULT, keys, NUMOF(keys), allKvDef, NUMOF(allKvDef)) : 0;

        // Check current configuration
        for (int ixKvRam = 0; ixKvRam < nAllKvRam; ixKvRam++)
        {
            const UBLOXCFG_KEYVAL_t *kvRam = &allKvRam[ixKvRam];

            // Check all items from config file
            bool itemIsInCfg = false;
            for (int ixKvCfg = 0; ixKvCfg < nAllKvCfg; ixKvCfg++)
            {
                const UBLOXCFG_KEYVAL_t *kvCfg = &allKvCfg[ixKvCfg];
                if (kvRam->id == kvCfg->id)
                {
                    itemIsInCfg = true;
                    if (kvRam->val._raw != kvCfg->val._raw)
                    {
                        configNeedsUpdate = true;
                        char strCfg[UBLOXCFG_MAX_KEYVAL_STR_SIZE];
                        char strRam[UBLOXCFG_MAX_KEYVAL_STR_SIZE];
                        if ( ubloxcfg_stringifyKeyVal(strCfg, sizeof(strCfg), kvCfg) &&
                             ubloxcfg_stringifyKeyVal(strRam, sizeof(strRam), kvRam) )
                        {
                            DEBUG("Config (%s) differs from current config (%s)", strCfg, strRam);
                        }
                    }
                }
            }

            // Check if different from config, if factory reset requested, and unless item set by cfg file
            if (!itemIsInCfg && (nAllKvDef > 0))
            {
                for (int ixKvDef = 0; ixKvDef < nAllKvDef; ixKvDef++)
                {
                    const UBLOXCFG_KEYVAL_t *kvDef = &allKvDef[ixKvDef];
                    if (kvRam->id == kvDef->id)
                    {
                        if (kvRam->val._raw != kvDef->val._raw)
                        {
                            configNeedsUpdate = true;
                            char strDef[UBLOXCFG_MAX_KEYVAL_STR_SIZE];
                            char strRam[UBLOXCFG_MAX_KEYVAL_STR_SIZE];
                            if ( ubloxcfg_stringifyKeyVal(strDef, sizeof(strDef), kvDef) &&
                                 ubloxcfg_stringifyKeyVal(strRam, sizeof(strRam), kvRam) )
                            {
                                DEBUG("Default (%s) differs from current config (%s)", strDef, strRam);
                            }
                        }
                    }
                }
            }
        }

        // reset == RX_RESET_FACTORY
        if (configNeedsUpdate)
        {
            PRINT("Current configuration differs from requestd config.");
        }
        else
        {
            PRINT("Current configuration is up to date. Skipping configuring receiver.");
            free(allKvCfg);
            rxClose(rx);
            free(rx);
            return EXIT_SUCCESS;
        }
    }

    if ( (resetArg != NULL) && !rxReset(rx, reset))
    {
        free(allKvCfg);
        rxClose(rx);
        free(rx);
        return EXIT_RXFAIL;
    }

    bool res = rxSetConfig(rx, allKvCfg, nAllKvCfg, ram, bbr, flash);

    if (res && applyConfig)
    {
        PRINT("Applying configuration");
        if (!rxReset(rx, RX_RESET_SOFT))
        {
            free(allKvCfg);
            rxClose(rx);
            free(rx);
            return EXIT_RXFAIL;
        }
    }

    if (res)
    {
        PRINT("Receiver successfully configured");
    }

    rxClose(rx);
    free(rx);
    free(allKvCfg);
    return res ? EXIT_SUCCESS : EXIT_OTHERFAIL;
}

/* ****************************************************************************************************************** */

typedef struct CFG_DB_s
{
    UBLOXCFG_KEYVAL_t     *kv;
    int                    nKv;
    int                    maxKv;
} CFG_DB_t;

static bool _cfgDbAdd(CFG_DB_t *db, IO_LINE_t *line, const bool allow_replace);

UBLOXCFG_KEYVAL_t *cfgToKeyVal(int *nKv, const bool allow_replace)
{
    const int kvSize = CFG_SET_MAX_KV * sizeof(UBLOXCFG_KEYVAL_t);
    UBLOXCFG_KEYVAL_t *kv = malloc(kvSize);
    if (kv == NULL)
    {
        WARNING("malloc fail");
        return NULL;
    }
    memset(kv, 0, kvSize);

    CFG_DB_t db = { .kv = kv, .nKv = 0, .maxKv = CFG_SET_MAX_KV };
    bool res = true;
    while (res)
    {
        IO_LINE_t *line = ioGetNextInputLine();
        if (line == NULL)
        {
            break;
        }
        if (!_cfgDbAdd(&db, line, allow_replace))
        {
            res = false;
            break;
        }
    }

    if (db.nKv <= 0)
    {
        WARNING("No configuration found in input file!");
        free(kv);
        return NULL;
    }

    if (!res)
    {
        WARNING("Failed reading config file!");
        free(kv);
        return NULL;
    }

    *nKv = db.nKv;
    return kv;
}

// ---------------------------------------------------------------------------------------------------------------------

typedef struct MSGRATE_CFG_s
{
    const char            *name;
    const char            *rate;
    const UBLOXCFG_ITEM_t *item;
} MSGRATE_CFG_t;

typedef struct BAUD_CFG_s
{
    const char    *str;
    const uint32_t val;
} BAUD_CFG_t;

typedef struct PROTFILT_CFG_s
{
    const char     *name;
    const uint32_t  id;
} PROTFILT_CFG_t;

typedef struct PORT_CFG_s
{
    const char    *name;
    uint32_t       baudrateId;
    PROTFILT_CFG_t inProt[3];
    PROTFILT_CFG_t outProt[3];
} PORT_CFG_t;

// separator for fields
static const char * const kCfgTokSep = " \t";
// separator for parts inside fields
static const char * const kCfgPartSep = ",";

static bool _cfgDbAddKeyVal(CFG_DB_t *db, IO_LINE_t *line, const uint32_t id, const UBLOXCFG_VALUE_t *value, const bool allow_replace);
static bool _cfgDbApplyProtfilt(CFG_DB_t *db, IO_LINE_t *line, char *protfilt, const PROTFILT_CFG_t *protfiltCfg, const int nProtfiltCfg, const bool allow_replace);

static bool _cfgDbAdd(CFG_DB_t *db, IO_LINE_t *line, const bool allow_replace)
{
    TRACE_INFILE("%s", line->line);
    // Named key-value pair
    if (strncmp(line->line, "CFG-", 4) == 0)
    {
        // Expect exactly two tokens separated by whitespace
        char *keyStr = strtok(line->line, kCfgTokSep);
        char *valStr = strtok(NULL, kCfgTokSep);
        char *none = strtok(NULL, kCfgTokSep);
        TRACE_INFILE("- key-val: keyStr=[%s] valStr=[%s]", keyStr, valStr);
        if ( (keyStr == NULL) || (valStr == NULL) || (none != NULL) )
        {
            WARNING_INFILE("Expected key-value pair!");
            return false;
        }

        // Get item
        const UBLOXCFG_ITEM_t *item = ubloxcfg_getItemByName(keyStr);
        if (item == NULL)
        {
            WARNING_INFILE("Unknown item '%s'!", keyStr);
            return false;
        }

        // Get value
        UBLOXCFG_VALUE_t value;
        if (!ubloxcfg_valueFromString(valStr, item->type, item, &value))
        {
            WARNING_INFILE("Could not parse value '%s' for item '%s' (type %s)!",
                valStr, item->name, ubloxcfg_typeStr(item->type));
            return false;
        }

        // Add key-value pari to the list
        if (!_cfgDbAddKeyVal(db, line, item->id, &value, allow_replace))
        {
            return false;
        }
    }
    // Hex key-value pair
    else if ( (line->line[0] == '0') && (line->line[1] == 'x') )
    {
        // Expect exactly two tokens separated by whitespace
        char *keyStr = strtok(line->line, kCfgTokSep);
        char *valStr = strtok(NULL, kCfgTokSep);
        char *none = strtok(NULL, kCfgTokSep);
        TRACE_INFILE("- hexid-val: keyStr=[%s] valStr=[%s]", keyStr, valStr);
        if ( (keyStr == NULL) || (valStr == NULL) || (none != NULL) )
        {
            WARNING_INFILE("Expected hex key-value pair!");
            return false;
        }

        uint32_t id = 0;
        int numChar = 0;
        if ( (sscanf(keyStr, "%"SCNx32"%n", &id, &numChar) != 1) || (numChar != (int)strlen(keyStr)) )
        {
            WARNING_INFILE("Bad hex item ID (%s)!", keyStr);
            return false;
        }

        UBLOXCFG_VALUE_t value = { ._raw = 0 };
        bool valueOk = false;
        const UBLOXCFG_SIZE_t size = UBLOXCFG_ID2SIZE(id);
        switch (size)
        {
            case UBLOXCFG_SIZE_BIT:
                if (ubloxcfg_valueFromString(valStr, UBLOXCFG_TYPE_L, NULL, &value))      // L
                {
                    valueOk = true;
                }
                break;
            case UBLOXCFG_SIZE_ONE:
                if (ubloxcfg_valueFromString(valStr, UBLOXCFG_TYPE_U1, NULL, &value) ||   // U1, X1
                    ubloxcfg_valueFromString(valStr, UBLOXCFG_TYPE_I1, NULL, &value))     // I1, E1
                {
                    valueOk = true;
                }
                break;
            case UBLOXCFG_SIZE_TWO:
                if (ubloxcfg_valueFromString(valStr, UBLOXCFG_TYPE_U2, NULL, &value) ||   // U2, X2
                    ubloxcfg_valueFromString(valStr, UBLOXCFG_TYPE_I2, NULL, &value))     // I2, E2
                {
                    valueOk = true;
                }
                break;
            case UBLOXCFG_SIZE_FOUR:
                if (ubloxcfg_valueFromString(valStr, UBLOXCFG_TYPE_U4, NULL, &value) ||   // U4, X4
                    ubloxcfg_valueFromString(valStr, UBLOXCFG_TYPE_I4, NULL, &value) ||   // I4, E4
                    ubloxcfg_valueFromString(valStr, UBLOXCFG_TYPE_R4, NULL, &value))     // R4
                {
                    valueOk = true;
                }
                break;
            case UBLOXCFG_SIZE_EIGHT:
                if (ubloxcfg_valueFromString(valStr, UBLOXCFG_TYPE_U8, NULL, &value) ||   // U8, X8
                    ubloxcfg_valueFromString(valStr, UBLOXCFG_TYPE_I8, NULL, &value) ||   // I8, E8
                    ubloxcfg_valueFromString(valStr, UBLOXCFG_TYPE_R8, NULL, &value))     // R8
                {
                    valueOk = true;
                }
                break;
            default:
                WARNING_INFILE("Bad size from item ID (%s)!", keyStr);
                return false;
        }
        if (!valueOk)
        {
            WARNING_INFILE("Bad value '%s' for item '%s'!", valStr, keyStr);
            return false;
        }

        // Add key-value pari to the list
        if (!_cfgDbAddKeyVal(db, line, id, &value, allow_replace))
        {
            return false;
        }
    }
    // Output message rate config
    else if ( (strncmp(line->line, "UBX-",  4) == 0) ||
              (strncmp(line->line, "NMEA-", 5) == 0) ||
              (strncmp(line->line, "RTCM-", 5) == 0) )
    {
        // <msgname> <uart1> <uart2> <spi> <i2c> <usb>
        char *name  = strtok(line->line, kCfgTokSep);
        char *uart1 = strtok(NULL, kCfgTokSep);
        char *uart2 = strtok(NULL, kCfgTokSep);
        char *spi   = strtok(NULL, kCfgTokSep);
        char *i2c   = strtok(NULL, kCfgTokSep);
        char *usb   = strtok(NULL, kCfgTokSep);
        TRACE_INFILE("- msgrate: name=[%s] uart1=[%s] uart2=[%s] spi=[%s] i2c=[%s] usb=[%s]", name, uart1, uart2, spi, i2c, usb);
        if ( (name == NULL) || (uart1 == NULL) || (uart2 == NULL) || (spi == NULL) || (i2c == NULL) || (usb == NULL) )
        {
            WARNING_INFILE("Expected output message rate config!");
            return false;
        }

        // Get config items for this message
        const UBLOXCFG_MSGRATE_t *items = ubloxcfg_getMsgRateCfg(name);
        if (items == NULL)
        {
            WARNING_INFILE("Unknown message name (%s)!", name);
            return false;
        }

        // Generate config key-value pairs...
        MSGRATE_CFG_t msgrateCfg[] =
        {
            { .name = "UART1", .rate = uart1, .item = items->itemUart1 },
            { .name = "UART2", .rate = uart2, .item = items->itemUart2 },
            { .name = "SPI",   .rate = spi,   .item = items->itemSpi },
            { .name = "I2C",   .rate = i2c,   .item = items->itemI2c },
            { .name = "USB",   .rate = usb,   .item = items->itemUsb }
        };
        for (int ix = 0; ix < NUMOF(msgrateCfg); ix++)
        {
            // "-" = don't configure, skip
            if ( (msgrateCfg[ix].rate[0] == '-') && (msgrateCfg[ix].rate[1] == '\0') )
            {
                continue;
            }

            // Can configure?
            if (msgrateCfg[ix].item == NULL)
            {
                WARNING_INFILE("No configuration available for %s output rate on part %s!", name, msgrateCfg[ix].name);
                return false;
            }

            // Get and check value
            UBLOXCFG_VALUE_t value;
            if (!ubloxcfg_valueFromString(msgrateCfg[ix].rate, msgrateCfg[ix].item->type, msgrateCfg[ix].item, &value))
            {
                WARNING_INFILE("Bad output message rate value (%s) for port %s!", msgrateCfg[ix].rate, msgrateCfg[ix].name);
                return false;
            }

            // Add key-value pair to the list
            if (!_cfgDbAddKeyVal(db, line, msgrateCfg[ix].item->id, &value, allow_replace))
            {
                return false;
            }
        }
    }
    // Port configuration
    else if ( (strncmp(line->line, "UART1 ", 5) == 0) ||
              (strncmp(line->line, "UART2 ", 5) == 0) ||
              (strncmp(line->line, "SPI ",   4) == 0) ||
              (strncmp(line->line, "I2C ",   4) == 0) ||
              (strncmp(line->line, "USB ",   4) == 0) )
    {
        char *port     = strtok(line->line, kCfgTokSep);
        char *baudrate = strtok(NULL, kCfgTokSep);
        char *inprot   = strtok(NULL, kCfgTokSep);
        char *outprot  = strtok(NULL, kCfgTokSep);
        TRACE_INFILE("- portcfg: port=[%s] baud=[%s] inport=[%s] outprot=[%s]", port, baudrate, inprot, outprot);
        if ( (port == NULL) || (baudrate == NULL) || (inprot == NULL) || (outprot == NULL) )
        {
            WARNING_INFILE("Expected port config!");
            return false;
        }

        // Acceptable baudrates (for UART)
        BAUD_CFG_t baudCfg[] =
        {
            { .str =   "9600", .val =   9600 },
            { .str =  "19200", .val =  19200 },
            { .str =  "38400", .val =  38400 },
            { .str =  "57600", .val =  57600 },
            { .str = "115200", .val = 115200 },
            { .str = "230400", .val = 230400 },
            { .str = "460800", .val = 460800 },
            { .str = "921600", .val = 921600 }
        };
        // Configurable ports and the corresponding configuration
        PORT_CFG_t portCfg[] =
        {
            {
              .name = "UART1", .baudrateId = UBLOXCFG_CFG_UART1_BAUDRATE_ID,
              .inProt  = { { .name = "UBX",    .id = UBLOXCFG_CFG_UART1INPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_UART1INPROT_NMEA_ID },
                           { .name = "RTCM3",  .id = UBLOXCFG_CFG_UART1INPROT_RTCM3X_ID } },
              .outProt = { { .name = "UBX",    .id = UBLOXCFG_CFG_UART1OUTPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_UART1OUTPROT_NMEA_ID },
                           { .name = "RTCM3",  .id = UBLOXCFG_CFG_UART1OUTPROT_RTCM3X_ID } }
            },
            {
              .name = "UART2", .baudrateId = UBLOXCFG_CFG_UART2_BAUDRATE_ID,
              .inProt  = { { .name = "UBX",    .id = UBLOXCFG_CFG_UART2INPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_UART2INPROT_NMEA_ID },
                           { .name = "RTCM3",  .id = UBLOXCFG_CFG_UART2INPROT_RTCM3X_ID } },
              .outProt = { { .name = "UBX",    .id = UBLOXCFG_CFG_UART2OUTPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_UART2OUTPROT_NMEA_ID },
                           { .name = "RTCM3",  .id = UBLOXCFG_CFG_UART2OUTPROT_RTCM3X_ID } }
            },
            {
              .name = "SPI", .baudrateId = 0,
              .inProt  = { { .name = "UBX",    .id = UBLOXCFG_CFG_SPIINPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_SPIINPROT_NMEA_ID },
                           { .name = "RTCM3",  .id = UBLOXCFG_CFG_SPIINPROT_RTCM3X_ID } },
              .outProt = { { .name = "UBX",    .id = UBLOXCFG_CFG_SPIOUTPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_SPIOUTPROT_NMEA_ID },
                           { .name = "RTCM3",  .id = UBLOXCFG_CFG_SPIOUTPROT_RTCM3X_ID } }
            },
            {
              .name = "I2C", .baudrateId = 0,
              .inProt  = { { .name = "UBX",    .id = UBLOXCFG_CFG_I2CINPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_I2CINPROT_NMEA_ID },
                           { .name = "RTCM3",  .id = UBLOXCFG_CFG_I2CINPROT_RTCM3X_ID } },
              .outProt = { { .name = "UBX",    .id = UBLOXCFG_CFG_I2COUTPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_I2COUTPROT_NMEA_ID },
                           { .name = "RTCM3",  .id = UBLOXCFG_CFG_I2COUTPROT_RTCM3X_ID } }
            },
            {
              .name = "USB", .baudrateId = 0,
              .inProt  = { { .name = "UBX",    .id = UBLOXCFG_CFG_USBINPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_USBINPROT_NMEA_ID },
                           { .name = "RTCM3",  .id = UBLOXCFG_CFG_USBINPROT_RTCM3X_ID } },
              .outProt = { { .name = "UBX",    .id = UBLOXCFG_CFG_USBOUTPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_USBOUTPROT_NMEA_ID },
                           { .name = "RTCM3",  .id = UBLOXCFG_CFG_USBOUTPROT_RTCM3X_ID } }
            },
        };
        // Find port config info
        PORT_CFG_t *cfg = NULL;
        for (int ix = 0; ix < NUMOF(portCfg); ix++)
        {
            if (strcmp(port, portCfg[ix].name) == 0)
            {
                cfg = &portCfg[ix];
                break;
            }
        }
        if (cfg == NULL)
        {
            WARNING_INFILE("Cannot configure port '%s'!", port);
            return false;
        }

        // Config baudrate
        if ( (cfg->baudrateId != 0) && (baudrate[0] != '-') && (baudrate[1] != '\0') ) // number or "-" for UART1, 2
        {
            bool baudrateOk = false;
            for (int ix = 0; ix < NUMOF(baudCfg); ix++)
            {
                // Add key-value pair to the list
                if (strcmp(baudCfg[ix].str, baudrate) == 0)
                {
                    UBLOXCFG_VALUE_t value = { .U4 = baudCfg[ix].val };
                    if (!_cfgDbAddKeyVal(db, line, cfg->baudrateId, &value, allow_replace))
                    {
                        return false;
                    }
                    baudrateOk = true;
                }
            }
            if (!baudrateOk)
            {
                WARNING_INFILE("Illegal baudrate value '%s'!", baudrate);
                return false;
            }
        }
        else if ( (baudrate[0] != '-') && (baudrate[1] != '\0') ) // other ports have no baudrate, so only "-" is acceptable
        {
            WARNING_INFILE("Baudrate value specified for port '%s'!", port);
            return false;
        }

        // Input/output protocol filters
        if ( (inprot[0] != '-') && (inprot[1] != '\0') )
        {
            if (!_cfgDbApplyProtfilt(db, line, inprot, cfg->inProt, NUMOF(cfg->inProt), allow_replace))
            {
                return false;
            }
        }
        if ( (outprot[0] != '-') && (outprot[1] != '\0') )
        {
            if (!_cfgDbApplyProtfilt(db, line, outprot, cfg->outProt, NUMOF(cfg->outProt), allow_replace))
            {
                return false;
            }
        }
    }
    else
    {
        WARNING_INFILE("Unknown config (%s)!", line->line);
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _cfgDbAddKeyVal(CFG_DB_t *db, IO_LINE_t *line, const uint32_t id, const UBLOXCFG_VALUE_t *value, const bool allow_replace)
{
    if (db->nKv >= db->maxKv)
    {
        WARNING_INFILE("Too many items!");
        return false;
    }

    // Check if item is already present in the list of items to configure
    int replace_ix = -1;
    for (int ix = 0; ix < db->maxKv; ix++)
    {
        if (db->kv[ix].id == id)
        {
            // Replace previously added item
            if (allow_replace)
            {
                replace_ix = ix;
                break;
            }
            // Complain about duplicate item
            else
            {
                const UBLOXCFG_ITEM_t *item = ubloxcfg_getItemById(id);
                if (item != NULL)
                {
                    WARNING_INFILE("Duplicate item '%s'!", item->name);
                }
                else
                {
                    WARNING_INFILE("Duplicate item!");
                }
                return false;
            }
        }
    }

    // Add/replace item
    const bool do_replace = (replace_ix >= 0);
    const int kv_ix = (do_replace ? replace_ix : db->nKv);
    db->kv[kv_ix].id = id;
    db->kv[kv_ix].val = *value;
    if (isDEBUG())
    {
        char debugStr[UBLOXCFG_MAX_KEYVAL_STR_SIZE];
        if (ubloxcfg_stringifyKeyVal(debugStr, sizeof(debugStr), &db->kv[kv_ix]))
        {
            DEBUG_INFILE("%s item %d: %s", do_replace ? "Replacing" : "Adding", kv_ix + 1, debugStr);
        }
    }
    if (!do_replace)
    {
        db->nKv++;
    }

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _cfgDbApplyProtfilt(CFG_DB_t *db, IO_LINE_t *line, char *protfilt, const PROTFILT_CFG_t *protfiltCfg, const int nProtfiltCfg, const bool allow_replace)
{
    char *pCfgFilt = strtok(protfilt, kCfgPartSep);
    while (pCfgFilt != NULL)
    {
        const bool protEna = pCfgFilt[0] != '!';
        if (pCfgFilt[0] == '!')
        {
            pCfgFilt++;
        }
        bool found = false;
        for (int ix = 0; ix < nProtfiltCfg; ix++)
        {
            if (strcmp(protfiltCfg[ix].name, pCfgFilt) == 0)
            {
                UBLOXCFG_VALUE_t value = { .L = protEna };
                if (!_cfgDbAddKeyVal(db, line, protfiltCfg[ix].id, &value, allow_replace))
                {
                    return false;
                }
                found = true;
                break;
            }
        }
        if (!found)
        {
            WARNING_INFILE("Illegal protocol filter '%s'!", pCfgFilt);
            return false;
        }
        pCfgFilt = strtok(NULL, kCfgPartSep);
    }
    return true;
}

/* ****************************************************************************************************************** */
// eof
