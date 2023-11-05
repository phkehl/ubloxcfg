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

#include "ubloxcfg.h"

#include "cfgtool_util.h"

#include "ff_rx.h"
#include "ff_ubx.h"

#include "cfgtool_rx2cfg.h"

/* ****************************************************************************************************************** */

const char *rx2cfgHelp(void)
{
    return
// -----------------------------------------------------------------------------
"Command 'rx2cfg':\n"
"\n"
"    Usage: cfgtool rx2cfg [-o <outfile>] [-y] -p <port> -l <layer> [-u] [-x]\n"
"\n"
"    This reads the configuration of a layer in the receiver and saves the data\n"
"    as a configuration file (see the 'cfg2rx' command for a specification of\n"
"    the format). Depending on the layer this produces all configuration\n"
"    (layers 'RAM' and 'Default') or only those that is available in the layer\n"
"    (layers 'BBR' and 'Flash'), which may be none at all.\n"
"\n"
"    The '-u' flag adds all unknown (to this tool) configuration items.\n"
"    The '-x' flag adds the item description as a comment.\n"
"\n"
"    Entries in the generated configuration file are commented out if their\n"
"    values are the default values.\n"
"\n";
}
const char *rx2listHelp(void)
{
    return
// -----------------------------------------------------------------------------
"Command 'rx2list':\n"
"\n"
"    Usage: cfgtool rx2list [-o <outfile>] [-y] -p <port> -l <layer> [-u] [-x]\n"
"\n"
"    This behaves like the 'rx2cfg' command with the differnece that all\n"
"    configuration is reported as <key> <value> pairs. That is, no <port> or\n"
"    <messagename> shortcuts are generated.\n"
"\n";
}

/* ****************************************************************************************************************** */


#define MAX_ITEMS 3000

typedef struct CFG_DB_REC_s
{
    UBLOXCFG_KEYVAL_t      kv;
    const UBLOXCFG_ITEM_t *item;
    bool                   flag;
} CFG_DB_REC_t;

typedef struct CFG_DB_s
{
    UBLOXCFG_LAYER_t layer;
    CFG_DB_REC_t     recs[MAX_ITEMS];
    int              nKv;
    int              nKvKnown;
    int              nKvUnknown;
} CFG_DB_t;

// Forward declarations
static CFG_DB_t *_getCfgDb(RX_t *rx, const UBLOXCFG_LAYER_t layer);
static const UBLOXCFG_KEYVAL_t *_dbFindKeyVal(const CFG_DB_t *db, const uint32_t id);
static void _dbFlag(CFG_DB_t *db, const uint32_t id);
static void _addOutputItemDesc(const UBLOXCFG_ITEM_t *item);

/* ****************************************************************************************************************** */

typedef struct PORT_CFG_s
{
    const char *name;
    uint32_t idBaudrate;
    uint32_t idInprotUbx;
    uint32_t idInprotNmea;
    uint32_t idInprotRtcm3x;
    uint32_t idOutprotUbx;
    uint32_t idOutprotNmea;
    uint32_t idOutprotRtcm3x;
} PORT_CFG_t;

static bool _portCfgStr(const PORT_CFG_t *cfg, const CFG_DB_t *db, char *str, const int size);
static bool _rateCfgStr(const UBLOXCFG_MSGRATE_t *cfg, const CFG_DB_t *db, char *str, const int size);
static bool _itemCfgStr(const UBLOXCFG_KEYVAL_t *kv, const UBLOXCFG_ITEM_t *item, char *str, const int size);

int rx2cfgRun(const char *portArg, const char *layerArg, const bool useUnknownItems, const bool extraInfo)
{
    // Check parameters
    UBLOXCFG_LAYER_t layer;
    if (!ubloxcfg_layerFromName(layerArg, &layer))
    {
        return EXIT_BADARGS;
    }

    RX_t *rx = rxInit(portArg, NULL);
    if ( (rx == NULL) || !rxOpen(rx) )
    {
        free(rx);
        return EXIT_RXFAIL;
    }

    // Get configuration
    CFG_DB_t *dbLayer = _getCfgDb(rx, layer);
    if (dbLayer == NULL)
    {
        rxClose(rx);
        free(rx);
        return EXIT_RXNODATA;
    }

    const char *layerName = ubloxcfg_layerName(layer);

    // Will we generate any useful output?
    const bool generateOutput = (useUnknownItems && (dbLayer->nKv > 0)) || (dbLayer->nKvKnown > 0);
    if (!generateOutput)
    {
        WARNING("No configuration available in layer %s!", layerName);
        free(dbLayer);
        rxClose(rx);
        free(rx);
        return EXIT_RXNODATA;
    }

    // Maybe get default configuration, too
    CFG_DB_t *dbDefault = layer == UBLOXCFG_LAYER_DEFAULT ? dbLayer : _getCfgDb(rx, UBLOXCFG_LAYER_DEFAULT);
    if (dbDefault == NULL)
    {
        free(dbLayer);
        rxClose(rx);
        free(rx);
        return EXIT_RXNODATA;
    }

    // Generate config file
    PRINT("Generating configuration file for layer %s", layerName);
    char verStr[100] = "unknown receiver";
    rxGetVerStr(rx, verStr, sizeof(verStr));
    ioOutputStr("# %s, layer %s\n", verStr, layerName);

    // Ports
    if (extraInfo)
    {
        ioOutputStr("# Port configuration: <port> <baudrate> <inprot> <outprot>\n");
    }
    const PORT_CFG_t portCfgs[] =
    {
        { "UART1", UBLOXCFG_CFG_UART1_BAUDRATE_ID,
          UBLOXCFG_CFG_UART1INPROT_UBX_ID, UBLOXCFG_CFG_UART1INPROT_NMEA_ID, UBLOXCFG_CFG_UART1INPROT_RTCM3X_ID,
          UBLOXCFG_CFG_UART1OUTPROT_UBX_ID, UBLOXCFG_CFG_UART1OUTPROT_NMEA_ID, UBLOXCFG_CFG_UART1OUTPROT_RTCM3X_ID },
        { "UART2", UBLOXCFG_CFG_UART2_BAUDRATE_ID,
          UBLOXCFG_CFG_UART2INPROT_UBX_ID, UBLOXCFG_CFG_UART2INPROT_NMEA_ID, UBLOXCFG_CFG_UART2INPROT_RTCM3X_ID,
          UBLOXCFG_CFG_UART2OUTPROT_UBX_ID, UBLOXCFG_CFG_UART2OUTPROT_NMEA_ID, UBLOXCFG_CFG_UART2OUTPROT_RTCM3X_ID },
        { "SPI", 0,
          UBLOXCFG_CFG_SPIINPROT_UBX_ID, UBLOXCFG_CFG_SPIINPROT_NMEA_ID, UBLOXCFG_CFG_SPIINPROT_RTCM3X_ID,
          UBLOXCFG_CFG_SPIOUTPROT_UBX_ID, UBLOXCFG_CFG_SPIOUTPROT_NMEA_ID, UBLOXCFG_CFG_SPIOUTPROT_RTCM3X_ID },
        { "I2C", 0,
          UBLOXCFG_CFG_I2CINPROT_UBX_ID, UBLOXCFG_CFG_I2CINPROT_NMEA_ID, UBLOXCFG_CFG_I2CINPROT_RTCM3X_ID,
          UBLOXCFG_CFG_I2COUTPROT_UBX_ID, UBLOXCFG_CFG_I2COUTPROT_NMEA_ID, UBLOXCFG_CFG_I2COUTPROT_RTCM3X_ID },
        { "USB", 0,
          UBLOXCFG_CFG_USBINPROT_UBX_ID, UBLOXCFG_CFG_USBINPROT_NMEA_ID, UBLOXCFG_CFG_USBINPROT_RTCM3X_ID,
          UBLOXCFG_CFG_USBOUTPROT_UBX_ID, UBLOXCFG_CFG_USBOUTPROT_NMEA_ID, UBLOXCFG_CFG_USBOUTPROT_RTCM3X_ID }
    };
    for (int ix = 0; ix < NUMOF(portCfgs); ix++)
    {
        char cfgStrLayer[100];
        const bool haveCfgStrLayer = _portCfgStr(&portCfgs[ix], dbLayer, cfgStrLayer, sizeof(cfgStrLayer));
        char cfgStrDefault[100];
        const bool haveCfgStrDefault = _portCfgStr(&portCfgs[ix], dbDefault, cfgStrDefault, sizeof(cfgStrDefault));

        // Port not available for this receiver
        if (!haveCfgStrDefault)
        {
            continue;
        }

        // Port not available in this layer
        if (!haveCfgStrLayer)
        {
            // Doing a default config file
            if (dbLayer == dbDefault)
            {
                const char *cfgStrNone = "       -  -                    -";
                ioOutputStr("#%-7s %-50s # default: %-50s\n", portCfgs[ix].name, cfgStrNone, cfgStrDefault);
            }
            continue;
        }

        // Port params for this layer, comment-out if they are the same as the default
        const bool cfgStrsAreSame = haveCfgStrLayer && haveCfgStrDefault && (strcmp(cfgStrDefault, cfgStrLayer) == 0);
        char portName[200];
        snprintf(portName, sizeof(portName), "%s%s", cfgStrsAreSame ? "#" : "", portCfgs[ix].name);
        ioOutputStr("%-8s %-50s # default: %s", portName, cfgStrLayer, cfgStrDefault);

        ioOutputStr("\n");

        // Flag all involved items
        _dbFlag(dbLayer, portCfgs[ix].idBaudrate);
        _dbFlag(dbLayer, portCfgs[ix].idInprotUbx);
        _dbFlag(dbLayer, portCfgs[ix].idInprotNmea);
        _dbFlag(dbLayer, portCfgs[ix].idInprotRtcm3x);
        _dbFlag(dbLayer, portCfgs[ix].idOutprotUbx);
        _dbFlag(dbLayer, portCfgs[ix].idOutprotNmea);
        _dbFlag(dbLayer, portCfgs[ix].idOutprotRtcm3x);
    }

    // Output message rates
    if (extraInfo)
    {
        ioOutputStr("# Output message rates: <messagename> <uart1> <uart2> <spi> <i2c> <usb>\n");
    }
    int numRateCfgs = 0;
    const UBLOXCFG_MSGRATE_t **rateCfgs = ubloxcfg_getAllMsgRateCfgs(&numRateCfgs);
    for (int ix = 0; ix < numRateCfgs; ix++)
    {
        char cfgStrLayer[100];
        const bool haveCfgStrLayer = _rateCfgStr(rateCfgs[ix], dbLayer, cfgStrLayer, sizeof(cfgStrLayer));
        char cfgStrDefault[100];
        const bool haveCfgStrDefault = _rateCfgStr(rateCfgs[ix], dbDefault, cfgStrDefault, sizeof(cfgStrDefault));

        // Message not available for this receiver
        if (!haveCfgStrDefault)
        {
            continue;
        }

        // Message not available in this layer and not doing a default config file
        if (!haveCfgStrLayer)
        {
            // Doing a default config file
            if (dbLayer == dbDefault)
            {
                const char *cfgStrNone = "  -   -   -   -   -";
                ioOutputStr("#%-24s %-33s # default: %s\n", rateCfgs[ix]->msgName, cfgStrNone, cfgStrDefault);
            }
            continue;
        }

        // Message params for this layer, comment-out if they are the same as the default
        const bool cfgStrsAreSame = haveCfgStrLayer && haveCfgStrDefault && (strcmp(cfgStrDefault, cfgStrLayer) == 0);
        char portName[200];
        snprintf(portName, sizeof(portName), "%s%s", cfgStrsAreSame ? "#" : "", rateCfgs[ix]->msgName);
        ioOutputStr("%-25s %-33s # default: %s", portName, cfgStrLayer, cfgStrDefault);

        ioOutputStr("\n");

        // Flag all involved items
        if (rateCfgs[ix]->itemUart1 != NULL)
        {
            _dbFlag(dbLayer, rateCfgs[ix]->itemUart1->id);
        }
        if (rateCfgs[ix]->itemUart2 != NULL)
        {
            _dbFlag(dbLayer, rateCfgs[ix]->itemUart2->id);
        }
        if (rateCfgs[ix]->itemSpi != NULL)
        {
            _dbFlag(dbLayer, rateCfgs[ix]->itemSpi->id);
        }
        if (rateCfgs[ix]->itemI2c != NULL)
        {
            _dbFlag(dbLayer, rateCfgs[ix]->itemI2c->id);
        }
        if (rateCfgs[ix]->itemUsb != NULL)
        {
            _dbFlag(dbLayer, rateCfgs[ix]->itemUsb->id);
        }
    }

    // All the remaining items
    for (int ix = 0; ix < dbLayer->nKv; ix++)
    {
        const UBLOXCFG_KEYVAL_t *kvLayer = &dbLayer->recs[ix].kv;
        const UBLOXCFG_ITEM_t *item = dbLayer->recs[ix].item; // NULL if item is unknown

        // Skip those used above, and unknown items unless we want to output them
        if ( dbLayer->recs[ix].flag || ((item == NULL) && !useUnknownItems) )
        {
            continue;
        }

        if (extraInfo && (item != NULL))
        {
            _addOutputItemDesc(item);
        }

        const UBLOXCFG_KEYVAL_t *kvDefault = _dbFindKeyVal(dbDefault, kvLayer->id);
        if (kvDefault == NULL)
        {
            continue;
        }

        char cfgStrLayer[UBLOXCFG_MAX_KEYVAL_STR_SIZE];
        const bool haveCfgStrLayer = _itemCfgStr(kvLayer, item, cfgStrLayer, sizeof(cfgStrLayer));
        char cfgStrDefault[UBLOXCFG_MAX_KEYVAL_STR_SIZE];
        const bool haveCfgStrDefault = _itemCfgStr(kvDefault, item, cfgStrDefault, sizeof(cfgStrDefault));
        if (!haveCfgStrLayer || !haveCfgStrDefault)
        {
            continue;
        }

        // Item layer, comment-out if the value isthe same as the default
        const bool cfgStrsAreSame = (strcmp(cfgStrDefault, cfgStrLayer) == 0);
        char itemName[UBLOXCFG_MAX_KEYVAL_STR_SIZE];
        if (item != NULL)
        {
            snprintf(itemName, sizeof(itemName), "%s%s", cfgStrsAreSame ? "#" : "", item->name);
        }
        else
        {
            snprintf(itemName, sizeof(itemName), "%s0x%08x", cfgStrsAreSame ? "#" : "", kvLayer->id);
        }
        char typeUnitScale[100];
        if (item == NULL)
        {
            strcpy(typeUnitScale, "unknown/undocumented item");
        }
        else
        {
            int len = snprintf(typeUnitScale, sizeof(typeUnitScale), "type %s", ubloxcfg_typeStr(item->type));
            if (item->scale != NULL)
            {
                /*len += */snprintf(&typeUnitScale[len], (int)sizeof(typeUnitScale) - len, ", %s", item->scale);
            }
            if (item->unit != NULL)
            {
                /*len += */snprintf(&typeUnitScale[len], (int)sizeof(typeUnitScale) - len, " [%s]", item->unit);
            }
        }

        ioOutputStr("%-35s %-23s # default: %-25s (%s)", itemName, cfgStrLayer, cfgStrDefault, typeUnitScale);

        ioOutputStr("\n");
    }

    // Clean up
    if (dbDefault != dbLayer)
    {
        free(dbDefault);
    }
    free(dbLayer);
    rxClose(rx);
    free(rx);

    // Write output, done
    if (generateOutput)
    {
        return ioWriteOutput(false) ? EXIT_SUCCESS : EXIT_OTHERFAIL;
    }
    else
    {
        return EXIT_SUCCESS;
    }
}

/* ****************************************************************************************************************** */

static void _addOutputKeyValuePair(const UBLOXCFG_KEYVAL_t *kv, const UBLOXCFG_ITEM_t *item, const UBLOXCFG_KEYVAL_t *defaultKv, const bool extraInfo);

int rx2listRun(const char *portArg, const char *layerArg, const bool useUnknownItems, const bool extraInfo)
{
    // Check parameters
    UBLOXCFG_LAYER_t layer;
    if (!ubloxcfg_layerFromName(layerArg, &layer))
    {
        return EXIT_BADARGS;
    }
    const char *layerName = ubloxcfg_layerName(layer);

    // Connect and detect receiver
    RX_t *rx = rxInit(portArg, NULL);
    if ( (rx == NULL) || !rxOpen(rx) )
    {
        free(rx);
        return EXIT_RXFAIL;
    }

    // Get configuration
    CFG_DB_t *dbLayer = _getCfgDb(rx, layer);
    if (dbLayer == NULL)
    {
        rxClose(rx);
        free(rx);
        return EXIT_RXNODATA;
    }

    // Will we generate any useful output?
    const bool generateOutput = (useUnknownItems && (dbLayer->nKv > 0)) || (dbLayer->nKvKnown > 0);
    if (!generateOutput)
    {
        WARNING("No configuration available in layer %s!", layerName);
        free(dbLayer);
        rxClose(rx);
        free(rx);
        return EXIT_RXNODATA;
    }

    // Maybe get default configuration, too
    CFG_DB_t *dbDefault = layer == UBLOXCFG_LAYER_DEFAULT ? dbLayer : _getCfgDb(rx, UBLOXCFG_LAYER_DEFAULT);
    if (dbDefault == NULL)
    {
        free(dbLayer);
        rxClose(rx);
        free(rx);
        return EXIT_RXNODATA;
    }

    // Generate list of key-value pairs
    PRINT("Generating list of key-value pairs for layer %s", layerName);
    char verStr[100] = "unknown receiver";
    rxGetVerStr(rx, verStr, sizeof(verStr));
    ioOutputStr("# %s, layer %s (%d/%d items)\n", verStr,
        layerName, useUnknownItems ? dbLayer->nKv : dbLayer->nKvKnown, dbLayer->nKv);
    for (int ix = 0; ix < dbLayer->nKv; ix++)
    {
        const UBLOXCFG_KEYVAL_t *kv = &dbLayer->recs[ix].kv;
        const UBLOXCFG_ITEM_t *item = dbLayer->recs[ix].item; // NULL if item is unknown

        // Skip unknown items unless we want to output them
        if ( (item == NULL) && !useUnknownItems )
        {
            continue;
        }

        const UBLOXCFG_KEYVAL_t *defaultKv = dbLayer != dbDefault ?
            _dbFindKeyVal(dbDefault, kv->id) : NULL;

        _addOutputKeyValuePair(kv, item, defaultKv, extraInfo);
    }

    // Clean up
    if (dbDefault != dbLayer)
    {
        free(dbDefault);
    }
    free(dbLayer);
    rxClose(rx);
    free(rx);

    // Write output, done
    if (generateOutput)
    {
        return ioWriteOutput(false) ? EXIT_SUCCESS : EXIT_OTHERFAIL;
    }
    else
    {
        return EXIT_SUCCESS;
    }
}

/* ****************************************************************************************************************** */

static int _dbSortFunc(const void *a, const void *b);

static CFG_DB_t *_getCfgDb(RX_t *rx, UBLOXCFG_LAYER_t layer)
{
    CFG_DB_t *db = malloc(sizeof(CFG_DB_t));
    UBLOXCFG_KEYVAL_t *kv = malloc(sizeof(UBLOXCFG_KEYVAL_t) * NUMOF(db->recs));
    if ( (db == NULL) || (kv == NULL) )
    {
        WARNING("_getCfgDb() malloc fail");
        free(db);
        free(kv);
        return NULL;
    }
    memset(db, 0, sizeof(*db));
    db->layer = layer;
    const char *layerName = ubloxcfg_layerName(layer);

    // Poll all configuration items
    PRINT("Polling receiver configuration for layer %s", layerName);


    const uint32_t keys[] = { UBX_CFG_VALGET_V0_ALL_WILDCARD };
    db->nKv = rxGetConfig(rx, layer, keys, NUMOF(keys), kv, NUMOF(db->recs));

    if (db->nKv < 0)
    {
        free(db);
        free(kv);
        return NULL;
    }

    // Check items, stringify and mark known ones
    for (int ix = 0; ix < db->nKv; ix++)
    {
        db->recs[ix].kv   = kv[ix];
        db->recs[ix].item = ubloxcfg_getItemById(kv[ix].id);
        if (db->recs[ix].item != NULL)
        {
            db->nKvKnown++;
        }
        else
        {
            db->nKvUnknown++;
        }
    }

    // Sort
    qsort(db->recs, db->nKv, sizeof(*db->recs), _dbSortFunc);

    PRINT("Layer %s: %d items (%d known, %d unknown)", layerName,
        db->nKv, db->nKvKnown, db->nKvUnknown);

    free(kv);
    return db;
}

static int _dbSortFunc(const void *a, const void *b)
{
    const CFG_DB_REC_t *rA = (const CFG_DB_REC_t *)a;
    const CFG_DB_REC_t *rB = (const CFG_DB_REC_t *)b;
    // Both items documented -> use documentation ordering
    if ( (rA->item != NULL) && (rB->item != NULL) )
    {
        return rA->item->order - rB->item->order;
    }
    // Keep documented before undocumented
    else if ( (rA->item != NULL) && (rB->item == NULL) )
    {
        return -1;
    }
    // Keep documented before undocumented
    else if ( (rA->item == NULL) && (rB->item != NULL) )
    {
        return 1;
    }
    // Sort undocumented by id
    else
    {
        const uint32_t iA = UBLOXCFG_ID2GROUP(rA->kv.id) | UBLOXCFG_ID2IDGRP(rA->kv.id);
        const uint32_t iB = UBLOXCFG_ID2GROUP(rB->kv.id) | UBLOXCFG_ID2IDGRP(rB->kv.id);
        return iA > iB ? 1 : -1;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

static const UBLOXCFG_KEYVAL_t *_dbFindKeyVal(const CFG_DB_t *db, const uint32_t id)
{
    const UBLOXCFG_KEYVAL_t *res = NULL;
    for (int ix = 0; ix < db->nKv; ix++)
    {
        if (db->recs[ix].kv.id == id)
        {
            res = &db->recs[ix].kv;
            break;
        }
    }
    return res;
}

static void _dbFlag(CFG_DB_t *db, const uint32_t id)
{
    for (int ix = 0; ix < db->nKv; ix++)
    {
        if (db->recs[ix].kv.id == id)
        {
            db->recs[ix].flag = true;
            break;
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _portCfgStr(const PORT_CFG_t *cfg, const CFG_DB_t *db, char *str, const int size)
{
    const UBLOXCFG_KEYVAL_t *kvBaudrate      = _dbFindKeyVal(db, cfg->idBaudrate);
    const UBLOXCFG_KEYVAL_t *kvInprotUbx     = _dbFindKeyVal(db, cfg->idInprotUbx);
    const UBLOXCFG_KEYVAL_t *kvInprotNmea    = _dbFindKeyVal(db, cfg->idInprotNmea);
    const UBLOXCFG_KEYVAL_t *kvInprotRtcm3x  = _dbFindKeyVal(db, cfg->idInprotRtcm3x);
    const UBLOXCFG_KEYVAL_t *kvOutprotUbx    = _dbFindKeyVal(db, cfg->idOutprotUbx);
    const UBLOXCFG_KEYVAL_t *kvOutprotNmea   = _dbFindKeyVal(db, cfg->idOutprotNmea);
    const UBLOXCFG_KEYVAL_t *kvOutprotRtcm3x = _dbFindKeyVal(db, cfg->idOutprotRtcm3x);
    if ( (kvBaudrate == NULL) && (kvInprotUbx == NULL) &&(kvInprotNmea == NULL) && (kvInprotRtcm3x == NULL) &&
         (kvOutprotUbx == NULL) && (kvOutprotNmea == NULL) && (kvOutprotRtcm3x == NULL) )
    {
        return false;
    }
    str[0] = '\0';
    if (kvBaudrate != NULL)
    {
        snprintf(&str[strlen(str)], size - strlen(str), "%8d", kvBaudrate->val.U4);
    }
    else
    {
        snprintf(&str[strlen(str)], size - strlen(str), "%8s", "-");
    }
    char protStr[50];
    protStr[0] = '\0';
    if ( (kvInprotUbx != NULL) && (kvInprotUbx->val.L) )
    {
        strcat(protStr, ",UBX");
    }
    if ((kvInprotNmea != NULL) && (kvInprotNmea->val.L) )
    {
        strcat(protStr, ",NMEA");
    }
    if ( (kvInprotRtcm3x != NULL) && (kvInprotRtcm3x->val.L) )
    {
        strcat(protStr, ",RTCM3");
    }
    snprintf(&str[strlen(str)], size - strlen(str), "  %-20s", strlen(protStr) > 0 ? &protStr[1] : "-");
    protStr[0] = '\0';
    if ( (kvInprotUbx != NULL) && (kvInprotUbx->val.L) )
    {
        strcat(protStr, ",UBX");
    }
    if ( (kvOutprotNmea != NULL) && (kvOutprotNmea->val.L) )
    {
        strcat(protStr, ",NMEA");
    }
    if ( (kvOutprotRtcm3x != NULL) && (kvOutprotRtcm3x->val.L) )
    {
        strcat(protStr, ",RTCM3");
    }
    snprintf(&str[strlen(str)], size - strlen(str), " %s", strlen(protStr) > 0 ? &protStr[1] : "-");
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _rateCfgStr(const UBLOXCFG_MSGRATE_t *cfg, const CFG_DB_t *db, char *str, const int size)
{
    const UBLOXCFG_KEYVAL_t *kvRateUart1 = (cfg->itemUart1 != NULL ? _dbFindKeyVal(db, cfg->itemUart1->id) : NULL);
    const UBLOXCFG_KEYVAL_t *kvRateUart2 = (cfg->itemUart2 != NULL ? _dbFindKeyVal(db, cfg->itemUart2->id) : NULL);
    const UBLOXCFG_KEYVAL_t *kvRateSpi   = (cfg->itemSpi   != NULL ? _dbFindKeyVal(db, cfg->itemSpi->id)   : NULL);
    const UBLOXCFG_KEYVAL_t *kvRateI2c   = (cfg->itemI2c   != NULL ? _dbFindKeyVal(db, cfg->itemI2c->id)   : NULL);
    const UBLOXCFG_KEYVAL_t *kvRateUsb   = (cfg->itemUsb   != NULL ? _dbFindKeyVal(db, cfg->itemUsb->id)   : NULL);
    if ( (kvRateUart1 == NULL) && (kvRateUart2 == NULL) && (kvRateSpi == NULL) && (kvRateI2c == NULL) && (kvRateUsb == NULL) )
    {
        return false;
    }
    str[0] = '\0';
    if (kvRateUart1 != NULL)
    {
        snprintf(&str[strlen(str)], size - strlen(str), "%3d", kvRateUart1->val.U1);
    }
    else
    {
        snprintf(&str[strlen(str)], size - strlen(str), "%3s", "-");
    }
    if (kvRateUart2 != NULL)
    {
        snprintf(&str[strlen(str)], size - strlen(str), " %3d", kvRateUart2->val.U1);
    }
    else
    {
        snprintf(&str[strlen(str)], size - strlen(str), " %3s", "-");
    }
    if (kvRateSpi != NULL)
    {
        snprintf(&str[strlen(str)], size - strlen(str), " %3d", kvRateSpi->val.U1);
    }
    else
    {
        snprintf(&str[strlen(str)], size - strlen(str), " %3s", "-");
    }
    if (kvRateI2c != NULL)
    {
        snprintf(&str[strlen(str)], size - strlen(str), " %3d", kvRateI2c->val.U1);
    }
    else
    {
        snprintf(&str[strlen(str)], size - strlen(str), " %3s", "-");
    }
    if (kvRateUsb != NULL)
    {
        snprintf(&str[strlen(str)], size - strlen(str), " %3d", kvRateUsb->val.U1);
    }
    else
    {
        snprintf(&str[strlen(str)], size - strlen(str), " %3s", "-");
    }

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _itemCfgStr(const UBLOXCFG_KEYVAL_t *kv, const UBLOXCFG_ITEM_t *item, char *str, const int size)
{
    UBLOXCFG_TYPE_t type = item != NULL ? item->type : UBLOXCFG_TYPE_X8;
    if (item == NULL)
    {
        const UBLOXCFG_SIZE_t valSize = UBLOXCFG_ID2SIZE(kv->id);
        switch (valSize)
        {
            case UBLOXCFG_SIZE_BIT:
                type = UBLOXCFG_TYPE_L;
                break;
            case UBLOXCFG_SIZE_ONE:
                type = UBLOXCFG_TYPE_X1;
                break;
            case UBLOXCFG_SIZE_TWO:
                type = UBLOXCFG_TYPE_X2;
                break;
            case UBLOXCFG_SIZE_FOUR:
                type = UBLOXCFG_TYPE_X4;
                break;
            case UBLOXCFG_SIZE_EIGHT:
                type = UBLOXCFG_TYPE_X8;
                break;
        }
    }
    if (!ubloxcfg_stringifyValue(str, size, type, item, &kv->val))
    {
        return false;
    }

    // Prefer pretty value
    char *valueStr;
    char *prettyStr;
    if (ubloxcfg_splitValueStr(str, &valueStr, &prettyStr) && (prettyStr != NULL))
    {
        strcpy(str, prettyStr);
    }

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static void _addOutputItemDesc(const UBLOXCFG_ITEM_t *item)
{
    ioOutputStr("# %s\n", item->title);
    for (int ix = 0; ix < item->nConsts; ix++)
    {
        char nameValueStr[100];
        snprintf(nameValueStr, sizeof(nameValueStr), "%s (%s)", item->consts[ix].name, item->consts[ix].value);

        ioOutputStr("# - %-20s %s\n", nameValueStr, item->consts[ix].title);
    }
}

static void _addOutputKeyValuePair(const UBLOXCFG_KEYVAL_t *kv, const UBLOXCFG_ITEM_t *item, const UBLOXCFG_KEYVAL_t *defaultKv, const bool extraInfo)
{
    UBLOXCFG_TYPE_t type = UBLOXCFG_TYPE_X8; // stringification format

    // Add CFG-FOO-BAR names for known items and use the correct type
    const UBLOXCFG_SIZE_t valSize = UBLOXCFG_ID2SIZE(kv->id);
    const char *sizeDesc = NULL;
    if (item != NULL)
    {
        if (extraInfo)
        {
            _addOutputItemDesc(item);
        }
        ioOutputStr("%-40s", item->name);
        type = item->type;
    }
    // Put ID instead of name for unknown items and map to X type based on size
    else
    {
        ioOutputStr("0x%08x                              ", kv->id);
        switch (valSize)
        {
            case UBLOXCFG_SIZE_BIT:
                type = UBLOXCFG_TYPE_L;
                sizeDesc = "1 bit";
                break;
            case UBLOXCFG_SIZE_ONE:
                type = UBLOXCFG_TYPE_X1;
                sizeDesc = "1 byte";
                break;
            case UBLOXCFG_SIZE_TWO:
                type = UBLOXCFG_TYPE_X2;
                sizeDesc = "2 bytes";
                break;
            case UBLOXCFG_SIZE_FOUR:
                type = UBLOXCFG_TYPE_X4;
                sizeDesc = "4 bytes";
                break;
            case UBLOXCFG_SIZE_EIGHT:
                type = UBLOXCFG_TYPE_X8;
                sizeDesc = "8 bytes";
                break;
        }
    }

    // Stringify value
    char valStr[UBLOXCFG_MAX_KEYVAL_STR_SIZE];
    char *valConstStr = NULL;
    if (ubloxcfg_stringifyValue(valStr, sizeof(valStr), type, item, &kv->val))
    {
        // Prefer pretty value
        char *dummy;
        ubloxcfg_splitValueStr(valStr, &dummy, &valConstStr);
        // We should now either have NULL or "FOO|BAR" style constant names
    }
    // Complain in case stringification unexpectedly failed
    else
    {
        WARNING("Failed to stringify value for item %s (0x%08x)!",
            item != NULL ? item->name : "CFG-?-?", kv->id);
    }

    // Value for unknown items
    if (item == NULL)
    {
        ioOutputStr("%-30s", valStr);
    }
    // Value for known items
    else
    {
        // If we have constant names, prefer these
        if (valConstStr != NULL)
        {
            ioOutputStr("%-30s", valConstStr);
        }
        // Otherwise the stringified value
        else
        {
            ioOutputStr("%-30s", valStr);
        }
    }

    // More info in comments...
    ioOutputStr(" # ");

    // ...for unknown items
    if (item == NULL)
    {
        ioOutputStr("unknown item, size %d (%s)", valSize, sizeDesc);
    }
    // ...for known items
    else
    {
        // Value string in case we put the constant names above
        if (valConstStr != NULL)
        {
            ioOutputStr("= %s, ", valStr);
        }
        // X8 is sometimes used to store text...
        else if (type == UBLOXCFG_TYPE_X8)
        {
            char str[9];
            int strLen = 0;
            for (int ix = 0; ix < 8; ix++)
            {
                if (kv->val._bytes[ix] == 0x00)
                {
                    break;
                }
                else if ( (kv->val._bytes[ix] > 0x1f) && (kv->val._bytes[ix] < 0x7f) ) // ASCII?
                {
                    str[strLen] = (char)kv->val._bytes[ix];
                    strLen++;
                }
            }
            if (strLen > 0)
            {
                str[strLen] = '\0';
                ioOutputStr("= \"%s\", ", str);
            }
        }

        // Add ID and type
        ioOutputStr("0x%08x, %s", kv->id, ubloxcfg_typeStr(type));

        if (extraInfo && (item != NULL))
        {
            if (item->scale != NULL)
            {
                ioOutputStr(", %s", item->scale);
            }
            if (item->unit != NULL)
            {
                ioOutputStr(" [%s]", item->unit);
            }
        }
    }

    // Add default value if available and different
    char defaultValStr[UBLOXCFG_MAX_KEYVAL_STR_SIZE];
    defaultValStr[0] = '\0';
    if ( (defaultKv != NULL) && (defaultKv->val._raw != kv->val._raw) )
    {
        if (ubloxcfg_stringifyValue(defaultValStr, sizeof(defaultValStr), type, item, &defaultKv->val))
        {

            ioOutputStr(", default: %s", defaultValStr);
        }
    }


    ioOutputStr("\n");
}

/* ****************************************************************************************************************** */
// eof
