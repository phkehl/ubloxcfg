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

#ifndef __RECEIVER_H__
#define __RECEIVER_H__

#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <functional>

#include "ubloxcfg.h"
#include "ff_parser.h"
#include "ff_epoch.h"
#include "ff_cpp.hpp"
#include "ff_rx.h"

#include "data.hpp"
#include "database.hpp"

/* ****************************************************************************************************************** */

struct ReceiverEvent;
struct ReceiverCommand;

class Receiver
{
    public:
        Receiver(const std::string &name, std::shared_ptr<Database> database);
       ~Receiver();

        bool                 Start(const std::string &port, const int baudrate = 0);
        bool                 Stop(const bool force = false);
        bool                 IsIdle();
        bool                 IsBusy();
        bool                 IsReady();

        void                 Send(const uint8_t *data, const int size, const uint64_t uid = 0);
        void                 Reset(const RX_RESET_t reset, const uint64_t uid = 0);
        void                 GetConfig(const UBLOXCFG_LAYER_t layer, const std::vector<uint32_t> &_keys, const uint64_t uid = 0);
        void                 SetConfig(const bool ram, const bool bbr, const bool flash, const bool apply, const std::vector<UBLOXCFG_KEYVAL_t> &keys, const uint64_t uid = 0);
        void                 SetBaudrate(const int baudrate, const uint64_t uid = 0);
        int                  GetBaudrate();

        void                 Loop(const double &now);

        void                 SetDataCb(std::function<void(const Data &)> cb);

        // Receiver thread only, but declared public because shared with c callback! SMELL
        uint32_t             _msgSeq;
        void                 _SendEvent(std::unique_ptr<ReceiverEvent> event);

    protected:

    private:
        enum State_e { IDLE, BUSY, READY };

        // Main thread stuff
        std::string          _name;
        std::thread         *_thread;

        std::function<void(const Data &)> _dataCb;

        void                 _SendCommand(std::unique_ptr<ReceiverCommand> command);
        std::unique_ptr<ReceiverEvent> _GetEvent();

        // Shared between main thread and receiver thread
        std::queue< std::unique_ptr<ReceiverEvent> >
                             _eventQueue;
        std::mutex           _eventMutex;
        std::queue< std::unique_ptr<ReceiverCommand> > _commandQueue;
        std::mutex           _commandMutex;
        std::atomic<enum State_e>
                             _state;
        std::atomic<int>     _baudrate;
        std::unique_ptr<Ff::Rx> _rx;

        // Receiver thread stuff
        bool                 _eventQueueSaturation;
        EPOCH_t              _epochColl;
        EPOCH_t              _epochRes;
        std::shared_ptr<Database> _database;
        void                 _ReceiverThreadWrap();
        void                 _ReceiverThread();
        std::unique_ptr<ReceiverCommand> _GetCommand();
};

/* ****************************************************************************************************************** */
#endif // __RECEIVER_H__
