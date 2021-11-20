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

#include <chrono>
#include <mutex>
#include <queue>
#include <memory>
#include <cinttypes>
#include <cstring>
#include <cmath>

#ifndef _WIN32
#  include <sys/prctl.h>
#endif

#include "ubloxcfg.h"
#include "ff_debug.h"
#include "ff_rx.h"
#include "ff_port.h"
#include "ff_stuff.h"
#include "ff_utils.hpp"
#include "ff_cpp.hpp"
#include "ubloxcfg.h"

#include "platform.hpp"
#include "receiver.hpp"

/* ****************************************************************************************************************** */

// Events from the receiver (thread)
struct ReceiverEvent
{
    enum Event_e { NOOP, MSG, FAIL, ERROR, WARN, NOTICE, EPOCH, KEYVAL, ACK };
    ReceiverEvent(const enum Event_e _event, const uint64_t _uid) :
        event{_event}, uid{_uid}
    {
    }
    virtual ~ReceiverEvent()
    {
    }
    enum Event_e event;
    uint64_t     uid;
};

struct ReceiverEventMsg : public ReceiverEvent
{
    ReceiverEventMsg(const PARSER_MSG_t *_msg, const uint64_t _uid = 0) :
        ReceiverEvent(MSG, _uid), msg{ std::make_shared<Ff::ParserMsg>(_msg) }
    {
    }
    std::shared_ptr<Ff::ParserMsg> msg;
};

struct ReceiverEventFail : public ReceiverEvent
{
    ReceiverEventFail(const std::string &_str, const uint64_t _uid = 0) :
        ReceiverEvent(FAIL, _uid), str{_str}
    {
    }
    std::string str;
};

struct ReceiverEventError : public ReceiverEvent
{
    ReceiverEventError(const std::string &_str, const uint64_t _uid = 0) :
        ReceiverEvent(ERROR, _uid), str{_str}
    {
    }
    std::string str;
};

struct ReceiverEventWarn : public ReceiverEvent
{
    ReceiverEventWarn(const std::string &_str, const uint64_t _uid = 0) :
        ReceiverEvent(WARN, _uid), str{_str}
    {
    }
    std::string str;
};

struct ReceiverEventNotice : public ReceiverEvent
{
    ReceiverEventNotice(const std::string &_str, const uint64_t _uid = 0) :
        ReceiverEvent(NOTICE, _uid), str{_str}
    {
    }
    std::string str;
};

struct ReceiverEventEpoch : public ReceiverEvent
{
    ReceiverEventEpoch(const EPOCH_t *_epoch, const uint64_t _uid = 0) :
        ReceiverEvent(EPOCH, _uid), epoch{ std::make_shared<Ff::Epoch>(_epoch) }
    {
    }
    std::shared_ptr<Ff::Epoch> epoch;
};

struct ReceiverEventKeyval : public ReceiverEvent
{
    ReceiverEventKeyval(const UBLOXCFG_LAYER_t _layer, const UBLOXCFG_KEYVAL_t *_kv, const int _numKv, const uint64_t _uid = 0) :
        ReceiverEvent(KEYVAL, _uid), keyval{ std::make_shared<Ff::KeyVal>(_layer, _kv, _numKv) }
    {
    }
    std::shared_ptr<Ff::KeyVal> keyval;
};

struct ReceiverEventAck : public ReceiverEvent
{
    ReceiverEventAck(const bool _ack, const uint64_t _uid = 0) :
        ReceiverEvent(ACK, _uid), ack{_ack}
    {
    }
    bool ack;
};

// ---------------------------------------------------------------------------------------------------------------------

// Commands to the receiver (thread)
struct ReceiverCommand
{
    enum Command_e { NOOP = 0, STOP, START, BAUD, RESET, SEND, GETCONFIG, SETCONFIG };
    ReceiverCommand(const enum Command_e _command, const uint64_t _uid) :
        command{_command}, uid{_uid}
    {
    }
    virtual ~ReceiverCommand() {};
    enum Command_e command;
    uint64_t       uid;
};

struct ReceiverCommandStop : public ReceiverCommand
{
    ReceiverCommandStop(const uint64_t _uid = 0) :
        ReceiverCommand(STOP, _uid)
    {
    }
};

struct ReceiverCommandStart : public ReceiverCommand
{
    ReceiverCommandStart(const std::string &_port, const int _baudrate, const uint64_t _uid = 0) :
        ReceiverCommand(START, _uid), port{_port}, baudrate{_baudrate}
    {
    }
    std::string port;
    int         baudrate;
};

struct ReceiverCommandBaud : public ReceiverCommand
{
    ReceiverCommandBaud(const int _baudrate, const uint64_t _uid = 0) :
        ReceiverCommand(BAUD, _uid), baudrate{_baudrate}
    {
    };
    int baudrate;
};

struct ReceiverCommandReset : public ReceiverCommand
{
    ReceiverCommandReset(const RX_RESET_t _reset, const uint64_t _uid = 0) :
        ReceiverCommand(RESET, _uid), reset{_reset}
    {
    }
    RX_RESET_t reset;
};

struct ReceiverCommandSend : public ReceiverCommand
{
    ReceiverCommandSend(const uint8_t *_data, const int _size, const uint64_t _uid = 0) :
        ReceiverCommand(SEND, _uid), data{}
    {
        data.assign(_data, &_data[_size]);
    }
    std::vector<uint8_t> data;
};

struct ReceiverCommandGetconfig : public ReceiverCommand
{
    ReceiverCommandGetconfig(const UBLOXCFG_LAYER_t _layer, const std::vector<uint32_t> &_keys, const uint64_t _uid = 0) :
        ReceiverCommand(GETCONFIG, _uid), layer{_layer}, keys{_keys}
    {
    };
    UBLOXCFG_LAYER_t      layer;
    std::vector<uint32_t> keys;
};

struct ReceiverCommandSetconfig : public ReceiverCommand
{
    ReceiverCommandSetconfig(const bool _ram, const bool _bbr, const bool _flash, const bool _apply, const std::vector<UBLOXCFG_KEYVAL_t> &_kv, const uint64_t _uid = 0) :
        ReceiverCommand(SETCONFIG, _uid), ram{_ram}, bbr{_bbr}, flash{_flash}, apply{_apply}, kv{_kv}
    {
    };
    bool ram;
    bool bbr;
    bool flash;
    bool apply;
    std::vector<UBLOXCFG_KEYVAL_t> kv;
};

/* ****************************************************************************************************************** */

Receiver::Receiver(const std::string &name, std::shared_ptr<Database> database) :
    _name{name},
    _dataCb{nullptr},
    _state{IDLE},
    _baudrate{0},
    _eventQueueSaturation{false},
    _database{database}
{
    DEBUG("Receiver(%s)", _name.c_str());
    //throw std::runtime_error("hmmm");
}

// ---------------------------------------------------------------------------------------------------------------------

Receiver::~Receiver()
{
    DEBUG("~Receiver(%s)", _name.c_str());

    // Stop calling subscribers with data
    _dataCb = nullptr;

    // Abort if still connected
    Stop();
}

// ---------------------------------------------------------------------------------------------------------------------

void Receiver::SetDataCb(std::function<void(const Data &)> cb)
{
    _dataCb = cb;
}

// ---------------------------------------------------------------------------------------------------------------------

bool Receiver::Start(const std::string &port, const int baudrate)
{
    DEBUG("Receiver::Start(%s)", port.c_str());
    if ( (_state != IDLE) || (port.length() < 1) )
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

    // Queue command to connect to the receiver
    _SendCommand( std::make_unique<ReceiverCommandStart>(port, baudrate) );

    // Start new thread
    _thread = std::make_unique<std::thread>(&Receiver::_ReceiverThreadWrap, this);

    if (_thread)
    {
        if (_dataCb)
        {
            _dataCb( Data(Data::Type::EVENT_START) );
        }
        return true;
    }
    else
    {
        if (_dataCb)
        {
            _dataCb( Data(Data::Type::EVENT_STOP) );
        }
        return false;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void Receiver::Stop()
{
    DEBUG("Receiver::Stop()");

    if (_thread && (std::this_thread::get_id() != _thread->get_id()) )
    {
        _SendCommand( std::make_unique<ReceiverCommandStop>() );
        if ( (_state != IDLE) && _rx )
        {
            _rx->Abort();
        }
        _thread->join();
        _thread = nullptr;
    }

    if (_dataCb)
    {
        _dataCb( Data(Data::Type::EVENT_STOP) );
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool Receiver::IsIdle()
{
    return _state == IDLE;
}

// ---------------------------------------------------------------------------------------------------------------------

bool Receiver::IsReady()
{
    return _state == READY;
}

// ---------------------------------------------------------------------------------------------------------------------

bool Receiver::IsBusy()
{
    return _state == BUSY;
}

// ---------------------------------------------------------------------------------------------------------------------

void Receiver::Send(const uint8_t *data, const int size, const uint64_t uid)
{
    _SendCommand( std::make_unique<ReceiverCommandSend>(data, size, uid) );
}

// ---------------------------------------------------------------------------------------------------------------------

void Receiver::GetConfig(const UBLOXCFG_LAYER_t layer, const std::vector<uint32_t> &keys, const uint64_t uid)
{
    _SendCommand( std::make_unique<ReceiverCommandGetconfig>(layer, keys, uid) );
}

// ---------------------------------------------------------------------------------------------------------------------

void Receiver::SetConfig(const bool ram, const bool bbr, const bool flash, const bool apply, const std::vector<UBLOXCFG_KEYVAL_t> &kv, const uint64_t uid)
{
    _SendCommand( std::make_unique<ReceiverCommandSetconfig>(ram, bbr, flash, apply, kv, uid) );
}

// ---------------------------------------------------------------------------------------------------------------------

void Receiver::SetBaudrate(const int baudrate, const uint64_t uid)
{
    _SendCommand( std::make_unique<ReceiverCommandBaud>(baudrate, uid) );
}

// ---------------------------------------------------------------------------------------------------------------------

int Receiver::GetBaudrate()
{
    return _baudrate;
}

// ---------------------------------------------------------------------------------------------------------------------

void Receiver::Reset(const RX_RESET_t reset, const uint64_t uid)
{
    _SendCommand( std::make_unique<ReceiverCommandReset>(reset, uid) );
}

// ---------------------------------------------------------------------------------------------------------------------

std::unique_ptr<ReceiverEvent> Receiver::_GetEvent()
{
    std::lock_guard<std::mutex> lock(_eventMutex);
    if (_eventQueue.empty())
    {
        return nullptr;
    }
    auto event = std::move( _eventQueue.front() );
    _eventQueue.pop();
    return event;
}

// ---------------------------------------------------------------------------------------------------------------------

void Receiver::Loop(const double &now)
{
    (void)now;
    // TODO: limit max. number of events handled in main thread? Though there is a check for that in Receivers...
    while (true)
    {
        auto event = _GetEvent();

        // No further events
        if (!event)
        {
            break;
        }

        // Internal events
        switch (event->event)
        {
            case ReceiverEvent::FAIL:
            {
                Stop();
                break;
            }
            default:
                break;
        }

        // No callback to pass the event to
        if (!_dataCb)
        {
            continue;
        }

        switch (event->event)
        {
            case ReceiverEvent::NOOP:
                break;

            case ReceiverEvent::NOTICE:
            {
                auto e = static_cast<ReceiverEventWarn *>( event.get() );
                _dataCb( Data(Data::Type::INFO_NOTICE, e->str, e->uid) );
                break;
            }
            case ReceiverEvent::WARN:
            {
                auto e = static_cast<ReceiverEventWarn *>( event.get() );
                _dataCb( Data(Data::Type::INFO_WARN, e->str, e->uid) );
                break;
            }
            case ReceiverEvent::FAIL:
            case ReceiverEvent::ERROR:
            {
                auto e = static_cast<ReceiverEventWarn *>( event.get() );
                _dataCb( Data(Data::Type::INFO_ERROR, e->str, e->uid) );
                break;
            }
            case ReceiverEvent::MSG:
            {
                auto e = static_cast<ReceiverEventMsg *>( event.get() );
                _dataCb( Data(e->msg, e->uid) );
                break;
            }
            case ReceiverEvent::EPOCH:
            {
                auto e = static_cast<ReceiverEventEpoch *>( event.get() );
                _dataCb( Data(e->epoch, e->uid) );
                break;
            }
            case ReceiverEvent::KEYVAL:
            {
                auto e = static_cast<ReceiverEventKeyval *>( event.get() );
                _dataCb( Data(e->keyval, e->uid) );
                break;
            }
            case ReceiverEvent::ACK:
            {
                auto e = static_cast<ReceiverEventAck *>( event.get() );
                _dataCb( Data(e->ack, e->uid) );
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void Receiver::_SendCommand(std::unique_ptr<ReceiverCommand> command)
{
    std::lock_guard<std::mutex> lock(_commandMutex);
    _commandQueue.push( std::move(command) );
}

/* ****************************************************************************************************************** */

#define _THREAD_DEBUG(fmt, args...) DEBUG("thread %s " fmt, _name.c_str(), ## args)

// ---------------------------------------------------------------------------------------------------------------------

void Receiver::_ReceiverThreadWrap()
{
    Platform::SetThreadName(_name);
    _THREAD_DEBUG("started");

    _state = BUSY;

    try
    {
        _ReceiverThread();
    }
    catch (const std::exception &e)
    {
        ERROR("thread %s crashed: %s", _name.c_str(), e.what());
        Stop();
    }

    _state = IDLE;

    _THREAD_DEBUG("stopped");
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ void Receiver::_ReceiverMsgCb(PARSER_MSG_t *msg, void *arg)
{
    Receiver *rx = static_cast<Receiver *>(arg);
    msg->seq = ++rx->_msgSeq;
    rx->_SendEvent( std::make_unique<ReceiverEventMsg>(msg) );
}

// ---------------------------------------------------------------------------------------------------------------------

#define _SEND_EVENT(_cls_, ...) _SendEvent( std::make_unique<_cls_>(__VA_ARGS__) )

void Receiver::_ReceiverThread()
{

    bool abort = false;
    auto lastMsg = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    std::string port;
    while (!abort)
    {
        bool yield = true;

        // Pump receiver handle
        if (_rx != nullptr)
        {
            while (true)
            {
                PARSER_MSG_t *msg = rxGetNextMessage(_rx->rx);
                if (msg == NULL)
                {
                    break;
                }
                lastMsg = std::chrono::steady_clock::now();
                msg->seq = ++_msgSeq;
                if (epochCollect(&_epochColl, msg, &_epochRes))
                {
                    _database->AddEpoch(&_epochRes);
                    _SendEvent(std::make_unique<ReceiverEventEpoch>(&_epochRes));
                }
                _database->AddMsg(msg);
                _SEND_EVENT(ReceiverEventMsg, msg);
                yield = false;
            }
        }

        // Handle incoming commands
        while (auto command = _GetCommand())
        {
            switch (command->command)
            {
                case ReceiverCommand::NOOP:
                {
                    _THREAD_DEBUG("command NOOP");
                    break;
                }
                case ReceiverCommand::STOP:
                {
                    _state = BUSY;
                    _THREAD_DEBUG("command STOP");
                    abort = true;
                    break;
                }
                case ReceiverCommand::START:
                {
                    _state = BUSY;
                    auto cmd = static_cast<ReceiverCommandStart *>(command.get());
                    _THREAD_DEBUG("command START (%s, %d)", cmd->port.c_str(), cmd->baudrate);
                    port = cmd->port;
                    int baudrate = cmd->baudrate;
                    _SEND_EVENT(ReceiverEventNotice, "Connecting receiver (" + port + ", " + (baudrate > 0 ? Ff::Sprintf("baudrate %d", baudrate) : "autobaud") +")");
                    _msgSeq = 0;
                    RX_ARGS_t args = RX_ARGS_DEFAULT();
                    args.msgcb = _ReceiverMsgCb;
                    args.cbarg = this;

                    if (baudrate > 0)
                    {
                        args.autobaud = false;
                        args.detect   = false;
                    }
                    try
                    {
                        _rx = std::unique_ptr<Ff::Rx>( new Ff::Rx(port, &args) );
                        if (rxOpen(_rx->rx))
                        {
                            _state = READY;
                            // Set baudrate manually
                            if (baudrate > 0)
                            {
                                rxSetBaudrate(_rx->rx, baudrate);
                            }
                            baudrate = rxGetBaudrate(_rx->rx);
                            _baudrate = baudrate;
                            _SEND_EVENT(ReceiverEventNotice, "Receiver connected (" + port + ", baudrate " + Ff::Sprintf("%d", baudrate) + ")");
                        }
                        else
                        {
                            _SEND_EVENT(ReceiverEventFail, "Failed connecting receiver (" + port + ")!");
                            abort = true;
                        }
                    }
                    catch (std::exception &ex)
                    {
                        _SEND_EVENT(ReceiverEventFail, ex.what());
                        abort = true;
                    }
                    epochInit(&_epochColl);
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
                            _SEND_EVENT(ReceiverEventNotice, std::string {str}, cmd->uid);
                        }
                        else
                        {
                            char str[100];
                            snprintf(str, sizeof(str), "Failed setting baudrate to %d", baudrate);
                            _SEND_EVENT(ReceiverEventWarn, std::string {str}, cmd->uid);
                        }
                    }
                    else
                    {
                        if (rxAutobaud(_rx->rx))
                        {
                            baudrate = rxGetBaudrate(_rx->rx);
                            char str[100];
                            snprintf(str, sizeof(str), "Autobauded to %d", baudrate);
                            _SEND_EVENT(ReceiverEventNotice, std::string {str}, cmd->uid);
                        }
                        else
                        {
                            _SEND_EVENT(ReceiverEventWarn, "Autobaud failed!", cmd->uid);
                        }
                    }
                    _baudrate = baudrate;
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
                    _SEND_EVENT(ReceiverEventNotice, std::string(str), cmd->uid);
                    if (rxReset(_rx->rx, reset))
                    {
                        snprintf(str, sizeof(str), "Reset successful: %s", resetStr);
                        _SEND_EVENT(ReceiverEventNotice, std::string(str), cmd->uid);
                    }
                    else
                    {
                        snprintf(str, sizeof(str), "Reset failed: %s", resetStr);
                        _SEND_EVENT(ReceiverEventWarn, std::string(str), cmd->uid);
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
                    _THREAD_DEBUG("command SEND (%d)", (int)cmd->data.size());
                    rxSend(_rx->rx, cmd->data.data(), (int)cmd->data.size());
                    break;
                }
                case ReceiverCommand::GETCONFIG:
                {
                    auto cmd = static_cast<ReceiverCommandGetconfig *>(command.get());
                    _THREAD_DEBUG("command GETCONFIG (%s, %d, 0x%016" PRIx64 ")", ubloxcfg_layerName(cmd->layer), (int)cmd->keys.size(), cmd->uid);
                    _state = BUSY;
                    _SEND_EVENT(ReceiverEventNotice, "Getting configuration for layer " + std::string(ubloxcfg_layerName(cmd->layer)), cmd->uid);
                    UBLOXCFG_KEYVAL_t kv[5000];
                    const int numKv = rxGetConfig(_rx->rx, cmd->layer, cmd->keys.data(), cmd->keys.size(), kv, NUMOF(kv));
                    if (numKv >= 0) // Can be 0 for BBR or Flash layer!
                    {
                        _SEND_EVENT(ReceiverEventKeyval, cmd->layer, kv, numKv, cmd->uid);
                    }
                    else
                    {
                        _SEND_EVENT(ReceiverEventWarn, "Failed polling configuration!", cmd->uid);
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
                    _SEND_EVENT(ReceiverEventNotice, "Setting configuration (" + info + ")", cmd->uid);
                    bool ack = true;
                    if (rxSetConfig(_rx->rx, cmd->kv.data(), (int)cmd->kv.size(), cmd->ram, cmd->bbr, cmd->flash))
                    {
                        _SEND_EVENT(ReceiverEventNotice, "Configuration set (" + info + ")", cmd->uid);
                        if (cmd->apply)
                        {
                            _SEND_EVENT(ReceiverEventNotice, "Applying configuration", cmd->uid);
                            if (rxReset(_rx->rx, RX_RESET_SOFT))
                            {
                                _SEND_EVENT(ReceiverEventNotice, "Configuration applied", cmd->uid);
                            }
                            else
                            {
                                _SEND_EVENT(ReceiverEventWarn, "Failed applying configuration!", cmd->uid);
                                ack = false;
                            }
                        }
                    }
                    else
                    {
                        _SEND_EVENT(ReceiverEventWarn, "Failed setting configuration (" + info + ")!", cmd->uid);
                        ack = false;
                    }
                    _SEND_EVENT(ReceiverEventAck, ack, cmd->uid);
                    _state = READY;
                    break;
                }
            }
            yield = false;
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
                _SEND_EVENT(ReceiverEventWarn, "No data from receiver!");
                lastMsg = std::chrono::steady_clock::now();
            }

            // Yield
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    if (_rx)
    {
        _rx = nullptr; // destroy
        _baudrate = 0;
        _SEND_EVENT(ReceiverEventNotice,  "Receiver disconnected (" + port + ")");
    }
}

// ---------------------------------------------------------------------------------------------------------------------

std::unique_ptr<ReceiverCommand> Receiver::_GetCommand()
{
    std::lock_guard<std::mutex> lock(_eventMutex);
    if (_commandQueue.empty())
    {
        return nullptr;
    }
    auto command = std::move( _commandQueue.front() );
    _commandQueue.pop();
    return command;
}

// ---------------------------------------------------------------------------------------------------------------------

void Receiver::_SendEvent(std::unique_ptr<ReceiverEvent> event)
{
    std::lock_guard<std::mutex> lock(_eventMutex);

    // Detect if we're sending more events than the consumer (main thread) can handle
    auto size = _eventQueue.size();
    if (size > 1000)
    {
        if (!_eventQueueSaturation)
        {
            WARNING("Receiver thread %p event queue saturation. Dropping messages!", this); // TODO XXX use name
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
