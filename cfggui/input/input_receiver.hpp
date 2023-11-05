/* ************************************************************************************************/ // clang-format off
// flipflip's cfggui
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

#ifndef __INPUT_RECEIVER_HPP__
#define __INPUT_RECEIVER_HPP__

#include <memory>
#include <mutex>
#include <queue>
#include <atomic>
#include <vector>
#include <string>
#include <functional>

#include "ubloxcfg.h"
#include "ff_parser.h"
#include "ff_cpp.hpp"

#include "input.hpp"
#include "database.hpp"

/* ****************************************************************************************************************** */

struct ReceiverEvent;
struct ReceiverCommand;

class InputReceiver : public Input
{
    public:
        InputReceiver(const std::string &name, std::shared_ptr<Database> database);
       ~InputReceiver();

        bool Start(const std::string &port, const RX_OPTS_t &opts);
        void Stop();
        bool IsIdle();
        bool IsBusy();
        bool IsReady();

        void Send(const uint8_t *data, const int size);
        void Reset(const RX_RESET_t reset);
        using GetConfigCb_t = std::function<void(Ff::KeyVal)>;
        void GetConfig(const UBLOXCFG_LAYER_t layer, const std::vector<uint32_t> &_keys, GetConfigCb_t cb = nullptr);
        using SetConfigCb_t = std::function<void(const bool)>;
        void SetConfig(const bool ram, const bool bbr, const bool flash, const bool apply, const std::vector<UBLOXCFG_KEYVAL_t> &keys, SetConfigCb_t cb = nullptr);
        void SetBaudrate(const int baudrate);
        int GetBaudrate();

        void Loop(const double &now) final;

    protected:

    private:
        enum State_e { IDLE, BUSY, READY };

        void _SendCommand(std::unique_ptr<ReceiverCommand> command);

        // Shared between main thread and receiver thread
        std::queue< std::unique_ptr<ReceiverEvent> >
                             _eventQueue;
        std::mutex           _eventMutex;
        std::queue< std::unique_ptr<ReceiverCommand> > _commandQueue;
        std::mutex           _commandMutex;
        std::atomic<enum State_e> _state;
        std::atomic<int>     _baudrate;
        RX_OPTS_t            _rxOpts;
        std::string          _port;
        std::unique_ptr<Ff::Rx> _rx;

        GetConfigCb_t _getConfigCb;
        SetConfigCb_t _setConfigCb;

        // InputReceiver thread stuff
        bool                 _eventQueueSaturation;

        void _ThreadPrepare() final;
        void _Thread(Ff::Thread *thread) final;
        void _ThreadCleanup() final;

        static void _ReceiverMsgCb(PARSER_MSG_t *msg, void *arg);
        uint32_t _msgSeq;
        void _SendEvent(std::unique_ptr<ReceiverEvent> event);
};

/* ****************************************************************************************************************** */
#endif // __INPUT_RECEIVER_HPP__
