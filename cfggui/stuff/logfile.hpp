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

#ifndef __LOGFILE_HPP__
#define __LOGFILE_HPP__

#include <memory>
#include <string>
#include <functional>
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>

#include "ff_parser.h"

#include "data.hpp"
#include "database.hpp"

/* ****************************************************************************************************************** */

struct LogfileEvent;
struct LogfileCommand;

class Logfile
{
    public:
        Logfile(const std::string &name, std::shared_ptr<Database> database);
       ~Logfile();

        void SetDataCb(std::function<void(const Data &)> cb);
        void Loop(const double &now);

        bool Open(const std::string &path);
        void Close();
        void Play();
        void Stop();
        void Pause();
        void StepMsg(const std::string &msgName = "");
        void StepEpoch();
        void Seek(const float pos);

        bool CanOpen();
        bool CanClose();
        bool CanPlay();
        bool CanStop();
        bool CanPause();
        bool CanStep();
        bool CanSeek();

        float GetPlayPos();
        float GetPlaySpeed();
        void  SetPlaySpeed(const float speed);

        static constexpr float PLAYSPEED_MIN =   0.1f;
        static constexpr float PLAYSPEED_MAX = 200.0f;
        static constexpr float PLAYSPEED_INF =   0.0f;

    protected:

    private:
        std::string _name;
        std::shared_ptr<Database> _database;
        std::function<void(const Data &)> _dataCb;

        // Shared between main thread and logfile thread
        std::queue< std::unique_ptr<LogfileEvent> > _eventQueue;
        std::mutex                     _eventMutex;
        std::queue< std::unique_ptr<LogfileCommand> > _commandQueue;
        std::mutex                     _commandMutex;
        std::atomic<uint64_t>          _playPos;
        std::atomic<uint64_t>          _playSize;
        std::atomic<float>             _playPosRel;
        std::atomic<float>             _playSpeed;
        enum State_e { CLOSED, STOPPED, PLAYING, PAUSED };
        std::atomic<enum State_e>      _playState;

        // Fake a semaphore to wake up sleeping player thread
        std::mutex                     _playThreadMutex;
        std::condition_variable        _playThreadCondVar;

        // Logfile player thread
        std::string                    _logfilePath;
        std::unique_ptr<std::ifstream> _logfileHandle;
        std::unique_ptr<std::thread>   _playThread;
        std::atomic<bool>              _playThreadAbort;
        PARSER_t                       _playParser;
        static constexpr uint32_t      EVENT_QUEUE_MAX_SIZE = 1000; // FIXME: ??
        void _LogfileThreadWrap();
        void _LogfileThread();
};

/* ****************************************************************************************************************** */
#endif // __LOGFILE_HPP__
