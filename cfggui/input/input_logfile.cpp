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

#include <cfloat>

#include "ff_debug.h"
#include "ff_ubx.h"

#include "platform.hpp"

#include "input_logfile.hpp"

/* ****************************************************************************************************************** */

// Events from the logfile (thread)
struct LogfileEvent
{
    enum Event_e { NOOP, MSG, EPOCH, ERROR, WARNING, NOTICE };
    LogfileEvent(const enum Event_e _event) : event{_event} { }
    enum Event_e event;
};

struct LogfileEventMsg : public LogfileEvent
{
    LogfileEventMsg(const PARSER_MSG_t *_msg) : LogfileEvent(MSG), msg{_msg} { }
    Ff::ParserMsg msg;
};

struct LogfileEventEpoch : public LogfileEvent
{
    LogfileEventEpoch(const EPOCH_t *_epoch) : LogfileEvent(EPOCH), epoch{_epoch} { }
    Ff::Epoch epoch;
};

struct LogfileEventError : public LogfileEvent
{
    LogfileEventError(const std::string &_str) : LogfileEvent(ERROR), str{_str} { }
    LogfileEventError(const char        *_str) : LogfileEvent(ERROR), str{_str} { }
    std::string str;
};

struct LogfileEventWarning : public LogfileEvent
{
    LogfileEventWarning(const std::string &_str) : LogfileEvent(WARNING), str{_str} { }
    LogfileEventWarning(const char        *_str) : LogfileEvent(WARNING), str{_str} { }
    std::string str;
};

struct LogfileEventNotice : public LogfileEvent
{
    LogfileEventNotice(const std::string &_str) : LogfileEvent(NOTICE), str{_str} { }
    LogfileEventNotice(const char        *_str) : LogfileEvent(NOTICE), str{_str} { }
    std::string str;
};

// ---------------------------------------------------------------------------------------------------------------------

// Commands to the logfile (thread)
struct LogfileCommand
{
    enum Command_e { NOOP = 0, STOP, PAUSE, PLAY, STEP_EPOCH, STEP_MSG, SEEK };
    LogfileCommand(enum Command_e _command) : command{_command} { }
    enum Command_e command;
};

struct LogfileCommandNoop : public LogfileCommand
{
    LogfileCommandNoop() : LogfileCommand(NOOP) { }
};

struct LogfileCommandStop : public LogfileCommand
{
    LogfileCommandStop() : LogfileCommand(STOP) { }
};

struct LogfileCommandPause : public LogfileCommand
{
    LogfileCommandPause() : LogfileCommand(PAUSE) { }
};

struct LogfileCommandPlay : public LogfileCommand
{
    LogfileCommandPlay() : LogfileCommand(PLAY) { }
};

struct LogfileCommandStepEpoch : public LogfileCommand
{
    LogfileCommandStepEpoch() : LogfileCommand(STEP_EPOCH) { }
};

struct LogfileCommandStepMsg : public LogfileCommand
{
    LogfileCommandStepMsg(const std::string &_msgName) : LogfileCommand(STEP_MSG), msgName{_msgName} { }
    std::string msgName;
};

struct LogfileCommandSeek : public LogfileCommand
{
    LogfileCommandSeek(const float _pos) : LogfileCommand(SEEK), pos{_pos} { }
    float pos;
};

/* ****************************************************************************************************************** */

InputLogfile::InputLogfile(const std::string &name, std::shared_ptr<Database> database) :
    Input(name, database),
    _playSpeed { 1.0 },
    _playState{ CLOSED }
{
    DEBUG("InputLogfile(%s)", _inputName.c_str());
}

// ---------------------------------------------------------------------------------------------------------------------

InputLogfile::~InputLogfile()
{
    DEBUG("~InputLogfile(%s)", _inputName.c_str());
    Close();
}

// ---------------------------------------------------------------------------------------------------------------------

void InputLogfile::Loop(const double &now)
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

        // No callback to pass the event to
        if (!_HaveDataCb())
        {
            continue;
        }

        switch (event->event)
        {
            case LogfileEvent::NOOP:
                break;

            case LogfileEvent::NOTICE:
            {
                auto e = static_cast<LogfileEventNotice *>( event.get() );
                _CallDataCb( InputData(InputData::INFO_NOTICE, std::move(e->str)) );
                break;
            }
            case LogfileEvent::WARNING:
            {
                auto e = static_cast<LogfileEventWarning *>( event.get() );
                _CallDataCb( InputData(InputData::INFO_WARNING, std::move(e->str)) );
                break;
            }
            case LogfileEvent::ERROR:
            {
                auto e = static_cast<LogfileEventError *>( event.get() );
                _CallDataCb( InputData(InputData::INFO_ERROR, std::move(e->str)) );
                break;
            }
            case LogfileEvent::MSG:
            {
                auto e = static_cast<LogfileEventMsg *>( event.get() );
                _CallDataCb( InputData(std::move(e->msg)) );
                break;
            }
            case LogfileEvent::EPOCH:
            {
                auto e = static_cast<LogfileEventEpoch *>( event.get() );
                _CallDataCb( InputData(std::move(e->epoch)) );
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool InputLogfile::Open(const std::string &path)
{
    if (_logfile.IsOpen())
    {
        Close();
    }

    if (_logfile.OpenRead(path))
    {
        _playSize = _logfile.Size();
        _playPos = 0;
        _playPosRel = 0.0;

        // Hand over to play thread
        return _ThreadStart();
    }
    else
    {
        WARNING("Failed opening logfile %s: %s", path.c_str(), _logfile.GetError().c_str());
        return false;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void InputLogfile::Close()
{
    _ThreadStop();

    if (_logfile.IsOpen())
    {
        _logfile.Close();
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

float InputLogfile::GetPlayPos()
{
    return _playPosRel;
}

// ---------------------------------------------------------------------------------------------------------------------

float InputLogfile::GetPlaySpeed()
{
    return _playSpeed;
}

// ---------------------------------------------------------------------------------------------------------------------

void InputLogfile::SetPlaySpeed(const float speed)
{
    if ( (_playSpeed == PLAYSPEED_INF) || ( (_playSpeed >= PLAYSPEED_MIN) && (_playSpeed <= PLAYSPEED_MAX) ) )
    {
        _playSpeed = speed;
        if (_playState != CLOSED)
        {
            _ThreadWakeup();
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool InputLogfile::CanOpen()
{
    return _playState == CLOSED;
}

bool InputLogfile::CanClose()
{
    return _playState != CLOSED;
}

bool InputLogfile::CanPlay()
{
    return (_playState == STOPPED) || (_playState == PAUSED);
}

bool InputLogfile::CanStop()
{
    return (_playState == PLAYING) || (_playState == PAUSED);
}

bool InputLogfile::CanPause()
{
    return (_playState == PLAYING); // || (_playState == STOPPED);
}

bool InputLogfile::CanStep()
{
    return (_playState == PLAYING) || (_playState == PAUSED) || (_playState == STOPPED);
}

bool InputLogfile::CanSeek()
{
    return (/*(_playState == PLAYING) ||*/ (_playState == PAUSED) || (_playState == STOPPED)) && _logfile.CanSeek();
}

const char *InputLogfile::StateStr()
{
    switch (_playState)
    {
        case CLOSED:  return "Closed";
        case STOPPED: return "Stopped";
        case PLAYING: return "Playing";
        case PAUSED:  return "Paused";
    }
    return "?";
}

// ---------------------------------------------------------------------------------------------------------------------

#define _SEND_COMMAND(_cls_, ...) \
    { \
        std::lock_guard<std::mutex> lock(_commandMutex); \
        _commandQueue.emplace( std::make_unique<_cls_>(__VA_ARGS__) ); \
        _ThreadWakeup(); \
    }

void InputLogfile::Play()
{
    if (CanPlay())
    {
        _SEND_COMMAND(LogfileCommandPlay);
    }
}

void InputLogfile::Stop()
{
    if (CanStop())
    {
        _SEND_COMMAND(LogfileCommandStop);
    }
}

void InputLogfile::Pause()
{
    if (CanPause())
    {
        _SEND_COMMAND(LogfileCommandPause);
    }
}

void InputLogfile::StepMsg(const std::string &msgName)
{
    if (CanStep())
    {
        _SEND_COMMAND(LogfileCommandStepMsg, msgName);
    }
}

void InputLogfile::StepEpoch()
{
    if (CanStep())
    {
        _SEND_COMMAND(LogfileCommandStepEpoch);
    }
}

void InputLogfile::Seek(const float pos)
{
    if (CanSeek())
    {
        // Set it already here, so that GetPlayPos() returns the new pos, so that the seekbar in GuiWinLogfile doesn't jump...
        _playPosRel = CLIP(pos, 0.0f, 1.0f);
        _SEND_COMMAND(LogfileCommandSeek, pos);
    }
}

/* ****************************************************************************************************************** */

#define _SEND_EVENT(_cls_, ...) \
    { \
        std::lock_guard<std::mutex> lock(_eventMutex); \
        _eventQueue.emplace( std::make_unique<_cls_>(__VA_ARGS__) ); \
    }

#define _THREAD_DEBUG(fmt, args...) DEBUG("thread %s " fmt, _inputName.c_str(), ## args)

void InputLogfile::_ThreadPrepare()
{
    _playState = STOPPED;
    _SEND_EVENT(LogfileEventNotice, "Logfile opened: " + _logfile.Path());
}

void InputLogfile::_ThreadCleanup()
{
    _playState = CLOSED;
    _SEND_EVENT(LogfileEventNotice, "Logfile closed: " + _logfile.Path());
}

void InputLogfile::_Thread(Ff::Thread *thread)
{
    PARSER_t parser;
    PARSER_MSG_t msg;
    EPOCH_t coll;
    EPOCH_t epoch;
    bool stepEpoch = false;
    bool stepMsg = false;
    std::string stepMsgName = "";

    parserInit(&parser);
    epochInit(&coll);

    while (!thread->ShouldAbort())
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
                    if (!_logfile.CanSeek())
                    {
                        const std::string path = _logfile.Path();
                        _logfile.Close();
                        _logfile.OpenRead(path);
                    }
                    else
                    {
                        _logfile.Seek(0);
                    }
                    _playPos = 0;
                    _playPosRel = 0.0;
                    parserInit(&parser);
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
                    float rel = cmd->pos;
                    if (rel < FLT_MIN)
                    {
                        _playPosRel = 0.0f;
                        _playPos = 0;
                    }
                    else
                    {
                        if (rel > (1.0 - FLT_EPSILON))
                        {
                            rel = 1.0f;
                        }
                        _playPosRel = rel;
                        _playPos = (double)_playSize * (double)_playPosRel;
                        const uint64_t endPos = _playSize - 1;
                        if (_playPos > endPos)
                        {
                            _playPos = endPos;
                        }
                    }
                    _logfile.Seek(_playPos);
                    parserInit(&parser);
                    break;
                }
            }
            command = nullptr;
        }

        // Player idle
        if ( (_playState == STOPPED) || (_playState == PAUSED) )
        {
            thread->Sleep(10);
            continue;
        }

        // Process logfile data
        bool intr = false;
        while (!intr && parserProcess(&parser, &msg, true) && !thread->ShouldAbort() && _commandQueue.empty())
        {
            _playPos += msg.size;
            _playPosRel = (double)_playPos / (double)_playSize;

            msg.src = PARSER_MSGSRC_LOG;

            // Collect epochs
            if (epochCollect(&coll, &msg, &epoch))
            {
                // Update database
                _latestRow = _inputDatabase->AddEpoch(epoch, false);

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
                    thread->Sleep(1000.0 / _playSpeed);

                    // Message timestamp is no longer valid after sleeping
                    msg.ts = TIME();
                }
            }

            // Propagate UBX-INF-{ERROR,WARNING}
            switch (msg.type)
            {
                case PARSER_MSGTYPE_UBX:
                    switch (UBX_CLSID(msg.data))
                    {
                        case UBX_INF_CLSID:
                            switch (UBX_MSGID(msg.data))
                            {
                                case UBX_INF_WARNING_MSGID:
                                    _SEND_EVENT(LogfileEventWarning, msg.info);
                                    break;
                                case UBX_INF_ERROR_MSGID:
                                    _SEND_EVENT(LogfileEventError, msg.info);
                                    break;
                            }
                            break;
                    }
                    break;
                case PARSER_MSGTYPE_NMEA:
                    if ( (msg.data[3] == 'T') && (msg.data[4] == 'X') && (msg.data[5] == 'T') && (msg.data[13] == '0') )
                    {
                        switch (msg.data[14])
                        {
                            case '0':
                                _SEND_EVENT(LogfileEventError, msg.info);
                                break;
                            case '1':
                                _SEND_EVENT(LogfileEventWarning, msg.info);
                                break;
                        }
                    }
                    break;
                default:
                    break;
            }

            // Wait a bit if the event queue is too long, to not overwhelm the main thread
            while (!intr && (_eventQueue.size() > EVENT_QUEUE_MAX_SIZE))
            {
                thread->Sleep(25);
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
        const int num = _logfile.Read(buf, sizeof(buf));
        if (num > 0)
        {
            if (!parserAdd(&parser, buf, num))
            {
                WARNING("InputLogfile(%s) parser overflow!", _inputName.c_str());
            }
        }
        else
        {
            _playState = PAUSED;
        }
    }
}

/* ****************************************************************************************************************** */
