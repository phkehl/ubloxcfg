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

#ifndef __GUI_WIN_NTRIP_HPP__
#define __GUI_WIN_NTRIP_HPP__

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <map>
#include <string>

#include "ff_thread.hpp"

#include "gui_widget_log.hpp"
#include "ntripclient.hpp"
#include "input_receiver.hpp"

#include "gui_win.hpp"

/* ***** NTRIP cliebt *********************************************************************************************** */

class GuiWinNtrip : public GuiWin
{
    public:
        GuiWinNtrip(const std::string &name);
       ~GuiWinNtrip();

        void Loop(const uint32_t &frame, const double &now) final;
        void DrawWindow() final;

        void AddReceiver(std::shared_ptr<InputReceiver> receiver);
        void RemoveReceivers();

    private:

        std::mutex _mutex;

        // Receivers
        std::shared_ptr<InputReceiver> _srcReceiver;
        std::unordered_map<std::string, bool> _dstReceivers;
        std::vector< std::shared_ptr<InputReceiver> > _receivers;

        // NTRIP client
        NtripClient  _ntripClient;
        bool _SetCaster(const std::string &spec);

        // GGA input form
        std::string  _casterInput;
        bool         _ggaAuto;
        std::string  _ggaInt;
        std::string  _ggaLat;
        std::string  _ggaLon;
        std::string  _ggaAlt;
        std::string  _ggaNumSv;
        bool         _ggaFix;
        double       _ggaLastUpdate;
        NtripClient::Pos _ntripPos;
        void _UpdateGga();

        // Other GUI stuff
        GuiWidgetLog _log;

        // NTRIP thread
        Ff::Thread _thread;
        void _Thread(Ff::Thread *thread);
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_NTRIP_HPP__
