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

#include "ff_debug.h"

#include "platform.hpp"

#include "logfile.hpp"


/* ****************************************************************************************************************** */

// Events from the logfile (thread)
struct LogfileEvent
{
    enum Event_e { NOOP, MSG, FAIL, ERROR, WARN, NOTICE, EPOCH, ACK };
    LogfileEvent(const enum Event_e _event, const uint64_t _uid) :
        event{_event}, uid{_uid}
    {
    }
    virtual ~LogfileEvent()
    {
    }
    enum Event_e event;
    uint64_t     uid;
};

struct LogfileEventMsg : public LogfileEvent
{
    LogfileEventMsg(const PARSER_MSG_t *_msg, const uint64_t _uid = 0) :
        LogfileEvent(MSG, _uid), msg{ std::make_shared<Ff::ParserMsg>(_msg) }
    {
    }
    std::shared_ptr<Ff::ParserMsg> msg;
};

struct LogfileEventFail : public LogfileEvent
{
    LogfileEventFail(const std::string &_str, const uint64_t _uid = 0) :
        LogfileEvent(FAIL, _uid), str{_str}
    {
    }
    std::string str;
};

struct LogfileEventError : public LogfileEvent
{
    LogfileEventError(const std::string &_str, const uint64_t _uid = 0) :
        LogfileEvent(ERROR, _uid), str{_str}
    {
    }
    std::string str;
};

struct LogfileEventWarn : public LogfileEvent
{
    LogfileEventWarn(const std::string &_str, const uint64_t _uid = 0) :
        LogfileEvent(WARN, _uid), str{_str}
    {
    }
    std::string str;
};

struct LogfileEventNotice : public LogfileEvent
{
    LogfileEventNotice(const std::string &_str, const uint64_t _uid = 0) :
        LogfileEvent(NOTICE, _uid), str{_str}
    {
    }
    std::string str;
};

struct LogfileEventEpoch : public LogfileEvent
{
    LogfileEventEpoch(const EPOCH_t *_epoch, const uint64_t _uid = 0) :
        LogfileEvent(EPOCH, _uid), epoch{ std::make_shared<Ff::Epoch>(_epoch) }
    {
    }
    std::shared_ptr<Ff::Epoch> epoch;
};

struct LogfileEventAck : public LogfileEvent
{
    LogfileEventAck(const bool _ack, const uint64_t _uid = 0) :
        LogfileEvent(ACK, _uid), ack{_ack}
    {
    }
    bool ack;
};

// ---------------------------------------------------------------------------------------------------------------------

// Commands to the logfile (thread)
struct LogfileCommand
{
    enum Command_e { NOOP = 0, STOP, PAUSE, PLAY, STEP_EPOCH, STEP_MSG, SEEK };
    LogfileCommand(enum Command_e _command, uint64_t _uid) :
        command{_command}, uid{_uid}
    {
    }
    virtual ~LogfileCommand() {};
    enum Command_e command;
    uint64_t       uid;
};

struct LogfileCommandNoop : public LogfileCommand
{
    LogfileCommandNoop(const uint64_t _uid = 0) :
        LogfileCommand(NOOP, _uid)
    {
    }
};

struct LogfileCommandStop : public LogfileCommand
{
    LogfileCommandStop(const uint64_t _uid = 0) :
        LogfileCommand(STOP, _uid)
    {
    }
};

struct LogfileCommandPause : public LogfileCommand
{
    LogfileCommandPause(const uint64_t _uid = 0) :
        LogfileCommand(PAUSE, _uid)
    {
    }
};

struct LogfileCommandPlay : public LogfileCommand
{
    LogfileCommandPlay(const uint64_t _uid = 0) :
        LogfileCommand(PLAY, _uid)
    {
    }
};

struct LogfileCommandStepEpoch : public LogfileCommand
{
    LogfileCommandStepEpoch(const uint64_t _uid = 0) :
        LogfileCommand(STEP_EPOCH, _uid)
    {
    }
};

struct LogfileCommandStepMsg : public LogfileCommand
{
    LogfileCommandStepMsg(const std::string &_msgName, const uint64_t _uid = 0) :
        LogfileCommand(STEP_MSG, _uid), msgName{_msgName}
    {
    }
    std::string msgName;
};

struct LogfileCommandSeek : public LogfileCommand
{
    LogfileCommandSeek(const float _pos, const uint64_t _uid = 0) :
        LogfileCommand(SEEK, _uid), pos{_pos}
    {
    };
    float pos;
};


/* ****************************************************************************************************************** */

Logfile::Logfile(const std::string &name, std::shared_ptr<Database> database) :
    _name{name}, _database{database}, _dataCb{nullptr}, _playSpeed{1.0}, _playState{CLOSED}
{
    DEBUG("Logfile(%s)", _name.c_str());
}

// ---------------------------------------------------------------------------------------------------------------------

Logfile::~Logfile()
{
    DEBUG("~Logfile(%s)", _name.c_str());
    Close();
}

// ---------------------------------------------------------------------------------------------------------------------

void Logfile::SetDataCb(std::function<void(const Data &)> cb)
{
    _dataCb = cb;
}

// ---------------------------------------------------------------------------------------------------------------------

void Logfile::Loop(const double &now)
{
    (void)now;
    // TODO: limit max. number of events handled in main thread? Though there is a check for that in Receivers...
    while (true)
    {
        // Get next event from queue
        std::unique_ptr<LogfileEvent> event;
        if (!_eventQueue.empty())
        {
            std::lock_guard<std::mutex> lock(_eventMutex);
            event = std::move( _eventQueue.front() );
            _eventQueue.pop();
        }

        // No further events
        if (!event)
        {
            break;
        }

        // // Internal events
        // switch (event->event)
        // {
        //     case LogfileEvent::FAIL:
        //     {
        //         // TODO
        //         break;
        //     }
        //     default:
        //         break;
        // }

        // No callback to pass the event to
        if (!_dataCb)
        {
            continue;
        }

        switch (event->event)
        {
            case LogfileEvent::NOOP:
                break;

            case LogfileEvent::NOTICE:
            {
                auto e = static_cast<LogfileEventWarn *>( event.get() );
                _dataCb( Data(Data::Type::INFO_NOTICE, e->str, e->uid) );
                break;
            }
            case LogfileEvent::WARN:
            {
                auto e = static_cast<LogfileEventWarn *>( event.get() );
                _dataCb( Data(Data::Type::INFO_WARN, e->str, e->uid) );
                break;
            }
            case LogfileEvent::FAIL:
            case LogfileEvent::ERROR:
            {
                auto e = static_cast<LogfileEventWarn *>( event.get() );
                _dataCb( Data(Data::Type::INFO_ERROR, e->str, e->uid) );
                break;
            }
            case LogfileEvent::MSG:
            {
                auto e = static_cast<LogfileEventMsg *>( event.get() );
                _dataCb( Data(e->msg, e->uid) );
                break;
            }
            case LogfileEvent::EPOCH:
            {
                auto e = static_cast<LogfileEventEpoch *>( event.get() );
                _dataCb( Data(e->epoch, e->uid) );
                break;
            }
            case LogfileEvent::ACK:
            {
                auto e = static_cast<LogfileEventAck *>( event.get() );
                _dataCb( Data(e->ack, e->uid) );
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool Logfile::Open(const std::string &path)
{
    if (_logfileHandle)
    {
        Close();
    }

    try
    {
        _logfileHandle = std::make_unique<std::ifstream>();
        _logfileHandle->exceptions(std::ifstream::failbit | std::ifstream::badbit);
        _logfileHandle->open(path, std::ifstream::binary);
        _logfilePath = path;

        // Get file size
        _logfileHandle->seekg(0, std::ios::end);
        _playSize = _logfileHandle->tellg();

        // Go to beginning of log
        _logfileHandle->seekg(0, std::ios::beg);
        _playPos = 0;
        _playPosRel = 0.0;
        parserInit(&_playParser);

        // Hand over to play thread
        _playThreadAbort = false;
        _playThread = std::make_unique<std::thread>(&Logfile::_LogfileThreadWrap, this);
    }
    catch (std::exception &e)
    {
        WARNING("Failed opening logfile %s: %s", path.c_str(), e.what());
        _logfileHandle = nullptr;
        _logfilePath.clear();
    }

    return _logfileHandle != nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------

void Logfile::Close()
{
    if ( _playThread && (std::this_thread::get_id() != _playThread->get_id()) )
    {
        _playThreadCondVar.notify_all();
        _playThreadAbort = true;
        _playThread->join();
        _playThread = nullptr;
    }

    if (_logfileHandle)
    {
        _logfileHandle->close();
        _logfileHandle = nullptr;
        _logfilePath.clear();
    }

    _playSize   = 0;
    _playPos    = 0;
    _playPosRel = 0.0;
    _playState  = CLOSED;

    // Clear command queue
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        while (!_commandQueue.empty())
        {
            _commandQueue.pop();
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

float Logfile::GetPlayPos()
{
    return _playPosRel;
}

// ---------------------------------------------------------------------------------------------------------------------

float Logfile::GetPlaySpeed()
{
    return _playSpeed;
}

// ---------------------------------------------------------------------------------------------------------------------

void Logfile::SetPlaySpeed(const float speed)
{
    if ( (_playSpeed == PLAYSPEED_INF) || ( (_playSpeed >= PLAYSPEED_MIN) && (_playSpeed <= PLAYSPEED_MAX) ) )
    {
        _playSpeed = speed;
        if (_playState != CLOSED)
        {
            _playThreadCondVar.notify_all();
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool Logfile::CanOpen()
{
    return _playState == CLOSED;
}

bool Logfile::CanClose()
{
    return _playState != CLOSED;
}

bool Logfile::CanPlay()
{
    return (_playState == STOPPED) || (_playState == PAUSED);
}

bool Logfile::CanStop()
{
    return (_playState == PLAYING) || (_playState == PAUSED);
}

bool Logfile::CanPause()
{
    return (_playState == PLAYING) || (_playState == STOPPED);
}

bool Logfile::CanStep()
{
    return (_playState == PLAYING) || (_playState == PAUSED);
}

bool Logfile::CanSeek()
{
    return /*(_playState == PLAYING) ||*/ (_playState == PAUSED) || (_playState == STOPPED);
}

// ---------------------------------------------------------------------------------------------------------------------

#define _SEND_COMMAND(_cls_, ...) \
    { \
        std::lock_guard<std::mutex> lock(_commandMutex); \
        _commandQueue.emplace( std::make_unique<_cls_>(__VA_ARGS__) ); \
        _playThreadCondVar.notify_all(); \
    }

void Logfile::Play()
{
    if (CanPlay())
    {
        _SEND_COMMAND(LogfileCommandPlay);
    }
}

void Logfile::Stop()
{
    if (CanStop())
    {
        _SEND_COMMAND(LogfileCommandStop);
    }
}

void Logfile::Pause()
{
    if (CanPause())
    {
        _SEND_COMMAND(LogfileCommandPause);
    }
}

void Logfile::StepMsg(const std::string &msgName)
{
    if (CanStep())
    {
        _SEND_COMMAND(LogfileCommandStepMsg, msgName);
    }
}

void Logfile::StepEpoch()
{
    if (CanStep())
    {
        _SEND_COMMAND(LogfileCommandStepEpoch);
    }
}

void Logfile::Seek(const float pos)
{
    if (CanSeek())
    {
        _SEND_COMMAND(LogfileCommandSeek, pos);
    }
}

/* ****************************************************************************************************************** */

#define _SEND_EVENT(_cls_, ...) \
    { \
        std::lock_guard<std::mutex> lock(_eventMutex); \
        _eventQueue.emplace( std::make_unique<_cls_>(__VA_ARGS__) ); \
    }

#define _THREAD_DEBUG(fmt, args...) DEBUG("thread %s " fmt, _name.c_str(), ## args)

void Logfile::_LogfileThreadWrap()
{
    _playState = STOPPED;

    Platform::SetThreadName(_name);
    _THREAD_DEBUG("started");

    _SEND_EVENT(LogfileEventNotice, "Logfile opened: " + _logfilePath);

    try
    {
        _LogfileThread();
    }
    catch (const std::exception &e)
    {
        ERROR("thread %s crashed: %s", _name.c_str(), e.what());
        Close();
    }

    _playThreadAbort = true;
    _playState = STOPPED;

    _SEND_EVENT(LogfileEventNotice, "Logfile closed: " + _logfilePath);

    _THREAD_DEBUG("stopped");
}

void Logfile::_LogfileThread()
{
    PARSER_MSG_t msg;
    EPOCH_t coll;
    EPOCH_t epoch;
    epochInit(&coll);
    bool stepEpoch = false;
    bool stepMsg = false;
    std::string stepMsgName = "";


    while (!_playThreadAbort)
    {
        // Handle commands
        std::unique_ptr<LogfileCommand> command;
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
                case LogfileCommand::NOOP:
                    _THREAD_DEBUG("NOOP");
                    break;
                case LogfileCommand::STOP:
                    _THREAD_DEBUG("STOP");
                    _logfileHandle->seekg(0, std::ios::beg);
                    _playPos = 0;
                    _playPosRel = 0.0;
                    parserInit(&_playParser);
                    _playState = STOPPED;
                    break;
                case LogfileCommand::PAUSE:
                    _THREAD_DEBUG("PAUSE");
                    _playState = PAUSED;
                    break;
                case LogfileCommand::PLAY:
                    _THREAD_DEBUG("PLAY");
                    _playState = PLAYING;
                    break;
                case LogfileCommand::STEP_EPOCH:
                    stepEpoch = true;
                    _playState = PLAYING;
                    break;
                case LogfileCommand::STEP_MSG:
                {
                    auto cmd = static_cast<LogfileCommandStepMsg *>(command.get());
                    stepMsg = true;
                    stepMsgName = cmd->msgName;
                    _playState = PLAYING;
                    break;
                }
                case LogfileCommand::SEEK:
                {
                    auto cmd = static_cast<LogfileCommandSeek *>(command.get());
                    _THREAD_DEBUG("SEEK %.3f", cmd->pos);
                    float rel = cmd->pos * 1e-2;
                    if (rel < FLT_MIN)
                    {
                        _playPosRel = 0.0f;
                        _playPos = 0;
                    }
                    else
                    {
                        if (rel > (100.0 - FLT_EPSILON))
                        {
                            rel = 100.0f;
                        }
                        _playPosRel = rel;
                        _playPos = (double)_playSize * (double)_playPosRel;
                        const uint64_t endPos = _playSize - 1;
                        if (_playPos > endPos)
                        {
                            _playPos = endPos;
                        }
                    }
                    _logfileHandle->seekg(_playPos, std::ios::beg);
                    parserInit(&_playParser);
                    break;
                }
            }
            command = nullptr;
        }

        // Player idle
        if ( (_playState == STOPPED) || (_playState == PAUSED) )
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Process logfile data
        bool intr = false;
        while (!intr && parserProcess(&_playParser, &msg, true) && !_playThreadAbort && _commandQueue.empty())
        {
            _playPos += msg.size;
            _playPosRel = (double)_playPos / (double)_playSize;

            // Update database
            _database->AddMsg(&msg);

            // Collect epochs
            if (epochCollect(&coll, &msg, &epoch))
            {
                // Update database
                _database->AddEpoch(&epoch);

                // Send epoch
                _SEND_EVENT(LogfileEventEpoch, &epoch);

                // Pause now if we stepped here
                if (stepEpoch)
                {
                    stepEpoch = false;
                    _playState = PAUSED;
                    intr = true;
                    break;
                }

                // Throttle playback
                if ( (_playSpeed > 0.0) && !stepMsg && !stepEpoch )
                {
                    const uint32_t sleep = (1000.0 / _playSpeed);
                    //std::this_thread::sleep_for(std::chrono::milliseconds(sleep));

                    // Sleep, but wake up early in case we're notified
                    // FIXME: study this stuff further, this may not be fully right
                    std::unique_lock<std::mutex> lock(_playThreadMutex);
                    intr = _playThreadCondVar.wait_for(lock, std::chrono::milliseconds(sleep)) != std::cv_status::timeout;

                    // Message timestamp is no longer valid after sleeping
                    msg.ts = TIME();
                }
            }

            // Wait a bit if the event queue is too long, to not overwhelm the main thread
            while (!intr && (_eventQueue.size() > EVENT_QUEUE_MAX_SIZE))
            {
                std::unique_lock<std::mutex> lock(_playThreadMutex);
                intr = _playThreadCondVar.wait_for(lock, std::chrono::milliseconds(25)) != std::cv_status::timeout;
                //std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            // Send message
            _SEND_EVENT(LogfileEventMsg, &msg);

            if (stepMsg && (stepMsgName.empty() || (stepMsgName == msg.name)))
            {
                stepMsg = false;
                _playState = PAUSED;
                intr = true;
                break;
            }
        }

        // We've been interrupted (woken), probably new command is pending (or we should abort)
        if (intr)
        {
            continue;
        }

        // Get more logfile data
        uint8_t buf[PARSER_MAX_ANY_SIZE];
        const int num = _logfileHandle->readsome((char *)buf, sizeof(buf));
        if (num > 0)
        {
            if (!parserAdd(&_playParser, buf, num))
            {
                WARNING("thread %s parser overflow!", _name.c_str());
            }
        }
        else
        {
            _playState = PAUSED;
        }
    }
}

/* ****************************************************************************************************************** */
