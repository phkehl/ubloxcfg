/* ************************************************************************************************/  // clang-format off
// Quectel positioning receivers configuration tool

/* ************************************************************************************************/


#include "cfgtool_cmd2rx.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cfgtool_util.h"
#include "ff_nmea.h"
#include "ff_rx.h"

/* ****************************************************************************************************************** */

const char* cmd2rxHelp(void) {
    return
// -----------------------------------------------------------------------------
"Command 'cmd2rx':\n"
"\n"
"    Usage: cfgtool cmd2rx [-n] -i <infile> -p <port>\n"
"\n"
"    This sends commands to a receiver and optionally waits for an expected\n"
"    response with a timeout. This is useful to configure non-u-blox receivers\n"
"    or other devices.\n"
"\n"
"    The command <infile> consists of one or more lines of commands to process.\n"
"    Leading and trailing whitespace, empty lines as well as comments (# style)\n"
"    are ignored. Acceptable separators for the tokens are (any number of) spaces\n"
"    and/or tabs. Each line in the <infile> has one of the following formats:\n"
"\n"
"        COMMAND <cmd> <timeout> <rsp>\n"
"        SLEEP <timeout>\n"
"        DETECT\n"
"        MESSAGE <timeout> <name>\n"
"\n"
"    Lines starting with 'COMMAND' specify a command to be sent to the receiver,\n"
"    where <cmd> and <rsp> specify command to send to the receiver and the\n"
"    response to expect. The <timeout> is in milliseconds and specifies the\n"
"    maximum time to wait for the expected response. Both <cmd> and <rsp> are\n"
"    in the form of <type>:<data>. The available <type>s are:\n"
"\n"
"        NMEA: NMEA sentence with <data> being the payload (framing is added\n"
"              automatically. For example: 'NMEA:PQTMCFGFIXRATE,W,200'\n"
"        HEX:  Arbitrary data with <data> being a hexdump of it. For example:\n"
"              'HEX:48656c6c6f20576f726c6421'\n"
"        NONE: No-op command, nothing is sent resp. expected. For example: 'NONE'\n"
"              If <cmd> is NONE nothing is sent to the receiver. If the <rsp> is\n"
"              NONE the <timeout> is used like in SLEEP\n"
"\n"
"    Lines starting with 'SLEEP' specify a pause of <timeout> milliseconds.\n"
"\n"
"    Lines starting with 'DETECT' specify a re-detection (autobauding) of the\n"
"    receiver, for example after configuring the baudrate\n"
"\n"
"    Lines starting with 'MESSAGE' specify waiting up to <timeout> milliseconds\n"
"    for any message matching the given <name>. For example: 'MESSAGE:NMEA-G' to\n"
"    wait for any NMEA positioning message. Or 'MESSAGE:RTCM3-TYPE1234' for\n"
"    waiting for a RTCM3 type 1234 message.\n"
"\n"
"    Example command file:\n"
"\n"
"        # Nav rate 5 Hz\n"
"        COMMAND  NMEA:PQTMCFGFIXRATE,W,200 1000  NMEA:PQTMCFGFIXRATE,OK\n"
"        # Save parameters\n"
"        COMMAND  NMEA:PQTMSAVEPAR          1000  NMEA:PQTMSAVEPAR,OK\n"
"        # Stop GNSS\n"
"        COMMAND  NMEA:PQTMGNSSSTOP         2000  NMEA:PQTMGNSSSTOP,OK\n"
"        # (Re-)start GNSS\n"
"        COMMAND  NMEA:PQTMGNSSSTART        2000  NMEA:PQTMGNSSSTART,OK\n"
"\n";
}

/* ****************************************************************************************************************** */

typedef enum CMD_TYPE_e
{
    CMD_TYPE_UNSPECIFIED = 0,
    CMD_TYPE_COMMAND,
    CMD_TYPE_SLEEP,
    CMD_TYPE_DETECT,
    CMD_TYPE_MESSAGE,
} CMD_TYPE_t;

typedef enum MSG_TYPE_e
{
    MSG_TYPE_UNSPECIFIED = 0,
    MSG_TYPE_NONE,
    MSG_TYPE_NMEA,
    MSG_TYPE_HEX,
} MSG_TYPE_t;

typedef struct MSG_s {
    uint8_t     data[PARSER_MAX_ANY_SIZE];  //!< Data to be sent
    int         size;                       //!< Size of data (may be 0!)
} MSG_t;

typedef struct CMD_RSP_s {
    CMD_TYPE_t type;       //!< Command type
    MSG_t      cmd;        //!< Message to be sent
    uint32_t   timeout;    //!< Timeout for response in ms
    MSG_t      rsp;        //!< Expected response
    char       name[100];  //!< Expected message name
    char       info[120];  //!< Progress info output info
} CMD_RSP_t;


typedef struct CMD_RSP_DB_s {
    CMD_RSP_t cr[100];
    int n;
} CMD_RSP_DB_t;

static const char* _cmdTypeStr(const CMD_TYPE_t type)
{
    switch (type) {
        case CMD_TYPE_UNSPECIFIED:  return "UNSPECIFIED";
        case CMD_TYPE_COMMAND:      return "COMMAND";
        case CMD_TYPE_SLEEP:        return "SLEEP";
        case CMD_TYPE_DETECT:       return "DETECT";
        case CMD_TYPE_MESSAGE:      return "MESSAGE";
    }
    return "?";
}

static CMD_TYPE_t _strCmdType(const char *str)
{
    if      (strcmp(str, "COMMAND")  == 0) { return CMD_TYPE_COMMAND; }
    else if (strcmp(str, "SLEEP")    == 0) { return CMD_TYPE_SLEEP; }
    else if (strcmp(str, "DETECT")   == 0) { return CMD_TYPE_DETECT; }
    else if (strcmp(str, "MESSAGE")  == 0) { return CMD_TYPE_MESSAGE; }
    return CMD_TYPE_UNSPECIFIED;
}

static MSG_TYPE_t _strMsgType(const char *str)
{
    if      (strcmp(str, "NONE") == 0) { return MSG_TYPE_NONE; }
    else if (strcmp(str, "NMEA") == 0) { return MSG_TYPE_NMEA; }
    else if (strcmp(str, "HEX")  == 0) { return MSG_TYPE_HEX; }
    return MSG_TYPE_UNSPECIFIED;
}

/* ****************************************************************************************************************** */

#define WARNING_INFILE(fmt, args...) WARNING("%s:%d: " fmt, line->file, line->lineNr, ##args)
#define DEBUG_INFILE(fmt, args...)   DEBUG("%s:%d: " fmt, line->file, line->lineNr, ##args)
#define TRACE_INFILE(fmt, args...)   TRACE("%s:%d: " fmt, line->file, line->lineNr, ##args)

// ---------------------------------------------------------------------------------------------------------------------

// separator for tokens
static const char* const kCfgTokSep = " \t";
// separator between type and message
static const char* const kMsgTypeSep = ":";

// ---------------------------------------------------------------------------------------------------------------------

static bool _dbAdd(CMD_RSP_DB_t* db, IO_LINE_t* line);

static bool _loadDb(CMD_RSP_DB_t *db) {
    memset(db, 0, sizeof(*db));

    bool res = true;
    while (res) {
        IO_LINE_t* line = ioGetNextInputLine();
        if (line == NULL) {
            break;
        }
        if (!_dbAdd(db, line)) {
            res = false;
            break;
        }
    }

    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _strToMsg(MSG_t *msg, char *str, const char **errorHint);

static bool _dbAdd(CMD_RSP_DB_t* db, IO_LINE_t* line)
{
    TRACE_INFILE("%s", line->line);

    if (db->n >= NUMOF(db->cr)) {
        WARNING_INFILE("Too many commands");
        return false;
    }

    // The DB entry to populate
    CMD_RSP_t *cr = &db->cr[db->n];


    // First token is COMMAND, SLEEP or DETECT
    bool entryOk = false;
    const char *entryStr = strtok(line->line, kCfgTokSep);
    cr->type = _strCmdType(entryStr);
    switch (cr->type)
    {
        case CMD_TYPE_DETECT: {
            const char *none = strtok(NULL, kCfgTokSep);
            if (none != NULL) {
                WARNING_INFILE("Bad DETECT");
            } else {
                snprintf(cr->info, sizeof(cr->info), "Detect receiver");
                entryOk = true;
            }
            break;
        }
        case CMD_TYPE_SLEEP: {
            const char *timeoutStr = strtok(NULL, kCfgTokSep);
            const char *none = strtok(NULL, kCfgTokSep);
            if ((timeoutStr == NULL) || (none != NULL) || (sscanf(timeoutStr, "%"SCNu32, &cr->timeout) != 1) || (cr->timeout == 0)) {
                WARNING_INFILE("Bad SLEEP");
            } else {
                snprintf(cr->info, sizeof(cr->info), "Sleep %.3fs", (double)cr->timeout * 1e-3);
                entryOk = true;
            }
            break;
        }
        case CMD_TYPE_MESSAGE: {
            const char *timeoutStr = strtok(NULL, kCfgTokSep);
            const char *nameStr = strtok(NULL, kCfgTokSep);
            const char *none = strtok(NULL, kCfgTokSep);
            if ((timeoutStr == NULL) || (nameStr == NULL) || (none != NULL) ||
                (sscanf(timeoutStr, "%"SCNu32, &cr->timeout) != 1) || (cr->timeout == 0)) {
                WARNING_INFILE("Bad MESSAGE");
            } else {
                snprintf(cr->name, sizeof(cr->name), "%s", nameStr);
                snprintf(cr->info, sizeof(cr->info), "Wait for message %s, timeout %.3f", nameStr, (double)cr->timeout * 1e-3);
                entryOk = true;
            }
            break;
        }
        case CMD_TYPE_COMMAND: {
            bool ok = true;
            char *cmdStr = strtok(NULL, kCfgTokSep);
            char *timeoutStr = strtok(NULL, kCfgTokSep);
            char *rspStr = strtok(NULL, kCfgTokSep);
            char *none = strtok(NULL, kCfgTokSep);
            // Need exactly three more tokens
            if ( (cmdStr == NULL) || (timeoutStr == NULL) || (rspStr == NULL) || (none != NULL) ) {
                WARNING_INFILE("Bad COMMAND");
                ok = false;
            }
            // Parse timeout value
            if (ok && (sscanf(timeoutStr, "%"SCNu32, &cr->timeout) != 1)) {
                WARNING_INFILE("Bad COMMAND timeout");
                ok = false;
            }

            // Build the info string, for progress info output
            if (ok) {
                const int max = sizeof(cr->info) / 3;
                snprintf(&cr->info[0], max, "> %s", cmdStr);
                snprintf(&cr->info[strlen(cr->info)], max, ", timeout %.3f", (double)cr->timeout * 1e-3);
                snprintf(&cr->info[strlen(cr->info)], max, ", < %s", rspStr);
            }

            // Parse cmd string
            const char *errorHint = "";
            if (ok && !_strToMsg(&cr->cmd, cmdStr, &errorHint)) {
                ok = false;
                WARNING_INFILE("Bad COMMAND cmd (%s)", errorHint);
            }

            // Parse rsp string
            if (ok && !_strToMsg(&cr->rsp, rspStr, &errorHint)) {
                ok = false;
                WARNING_INFILE("Bad COMMAND cmd (%s)", errorHint);
            }

            // If we're expecting a response, the timeout must be > 0
            if ((cr->rsp.size != 0) && (cr->timeout == 0)) {
                WARNING_INFILE("Bad COMMAND timeout");
                ok = false;
            }

            if (ok) {
                entryOk = true;
            }
            break;
        }
        case CMD_TYPE_UNSPECIFIED:
            WARNING_INFILE("syntax error");
            break;
    }

    DEBUG("----- db[%d] %s-----", db->n, entryOk ? "ok" : "bad");
    DEBUG("db[%d].type=%s", db->n, _cmdTypeStr(cr->type));
    DEBUG("db[%d].timeout=%u", db->n, cr->timeout);
    DEBUG("db[%d].info=%s", db->n, cr->info);
    DEBUG_HEXDUMP(cr->cmd.data, cr->cmd.size, "cr[%d].cmd", db->n);
    DEBUG_HEXDUMP(cr->rsp.data, cr->rsp.size, "cr[%d].rsp", db->n);

    if (entryOk) {
        db->n++;
    }
    return entryOk;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _strToMsg(MSG_t *msg, char *str, const char** errorHint)
{
    // E.g. "NMEA:blabla"  strLen=11             "NMEA:" strLen=4
    //       01234567890                          01234
    //            ^typeStrOffs=5 (4 + 1)               ^typeStrOffs=5 (4 + 1) > strLen
    const int strLen = strlen(str);
    const char *typeStr = strtok(str, kMsgTypeSep);
    const int typeStrOffs = strlen(typeStr) + 1;
    const char *dataStr = &str[typeStrOffs];
    const MSG_TYPE_t type = _strMsgType(typeStr);
    if ((typeStr == NULL) || (type == MSG_TYPE_UNSPECIFIED) || ((type != MSG_TYPE_NONE) && (typeStrOffs > strLen))) {
        *errorHint = "bad spec";
        return false;
    }
    switch (type) {
        // Handled above
        case MSG_TYPE_UNSPECIFIED:
        case MSG_TYPE_NONE:
            break;
        case MSG_TYPE_NMEA:
            msg->size = nmeaMakeMessage("", "", dataStr, (char*)msg->data);
            break;
        case MSG_TYPE_HEX: {
            const int len = strlen(dataStr);
            if ((len % 2) != 0) {
                *errorHint = "bad hex";
                return false;
            } else {
                for (int ix = 0; ix < len; ix += 2) {
                    if (sscanf(&dataStr[ix], "%2hhx", &msg->data[msg->size]) == 1) {
                        msg->size++;
                    } else {
                        *errorHint = "bad hex";
                        return false;
                    }
                }
            }
            break;
        }
    }
    return true;
}

/* ****************************************************************************************************************** */

static bool _processCommand(RX_t* rx, const CMD_RSP_t* cmdRsp);
static bool _processMessage(RX_t* rx, const CMD_RSP_t* cmdRsp);

int cmd2rxRun(const char* portArg, const bool noProbe)
{
    PRINT("Loading configuration");
    CMD_RSP_DB_t* db = malloc(sizeof(CMD_RSP_DB_t));
    if (db == NULL) {
        WARNING("malloc fail");
        return EXIT_OTHERFAIL;
    }
    if (!_loadDb(db) || (db->n <= 0)) {
        WARNING("Failed reading config file");
        free(db);
        return EXIT_OTHERFAIL;
    }

    RX_OPTS_t rxOpts = RX_OPTS_DEFAULT();
    rxOpts.detect = RX_DET_PASSIVE; // Should work for non u-blox receivers, too
    if (noProbe) {
        rxOpts.autobaud = false;
        rxOpts.detect   = RX_DET_NONE;
    }
    RX_t *rx = rxInit(portArg, &rxOpts);
    if ( (rx == NULL) || !rxOpen(rx) )
    {
        WARNING("Could not open rx port");
        free(db);
        free(rx);
        return EXIT_RXFAIL;
    }

    // for each command in vector
    PRINT("Processing %d commands", db->n);
    bool ok = true;
    for (int ix = 0; ok && (ix < db->n); ix++) {
        const CMD_RSP_t *cmdRsp = &db->cr[ix];
        PRINT("Command %d/%d: %s", ix + 1, db->n, cmdRsp->info);
        switch (cmdRsp->type) {
            case CMD_TYPE_COMMAND:
                ok = _processCommand(rx, cmdRsp);
                break;
            case CMD_TYPE_MESSAGE:
                ok = _processMessage(rx, cmdRsp);
                break;
            case CMD_TYPE_SLEEP: {
                const uint32_t t0 = TIME();
                while ((TIME() - t0) < cmdRsp->timeout) {
                    PARSER_MSG_t *msg = rxGetNextMessage(rx);
                    if (msg == NULL) {
                        SLEEP(5);
                    }
                }
                break;
            }
            case CMD_TYPE_DETECT: {
                // FIXME: rx should provide an API for doing a (re-)detection. Meanwhile we just close the rx and re-init/open it
                rxClose(rx);
                free(rx);
                rx = NULL;
                rxOpts.autobaud = true;
                rxOpts.detect   = RX_DET_PASSIVE;
                rx = rxInit(portArg, &rxOpts);
                if ((rx == NULL) || !rxOpen(rx) ) {
                    WARNING("Could not open rx port");
                    ok = false;
                }
                break;
            }
            case CMD_TYPE_UNSPECIFIED:
                break;
        }
    }

    if (!ok) {
        WARNING("Failed processing all commands");
    } else {
        PRINT("Successfully processed %d commands", db->n);
    }

    if (rx != NULL) {
        rxClose(rx);
        free(rx);
    }
    free(db);

    return ok ? EXIT_SUCCESS : EXIT_OTHERFAIL;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _processCommand(RX_t *rx, const CMD_RSP_t* cmdRsp)
{
    // Send command
    DEBUG_HEXDUMP(cmdRsp->cmd.data, cmdRsp->cmd.size, cmdRsp->cmd.size > 0 ? "> command" : "> (nothing)");
    if (cmdRsp->cmd.size > 0) {
        if (!rxSend(rx, cmdRsp->cmd.data, cmdRsp->cmd.size)) {
            WARNING("Failed sending command");
            return false;
        }
    }

    // Wait for response (or just wait if we don't expect a response)
    DEBUG_HEXDUMP(cmdRsp->rsp.data, cmdRsp->rsp.size, cmdRsp->rsp.size > 0 ? "< expected response" : "< (no response expected)");
    bool found_resp = false;
    const uint32_t t0 = TIME();
    while (!found_resp && ((TIME() - t0) < cmdRsp->timeout)) {
        PARSER_MSG_t *msg = rxGetNextMessage(rx);
        if (msg != NULL) {
            TRACE_HEXDUMP(msg->data, msg->size, "< received %s %s", msg->name, msg->info);

            // If we expect a response, abort loop as soon as we see the response,
            if ((cmdRsp->rsp.size > 0) && (msg->size == cmdRsp->rsp.size)) {
                if (memcmp(cmdRsp->rsp.data, msg->data, cmdRsp->rsp.size) == 0) {
                    found_resp = true;
                    DEBUG_HEXDUMP(msg->data, msg->size, "< response found");
                }
            }
        } else {
            SLEEP(5);
        }
    }

    // Complain if we expected a response and it was not found
    if ((cmdRsp->rsp.size > 0) && !found_resp) {
        WARNING("Timeout waiting for response");
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

static bool _processMessage(RX_t *rx, const CMD_RSP_t* cmdRsp)
{
    // Wait for a message that matches the name
    bool found_name = false;
    const uint32_t t0 = TIME();
    while (!found_name && ((TIME() - t0) < cmdRsp->timeout)) {
        PARSER_MSG_t *msg = rxGetNextMessage(rx);
        if (msg != NULL) {
            if (strncmp(msg->name, cmdRsp->name, strlen(cmdRsp->name)) == 0) {
                found_name = true;
                TRACE_HEXDUMP(msg->data, msg->size, "message found");
            }
        } else {
            SLEEP(5);
        }
    }

    if (!found_name) {
        WARNING("Timeout waiting for message");
        return false;
    }

    return true;
}

/* ****************************************************************************************************************** */
// eof
