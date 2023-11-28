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

#include <chrono>
#include <mutex>
#include <queue>
#include <memory>
#include <cinttypes>
#include <cstring>
#include <cmath>
#include <utility>

#include "ubloxcfg.h"
#include "ff_debug.h"
#include "ff_rx.h"
#include "ff_port.h"
#include "ff_stuff.h"
#include "ff_ubx.h"
#include "ff_utils.hpp"
#include "ff_cpp.hpp"
#include "ubloxcfg.h"

#include "platform.hpp"
#include "input_receiver.hpp"

/* ****************************************************************************************************************** */

// Events from the receiver (thread)
struct ReceiverEvent
{
    enum Event_e { NOOP, MSG, EPOCH, ERROR, WARNING, NOTICE, GETCONFIG, SETCONFIG };
    ReceiverEvent(const enum Event_e _event) : event{_event} { }
    enum Event_e event;
};

struct ReceiverEventMsg : public ReceiverEvent
{
    ReceiverEventMsg(const PARSER_MSG_t *_msg) : ReceiverEvent(MSG), msg{_msg} { }
    Ff::ParserMsg msg;
};

struct ReceiverEventEpoch : public ReceiverEvent
{
    ReceiverEventEpoch(const EPOCH_t *_epoch) : ReceiverEvent(EPOCH), epoch{_epoch} { }
    Ff::Epoch epoch;
};

struct ReceiverEventError : public ReceiverEvent
{
    ReceiverEventError(const std::string &_str) : ReceiverEvent(ERROR), str{_str} { }
    ReceiverEventError(const char        *_str) : ReceiverEvent(ERROR), str{_str} { }
    std::string str;
};

struct ReceiverEventWarning : public ReceiverEvent
{
    ReceiverEventWarning(const std::string &_str) : ReceiverEvent(WARNING), str{_str} { }
    ReceiverEventWarning(const char        *_str) : ReceiverEvent(WARNING), str{_str} { }
    std::string str;
};

struct ReceiverEventNotice : public ReceiverEvent
{
    ReceiverEventNotice(const std::string &_str) : ReceiverEvent(NOTICE), str{_str} { }
    ReceiverEventNotice(const char        *_str) : ReceiverEvent(NOTICE), str{_str} { }
    std::string str;
};

struct ReceiverEventGetconfig : public ReceiverEvent
{
    ReceiverEventGetconfig(const UBLOXCFG_LAYER_t _layer, const UBLOXCFG_KEYVAL_t *_kv, const int _numKv, const uint64_t _uid) :
        ReceiverEvent(GETCONFIG), uid{_uid}, keyval{ Ff::KeyVal(_layer, _kv, _numKv) } { }
    uint64_t   uid;
    Ff::KeyVal keyval;
};

struct ReceiverEventSetconfig : public ReceiverEvent
{
    ReceiverEventSetconfig(const bool _ack, const uint64_t _uid) : ReceiverEvent(SETCONFIG), uid{_uid}, ack{_ack} { }
    uint64_t uid;
    bool     ack;
};

// ---------------------------------------------------------------------------------------------------------------------

// Commands to the receiver (thread)
struct ReceiverCommand
{
    enum Command_e { NOOP = 0, BAUD, RESET, SEND, GETCONFIG, SETCONFIG };
    ReceiverCommand(const enum Command_e _command) : command{_command} { }
    enum Command_e command;
};

struct ReceiverCommandBaud : public ReceiverCommand
{
    ReceiverCommandBaud(const int _baudrate) : ReceiverCommand(BAUD), baudrate{_baudrate} { }
    int baudrate;
};

struct ReceiverCommandReset : public ReceiverCommand
{
    ReceiverCommandReset(const RX_RESET_t _reset) : ReceiverCommand(RESET), reset{_reset} { }
    RX_RESET_t reset;
};

struct ReceiverCommandSend : public ReceiverCommand
{
    ReceiverCommandSend(const uint8_t *_data, const int _size) : ReceiverCommand(SEND), data{}
    {
        data.assign(_data, &_data[_size]);
    }
    std::vector<uint8_t> data;
};

struct ReceiverCommandGetconfig : public ReceiverCommand
{
    ReceiverCommandGetconfig(const UBLOXCFG_LAYER_t _layer, const std::vector<uint32_t> &_keys, const uint64_t _uid) :
        ReceiverCommand(GETCONFIG), uid{_uid}, layer{_layer}, keys{_keys} { }
    uint64_t              uid;
    UBLOXCFG_LAYER_t      layer;
    std::vector<uint32_t> keys;
};

struct ReceiverCommandSetconfig : public ReceiverCommand
{
    ReceiverCommandSetconfig(const bool _ram, const bool _bbr, const bool _flash, const bool _apply, const std::vector<UBLOXCFG_KEYVAL_t> &_kv, const uint64_t _uid) :
        ReceiverCommand(SETCONFIG), uid{_uid}, ram{_ram}, bbr{_bbr}, flash{_flash}, apply{_apply}, kv{_kv}
    {
    };
    uint64_t uid;
    bool     ram;
    bool     bbr;
    bool     flash;
    bool     apply;
    std::vector<UBLOXCFG_KEYVAL_t> kv;
};

/* ****************************************************************************************************************** */

InputReceiver::InputReceiver(const std::string &name, std::shared_ptr<Database> database) :
    Input(name, database),
    _state                 { IDLE },
    _baudrate              { 0 },
    _eventQueueSaturation  { false }
{
    DEBUG("InputReceiver(%s)", _inputName.c_str());
    _rxOpts = RX_OPTS_DEFAULT();
}

// ---------------------------------------------------------------------------------------------------------------------

InputReceiver::~InputReceiver()
{
    DEBUG("~InputReceiver(%s)", _inputName.c_str());
    Stop();
}

// ---------------------------------------------------------------------------------------------------------------------

bool InputReceiver::Start(const std::string &port, const RX_OPTS_t &opts)
{
    DEBUG("InputReceiver::Start(%s)", port.c_str());
    if ( (_state != IDLE) || port.empty() )
    {
        return false;
    }

    // Clear event and command queues
    {
        std::lock_guard<std::mutex> lock(_eventMutex);
        while (!_eventQueue.empty())
        {
            _eventQueue.pop();
        }
    }
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        while (!_commandQueue.empty())
        {
            _commandQueue.pop();
        }
    }

    _port     = port;
    _rxOpts = opts;
    return _ThreadStart();
}

// ---------------------------------------------------------------------------------------------------------------------

void InputReceiver::Stop()
{
    DEBUG("InputReceiver::Stop()");

    _ThreadStop();
}

// ---------------------------------------------------------------------------------------------------------------------

bool InputReceiver::IsIdle()
{
    return _state == IDLE;
}

// ---------------------------------------------------------------------------------------------------------------------

bool InputReceiver::IsReady()
{
    return _state == READY;
}

// ---------------------------------------------------------------------------------------------------------------------

bool InputReceiver::IsBusy()
{
    return _state == BUSY;
}

// ---------------------------------------------------------------------------------------------------------------------

void InputReceiver::Send(const uint8_t *data, const int size)
{
    _SendCommand( std::make_unique<ReceiverCommandSend>(data, size) );
}

// ---------------------------------------------------------------------------------------------------------------------

void InputReceiver::GetConfig(const UBLOXCFG_LAYER_t layer, const std::vector<uint32_t> &keys, InputReceiver::GetConfigCb_t cb)
{
    if (_getConfigCb)
    {
        WARNING("InputReceiver::GetConfig() operation pending!");
        return;
    }
    _getConfigCb = cb;
    _SendCommand( std::make_unique<ReceiverCommandGetconfig>(layer, keys,
        reinterpret_cast<std::uintptr_t>(std::addressof(_getConfigCb))) );
}

// ---------------------------------------------------------------------------------------------------------------------

void InputReceiver::SetConfig(const bool ram, const bool bbr, const bool flash, const bool apply, const std::vector<UBLOXCFG_KEYVAL_t> &kv, SetConfigCb_t cb)
{
    if (_setConfigCb)
    {
        WARNING("InputReceiver::SetConfig() operation pending!");
        return;
    }
    _setConfigCb = cb;
    _SendCommand( std::make_unique<ReceiverCommandSetconfig>(ram, bbr, flash, apply, kv,
        reinterpret_cast<std::uintptr_t>(std::addressof(_setConfigCb))) );
}

// ---------------------------------------------------------------------------------------------------------------------

void InputReceiver::SetBaudrate(const int baudrate)
{
    _SendCommand( std::make_unique<ReceiverCommandBaud>(baudrate) );
}

// ---------------------------------------------------------------------------------------------------------------------

int InputReceiver::GetBaudrate()
{
    return _baudrate;
}

// ---------------------------------------------------------------------------------------------------------------------

void InputReceiver::Reset(const RX_RESET_t reset)
{
    _SendCommand( std::make_unique<ReceiverCommandReset>(reset) );
}

// ---------------------------------------------------------------------------------------------------------------------

void InputReceiver::Loop(const double &now)
{
    (void)now;
    // TODO: limit max. number of events handled in main thread? Though there is a check for that in Receivers...
    while (!_eventQueue.empty())
    {
        std::unique_ptr<ReceiverEvent> event = nullptr;
        {
            std::lock_guard<std::mutex> lock(_eventMutex);
            event = std::move(_eventQueue.front());
            _eventQueue.pop();
        }

        // No further events
        if (!event)
        {
            break;
        }

        // No callback to pass the event to
        if (!_HaveDataCb())
        {
            continue;
        }

        switch (event->event)
        {
            case ReceiverEvent::NOOP:
                break;

            case ReceiverEvent::NOTICE:
            {
                auto e = static_cast<ReceiverEventNotice *>( event.get() );
                _CallDataCb( InputData(InputData::INFO_NOTICE, std::move(e->str)) );
                break;
            }
            case ReceiverEvent::WARNING:
            {
                auto e = static_cast<ReceiverEventWarning *>( event.get() );
                _CallDataCb( InputData(InputData::INFO_WARNING, std::move(e->str)) );
                break;
            }
            case ReceiverEvent::ERROR:
            {
                auto e = static_cast<ReceiverEventError *>( event.get() );
                _CallDataCb( InputData(InputData::INFO_ERROR, std::move(e->str)) );
                break;
            }
            case ReceiverEvent::MSG:
            {
                auto e = static_cast<ReceiverEventMsg *>( event.get() );
                _CallDataCb( InputData(std::move(e->msg)) );
                break;
            }
            case ReceiverEvent::EPOCH:
            {
                auto e = static_cast<ReceiverEventEpoch *>( event.get() );
                _CallDataCb( InputData(std::move(e->epoch)) );
                break;
            }
            case ReceiverEvent::GETCONFIG:
            {
                if (_getConfigCb)
                {
                    auto e = static_cast<ReceiverEventGetconfig *>( event.get() );
                    if (e->uid == reinterpret_cast<std::uintptr_t>(std::addressof(_getConfigCb)))
                    {
                        _getConfigCb(std::move(e->keyval));
                        _getConfigCb = nullptr;
                    }
                }
                break;
            }
            case ReceiverEvent::SETCONFIG:
            {
                if (_setConfigCb)
                {
                    auto e = static_cast<ReceiverEventSetconfig *>( event.get() );
                    if (e->uid == reinterpret_cast<std::uintptr_t>(std::addressof(_setConfigCb)))
                    {
                        _setConfigCb(e->ack);
                        _setConfigCb = nullptr;
                    }
                }
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void InputReceiver::_SendCommand(std::unique_ptr<ReceiverCommand> command)
{
    std::lock_guard<std::mutex> lock(_commandMutex);
    _commandQueue.push( std::move(command) );
}

/* ****************************************************************************************************************** */

#define _SEND_EVENT(_cls_, ...) _SendEvent( std::make_unique<_cls_>(__VA_ARGS__) )

#define _THREAD_DEBUG(fmt, args...) DEBUG("InputReceiver(%s) thread: " fmt, _inputName.c_str(), ## args)

void InputReceiver::_ThreadPrepare()
{
    _rx = nullptr;
    _state = BUSY;
}

void InputReceiver::_ThreadCleanup()
{
    if (_rx)
    {
        _SEND_EVENT(ReceiverEventNotice,  "InputReceiver disconnected (" + _port + ")");
    }
    _rx = nullptr; // destroy
    _baudrate = 0;
    _port.clear();
    _state = IDLE;
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ void InputReceiver::_ReceiverMsgCb(PARSER_MSG_t *msg, void *arg)
{
    InputReceiver *rx = static_cast<InputReceiver *>(arg);
    msg->seq = ++rx->_msgSeq;
    rx->_SendEvent( std::make_unique<ReceiverEventMsg>(msg) );
}

// ---------------------------------------------------------------------------------------------------------------------

void InputReceiver::_Thread(Ff::Thread *thread)
{
    EPOCH_t coll;
    EPOCH_t epoch;
    epochInit(&coll);

    // Open connection to receiver, abort thread if that fails..
    {
        bool connected = false;
        _state = BUSY;
        _msgSeq = 0;
        _SEND_EVENT(ReceiverEventNotice, "Connecting receiver (" + _port + ")");

        _rxOpts.msgcb = _ReceiverMsgCb;
        _rxOpts.cbarg = this;

        try
        {
            _rx = std::make_unique<Ff::Rx>(_port, &_rxOpts);
            if (rxOpen(_rx->rx))
            {
                _state = READY;
                _baudrate = rxGetBaudrate(_rx->rx);
                _SEND_EVENT(ReceiverEventNotice, "InputReceiver connected (" + _port + ", baudrate " +
                    Ff::Sprintf("%d", (int)_baudrate) + ")");
                connected = true;
            }
            else
            {
                _SEND_EVENT(ReceiverEventError, "Failed connecting receiver (" + _port + ")!");
            }
        }
        catch (std::exception &ex)
        {
            _SEND_EVENT(ReceiverEventError, ex.what());
        }
        if (!connected)
        {
            return;
        }
    }

    // Keep going...
    bool abort = false;
    auto lastMsg = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!abort && !thread->ShouldAbort())
    {
        bool yield = true;

        // Pump receiver handle
        while (true)
        {
            PARSER_MSG_t *msg = rxGetNextMessage(_rx->rx);
            if (msg == NULL)
            {
                break;
            }
            lastMsg = std::chrono::steady_clock::now();
            msg->seq = ++_msgSeq;
            if (epochCollect(&coll, msg, &epoch))
            {
                _latestRow = _inputDatabase->AddEpoch(epoch, true);
                _SendEvent(std::make_unique<ReceiverEventEpoch>(&epoch));
            }
            _SEND_EVENT(ReceiverEventMsg, msg);

            switch (msg->type)
            {
                case PARSER_MSGTYPE_UBX:
                    switch (UBX_CLSID(msg->data))
                    {
                        case UBX_INF_CLSID:
                            switch (UBX_MSGID(msg->data))
                            {
                                case UBX_INF_WARNING_MSGID:
                                    _SEND_EVENT(ReceiverEventWarning, msg->info);
                                    break;
                                case UBX_INF_ERROR_MSGID:
                                    _SEND_EVENT(ReceiverEventError, msg->info);
                                    break;
                            }
                            break;
                    }
                    break;
                case PARSER_MSGTYPE_NMEA:
                    if ( (msg->data[3] == 'T') && (msg->data[4] == 'X') && (msg->data[5] == 'T') && (msg->data[13] == '0') )
                    {
                        switch (msg->data[14])
                        {
                            case '0':
                                _SEND_EVENT(ReceiverEventError, msg->info);
                                break;
                            case '1':
                                _SEND_EVENT(ReceiverEventWarning, msg->info);
                                break;
                        }
                    }
                    break;
                default:
                    break;
            }

            yield = false;
        }

        // Handle incoming commands

        std::unique_ptr<ReceiverCommand> command;
        if (!_commandQueue.empty())
        {
            std::lock_guard<std::mutex> lock(_eventMutex);
            command = std::move( _commandQueue.front() );
            _commandQueue.pop();
        }
        if (command)
        {
            switch (command->command)
            {
                case ReceiverCommand::NOOP:
                {
                    _THREAD_DEBUG("command NOOP");
                    break;
                }
                case ReceiverCommand::BAUD:
                {
                    _state = BUSY;
                    auto cmd = static_cast<ReceiverCommandBaud *>(command.get());
                    int baudrate = cmd->baudrate;
                    _THREAD_DEBUG("command BAUD (%d)", baudrate);
                    if (baudrate > 0)
                    {
                        if (rxSetBaudrate(_rx->rx, baudrate))
                        {
                            char str[100];
                            snprintf(str, sizeof(str), "Baudrate set to %d", baudrate);
                            _SEND_EVENT(ReceiverEventNotice, std::string {str});
                        }
                        else
                        {
                            char str[100];
                            snprintf(str, sizeof(str), "Failed setting baudrate to %d", baudrate);
                            _SEND_EVENT(ReceiverEventWarning, std::string {str});
                            baudrate = rxGetBaudrate(_rx->rx);
                        }
                    }
                    else
                    {
                        if (rxAutobaud(_rx->rx))
                        {
                            baudrate = rxGetBaudrate(_rx->rx);
                            char str[100];
                            snprintf(str, sizeof(str), "Autobaud to %d", baudrate);
                            _SEND_EVENT(ReceiverEventNotice, std::string {str});
                        }
                        else
                        {
                            _SEND_EVENT(ReceiverEventWarning, "Autobaud failed!");
                            baudrate = rxGetBaudrate(_rx->rx);
                        }
                    }
                    _baudrate = baudrate;
                    if (_state == BUSY)
                    {
                        _state = READY;
                    }
                    break;
                }
                case ReceiverCommand::RESET:
                {
                    _state = BUSY;
                    auto cmd = static_cast<ReceiverCommandReset *>(command.get());
                    const RX_RESET_t reset = cmd->reset;
                    _THREAD_DEBUG("command RESET (%d)", reset);
                    char str[100];
                    const char *resetStr = rxResetStr(reset);
                    snprintf(str, sizeof(str), "Resetting: %s", resetStr);
                    _SEND_EVENT(ReceiverEventNotice, std::string(str));
                    if (rxReset(_rx->rx, reset))
                    {
                        snprintf(str, sizeof(str), "Reset successful: %s", resetStr);
                        _SEND_EVENT(ReceiverEventNotice, std::string(str));
                    }
                    else
                    {
                        snprintf(str, sizeof(str), "Reset failed: %s", resetStr);
                        _SEND_EVENT(ReceiverEventWarning, std::string(str));
                        // FIXME: abort = true and ReceiverEventFail?
                    }
                    if (_state == BUSY)
                    {
                        _state = READY;
                    }
                    break;
                }
                case ReceiverCommand::SEND:
                {
                    auto cmd = static_cast<ReceiverCommandSend *>(command.get());
                    //_THREAD_DEBUG("command SEND (%d)", (int)cmd->data.size());
                    rxSend(_rx->rx, cmd->data.data(), (int)cmd->data.size());
                    break;
                }
                case ReceiverCommand::GETCONFIG:
                {
                    auto cmd = static_cast<ReceiverCommandGetconfig *>(command.get());
                    _THREAD_DEBUG("command GETCONFIG (%s, %d, 0x%016" PRIx64 ")", ubloxcfg_layerName(cmd->layer), (int)cmd->keys.size(), cmd->uid);
                    _state = BUSY;
                    _SEND_EVENT(ReceiverEventNotice, "Getting configuration for layer " + std::string(ubloxcfg_layerName(cmd->layer)));
                    UBLOXCFG_KEYVAL_t kv[5000];
                    const int numKv = rxGetConfig(_rx->rx, cmd->layer, cmd->keys.data(), cmd->keys.size(), kv, NUMOF(kv));
                    if (numKv >= 0) // Can be 0 for BBR or Flash layer!
                    {
                        _SEND_EVENT(ReceiverEventGetconfig, cmd->layer, kv, numKv, cmd->uid);
                    }
                    else
                    {
                        _SEND_EVENT(ReceiverEventWarning, "Failed polling configuration!");
                        _SEND_EVENT(ReceiverEventGetconfig, cmd->layer, kv, 0, cmd->uid);
                    }
                    _state = READY;
                    break;
                }
                case ReceiverCommand::SETCONFIG:
                {
                    auto cmd = static_cast<ReceiverCommandSetconfig *>(command.get());
                    _THREAD_DEBUG("command SETCONFIG (%d, %d, %d, %d, %d)", cmd->ram, cmd->bbr, cmd->flash, cmd->apply, (int)cmd->kv.size());
                    _state = BUSY;
                    std::string layers = (cmd->ram   ? std::string(",RAM")   : std::string("")) +
                                         (cmd->bbr   ? std::string(",BBR")   : std::string("")) +
                                         (cmd->flash ? std::string(",FLASH") : std::string(""));
                    std::string info = Ff::Sprintf("%d values, layers %s", (int)cmd->kv.size(), layers.substr(1).c_str());
                    _SEND_EVENT(ReceiverEventNotice, "Setting configuration (" + info + ")");
                    bool ack = true;
                    if (rxSetConfig(_rx->rx, cmd->kv.data(), (int)cmd->kv.size(), cmd->ram, cmd->bbr, cmd->flash))
                    {
                        _SEND_EVENT(ReceiverEventNotice, "Configuration set (" + info + ")");
                        if (cmd->apply)
                        {
                            _SEND_EVENT(ReceiverEventNotice, "Applying configuration");
                            if (rxReset(_rx->rx, RX_RESET_SOFT))
                            {
                                _SEND_EVENT(ReceiverEventNotice, "Configuration applied");
                            }
                            else
                            {
                                _SEND_EVENT(ReceiverEventWarning, "Failed applying configuration!");
                                ack = false;
                            }
                        }
                    }
                    else
                    {
                        _SEND_EVENT(ReceiverEventWarning, "Failed setting configuration (" + info + ")!");
                        ack = false;
                    }
                    _SEND_EVENT(ReceiverEventSetconfig, ack, cmd->uid);
                    _state = READY;
                    break;
                }
            }
            yield = false;
            command = nullptr;
        }

        // Abort thread?
        if (abort)
        {
            break;
        }

        if (yield)
        {
            // Complain from time to time if there is no data for too long
            auto now = std::chrono::steady_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMsg);
            const std::chrono::milliseconds thrs(5000);
            if ( _rx && (diff > thrs) )
            {
                _SEND_EVENT(ReceiverEventWarning, "No data from receiver!");
                lastMsg = std::chrono::steady_clock::now();
            }

            // Yield
            thread->Sleep(10);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void InputReceiver::_SendEvent(std::unique_ptr<ReceiverEvent> event)
{
    std::lock_guard<std::mutex> lock(_eventMutex);

    // Detect if we're sending more events than the consumer (main thread) can handle
    auto size = _eventQueue.size();
    if (size > 1000)
    {
        if (!_eventQueueSaturation)
        {
            WARNING("InputReceiver(%s) thread: event queue saturation. Dropping messages!", _inputName.c_str());
            _eventQueue.push(std::make_unique<ReceiverEventWarning>("Event queue saturation. Dropping messages!"));

        }
        _eventQueueSaturation = true;
    }
    else if (_eventQueueSaturation && (size < 10))
    {
        _THREAD_DEBUG("Resuming queuing message.");
        _eventQueueSaturation = false;
    }

    // Skip MSG events if queue is saturated, always send all other events
    if (!_eventQueueSaturation || (event->event != ReceiverEvent::MSG))
    {
        _eventQueue.push( std::move(event) );
    }
}

/* ****************************************************************************************************************** */
