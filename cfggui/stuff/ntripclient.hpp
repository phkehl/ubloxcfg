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

#ifndef __NTRIPCLIENT_HPP__
#define __NTRIPCLIENT_HPP__

#include <string>
#include <map>
#include <cstdint>

#include "ff_port.h"
#include "ff_parser.h"

/* ****************************************************************************************************************** */

class NtripClient
{
    public:

        NtripClient();
       ~NtripClient();

        bool Init(const std::string &spec); // [<user>:<pass>@]<host>:<port>/<mount>
        bool CanConnect();
        bool Connect();
        bool IsConnected();
        void Disconnect();

        bool GetData(PARSER_MSG_t *msg);

        const std::string &GetStatusStr();

        struct Pos
        {
            Pos() : valid{false}, lat{0}, lon{0}, alt{0}, numSv{0}, fix{false} {}
            bool   valid;
            double lat;
            double lon;
            double alt;
            int    numSv;
            bool   fix;
            static constexpr double MIN_LAT =    -90.0;
            static constexpr double MAX_LAT =     90.0;
            static constexpr double MIN_LON =   -180.0;
            static constexpr double MAX_LON =    180.0;
            static constexpr double MIN_ALT =      0.0;
            static constexpr double MAX_ALT =  10000.0;
            static constexpr int    MIN_NUMSV =    0;
            static constexpr int    MAX_NUMSV =   99;
        };
        void SetPos(const Pos &pos, const double interval);
        void SendPos();

        // Status
        struct MsgInfo
        {
            MsgInfo();
            uint32_t    count;
            uint32_t    ts;
            std::string info;
            uint32_t    bytes;
            float       msgRate;
            float       byteRate;
            void Add(const PARSER_MSG_t *msg);
            private:
                uint32_t statsDt[50];
                int      statsBytes[NUMOF(statsDt)];
                int      statsIx;
                uint32_t statsTs;
        };
        const std::map<std::string, MsgInfo> &GetInMsgInfos();
        const std::map<std::string, MsgInfo> &GetOutMsgInfos();

    private:

        enum State_e { STATE_NONE, STATE_INIT, STATE_CONNECTING, STATE_CONNECTED };
        State_e      _state;

        std::string  _spec;
        std::string  _host;
        int          _port;
        std::string  _mount;
        std::string  _auth;
        int          _version; // TODO: add [::v=<1|2>]] to spec...

        PORT_t       _tcpPort;
        PARSER_t     _parser;
        std::string  _status;
        std::string  _error;
        void         _Disconnect(const std::string &error);
        void         _SetState(const State_e state);

        std::map<std::string, MsgInfo> _inMsgInfos;
        std::map<std::string, MsgInfo> _outMsgInfos;

        Pos          _pos;
        uint32_t     _ggaInt;
        uint32_t     _ggaLast;
        bool         _ggaForce;
        uint8_t      _ggaMsg[256];
        char         _ggaInfo[256];
        PARSER_MSG_t _MakeGga();
};

/* ****************************************************************************************************************** */
#endif // __NTRIPCLIENT_HPP__
