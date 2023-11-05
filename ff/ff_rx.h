// flipflip's u-blox positioning receiver control library
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

#ifndef __FF_RX_H__
#define __FF_RX_H__

#include <stdint.h>
#include <stdbool.h>

#include "ubloxcfg.h"
#include "ff_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */

//! Receiver handle
typedef struct RX_s RX_t;

//! Receiver detection
typedef enum RX_DET_e
{
    RX_DET_NONE = 0,  //!< No receiver detection, assume port settings are correct and receiver is present
    RX_DET_UBX,       //!< Actively check for a u-blox receiver
    RX_DET_PASSIVE,   //!< Passively check for any receiver (that outputs messages using a known protocol)
} RX_DET_t;

//! Receiver options
typedef struct RX_OPTS_s
{
    RX_DET_t detect;   //!< Receiver detection method
    bool     autobaud; //!< Automatically find baudrate (on ports that can change baudrate, and only for detect != RX_DET_NONE)
    int      baudrate; //!< (Initial) baudrate
    bool     verbose;  //!< Print what's going on for some operations
    char    *name;     //!< Name of the receiver (automatic if NULL)
    void   (*msgcb)(PARSER_MSG_t *, void *arg); //!< Optional callback for every message received
    void    *cbarg;    //!< Optional user argument for callback
} RX_OPTS_t;

#define RX_OPTS_DEFAULT() { .detect = RX_DET_UBX, .autobaud = true, .baudrate = 115200, .verbose = true, .name = NULL, .msgcb = NULL, .cbarg = NULL }

RX_t *rxInit(const char *port, const RX_OPTS_t *opts);

bool rxOpen(RX_t *rx);
void rxClose(RX_t *rx);

PARSER_MSG_t *rxGetNextMessage(RX_t *rx);
PARSER_MSG_t *rxGetNextMessageTimeout(RX_t *rx, const uint32_t timeout);

bool rxSend(RX_t *rx, const uint8_t *data, const int size);

bool rxAutobaud(RX_t *rx);
int rxGetBaudrate(RX_t *rx);
bool rxSetBaudrate(RX_t *rx, const int baudrate);

void rxAbort(RX_t *rx);

const PARSER_t *rxGetParser(RX_t *rx);

/* ****************************************************************************************************************** */

bool rxGetVerStr(RX_t *rx, char *str, const int size);

typedef struct RX_POLL_UBX_s
{
    uint8_t        clsId;
    uint8_t        msgId;
    const uint8_t *payload;
    int            payloadSize;
    uint32_t       timeout;
    int            retries;
    int            respSizeMin;
} RX_POLL_UBX_t;

PARSER_MSG_t *rxPollUbx(RX_t *rx, const RX_POLL_UBX_t *param, bool *pollNak);

bool rxSendUbxCfg(RX_t *rx, const uint8_t *msg, const int size, const uint32_t timeout);

typedef enum RX_RESET_e
{
    RX_RESET_NONE,          // No reset
    RX_RESET_SOFT,          // Controlled software reset
    RX_RESET_HARD,          // Controlled hardware reset
    RX_RESET_HOT,           // Hotstart (like u-center)
    RX_RESET_WARM,          // Warmstart (like u-center)
    RX_RESET_COLD,          // Coldstart (like u-center)
    RX_RESET_DEFAULT,       // Revert config to default, keep nav data
    RX_RESET_FACTORY,       // Revert config to default and coldstart
    RX_RESET_GNSS_STOP,     // Stop GNSS (stop navigation)
    RX_RESET_GNSS_START,    // Start GNSS (start navigation)
    RX_RESET_GNSS_RESTART,  // Restart GNSS (restart navigation)
    RX_RESET_SAFEBOOT,      // Safeboot mode
} RX_RESET_t;

bool rxReset(RX_t *rx, const RX_RESET_t reset);

const char *rxResetStr(const RX_RESET_t reset);

int rxGetConfig(RX_t *rx, const UBLOXCFG_LAYER_t layer, const uint32_t *keys, const int numKeys, UBLOXCFG_KEYVAL_t *kv, const int maxKv);

bool rxSetConfig(RX_t *rx, const UBLOXCFG_KEYVAL_t *kv, const int nKv, const bool ram, const bool bbr, const bool flash);

/* ****************************************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_RX_H__
