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
#include <stdlib.h>
#include <inttypes.h>

#include "ubloxcfg.h"

#include "cfgtool_util.h"

#include "ff_rx.h"
#include "ff_ubx.h"
#include "ff_parser.h"

#include "cfgtool_reset.h"

#include "cfgtool_cfg2rx.h"

/* ********************************************************************************************** */

const char *cfg2rxHelp(void)
{
    return
// -----------------------------------------------------------------------------
"Command 'cfg2rx':\n"
"\n"
"    Usage: cfgtool cfg2rx [-i <infile>] -p <port> -l <layers> [-r <reset>] [-a]\n"
"\n"
"    This configures a receiver from a configuration file. The configuration is\n"
"    stored to one or more (comma-separated) of the following <layers>: 'RAM',\n"
"    'BBR' or 'Flash'. See the notes on using the RAM layer below.\n"
"\n"
"    Optionally the receiver can be reset to default configuration before storing\n"
"    the configuration. See the 'reset' command for details.\n"
"\n"
"    The -a switch hotstarts the receiver after storing the configuration in\n"
"    order to activate the changed configuration. See the notes below.\n"
"\n"
"    A configuration file consists of one or more lines of configuration\n"
"    parameters. Leading and trailing whitespace, empty lines as well as comments\n"
"    (# style) are ignored. Acceptable separators for the tokens are (any number\n"
"    of) spaces and/or tabs.\n"
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
"         <messagename>> <uart1> <uart2> <spi> <i2c> <usb>\n"
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
"            protocols ('UBX', 'NMEA', 'RTCM3X') or '-' to not configure the\n"
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
"        # Disable NMEA and RTCM3X output on USB and SPI\n"
"        USB            -        !NMEA,!RTCM3X\n"
"        SPI            -        !NMEA,!RTCM3X\n"
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

/* ********************************************************************************************** */

#define WARNING_INFILE(fmt, args...) WARNING("%s:%d: " fmt, line->file, line->lineNr, ## args)
#define DEBUG_INFILE(fmt, args...)   DEBUG("%s:%d: " fmt, line->file, line->lineNr, ## args)
#define TRACE_INFILE(fmt, args...)   TRACE("%s:%d: " fmt, line->file, line->lineNr, ## args)
#define CFG_SET_MAX_MSGS 20
#define CFG_SET_MAX_KV   (UBX_CFG_VALSET_V1_MAX_KV * CFG_SET_MAX_MSGS)

/* ********************************************************************************************** */

int cfg2rxRun(const char *portArg, const char *layerArg, const char *resetArg, const bool applyConfig)
{
    const uint8_t layers = ubxCfgValsetLayer(layerArg);
    if (layers == 0x00)
    {
        return EXIT_BADARGS;
    }

    RX_RESET_t reset;
    if ((resetArg != NULL) && !resetTypeFromStr(resetArg, &reset))
    {
        return EXIT_BADARGS;
    }

    PRINT("Loading configuration");
    int nKv = 0;
    UBLOXCFG_KEYVAL_t *kv = cfgToKeyVal(&nKv);
    if (kv == NULL)
    {
        return EXIT_OTHERFAIL;
    }

    PRINT("Converting %d items into UBX-CFG-VALSET messages", nKv);
    int nMsgs = 0;
    VALSET_MSG_t *msgs = keyValToUbxCfgValset(kv, nKv, layers, &nMsgs);
    if (msgs == NULL)
    {
        free(kv);
        return EXIT_OTHERFAIL;
    }

    RX_t *rx = rxOpen(portArg, NULL);
    if (rx == NULL)
    {
        return EXIT_RXFAIL;
    }

    if ( (resetArg != NULL) && !rxReset(rx, reset))
    {
        rxClose(rx);
        return EXIT_RXFAIL;
    }

    PRINT("Sending %d UBX-CFG-VALSET messages to the receiver", nMsgs);
    bool res = true;
    for (int ix = 0; ix < nMsgs; ix++)
    {
        PRINT("Sending UBX-CFG-VALSET %d/%d (%d items, %s)",
            ix + 1, nMsgs, msgs[ix].nKv, msgs[ix].info);
        if (!rxSendUbxCfg(rx, msgs[ix].msg, msgs[ix].size, 2500))
        {
            WARNING("Failed configuring receiver!");
            res = false;
            // TODO: send empty VALSET to abort transaction?
            break;
        }
    }

    if (res && applyConfig)
    {
        PRINT("Applying configuration");
        if (!rxReset(rx, RX_RESET_HOT))
        {
            rxClose(rx);
            return EXIT_RXFAIL;
        }
    }

    if (res)
    {
        PRINT("Receiver successfully configured");
    }

    //rxClose(rx);
    free(kv);
    free(msgs);
    return res ? EXIT_SUCCESS : EXIT_OTHERFAIL;
}

/* ********************************************************************************************** */

typedef struct CFG_DB_s
{
    UBLOXCFG_KEYVAL_t     *kv;
    int                    nKv;
    int                    maxKv;
} CFG_DB_t;

static bool _cfgDbAdd(CFG_DB_t *db, IO_LINE_t *line);

UBLOXCFG_KEYVAL_t *cfgToKeyVal(int *nKv)
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
        if (!_cfgDbAdd(&db, line))
        {
            res = false;
            break;
        }
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

// -------------------------------------------------------------------------------------------------

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

static bool _cfgDbAddKeyVal(CFG_DB_t *db, IO_LINE_t *line, const uint32_t id, const UBLOXCFG_VALUE_t *value);
static bool _cfgDbApplyProtfilt(CFG_DB_t *db, IO_LINE_t *line, char *protfilt, const PROTFILT_CFG_t *protfiltCfg, const int nProtfiltCfg);

static bool _cfgDbAdd(CFG_DB_t *db, IO_LINE_t *line)
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
        const UBLOXCFG_ITEM_t *item = ubloxcfg_getItemByName(keyStr);;
        if (item == NULL)
        {
            WARNING_INFILE("Unknown item '%s'!", keyStr);
            return false;
        }

        // Get value
        UBLOXCFG_VALUE_t value;
        if (!ubloxcfg_valueFromString(valStr, item->type, item, &value))
        {
            WARNING_INFILE("Could not parse value '%s' for item '%s'!", valStr, item->name);
            return false;
        }

        // Add key-value pari to the list
        if (!_cfgDbAddKeyVal(db, line, item->id, &value))
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
            WARNING_INFILE("Bad value '%s' for item %s!", valStr, keyStr);
            return false;
        }

        // Add key-value pari to the list
        if (!_cfgDbAddKeyVal(db, line, id, &value))
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
            if (!_cfgDbAddKeyVal(db, line, msgrateCfg[ix].item->id, &value))
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
                           { .name = "RTCM3X", .id = UBLOXCFG_CFG_UART1INPROT_RTCM3X_ID } },
              .outProt = { { .name = "UBX",    .id = UBLOXCFG_CFG_UART1OUTPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_UART1OUTPROT_NMEA_ID },
                           { .name = "RTCM3X", .id = UBLOXCFG_CFG_UART1OUTPROT_RTCM3X_ID } }
            },
            {
              .name = "UART2", .baudrateId = UBLOXCFG_CFG_UART2_BAUDRATE_ID,
              .inProt  = { { .name = "UBX",    .id = UBLOXCFG_CFG_UART2INPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_UART2INPROT_NMEA_ID },
                           { .name = "RTCM3X", .id = UBLOXCFG_CFG_UART2INPROT_RTCM3X_ID } },
              .outProt = { { .name = "UBX",    .id = UBLOXCFG_CFG_UART2OUTPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_UART2OUTPROT_NMEA_ID },
                           { .name = "RTCM3X", .id = UBLOXCFG_CFG_UART2OUTPROT_RTCM3X_ID } }
            },
            {
              .name = "SPI", .baudrateId = 0,
              .inProt  = { { .name = "UBX",    .id = UBLOXCFG_CFG_SPIINPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_SPIINPROT_NMEA_ID },
                           { .name = "RTCM3X", .id = UBLOXCFG_CFG_SPIINPROT_RTCM3X_ID } },
              .outProt = { { .name = "UBX",    .id = UBLOXCFG_CFG_SPIOUTPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_SPIOUTPROT_NMEA_ID },
                           { .name = "RTCM3X", .id = UBLOXCFG_CFG_SPIOUTPROT_RTCM3X_ID } }
            },
            {
              .name = "I2C", .baudrateId = 0,
              .inProt  = { { .name = "UBX",    .id = UBLOXCFG_CFG_I2CINPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_I2CINPROT_NMEA_ID },
                           { .name = "RTCM3X", .id = UBLOXCFG_CFG_I2CINPROT_RTCM3X_ID } },
              .outProt = { { .name = "UBX",    .id = UBLOXCFG_CFG_I2COUTPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_I2COUTPROT_NMEA_ID },
                           { .name = "RTCM3X", .id = UBLOXCFG_CFG_I2COUTPROT_RTCM3X_ID } }
            },
            {
              .name = "USB", .baudrateId = 0,
              .inProt  = { { .name = "UBX",    .id = UBLOXCFG_CFG_USBINPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_USBINPROT_NMEA_ID },
                           { .name = "RTCM3X", .id = UBLOXCFG_CFG_USBINPROT_RTCM3X_ID } },
              .outProt = { { .name = "UBX",    .id = UBLOXCFG_CFG_USBOUTPROT_UBX_ID },
                           { .name = "NMEA",   .id = UBLOXCFG_CFG_USBOUTPROT_NMEA_ID },
                           { .name = "RTCM3X", .id = UBLOXCFG_CFG_USBOUTPROT_RTCM3X_ID } }
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
                    if (!_cfgDbAddKeyVal(db, line, cfg->baudrateId, &value))
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
            if (!_cfgDbApplyProtfilt(db, line, inprot, cfg->inProt, NUMOF(cfg->inProt)))
            {
                return false;
            }
        }
        if ( (outprot[0] != '-') && (outprot[1] != '\0') )
        {
            if (!_cfgDbApplyProtfilt(db, line, outprot, cfg->outProt, NUMOF(cfg->outProt)))
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

// -------------------------------------------------------------------------------------------------

static bool _cfgDbAddKeyVal(CFG_DB_t *db, IO_LINE_t *line, const uint32_t id, const UBLOXCFG_VALUE_t *value)
{
    if (db->nKv >= db->maxKv)
    {
        WARNING_INFILE("Too many items!");
        return false;
    }

    for (int ix = 0; ix < db->maxKv; ix++)
    {
        if (db->kv[ix].id == id)
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

    db->kv[db->nKv].id = id;
    db->kv[db->nKv].val = *value;
    if (isDEBUG())
    {
        char debugStr[UBLOXCFG_MAX_KEYVAL_STR_SIZE];
        if (ubloxcfg_stringifyKeyVal(debugStr, sizeof(debugStr), &db->kv[db->nKv]))
        {
            DEBUG_INFILE("Adding item %d: %s", db->nKv + 1, debugStr);
        }
    }
    db->nKv++;

    return true;
}

// -------------------------------------------------------------------------------------------------

static bool _cfgDbApplyProtfilt(CFG_DB_t *db, IO_LINE_t *line, char *protfilt, const PROTFILT_CFG_t *protfiltCfg, const int nProtfiltCfg)
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
                if (!_cfgDbAddKeyVal(db, line, protfiltCfg[ix].id, &value))
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

// -------------------------------------------------------------------------------------------------

uint8_t ubxCfgValsetLayer(const char *layerStr)
{
    char tmp[200];
    if (snprintf(tmp, sizeof(tmp), "%s", layerStr) >= (int)sizeof(tmp))
    {
        return 0;
    }

    uint8_t layer = 0;
    bool res = true;
    char *tok = strtok(tmp, ",");
    while (tok != NULL)
    {
        if ( (strcasecmp("RAM", tok) == 0) && ((layer & UBX_CFG_VALSET_V1_LAYER_RAM) == 0) )
        {
            layer |= UBX_CFG_VALSET_V1_LAYER_RAM;
        }
        else if ( (strcasecmp("BBR", tok) == 0) && ((layer & UBX_CFG_VALSET_V1_LAYER_BBR) == 0) )
        {
            layer |= UBX_CFG_VALSET_V1_LAYER_BBR;
        }
        else if ( (strcasecmp("Flash", tok) == 0) && ((layer & UBX_CFG_VALSET_V1_LAYER_FLASH) == 0) )
        {
            layer |= UBX_CFG_VALSET_V1_LAYER_FLASH;
        }
        else
        {
            WARNING("Illegal layer '%s' in '%s'!", tok, layerStr);
            res = false;
            break;
        }
        tok = strtok(NULL, ",");
    }
    
    if (!res)
    {
        return 0;
    }

    return layer;
}

/* ********************************************************************************************** */

// It seems that when using transactions, the last message (with UBX-CFG-VALSET.transaction = 3, i.e. end/commit)
// must not contain any key-value pairs or those key-value pairs in the last message are ignored and not applied.
#define NEED_EMPTY_TRANSACTION_END

VALSET_MSG_t *keyValToUbxCfgValset(UBLOXCFG_KEYVAL_t *kv, const int nKv, const uint8_t layers, int *nValset)
{
    if (nValset == NULL)
    {
        return NULL;
    }
    const int msgsSize = (CFG_SET_MAX_MSGS + 1) * sizeof(VALSET_MSG_t);
    VALSET_MSG_t *msgs = malloc(msgsSize);
    if (msgs == NULL)
    {
        WARNING("keyValToUbx() malloc fail");
        return NULL;
    }
    memset(msgs, 0, msgsSize);

    // Create as many UBX-CFG-VALSET as required
    bool res = true;
    const int nMsgs = (nKv / (UBX_CFG_VALSET_V1_MAX_KV)) + ((nKv % UBX_CFG_VALSET_V1_MAX_KV) != 0 ? 1 : 0);
    for (int msgIx = 0; msgIx < nMsgs; msgIx++)
    {
        const int kvOffs = msgIx * UBX_CFG_VALSET_V1_MAX_KV;
        const int remKv = nKv - kvOffs;
        const int nKvThisMsg = remKv > UBX_CFG_VALSET_V1_MAX_KV ? UBX_CFG_VALSET_V1_MAX_KV : remKv;
        //DEBUG("msgIx=%d nMsgs=%d nKv=%d kvOffs=%d remKv=%d nKvThisMsg=%d", msgIx, nMsgs, nKv, kvOffs, remKv, nKvThisMsg);

        // Message header
        uint8_t transaction = UBX_CFG_VALSET_V1_TRANSACTION_NONE;
        const char *transactionStr = "no transaction";
        if (nMsgs > 1)
        {
            if (msgIx == 0)
            {
                transaction = UBX_CFG_VALSET_V1_TRANSACTION_BEGIN;
                transactionStr = "transaction begin";
            }
            else
#ifndef NEED_EMPTY_TRANSACTION_END
                  if (msgIx < (nMsgs - 1))
#endif
            {
                transaction = UBX_CFG_VALSET_V1_TRANSACTION_CONTINUE;
                transactionStr = "transaction continue";
            }
#ifndef NEED_EMPTY_TRANSACTION_END
            else
            {
                transaction = UBX_CFG_VALSET_V1_TRANSACTION_END;
                transactionStr = "transaction end";
            }
#endif
        }
        DEBUG("Creating UBX-CFG-VALSET %d items (%d..%d/%d, %s)",
            nKvThisMsg, kvOffs + 1, kvOffs + nKvThisMsg, nKv, transactionStr);

        uint8_t *pUbxData = msgs[msgIx].msg;
        snprintf(msgs[msgIx].info, sizeof(msgs[msgIx].info), "%s", transactionStr);
        msgs[msgIx].nKv = nKvThisMsg;

        // Add payload head
        const UBX_CFG_VALSET_V1_GROUP0_t payloadHead =
        {
            .version     = UBX_CFG_VALSET_V1_VERSION,
            .layers      = layers,
            .transaction = transaction,
            .reserved    = UBX_CFG_VALSET_V1_RESERVED
        };
        const int payloadHeadSize = sizeof(payloadHead);
        memcpy(pUbxData, &payloadHead, payloadHeadSize);

        // Add config data
        int cfgDataSize = 0;
        if (!ubloxcfg_makeData(&pUbxData[payloadHeadSize], UBX_CFG_VALGET_V1_MAX_SIZE,
                &kv[kvOffs], nKvThisMsg, &cfgDataSize))
        {
            WARNING("UBX-CFG-VALSET.cfgData encode fail!");
            res = false;
            break;
        }

        // Make UBX message
        const int msgSize = ubxMakeMessage(UBX_CFG_CLSID, UBX_CFG_VALSET_MSGID,
            pUbxData, payloadHeadSize + cfgDataSize, pUbxData);

        msgs[msgIx].size = msgSize;
    }

    if (!res)
    {
        free(msgs);
        return NULL;
    }

#ifdef NEED_EMPTY_TRANSACTION_END
    // Add empty transaction-complete message
    if (nMsgs > 1)
    {
        const UBX_CFG_VALSET_V1_GROUP0_t payload =
        {
            .version     = UBX_CFG_VALSET_V1_VERSION,
            .layers      = layers,
            .transaction = UBX_CFG_VALSET_V1_TRANSACTION_END,
            .reserved    = UBX_CFG_VALSET_V1_RESERVED
        };
        snprintf(msgs[nMsgs].info, sizeof(msgs[nMsgs].info), "transaction end");
        msgs[nMsgs].nKv = 0;
        msgs[nMsgs].size = ubxMakeMessage(UBX_CFG_CLSID, UBX_CFG_VALSET_MSGID,
            (const uint8_t *)&payload, sizeof(payload), msgs[nMsgs].msg);
        *nValset = nMsgs + 1;
    }
    else
#endif
    {
        *nValset = nMsgs;
    }
    return msgs;
}

/* ********************************************************************************************** */
// eof
