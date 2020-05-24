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

#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>

#include "ff_debug.h"
#include "ff_stuff.h"

#include "cfgtool_cfg2ubx.h"
#include "cfgtool_rx2cfg.h"
#include "cfgtool_cfg2rx.h"
#include "cfgtool_rxtest.h"
#include "cfgtool_uc2cfg.h"
#include "cfgtool_cfginfo.h"
#include "cfgtool_parse.h"
#include "cfgtool_reset.h"
#include "cfgtool.h"

/* ********************************************************************************************** */

#define VERSION "v1.0"

typedef struct CMD_s
{
    const char   *name;
    bool          need_i;
    bool          need_o;
    bool          need_p;
    bool          need_l;
    bool          need_r;
    bool          may_r;
    const char   *info;
    const char *(*help)(void);
    int         (*run)(void);

} CMD_t;

typedef struct STATE_s
{
    const CMD_t *cmd;

    const char  *inName;
    FILE        *inFile;
    int          inLineNr;

    const char  *outName;
    FILE        *outFile;
    bool         outOverwrite;

    const char  *rxPort;
    const char  *cfgLayer;
    const char  *resetType;
    bool         useUnknown;
    bool         extraInfo;
    bool         applyConfig;

} STATE_t;

static STATE_t gState;

static int rx2cfg(void)  { return rx2cfgRun( gState.rxPort, gState.cfgLayer, gState.useUnknown); }
static int rx2list(void) { return rx2listRun(gState.rxPort, gState.cfgLayer, gState.useUnknown); }
static int cfg2rx(void)  { return cfg2rxRun( gState.rxPort, gState.cfgLayer, gState.resetType, gState.applyConfig); }
static int cfg2ubx(void) { return cfg2ubxRun(gState.cfgLayer, gState.extraInfo); }
static int cfg2hex(void) { return cfg2hexRun(gState.cfgLayer, gState.extraInfo); }
static int cfg2c(void)   { return cfg2cRun(  gState.cfgLayer, gState.extraInfo); }
static int uc2cfg(void)  { return uc2cfgRun(); }
static int cfginfo(void) { return cfginfoRun(); }
static int rxtest(void)  { return rxtestRun( gState.rxPort, gState.extraInfo); }
static int parse(void)   { return parseRun(  gState.extraInfo); }
static int reset(void)   { return resetRun(  gState.rxPort, gState.resetType); }

const CMD_t kCmds[] =
{
    { .name = "cfg2rx",  .info = "Configure a receiver from a configuration file",             .help = cfg2rxHelp,  .run = cfg2rx,
      .need_i = true,  .need_o = false, .need_p = true,  .need_l = true,  .may_r  = true  },

    { .name = "rx2cfg",  .info = "Create configuration file from config in a receiver",        .help = rx2cfgHelp,  .run = rx2cfg,
      .need_i = false, .need_o = true,  .need_p = true,  .need_l = true,  .need_r = false },

    { .name = "rx2list", .info = "Like rx2cfg but output a flat list of key-value pairs",      .help = rx2listHelp, .run = rx2list,
      .need_i = false, .need_o = true,  .need_p = true,  .need_l = true,  .need_r = false },

    { .name = "cfg2ubx", .info = "Convert config file to UBX-CFG-VALSET message(s)",           .help = cfg2ubxHelp, .run = cfg2ubx,
      .need_i = true,  .need_o = true,  .need_p = false, .need_l = true,  .need_r = false },

    { .name = "cfg2hex", .info = "Like cfg2ubx but prints a hex dump of the message(s)",       .help = NULL,        .run = cfg2hex,
      .need_i = true,  .need_o = true,  .need_p = false, .need_l = true,  .need_r = false },

    { .name = "cfg2c",   .info = "Like cfg2ubx but prints a c source code of the message(s)",  .help = NULL,        .run = cfg2c,
      .need_i = true,  .need_o = true,  .need_p = false, .need_l = true,  .need_r = false },

    { .name = "uc2cfg",  .info = "Convert u-center config file to sane config file",           .help = uc2cfgHelp,  .run = uc2cfg,
      .need_i = true,  .need_o = true,  .need_p = false, .need_l = false, .need_r = false },

    { .name = "cfginfo", .info = "Print information about known configuration items etc.",     .help = cfginfoHelp, .run = cfginfo,
      .need_i = false, .need_o = true,  .need_p = false, .need_l = false, .need_r = false },

    { .name = "rxtest",  .info = "Connects to receiver and prints received message frames",    .help = rxtestHelp,  .run = rxtest,
      .need_i = false, .need_o = true,  .need_p = true,  .need_l = false, .need_r = false },

    { .name = "parse",   .info = "Parse file and outputs message frames",                      .help = parseHelp,   .run = parse,
      .need_i = true,  .need_o = true,  .need_p = false, .need_l = false, .need_r = false },

    { .name = "reset",   .info = "Reset receiver",                                             .help = resetHelp,   .run = reset,
      .need_i = false, .need_o = false, .need_p = true,  .need_l = false, .need_r = true  },
};

const char * const kHelpStr = 
    // -----------------------------------------------------------------------------
    "cfgtool "VERSION" -- u-blox 9 configuration interface tool\n"
    "\n"
    "Copyright (c) 2020 Philippe Kehl (flipflip at oinkzwurgl dot org)\n"
    "https://oinkzwurgl.org/hacking/ubloxcfg/\n"
    "\n"
    // -----------------------------------------------------------------------------
    "Usage:\n"
    "\n"
    "    cfgtool [-h] [-v] [-q] [...] <command>\n"
    "\n"
    // -----------------------------------------------------------------------------
    "Where:\n"
    "\n"
    "    -h / -H        Prints help summary / full help\n"
    "    -v / -q        Increases / decreases verbosity\n"
    "    <command>      Command\n"
    "\n"
    "    And depending on the <command>:\n"
    "\n"
    "    -i <infile>    Input file (default: '-', i.e. standard input)\n"
    "    -o <outfile>   Output file (default: '-', i.e. standard output)\n"
    "    -y             Overwrite output file if it already exists\n"
    "    -p <serial>    Serial port where the receiver is connected:\n"
    "                       [ser://]<device>[:<baudrate>]\n"
    "                       tcp://<host>:<port>[:<baudrate>]\n"
    "                       telnet://<host>:<port>[:<baudrate>]\n"
    "    -l <layer(s)>  Configuration layer(s) to use:\n"
    "                       RAM, BBR, Flash, Default\n"
    "    -r <reset>     Reset mode to use to reset the receiver:\n"
    "                       hot, warm, cold, default, factory, stop, start, gnss\n"
    "    -u             Use unknown configuation items\n"
    "    -x             Output extra information (comments, hex dumps)\n"
    "    -a             Activate configuration after storing.\n"
    "\n"
    // -----------------------------------------------------------------------------
    "    Available <commands>s:\n"
    "\n";

const char * const kLicenseHelp =
    // -----------------------------------------------------------------------------
    "License:\n"
    "\n"
    "    This program is free software: you can redistribute it and/or modify it\n"
    "    under the terms of the GNU General Public License as published by the Free\n"
    "    Software Foundation, either version 3 of the License, or (at your option)\n"
    "    any later version.\n"
    "\n"
    "    This program is distributed in the hope that it will be useful, but WITHOUT\n"
    "    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or\n"
    "    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for\n"
    "    more details.\n"
    "\n"
    "    You should have received a copy of the GNU General Public License along with\n"
    "    this program. If not, see <https://www.gnu.org/licenses/>.\n"
    "\n"
    "Third-party code:\n"
    "\n"
    "    This program includes CRC24Q routines from the GPSD project, under a\n"
    "    BSD-2-Clause license. See source code or https://gitlab.com/gpsd/.\n"
    "\n";

const char * const kPortHelp =
    "Serial ports:\n"
    "\n"
    "    Local serial ports: [ser://]<device>[@<baudrate>], where:\n"
    "\n"
#ifdef _WIN    
    "        <device>     COM1, COM23, etc.\n"
#else
    "        <device>     /dev/ttyUSB0, /dev/ttyACM1, /dev/serial/..., etc.\n"
#endif
    "        <baudrate>   Baudrate (optional)\n"
    "\n"
    "        Note that 'ser://' is the default and can be omitted. If no <baudrate>\n"
    "        is specified, it will be automatically detected. That is, '-p <device>'\n"
    "        will work in most cases.\n"
    "\n"
#ifndef _WIN    
    "        It is recommended to use /dev/serial/by-path/... device names for USB\n"
    "        (CDC ACM) connections as the names will remain after a hardware reset\n"
    "        and USB re-enumeration. See the 'reset' command.\n"
    "\n"
#endif
    "    Raw TCP/IP ports: tcp://<addr>:<port>, where:\n"
    "\n"
    "        <addr>       Hostname or IP address\n"
    "        <port>       Port number\n"
    "\n"
    "    Telnet TCP/IP ports: telnet://<addr>:<port>[@<baudrate>], where\n"
    "\n"
    "        <addr>       Hostname or IP address\n"
    "        <port>       Port number\n"
    "        <baudrate>   Baudrate (optional)\n"
    "\n"
    "        This uses telnet (RFC854, etc.) in-band control using com port control\n"
    "        option (RFC2217) to set the baudrate on the remote serial port. This\n"
    "        works for example with ser2net(8) and some hardware RS232 servers.\n"
    "\n"
    "        A minimal ser2net command line that should work is:\n"
    "           ser2net -d -C \"12345:telnet:0:/dev/ttyUSB0: remctl\"\n"
    "        This should allow using '-p telnet://localhost:12345'.\n"
    "\n";

const char * const kLayersHelp =
    // -----------------------------------------------------------------------------
    "Configuration layers:\n"
    "\n"
    "    RAM         Current(ly used) configuration, has all items\n"
    "    BBR         Battery-backed RAM item storage, may have no items at all\n"
    "    Flash       Flash (if available) items storage, may have no items at all\n"
    "    Default     Default configuration, has all items\n"
    "\n";

const char * const kExitHelp =
    "Exit status:\n"
    "\n"
    "    Command successful:            "STRINGIFY(EXIT_SUCCESS)"\n"
    "    Bad command-line arguments:    "STRINGIFY(EXIT_BADARGS)"\n"
    "    Detecting receiver failed:     "STRINGIFY(EXIT_RXFAIL)"\n"
    "    No data from receiver:         "STRINGIFY(EXIT_RXNODATA)"\n"
    "    Unspecified error:             "STRINGIFY(EXIT_OTHERFAIL)"\n"
    "\n";

const char * const kGreeting =
    "Happy hacking! :-)\n"
    "\n";

#define _ARG_STR(_flag_, _var_) \
        else if (strcmp(_flag_, argv[argIx]) == 0) \
        { \
            if ((argIx + 1) < argc) \
            { \
                _var_ = argv[argIx + 1]; \
                argIx++; \
            } \
            else \
            { \
                argOk = false; \
            } \
        }

#define _ARG_BOOL(_flag_, _var_, _state_) \
        else if (strcmp(_flag_, argv[argIx]) == 0) \
        { \
            _var_ = _state_; \
        }

void printHelp(const bool full)
{
    fputs(kHelpStr, stdout);
    for (int ix = 0; ix < NUMOF(kCmds); ix++)
    {
        fprintf(stdout, "    %-14s %s\n", kCmds[ix].name, kCmds[ix].info);
    }
    fputs("\n", stdout);
    if (full)
    {
        fputs(kLicenseHelp, stdout);
        fputs(kPortHelp, stdout);
        fputs(kLayersHelp, stdout);
        fputs(kExitHelp, stdout);
        for (int ix = 0; ix < NUMOF(kCmds); ix++)
        {
            if (kCmds[ix].help != NULL)
            {
                fputs(kCmds[ix].help(), stdout);
            }
        }
    }
    fputs(kGreeting, stdout);
}

int main(int argc, char **argv)
{
    memset(&gState, 0, sizeof(gState));

    FILE       *logFile      = stderr;
    int         logVerbosity = 0;
    bool        logColour    = isatty(fileno(logFile));
    debugSetup(logFile, logVerbosity, logColour, NULL);

    for (int argIx = 1; argIx < argc; argIx++)
    {
        bool argOk = true;
        if (strcmp("-h", argv[argIx]) == 0)
        {
            printHelp(false);
            exit(EXIT_SUCCESS);
        }
        else if (strcmp("-H", argv[argIx]) == 0)
        {
            printHelp(true);
            exit(EXIT_SUCCESS);
        }
        else if (strcmp("-v", argv[argIx]) == 0)
        {
            logVerbosity++;
        }
        else if (strcmp("-q", argv[argIx]) == 0)
        {
            logVerbosity--;
        }
        _ARG_STR("-i", gState.inName)
        _ARG_STR("-o", gState.outName)
        _ARG_STR("-p", gState.rxPort)
        _ARG_STR("-l", gState.cfgLayer)
        _ARG_STR("-r", gState.resetType)
        _ARG_BOOL("-u", gState.useUnknown, true)
        _ARG_BOOL("-x", gState.extraInfo, true)
        _ARG_BOOL("-a", gState.applyConfig, true)
        _ARG_BOOL("-y", gState.outOverwrite, true)
        else if (gState.cmd != NULL)
        {
            argOk = false;
        }
        else
        {
            for (int ix = 0; ix < NUMOF(kCmds); ix++)
            {
                if (strcmp(kCmds[ix].name, argv[argIx]) == 0)
                {
                    gState.cmd = &kCmds[ix];
                    break;
                }
            }
            if (gState.cmd == NULL)
            {
                argOk = false;
            }
        }

        if (!argOk)
        {
            WARNING("Illegal argument '%s'!", argv[argIx]);
            gState.cmd = NULL;
            break;
        }
    }

    debugSetup(logFile, logVerbosity, logColour, gState.cmd != NULL ? gState.cmd->name : NULL);

    bool res = true;
    if (gState.cmd == NULL)
    {
        WARNING("Try '%s -h'.", argv[0]);
        res = false;;
    }

    // Open input file
    if (res && gState.cmd->need_i)
    {
        if ( (gState.inName == NULL) || ((gState.inName[0] == '-') && (gState.inName[1] == '\0')) )
        {
            gState.inName = "-";
            gState.inFile = stdin;
            DEBUG("input from stdin");
#ifndef _WIN32
            int fd = fileno(gState.inFile);
            const int flags = fcntl(fd, F_GETFL, 0);
            int res = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
            if ( ((flags < 0)) || (res < 0) )
            {
                WARNING("Failed setting stdin to non-blocking: %s", strerror(errno));
                res = false;
            }
#else
            // FIXME: for windows?
#endif
        }
        else if (gState.inName[0] == '\0')
        {
            WARNING("Need '-i <infile>' argument!");
            res = false;
        }
        else
        {
            DEBUG("Opening input: %s", gState.inName);
            gState.inFile = fopen(gState.inName, "r");
            if (gState.inFile == NULL)
            {
                WARNING("Failed opening '%s' for reading: %s!\n",
                    gState.inName, strerror(errno));
                res = false;
            }
        }
    }
    else if ( (gState.cmd != NULL) && !gState.cmd->need_i && (gState.inName != NULL) )
    {
        WARNING("Illegal argument '-i %s'!", gState.inName);
        res = false;
    }

    // Open output file
    if (res && gState.cmd->need_o)
    {
        if ( (gState.outName == NULL) || ((gState.outName[0] == '-') && (gState.outName[1] == '\0')) )
        {
            gState.outName = "-";
            gState.outFile = stdout;
            DEBUG("output to stdout");
        }
        else if (gState.outName[0] == '\0')
        {
            WARNING("Need '-o <infile>' argument!");
            res = false;
        }
        else
        {
            // Don't open now, writeOutput() will do that
        }
    }

    // Require -p arg?
    if ( (gState.cmd != NULL) && gState.cmd->need_p )
    {
        if ( (gState.rxPort == NULL) || (gState.rxPort[0] == '\0') )
        {
            WARNING("Need '-p <port>' argument!");
            res = false;
        }
    }
    else if ( (gState.cmd != NULL) && !gState.cmd->need_p && (gState.rxPort != NULL) )
    {
        WARNING("Illegal argument '-p %s'!", gState.rxPort);
        res = false;
    }

    // Require -l arg?
    if ((gState.cmd != NULL) && gState.cmd->need_l)
    {
        if ( (gState.cfgLayer == NULL) || (gState.cfgLayer[0] == '\0') )
        {
            WARNING("Need '-l <layer(s)>' argument!");
            res = false;
        }
    }
    else if ( (gState.cmd != NULL) && !gState.cmd->need_l && (gState.cfgLayer != NULL) )
    {
        WARNING("Illegal argument '-l %s'!", gState.cfgLayer);
        res = false;
    }

    // Require -r arg?
    if ((gState.cmd != NULL) && gState.cmd->need_r)
    {
        if ( (gState.resetType == NULL) || (gState.resetType[0] == '\0') )
        {
            WARNING("Need '-r <mode>' argument!");
            res = false;
        }
    }
    else if ( (gState.cmd != NULL) && !(gState.cmd->need_r || gState.cmd->may_r) && (gState.resetType != NULL) )
    {
        WARNING("Illegal argument '-r %s'!", gState.resetType);
        res = false;
    }

    // Are we happy with the arguments?
    if (!res)
    {
        exit(EXIT_BADARGS);
    }

    // Execute
    DEBUG("args: inName=%s outName=%s rxPort=%s cfgLayer=%s useUnknown=%d",
        gState.inName, gState.outName, gState.rxPort, gState.cfgLayer, gState.useUnknown);
    const int exitCode = gState.cmd->run();

    return exitCode;
}

/* ********************************************************************************************** */

#define INPUT_MAX_LINE_LEN 2000

LINE_t *getNextInputLine(void)
{
    static char line[INPUT_MAX_LINE_LEN];
    static LINE_t resLine;
    resLine.line = NULL;
    resLine.lineLen = 0;
    bool res = false;
    // get next useful line from input
    while (fgets(line, sizeof(line), gState.inFile) != NULL)
    {
        gState.inLineNr++;
        char *pLine = line;

        // remove comments
        char *comment = strchr(pLine, '#');
        if (comment != NULL)
        {
            *comment = '\0';
        }

        // remove leading space
        while (isspace(*pLine) != 0)
        {
            pLine++;
        }

        // all whitespace?
        if (*pLine == '\0')
        {
            continue;
        }

        // remove trailing whitespace
        int inLineLen = strlen(pLine);
        char *pEnd = pLine + inLineLen - 1;
        while( (pEnd > pLine) && (isspace(*pEnd) != 0) )
        {
            pEnd--;
            inLineLen--;
        }
        pEnd[1] = '\0';

        resLine.line    = pLine;
        resLine.lineLen = inLineLen;
        resLine.lineNr  = gState.inLineNr;
        resLine.file    = gState.inName;
        res = true;
        break;
    }

    return res ? &resLine : NULL;
}

int readInput(uint8_t *data, const int size)
{
    int res = 0;
    if ( (data != NULL) && (size > 0) )
    {
        if (feof(gState.inFile) != 0) // FIXME: why does this flag on /dev/tty... devices?
        {
            res = -1;
        }
        else
        {
             // TODO: only read as much as is currently available
            res = fread(data, 1, size, gState.inFile);
        }
    }
    return res;
}


static char gOutputBuf[1024 * 1024] = { 0 };
static int gOutputBufSize = 0;

void addOutputStr(const char *fmt, ...)
{
    const int totSize = sizeof(gOutputBuf);
    if (gOutputBufSize >= totSize)
    {
        return;
    }

    const int remSize = totSize - gOutputBufSize;

    va_list args;
    va_start(args, fmt);
    const int writeSize = vsnprintf(&gOutputBuf[gOutputBufSize], remSize, fmt, args);
    va_end(args);

    if (writeSize > remSize)
    {
        gOutputBufSize = totSize;
    }
    else
    {
        gOutputBufSize += writeSize;
    }
}

void addOutputBin(const uint8_t *data, const int size)
{
    const int totSize = sizeof(gOutputBuf);
    if (gOutputBufSize >= totSize)
    {
        return;
    }

    const int remSize = totSize - gOutputBufSize;
    if (remSize < size)
    {
        return;
    }

    memcpy(&gOutputBuf[gOutputBufSize], data, size);
    gOutputBufSize += size;
}

void addOutputHex(const uint8_t *data, const int size, const int wordsPerLine)
{
    const int bytesPerLine = wordsPerLine * 4;
    for (int ix = 0; ix < size; ix++)
    {
        const int pos = ix % bytesPerLine;
        if (pos > 0)
        {
            addOutputStr((pos % 4) == 0 ? "  " : " ");
        }
        addOutputStr("%02"PRIx8, data[ix]);
        if ( (pos == (bytesPerLine - 1)) || (ix == (size - 1)) )
        {
            addOutputStr("\n");
        }
    }
}

void addOutputC(const uint8_t *data, const int size, const int wordsPerLine, const char *indent)
{
    const int bytesPerLine = wordsPerLine * 4;
    for (int ix = 0; ix < size; ix++)
    {
        const int pos = ix % bytesPerLine;
        if (pos == 0)
        {
            addOutputStr("%s", indent);
        }
        if (pos > 0)
        {
            addOutputStr((pos % 4) == 0 ? "  " : " ");
        }
        addOutputStr("0x%02"PRIx8"%s", data[ix], (ix + 1) == size ? "" : ",");
        if ( (pos == (bytesPerLine - 1)) || (ix == (size - 1)) )
        {
            addOutputStr("\n");
        }
    }
}

void addOutputHexdump(const uint8_t *data, const int size)
{
    const char i2hex[] = "0123456789abcdef";
    const uint8_t *pData = data;
    for (int ix = 0; ix < size; )
    {
        char str[70];
        memset(str, ' ', sizeof(str));
        str[50] = '|';
        str[67] = '|';
        str[68] = '\0';
        for (int ix2 = 0; (ix2 < 16) && ((ix + ix2) < size); ix2++)
        {
            //           1         2         3         4         5         6
            // 012345678901234567890123456789012345678901234567890123456789012345678
            // xx xx xx xx xx xx xx xx  xx xx xx xx xx xx xx xx  |................|\0
            // 0  1  2  3  4  5  6  7   8  9  10 11 12 13 14 15
            const uint8_t c = pData[ix + ix2];
            int pos1 = 3 * ix2;
            int pos2 = 51 + ix2;
            if (ix2 > 7)
            {
                   pos1++;
            }
            str[pos1    ] = i2hex[ (c >> 4) & 0xf ];
            str[pos1 + 1] = i2hex[  c       & 0xf ];

            str[pos2] = isprint((int)c) ? c : '.';
        }
        addOutputStr("0x%04"PRIx8" %05d  %s\n", ix, ix, str);
        ix += 16;
    }
}

bool writeOutput(const bool append)
{
    const int totSize = sizeof(gOutputBuf);
    if (gOutputBufSize >= totSize)
    {
        WARNING("Too much output data, not writing to '%s'!", gState.outName);
        return false;
    }

    const char *failStr = NULL;
    bool res = true;
    if (gState.outFile != stdout)
    {
        if (!append && !gState.outOverwrite && (access(gState.outName, F_OK) == 0))
        {
            failStr = "File already exists";
            res = false;
        }

        if (res)
        {
            gState.outFile = fopen(gState.outName, append ? "a" : "w");
            if (gState.outFile == NULL)
            {
                failStr = strerror(errno);
                res = false;
            }
        }
    }

    if (res)
    {
        if (!append)
        {
            PRINT("Writing output to '%s'.", gState.outName);
        }
        DEBUG("Writing output to '%s' (%d bytes).", gState.outName, gOutputBufSize);
        res = (int)fwrite(gOutputBuf, 1, gOutputBufSize, gState.outFile) == gOutputBufSize;
    }

    gOutputBufSize = 0;

    if (!res)
    {
        WARNING("Failed writing '%s': %s", gState.outName, failStr == NULL ? "Unknown error" : failStr);
    }
    return res;
}

/* ********************************************************************************************** */
// eof
