/* ************************************************************************************************/ // clang-format off
// flipflip's cfggui
//
// Copyright (c) 2021 Philippe Kehl (flipflip at oinkzwurgl dot org),
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

#include <regex>
#include <cmath>

#include "base64.hpp"

#include "ff_stuff.h"
#include "ff_debug.h"
#include "ff_nmea.h"
#include "ff_utils.hpp"
#include "config.h"

#include "ntripclient.hpp"

/* ****************************************************************************************************************** */

NtripClient::NtripClient() :
    _port         { 0 },
    _version      { 1 },
    _tcpPort      { },
    _parser       { }
{
    _SetState(STATE_NONE);
}

NtripClient::~NtripClient()
{
    Disconnect();
}

#define NTRIP_DEBUG(fmt, args...) DEBUG(fmt, ## args)
//#define NTRIP_DEBUG(fmt, args...) /* nothing */

// ---------------------------------------------------------------------------------------------------------------------

const std::string &NtripClient::GetStatusStr()
{
    return _status;
}

// ---------------------------------------------------------------------------------------------------------------------

bool NtripClient::CanConnect()
{
    return _state == STATE_INIT;
}

// ---------------------------------------------------------------------------------------------------------------------

bool NtripClient::IsConnected()
{
    return (_state == STATE_CONNECTED) || (_state == STATE_CONNECTING);
}

// ---------------------------------------------------------------------------------------------------------------------

bool NtripClient::Init(const std::string &spec)
{
    if ( (_state != STATE_NONE) && (_state != STATE_INIT) )
    {
        return false;
    }

    _error.clear();
    _spec.clear();

    if (spec.empty())
    {
        _SetState(STATE_NONE);
        return true;
    }


    std::regex re("(?:(.+:.+)@)?([^:]+):([0-9]+)/(.+)$");
    std::smatch m;
    if ( !std::regex_match(spec, m, re) || (m.size() != (4 + 1)) )
    {
        return false;
    }

    _auth    = m[1].str();
    _host    = m[2].str();
    _port    = std::atoi(m[3].str().c_str());
    _mount   = m[4].str();
    _version = 1;
    if (_host.empty() || (_port < 1024) || (_port > 65534) || _mount.empty() || (_version != 1) )
    {
        _error = "bad spec";
        return false;
    }

    if (!_auth.empty())
    {
        _auth = Base64::Encode(_auth);
        // auth.pop_back();
        // auth.pop_back();
    }

    _spec = spec;
    _SetState(STATE_INIT);
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

bool NtripClient::Connect()
{
    if (!CanConnect())
    {
        return false;
    }

    _SetState(STATE_CONNECTING);

    _error.clear();
    _inMsgInfos.clear();
    _outMsgInfos.clear();
    _ggaLast = 0;
    _ggaForce = false;

    NTRIP_DEBUG("connect [%s] %d [%s] [%s]", _host.c_str(), _port, _mount.c_str(), _auth.c_str());

    parserInit(&_parser);

    // Connect port
    char spec[1000];
    snprintf(spec, sizeof(spec), "tcp://%s:%d", _host.c_str(), _port);
    NTRIP_DEBUG("Connect %s", spec);
    if (!portInit(&_tcpPort, spec) || !portOpen(&_tcpPort))
    {
        _Disconnect("port open");
        return false;
    }

    // https://www.use-snip.com/kb/knowledge-base/ntrip-rev1-versus-rev2-formats/
    //
    // NTRIP v1
    //   > GET /mountPt HTTP/1.0\r\n
    //   > User-Agent: NTRIP theSoftware/theRevision\r\n
    //   > Authorization: dXNlcjpwYXNzd29yZA==\r\n
    //   > \r\n\r\n
    //
    //   < ICY 200 OK\r\n
    //   < HTTP/1.0 401 Unauthorized\r\n
    //
    // Some v1 clients also send the Host: and a Connection: header.
    //
    // NTRIP v2
    //   > GET /mountPt HTTP/1.1\r\n
    //   > Host: theCaster.com:2101\r\n
    //   > Ntrip-Version: Ntrip/2.0\r\n
    //   > User-Agent: NTRIP theSoftware/theRevision\r\n
    //   > Authorization: dXNlcjpwYXNzd29yZA==\r\n
    //   > \r\n\r\n
    //
    //   < HTTP/1.1 200 OK\r\n
    //   < HTTP/1.1 401 Unauthorized

    // Send request
    char request[1000];
    int requestLen = 0;
    requestLen += snprintf(request, sizeof(request),
        "GET /%s HTTP/1.0\r\n"
        "Host: %s:%d\r\n"
        "User-Agent: NTRIP cfggui/" CONFIG_VERSION "\r\n"
        "Connection: close\r\n",
        _mount.c_str(), _host.c_str(), _port);
    if (!_auth.empty())
    {
        requestLen += snprintf(&request[requestLen], sizeof(request) - requestLen,
            "Authorization: Basic %s\r\n", _auth.c_str());
    }
    requestLen += snprintf(&request[requestLen], sizeof(request) - requestLen, "\r\n\r\n");

    NTRIP_DEBUG("Request: [%s]", request);
    if (!portWrite(&_tcpPort, (const uint8_t *)request, strlen(request)))
    {
        _Disconnect("write fail");
        return false;
    }

    // Get response
    const uint32_t timeout = TIME() + 5000;
    int offs = 0;
    uint8_t resp[1000];
    resp[0] = '\0';
    bool connected = false;
    while (TIME() < timeout)
    {
        int len = 0;
        if (!portRead(&_tcpPort, &resp[offs], sizeof(resp) - offs, &len))
        {
            _Disconnect("read fail");
            return false;
        }
        if (len > 0)
        {
            DEBUG_HEXDUMP(&resp[offs], len, "resp");
            offs += len;
            resp[offs] = '\0';
            char *crlf = strstr((char *)resp, "\r\n");
            // Have response
            if (crlf != NULL)
            {
                *crlf = '\0';
                NTRIP_DEBUG("response: %s", (const char *)resp);
                if (strcmp("ICY 200 OK", (const char *)resp) == 0)
                {
                    connected = true;
                }
                // else for example: "HTTP/1.0 401 Unauthorized"

                // Add remaining data to parser
                // TODO: Some servers send some more "\r\n" or other rubbish
                // const int rem = offs - strlen((const char *)resp) - 2;
                // if (rem > 0)
                // {
                //     parserAdd(&_parser, (const uint8_t *)&crlf[2], rem);
                // }
                break;
            }
        }
        else
        {
            SLEEP(100);
        }
    }
    if (connected)
    {
        _SetState(STATE_CONNECTED);
        return true;
    }
    else
    {
        _Disconnect(resp[0] == '\0' ? "timeout" : (const char *)resp);
        return false;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void NtripClient::_Disconnect(const std::string &error)
{
    if ( (_state == STATE_CONNECTING) || (_state == STATE_CONNECTED) )
    {
        _error = error;
        portClose(&_tcpPort);
        _SetState(STATE_INIT);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void NtripClient::Disconnect()
{
    if (_state == STATE_CONNECTED)
    {
        _Disconnect("");
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool NtripClient::GetData(PARSER_MSG_t *msg)
{
    if (_state != STATE_CONNECTED)
    {
        return false;
    }

    // Read more data from NTRIP caster
    while (true)
    {
        int len = 0;
        uint8_t buf[1024];
        if (!portRead(&_tcpPort, buf, sizeof(buf), &len))
        {
            _Disconnect("read fail");
            return false;
        }

        if (len > 0)
        {
            if (!parserAdd(&_parser, buf, len))
            {
                WARNING("Parser overflow!");
            }
        }
        else
        {
            break;
        }
    }

    // Send position to NTRIP caster
    if (_ggaForce || (_ggaInt != 0))
    {
        const uint32_t now = TIME();
        if ( _ggaForce || ((now - _ggaLast) >= _ggaInt) )
        {
            PARSER_MSG_t gga = _MakeGga();
            if (gga.size > 0)
            {
                if (_ggaForce)
                {
                    char str[200];
                    std::memcpy(str, gga.data, MIN((int)sizeof(str), gga.size));
                    str[gga.size - 2] = '\0';
                    DEBUG("manual send GGA: %s", str);
                }
                _outMsgInfos["GGA"].Add(&gga);

                if (!portWrite(&_tcpPort, gga.data, gga.size))
                {
                    _Disconnect("write fail");
                    return false;
                }
            }
            _ggaLast = now;
            _ggaForce = false;
        }
    }


    // Return next message received from NTRIP caster
    const bool res = parserProcess(&_parser, msg, true);
    if (res)
    {
        _inMsgInfos[msg->name].Add(msg);
    }

    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

void NtripClient::_SetState(const State_e state)
{
    _state = state;
    switch (_state)
    {
        case STATE_NONE:       _status = "Uninitialised"; break;
        case STATE_INIT:       _status = "Disconnected";  break;
        case STATE_CONNECTING: _status = "Connecting";    break;
        case STATE_CONNECTED:  _status = "Connected";     break;
    }
    if (!_error.empty())
    {
        _status += " (" + _error + ")";
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void NtripClient::SetPos(const Pos &pos, const double interval)
{
    _pos = pos;
    _pos.lat   = CLIP(_pos.lat, Pos::MIN_LAT, Pos::MAX_LAT);
    _pos.lon   = CLIP(_pos.lon, Pos::MIN_LON, Pos::MAX_LON);
    _pos.alt   = CLIP(_pos.alt, Pos::MIN_ALT, Pos::MAX_ALT);
    _pos.numSv = CLIP(_pos.numSv, Pos::MIN_NUMSV, Pos::MAX_NUMSV);

    const uint32_t ggaInt = interval * 1e3;
    _ggaInt = (ggaInt < 1000 ? 0 : CLIP(ggaInt, 1000, 60000));
}

// ---------------------------------------------------------------------------------------------------------------------

void NtripClient::SendPos()
{
    _ggaForce = true;
}

// ---------------------------------------------------------------------------------------------------------------------

PARSER_MSG_t NtripClient::_MakeGga()
{
    PARSER_MSG_t msg =
    {
        .type = PARSER_MSGTYPE_NMEA,
        .data = _ggaMsg,
        .size = 0,
        .seq  = 0,
        .ts   = TIME(),
        .src  = PARSER_MSGSRC_TO_RX,
        .name = "NMEA-GN-GGA",
        .info = _ggaInfo,
    };
    _ggaInfo[0] = '\0';

    if (!_pos.valid)
    {
        return msg;
    }
    const double lat = std::floor(std::abs(_pos.lat));
    const double lon = std::floor(std::abs(_pos.lon));
    std::string payload = Ff::Strftime("%H%M%S.00", 0, true) +
        Ff::Sprintf(",%02.0f%8.5f,%c,%03.0f%8.5f,%c,%c,%d,1.00,%.1f,M,0.0,M,,",
            lat, (std::abs(_pos.lat) - lat) * 60.0, _pos.lat < 0 ? 'S' : 'N',
            lon, (std::abs(_pos.lon) - lon) * 60.0, _pos.lon < 0 ? 'W' : 'E',
            _pos.fix ? '1' : '0', _pos.numSv, _pos.alt);

    msg.size = nmeaMakeMessage("GN", "GGA", payload.data(), (char *)msg.data);
    std::memcpy(_ggaInfo, _ggaMsg, msg.size - 2);
    _ggaInfo[msg.size - 2] = '\0';

    return msg;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::map<std::string, NtripClient::MsgInfo> &NtripClient::GetInMsgInfos()
{
    return _inMsgInfos;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::map<std::string, NtripClient::MsgInfo> &NtripClient::GetOutMsgInfos()
{
    return _outMsgInfos;
}

// ---------------------------------------------------------------------------------------------------------------------

NtripClient::MsgInfo::MsgInfo() :
    count       { 0 },
    ts          { 0 },
    bytes       { 0 },
    msgRate     { 0 },
    byteRate    { 0 },
    statsDt     { },
    statsBytes  { },
    statsIx     { 0 },
    statsTs     { 0 }
{
}

// ---------------------------------------------------------------------------------------------------------------------

void NtripClient::MsgInfo::Add(const PARSER_MSG_t *msg)
{
    if (!msg)
    {
        return;
    }

    count++;
    bytes += msg->size;

    if (msg->info)
    {
        info = msg->info;
    }
    else
    {
        info.clear();
    }

    if ( (msg->type != PARSER_MSGTYPE_RTCM3) && (msg->src != PARSER_MSGSRC_TO_RX) )
    {
        return;
    }

    ts = msg->ts;

    statsDt[statsIx]    = (statsTs != 0 ? (msg->ts - statsTs) : 0);
    statsTs = msg->ts;
    statsBytes[statsIx] = msg->size;
    statsIx++;
    statsIx %= NUMOF(statsDt);

    int n = 0;
    int s = 0;
    uint32_t dt = 0;
    for (int ix = 0; ix < NUMOF(statsDt); ix++)
    {
        if (statsDt[ix] != 0)
        {
            n++;
            dt += statsDt[ix];
            s += statsBytes[ix];
        }
    }
    if (dt != 0)
    {
        const float f = 1e3f / (float)dt;
        msgRate  = (float)n * f;
        byteRate = (float)s * f;
    }
}

/* ****************************************************************************************************************** */
