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

#include "cfgtool_util.h"
#include "cfgtool_cfg2ubx.h"
#include "cfgtool_rx2cfg.h"
#include "cfgtool_cfg2rx.h"
#include "cfgtool_dump.h"
#include "cfgtool_uc2cfg.h"
#include "cfgtool_cfginfo.h"
#include "cfgtool_parse.h"
#include "cfgtool_reset.h"
#include "cfgtool_status.h"
#include "cfgtool_bin2hex.h"
#include "config.h"

/* ****************************************************************************************************************** */

// FIXME: there's room for improvement...

typedef struct CMD_s
{
    const char   *name;
    bool          need_i;
    bool          need_o;
    bool          need_p;
    bool          need_l;
    bool          need_r;
    bool          may_r;
    bool          may_n;
    bool          may_e;
    bool          may_u;
    bool          may_U;
    bool          may_R;
    const char   *info;
    const char *(*help)(void);
    int         (*run)(void);

} CMD_t;

typedef struct ARGS_s
{
    const CMD_t *cmd;

    const char  *inName;
    FILE        *inFile;

    const char  *outName;
    FILE        *outFile;
    bool         outOverwrite;

    const char  *rxPort;
    const char  *cfgLayer;
    const char  *resetType;
    bool         useUnknown;
    bool         extraInfo;
    bool         applyConfig;
    bool         noProbe;
    bool         doEpoch;
    bool         updateOnly;
    bool         allowReplace;

} ARGS_t;

static ARGS_t gArgs;

static int rx2cfg(void)  { return rx2cfgRun( gArgs.rxPort, gArgs.cfgLayer, gArgs.useUnknown, gArgs.extraInfo); }
static int rx2list(void) { return rx2listRun(gArgs.rxPort, gArgs.cfgLayer, gArgs.useUnknown, gArgs.extraInfo); }
static int cfg2rx(void)  { return cfg2rxRun( gArgs.rxPort, gArgs.cfgLayer, gArgs.resetType, gArgs.applyConfig, gArgs.updateOnly, gArgs.allowReplace); }
static int cfg2ubx(void) { return cfg2ubxRun(gArgs.cfgLayer, gArgs.extraInfo, gArgs.allowReplace); }
static int cfg2hex(void) { return cfg2hexRun(gArgs.cfgLayer, gArgs.extraInfo, gArgs.allowReplace); }
static int cfg2c(void)   { return cfg2cRun(  gArgs.cfgLayer, gArgs.extraInfo, gArgs.allowReplace); }
static int uc2cfg(void)  { return uc2cfgRun(); }
static int cfginfo(void) { return cfginfoRun(); }
static int dump(void)    { return dumpRun( gArgs.rxPort, gArgs.extraInfo, gArgs.noProbe); }
static int parse(void)   { return parseRun(  gArgs.extraInfo, gArgs.doEpoch ); }
static int reset(void)   { return resetRun(  gArgs.rxPort, gArgs.resetType); }
static int status(void)  { return statusRun( gArgs.rxPort, gArgs.extraInfo, gArgs.noProbe); }
static int bin2hex(void) { return bin2hexRun(); }
static int hex2bin(void) { return hex2binRun(); }

const CMD_t kCmds[] =
{
    { .name = "cfg2rx",  .info = "Configure a receiver from a configuration file",             .help = cfg2rxHelp,  .run = cfg2rx,
      .need_i = true,  .need_o = false, .need_p = true,  .need_l = true,  .may_r  = true,  .may_n = false, .may_e = false, .may_u = true,  .may_U = true,  .may_R = true,  },

    { .name = "rx2cfg",  .info = "Create configuration file from config in a receiver",        .help = rx2cfgHelp,  .run = rx2cfg,
      .need_i = false, .need_o = true,  .need_p = true,  .need_l = true,  .need_r = false, .may_n = false, .may_e = false, .may_u = false, .may_U = false, .may_R = false, },

    { .name = "rx2list", .info = "Like rx2cfg but output a flat list of key-value pairs",      .help = rx2listHelp, .run = rx2list,
      .need_i = false, .need_o = true,  .need_p = true,  .need_l = true,  .need_r = false, .may_n = false, .may_e = false, .may_u = false, .may_U = false, .may_R = true,  },

    { .name = "cfg2ubx", .info = "Convert config file to UBX-CFG-VALSET message(s)",           .help = cfg2ubxHelp, .run = cfg2ubx,
      .need_i = true,  .need_o = true,  .need_p = false, .need_l = true,  .need_r = false, .may_n = false, .may_e = false, .may_u = false, .may_U = false, .may_R = true,  },

    { .name = "cfg2hex", .info = "Like cfg2ubx but prints a hex dump of the message(s)",       .help = NULL,        .run = cfg2hex,
      .need_i = true,  .need_o = true,  .need_p = false, .need_l = true,  .need_r = false, .may_n = false, .may_e = false, .may_u = false, .may_U = false, .may_R = true,  },

    { .name = "cfg2c",   .info = "Like cfg2ubx but prints a c source code of the message(s)",  .help = NULL,        .run = cfg2c,
      .need_i = true,  .need_o = true,  .need_p = false, .need_l = true,  .need_r = false, .may_n = false, .may_e = false, .may_u = false, .may_U = false, .may_R = false, },

    { .name = "uc2cfg",  .info = "Convert u-center config file to sane config file",           .help = uc2cfgHelp,  .run = uc2cfg,
      .need_i = true,  .need_o = true,  .need_p = false, .need_l = false, .need_r = false, .may_n = false, .may_e = false, .may_u = false, .may_U = false, .may_R = false, },

    { .name = "cfginfo", .info = "Print information about known configuration items etc.",     .help = cfginfoHelp, .run = cfginfo,
      .need_i = false, .need_o = true,  .need_p = false, .need_l = false, .need_r = false, .may_n = false, .may_e = false, .may_u = false, .may_U = false, .may_R = false, },

    { .name = "dump",    .info = "Connects to receiver and prints received message frames",    .help = dumpHelp,    .run = dump,
      .need_i = false, .need_o = true,  .need_p = true,  .need_l = false, .need_r = false, .may_n = true,  .may_e = false, .may_u = false, .may_U = false, .may_R = false, },

    { .name = "parse",   .info = "Parse file and output message frames",                       .help = parseHelp,   .run = parse,
      .need_i = true,  .need_o = true,  .need_p = false, .need_l = false, .need_r = false, .may_n = false, .may_e = true,  .may_u = false, .may_U = false, .may_R = false, },

    { .name = "reset",   .info = "Reset receiver",                                             .help = resetHelp,   .run = reset,
      .need_i = false, .need_o = false, .need_p = true,  .need_l = false, .need_r = true,  .may_n = false, .may_e = false, .may_u = false, .may_U = false, .may_R = false, },

    { .name = "status",  .info = "Connects to receiver and prints status",                     .help = statusHelp,  .run = status,
      .need_i = false, .need_o = true,  .need_p = true,  .need_l = false, .need_r = false, .may_n = true,  .may_e = false, .may_u = false, .may_U = false, .may_R = false, },

    { .name = "bin2hex", .info = "Convert to hex dump",                                        .help = bin2hexHelp, .run = bin2hex,
      .need_i = true,  .need_o = true,  .need_p = false, .need_l = false, .need_r = false, .may_n = false, .may_e = false, .may_u = false, .may_U = false, .may_R = false, },

    { .name = "hex2bin", .info = "Convert from hex dump",                                      .help = NULL,        .run = hex2bin,
      .need_i = true,  .need_o = true,  .need_p = false, .need_l = false, .need_r = false, .may_n = false, .may_e = false, .may_u = false, .may_U = false, .may_R = false, },

};

const char * const kTitleStr =
    // -----------------------------------------------------------------------------
    "cfgtool " CONFIG_VERSION " -- u-blox 9 configuration interface tool\n"
    "\n";

const char * const kCopyrightStr =
    // -----------------------------------------------------------------------------
    "    Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org)\n"
    "    https://oinkzwurgl.org/hacking/ubloxcfg/\n"
    "\n";

const char * const kVersionStr =
    // -----------------------------------------------------------------------------
    "    version: " CONFIG_VERSION ", git hash: " CONFIG_GITHASH ", build date and time: " CONFIG_DATE " " CONFIG_TIME "\n"
    "\n";

const char * const kHelpStr =
    // -----------------------------------------------------------------------------
    "Usage:\n"
    "\n"
    "    cfgtool -h | -H | -h <command> | -V\n"
    "    cfgtool [-v] [-q] [...] <command>\n"
    "\n"
    // -----------------------------------------------------------------------------
    "Where:\n"
    "\n"
    "    -h             Prints help summary\n"
    "    -H             Prints full help\n"
    "    -h <command>   Prints help for a command\n"
    "    -V             Displays version and license information\n"
    "\n"
    "    -v / -q        Increases / decreases verbosity\n"
    "\n"
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
    "                       soft, hard, hot, warm, cold, default, factory,\n"
    "                       stop, start, gnss\n"
    "    -u             Use unknown (undocumented) configuation items\n"
    "    -x             Output extra information (comments, hex dumps, ...)\n"
    "    -a             Activate configuration after storing\n"
    "    -n             Do not probe/autobaud receiver, use passive reading only.\n"
    "                   For example, for other receivers or read-only connection.\n"
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
    "    this program. If not, see https://www.gnu.org/licenses/.\n"
    "\n"
    "Third-party data:\n"
    "\n"
    "    This program includes data, such as identifiers, constants and descriptions\n"
    "    of configuration items, from public sources. Namely:\n"
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
    "        is specified, it is be automatically detected. That is, '-p <device>'\n"
    "        works in most cases.\n"
    "\n"
#ifndef _WIN
    "        It is recommended to use /dev/serial/by-path/... device names for USB\n"
    "        (CDC ACM) connections as the names remain after a hardware reset and\n"
    "        USB re-enumeration. See the 'reset' command.\n"
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
    "    Command successful:            " STRINGIFY(EXIT_SUCCESS) "\n"
    "    Bad command-line arguments:    " STRINGIFY(EXIT_BADARGS) "\n"
    "    Detecting receiver failed:     " STRINGIFY(EXIT_RXFAIL) "\n"
    "    No data from receiver:         " STRINGIFY(EXIT_RXNODATA) "\n"
    "    Unspecified error:             " STRINGIFY(EXIT_OTHERFAIL) "\n"
    "\n";

const char * const kGreeting =
    "Happy hacking! :-)\n"
    "\n";

#define _ARGS_STR(_flag_, _var_) \
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

#define _ARGS_BOOL(_flag_, _var_, _ARGS_) \
        else if (strcmp(_flag_, argv[argIx]) == 0) \
        { \
            _var_ = _ARGS_; \
        }

void printHelp(const bool full)
{
    fputs(kTitleStr, stdout);
    fputs(kCopyrightStr, stdout);
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

void printVersion(void)
{
    fputs(kTitleStr, stdout);
    fputs("Version:\n\n", stdout);
    fputs(kVersionStr, stdout);
    fputs("Copyright:\n\n", stdout);
    fputs(kCopyrightStr, stdout);
    fputs(kLicenseHelp, stdout);
    int nSources = 0;
    const char **sources = ubloxcfg_getSources(&nSources);
    for (int ix = 0; ix < nSources; ix++)
    {
        fprintf(stdout, "    - %s\n", sources[ix]);
    }
    fputs("\n", stdout);

    fputs(kGreeting, stdout);
}

int main(int argc, char **argv)
{
    memset(&gArgs, 0, sizeof(gArgs));

    DEBUG_CFG_t debugCfg =
    {
        .level  = DEBUG_LEVEL_PRINT,
#ifdef _WIN32
        .colour = true,
#else
        .colour = isatty(fileno(stderr)) == 1,
#endif
        .mark   = NULL,
        .func   = NULL,
        .arg    = NULL,
    };
    debugSetup(&debugCfg);
    bool doHelp = false;

    for (int argIx = 1; argIx < argc; argIx++)
    {
        bool argOk = true;
        if (strcmp("-h", argv[argIx]) == 0)
        {
            doHelp = true;
        }
        else if (strcmp("-H", argv[argIx]) == 0)
        {
            printHelp(true);
            exit(EXIT_SUCCESS);
        }
        else if (strcmp("-V", argv[argIx]) == 0)
        {
            printVersion();
            exit(EXIT_SUCCESS);
        }
        else if (strcmp("-v", argv[argIx]) == 0)
        {
            debugCfg.level++;
        }
        else if (strcmp("-q", argv[argIx]) == 0)
        {
            debugCfg.level--;
        }
        _ARGS_STR("-i", gArgs.inName)
        _ARGS_STR("-o", gArgs.outName)
        _ARGS_STR("-p", gArgs.rxPort)
        _ARGS_STR("-l", gArgs.cfgLayer)
        _ARGS_STR("-r", gArgs.resetType)
        _ARGS_BOOL("-u", gArgs.useUnknown, true)
        _ARGS_BOOL("-x", gArgs.extraInfo, true)
        _ARGS_BOOL("-a", gArgs.applyConfig, true)
        _ARGS_BOOL("-y", gArgs.outOverwrite, true)
        _ARGS_BOOL("-n", gArgs.noProbe, true)
        _ARGS_BOOL("-e", gArgs.doEpoch, true)
        _ARGS_BOOL("-U", gArgs.updateOnly, true)
        _ARGS_BOOL("-R", gArgs.allowReplace, true)
        else if (gArgs.cmd != NULL)
        {
            argOk = false;
        }
        else
        {
            for (int ix = 0; ix < NUMOF(kCmds); ix++)
            {
                if (strcmp(kCmds[ix].name, argv[argIx]) == 0)
                {
                    gArgs.cmd = &kCmds[ix];
                    break;
                }
            }
            if (gArgs.cmd == NULL)
            {
                argOk = false;
            }
        }

        if (!argOk)
        {
            WARNING("Illegal argument '%s'!", argv[argIx]);
            gArgs.cmd = NULL;
            doHelp = false;
            break;
        }
    }

    if (gArgs.cmd != NULL)
    {
        debugCfg.mark = gArgs.cmd->name;
    }
    debugSetup(&debugCfg);

    if (doHelp)
    {
        if (gArgs.cmd != NULL)
        {
            fputs(kTitleStr, stdout);
            if (gArgs.cmd->help != NULL)
            {
                fputs(gArgs.cmd->help(), stdout);
            }
            fputs(kGreeting, stdout);
        }
        else
        {
            printHelp(false);
        }
        exit(EXIT_SUCCESS);
    }

    bool res = true;
    if (gArgs.cmd == NULL)
    {
        WARNING("Try '%s -h'.", argv[0]);
        res = false;
    }

    // Open input file
    if (res && gArgs.cmd->need_i)
    {
        if ( (gArgs.inName == NULL) || ((gArgs.inName[0] == '-') && (gArgs.inName[1] == '\0')) )
        {
            gArgs.inName = "-";
            gArgs.inFile = stdin;
            DEBUG("input from stdin");
#ifndef _WIN32
            int fd = fileno(gArgs.inFile);
            const int flags = fcntl(fd, F_GETFL, 0);
            int r = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
            if ( ((flags < 0)) || (r < 0) )
            {
                WARNING("Failed setting stdin to non-blocking: %s", strerror(errno));
                res = false;
            }
#else
            // FIXME: for windows?
#endif
        }
        else if (gArgs.inName[0] == '\0')
        {
            WARNING("Need '-i <infile>' argument!");
            res = false;
        }
        else
        {
            DEBUG("Opening input: %s", gArgs.inName);
            gArgs.inFile = fopen(gArgs.inName, "r");
            if (gArgs.inFile == NULL)
            {
                WARNING("Failed opening '%s' for reading: %s!\n",
                    gArgs.inName, strerror(errno));
                res = false;
            }
        }
    }
    else if ( (gArgs.cmd != NULL) && !gArgs.cmd->need_i && (gArgs.inName != NULL) )
    {
        WARNING("Illegal argument '-i %s'!", gArgs.inName);
        res = false;
    }
    ioSetInput(gArgs.inName, gArgs.inFile);

    // Open output file
    if (res && gArgs.cmd->need_o)
    {
        if ( (gArgs.outName == NULL) || ((gArgs.outName[0] == '-') && (gArgs.outName[1] == '\0')) )
        {
            gArgs.outName = "-";
            gArgs.outFile = stdout;
            DEBUG("output to stdout");
        }
        else if (gArgs.outName[0] == '\0')
        {
            WARNING("Need '-o <infile>' argument!");
            res = false;
        }
        else
        {
            // Don't open now, ioWriteOutput() will do that
        }
    }
    ioSetOutput(gArgs.outName, gArgs.outFile, gArgs.outOverwrite);

    // Require -p arg?
    if ( (gArgs.cmd != NULL) && gArgs.cmd->need_p )
    {
        if ( (gArgs.rxPort == NULL) || (gArgs.rxPort[0] == '\0') )
        {
            WARNING("Need '-p <port>' argument!");
            res = false;
        }
    }
    else if ( (gArgs.cmd != NULL) && !gArgs.cmd->need_p && (gArgs.rxPort != NULL) )
    {
        WARNING("Illegal argument '-p %s'!", gArgs.rxPort);
        res = false;
    }

    // Require -l arg?
    if ((gArgs.cmd != NULL) && gArgs.cmd->need_l)
    {
        if ( (gArgs.cfgLayer == NULL) || (gArgs.cfgLayer[0] == '\0') )
        {
            WARNING("Need '-l <layer(s)>' argument!");
            res = false;
        }
    }
    else if ( (gArgs.cmd != NULL) && !gArgs.cmd->need_l && (gArgs.cfgLayer != NULL) )
    {
        WARNING("Illegal argument '-l %s'!", gArgs.cfgLayer);
        res = false;
    }

    // Require -r arg?
    if ( (gArgs.cmd != NULL) && gArgs.cmd->need_r )
    {
        if ( (gArgs.resetType == NULL) || (gArgs.resetType[0] == '\0') )
        {
            WARNING("Need '-r <mode>' argument!");
            res = false;
        }
    }
    else if ( (gArgs.cmd != NULL) && !(gArgs.cmd->need_r || gArgs.cmd->may_r) && (gArgs.resetType != NULL) )
    {
        WARNING("Illegal argument '-r %s'!", gArgs.resetType);
        res = false;
    }

    // May use -n arg?
    if ( (gArgs.cmd != NULL) && (!gArgs.cmd->may_n && gArgs.noProbe) )
    {
        WARNING("Illegal argument '-n'!");
        res = false;
    }

    // May use -e arg?
    if ( (gArgs.cmd != NULL) && (!gArgs.cmd->may_e && gArgs.doEpoch) )
    {
        WARNING("Illegal argument '-e'!");
        res = false;
    }

    // May use -u arg?
    if ( (gArgs.cmd != NULL) && (!gArgs.cmd->may_u && gArgs.updateOnly) )
    {
        WARNING("Illegal argument '-u'!");
        res = false;
    }

    // May use -U arg?
    if ( (gArgs.cmd != NULL) && (!gArgs.cmd->may_U && gArgs.allowReplace) )
    {
        WARNING("Illegal argument '-U'!");
        res = false;
    }

    // Are we happy with the arguments?
    if (!res)
    {
        exit(EXIT_BADARGS);
    }

    // Execute
    DEBUG("args: inName=%s outName=%s outOverwrite=%d rxPort=%s cfgLayer=%s useUnknown=%d extraInfo=%d applyConfig=%d noProbe=%d doEpoch=%d updateOnly=%d allowReplace=%d",
        gArgs.inName, gArgs.outName, gArgs.outOverwrite, gArgs.rxPort, gArgs.cfgLayer, gArgs.useUnknown, gArgs.extraInfo, gArgs.applyConfig, gArgs.noProbe, gArgs.doEpoch, gArgs.updateOnly, gArgs.allowReplace);

    const int exitCode = gArgs.cmd->run();

    return exitCode;
}

/* ****************************************************************************************************************** */
// eof
