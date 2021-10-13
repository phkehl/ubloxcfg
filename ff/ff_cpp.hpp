// flipflip's c++ stuff for flipflip's c stuff
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

#ifndef __FF_CPP_H__
#define __FF_CPP_H__

#include <string>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <map>
#include <fstream>

#include "ff_parser.h"
#include "ff_epoch.h"
#include "ff_rx.h"
#include "ff_stuff.h"

/* ****************************************************************************************************************** */

namespace Ff
{
    // PARSER_MSG_t
    struct ParserMsg
    {
        ParserMsg(const PARSER_MSG_t *_msg) :
            type{}, data{}, size{_msg->size}, seq{_msg->seq}, ts{_msg->ts}, name{}, info{}
        {
            switch (_msg->type)
            {
                case PARSER_MSGTYPE_UBX:     type = UBX;     typeStr = "UBX";     break;
                case PARSER_MSGTYPE_NMEA:    type = NMEA;    typeStr = "NMEA";    break;
                case PARSER_MSGTYPE_RTCM3:   type = RTCM3;   typeStr = "RTCM3";   break;
                case PARSER_MSGTYPE_GARBAGE: type = GARBAGE; typeStr = "GARBAGE"; break;
            }
            switch (_msg->src)
            {
                case PARSER_MSGSRC_UNKN:    src = UNKN;    srcStr = "UNKN";    break;
                case PARSER_MSGSRC_FROM_RX: src = FROM_RX; srcStr = "FROM_RX"; break;
                case PARSER_MSGSRC_TO_RX:   src = TO_RX;   srcStr = "TO_RX";   break;
                case PARSER_MSGSRC_VIRTUAL: src = VIRTUAL; srcStr = "VIRTUAL"; break;
                case PARSER_MSGSRC_USER:    src = USER;    srcStr = "USER";    break;
                case PARSER_MSGSRC_LOG:     src = LOG;     srcStr = "LOG";     break;
            }
            memcpy(data, _msg->data, size > PARSER_MAX_ANY_SIZE ? PARSER_MAX_ANY_SIZE : size);
            name = _msg->name;
            if (_msg->info != NULL)
            {
                info = _msg->info;
            }
        }

        enum Type_e { UBX, NMEA, RTCM3, GARBAGE };
        enum Type_e type;
        std::string typeStr;
        uint8_t     data[PARSER_MAX_ANY_SIZE];
        int         size;
        uint32_t    seq;
        uint32_t    ts;
        enum Src_e { UNKN, FROM_RX, TO_RX, VIRTUAL, USER, LOG };
        Src_e       src;
        std::string srcStr;
        std::string name;
        std::string info;
    };

    // EPOCH_t
    struct Epoch
    {
        Epoch(const EPOCH_t *_epoch) : seq{_epoch->seq}, str{_epoch->str}, epoch{*_epoch} { }
        uint32_t    seq;
        std::string str;
        EPOCH_t     epoch;
    };

    // RX_t
    struct Rx
    {
        Rx(const std::string &port, const RX_ARGS_t *args) :
            rx{NULL}
        {
            rx = rxInit(port.c_str(), args);
            if (rx == NULL)
            {
                throw std::runtime_error("rxInit() fail");
            }
        }
       ~Rx()
        {
            if (rx != NULL)
            {
                rxClose(rx);
                free(rx);
                rx = NULL;
            }
        }
        RX_t *rx;
        void Abort()
        {
            if (rx != NULL)
            {
                rxAbort(rx);
            }
        }
    };

    // UBLOXCFG_KEYVAL_t
    struct KeyVal
    {
        KeyVal(const UBLOXCFG_LAYER_t _layer, const UBLOXCFG_KEYVAL_t *_kv, const int numKv)
            : layer{_layer}, kv{}
        {
            if (numKv > 0)
            {
                kv.resize(numKv);
                memcpy(kv.data(), _kv, sizeof(*_kv) * kv.size());
            }
        }
        UBLOXCFG_LAYER_t               layer;
        std::vector<UBLOXCFG_KEYVAL_t> kv;
    };

    // sprintf()
    std::string Sprintf(const char * const zcFormat, ...) PRINTF_ATTR(1);

    // strftime()
    std::string Strftime(const int64_t ts, const char * const fmt);

    // String search & replace
    int StrReplace(std::string &str, const std::string &search, const std::string &replace);

    // Trim string
    void StrTrimLeft(std::string &str);
    void StrTrimRight(std::string &str);
    void StrTrim(std::string &str);

    // Split string
    std::vector<std::string> StrSplit(const std::string &str, const std::string &sep);

    // Join string
    std::string StrJoin(const std::vector<std::string> &strs, const std::string &sep);

    // 2d Vector
    template<typename T>
    struct Vec2
    {
        T                                     x, y;
        Vec2()                                { x = y = 0.0f; }
        Vec2(const T _x, const T _y)  { x = _x; y = _y; }
        Vec2 operator*(const T f)     const { return Vec2(x * f, y * f); }
        Vec2 operator*(const Vec2 &v) const { return Vec2(x * v.x, y * v.y); }
        Vec2 operator/(const T f)     const { return Vec2(x / f, y / f); }
        Vec2 operator/(const Vec2 &v) const { return Vec2(x / v.x, y / v.y); }
        Vec2 operator+(const Vec2 &v) const { return Vec2(x + v.x, y + v.y); }
        Vec2 operator-(const Vec2 &v) const { return Vec2(x - v.x, y - v.y); }
    };

    // Config file
    class ConfFile
    {
        public:
            ConfFile();
            void  Clear();
            bool  Load(const std::string &file, const std::string &section);
            bool  Save(const std::string &file, const std::string &section, const bool append = false);

            void  Set(const std::string &key, const std::string &val);
            void  Set(const std::string &key, const int32_t     val);
            void  Set(const std::string &key, const uint32_t    val, const bool hex = false);
            void  Set(const std::string &key, const double      val);
            void  Set(const std::string &key, const bool        val);

            bool  Get(const std::string &key, std::string &val);
            bool  Get(const std::string &key, int32_t     &val);
            bool  Get(const std::string &key, uint32_t    &val);
            bool  Get(const std::string &key, double      &val);
            bool  Get(const std::string &key, float       &val);
            bool  Get(const std::string &key, bool        &val);

        private:
            std::map<std::string, std::string> _keyVal;
            int         _lastLine;
            std::string _file;
    };
};

/* ****************************************************************************************************************** */

#endif // __FF_CPP_H__
