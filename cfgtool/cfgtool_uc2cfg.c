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
//#include <stdlib.h>
//#include <inttypes.h>

#include "ubloxcfg.h"

#include "cfgtool_util.h"

#include "ff_rx.h"
#include "ff_ubx.h"

#include "cfgtool_uc2cfg.h"

/* ****************************************************************************************************************** */

#define MAX_KEY 50
#define MAX_VAL 25
#define NUM_KVSTRS 200

const char *uc2cfgHelp(void)
{
    return
// -----------------------------------------------------------------------------
"Command 'uc2cfg':\n"
"\n"
"    Usage: cfgtool uc2cfg [-i <infile>] [-o <outfile>] [-y]\n"
"\n"
"    This converts a configuration file from u-center's 'Generation 9 Advanced\n"
"    Configuration' tool and converts it into the format cfgtool understands.\n"
"    It reads all key-value pairs in the [set] section of the input file. The\n"
"    layer tokens are ignored. Duplicate key-value pairs are silently ignored\n"
"    unless the values are inconsistent. No formal checks on the key names or\n"
"    values are performed and the data is passed-through as-is. Empty lines,\n"
"    comments as well as sections other than the [set] section are ignored.\n"
"    A maximum of "STRINGIFY(NUM_KVSTRS)" key-value pairs are processed. Key and value strings must\n"
"    not exceed "STRINGIFY(MAX_KEY)" respectively "STRINGIFY(MAX_VAL)" characters.\n"
"\n";
}

/* ****************************************************************************************************************** */

#define WARNING_INFILE(fmt, args...) WARNING("%s:%d: " fmt, line->file, line->lineNr, ## args)
#define DEBUG_INFILE(fmt, args...)   DEBUG("%s:%d: " fmt, line->file, line->lineNr, ## args)
#define TRACE_INFILE(fmt, args...)   TRACE("%s:%d: " fmt, line->file, line->lineNr, ## args)

/* ****************************************************************************************************************** */

typedef struct KV_STRS_s
{
    char key[MAX_KEY];
    char val[MAX_VAL];
} KVSTRS_t;

int uc2cfgRun(void)
{
    const int kvStrsSize = NUM_KVSTRS * sizeof(KVSTRS_t);
    KVSTRS_t *kvStrs = malloc(kvStrsSize);
    if (kvStrs == NULL)
    {
        WARNING("malloc fail!");
        return EXIT_FAILURE;
    }
    memset(kvStrs, 0, kvStrsSize);
    int nKvStrs = 0;

    bool inSet = false;
    int errors = 0;
    while (true)
    {
        IO_LINE_t *line = ioGetNextInputLine();
        if (line == NULL)
        {
            break;
        }
        if (strcasecmp(line->line, "[set]") == 0)
        {
            inSet = true;
        }
        else if (line->line[0] == '[')
        {
            inSet = false;
        }
        else if (inSet)
        {
            const char *sep = " \t";
            char *layer = strtok(line->line, sep);
            char *key = strtok(NULL, sep);
            char *val = strtok(NULL, sep);
            if ( (layer == NULL) || (key == NULL) || (val == NULL) || (strncmp(key, "CFG-", 4) != 0) ||
                 ((strcasecmp(layer, "RAM") != 0) && (strcasecmp(layer, "BBR") != 0) && (strcasecmp(layer, "Flash") != 0)) )
            {
                WARNING_INFILE("Bad data!");
            }
            else if (strlen(key) > (MAX_KEY - 1))
            {
                WARNING_INFILE("Too long key!");
                errors++;
            }
            else if (strlen(val) > (MAX_VAL - 1))
            {
                WARNING_INFILE("Too long value!");
                errors++;
            }
            else
            {
                //DEBUG_INFILE("layer=[%s] key=[%s] val=[%s]", layer, key, val);
                bool kvOk = true;
                for (int ix = 0; ix < nKvStrs; ix++)
                {
                    if (strcmp(kvStrs[ix].key, key) == 0)
                    {
                        if (strcmp(kvStrs[ix].val, val) == 0)
                        {
                            DEBUG_INFILE("Ignoring duplicate key-value pair (%s %s)!", key, val);
                        }
                        else
                        {
                            WARNING_INFILE("Inconsistent key-value pair (%s %s, previously %s)!",
                                key, val, kvStrs[ix].val);
                            errors++;
                        }
                        kvOk = false;
                    }
                }
                if (kvOk)
                {
                    if (nKvStrs >= NUM_KVSTRS)
                    {
                        WARNING_INFILE("Too many key-value pairs!");
                        errors++;
                        break;
                    }
                    strcat(kvStrs[nKvStrs].key, key);
                    strcat(kvStrs[nKvStrs].val, val);
                    nKvStrs++;
                }
            }
        }
    }

    bool res = true;
    if (errors > 0)
    {
        WARNING("Bad input!");
        res = false;
    }
    else
    {
        ioOutputStr("# uc2cfg\n");
        for (int ix = 0; ix < nKvStrs; ix++)
        {
            ioOutputStr("%-30s %s\n", kvStrs[ix].key, kvStrs[ix].val);
        }
        res = ioWriteOutput(false);

    }

    free(kvStrs);
    return res ? EXIT_SUCCESS : EXIT_OTHERFAIL;
}

/* ****************************************************************************************************************** */


// ---------------------------------------------------------------------------------------------------------------------

/* ****************************************************************************************************************** */
// eof
