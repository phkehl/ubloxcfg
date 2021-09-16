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

#ifndef __GUI_APP_H__
#define __GUI_APP_H__

#include <memory>
#include <mutex>
#include <vector>
#include <string>

#include <time.h>

#include "imgui.h"

#include "database.hpp"
#include "receiver.hpp"
#include "logfile.hpp"
#include "gui_settings.hpp"
#include "gui_win_app.hpp"
#include "gui_win_receiver.hpp"
#include "gui_win_data.hpp"

struct GuiAppInitialLog
{
    GuiAppInitialLog();
    void Add(DEBUG_LEVEL_t level, std::string line);
    void Clear();
    std::vector< std::pair<DEBUG_LEVEL_t, std::string> > log;
};

class GuiApp
{
    public:
        GuiApp(const std::vector<std::string> &argv, const GuiAppInitialLog &initLog);
       ~GuiApp();

        static GuiApp &GetInstance();

        // Main application loop, do what needs to be done regularly
        void Loop();

        // Called in between ImGui::Render() and ImGui::NewFrame()
        bool                 PrepFrame();
        // Called just after ImGui::NewFrame();
        void                 NewFrame();

        // Draw one app frame, call all windows drawing methods
        void                 DrawFrame();

        // Callback for ff_debug
        void                 DebugLog(const DEBUG_LEVEL_t level, const char *str);

        void                 ConfirmClose(bool &display, bool &done);
        ImVec4               BackgroundColour();

        std::shared_ptr<GuiSettings> GetSettings();

        // void                 DrawDataWinButtons(const Receiver &receiver);

    private:

        void                 _MainMenu();

        std::string          _settingsFile;
        std::shared_ptr<GuiSettings>        _settings;

        // Application windows
        std::unique_ptr<GuiWinDebug>         _winDebug;
        std::unique_ptr<GuiWinAbout>         _winAbout;
        std::unique_ptr<GuiWinSettings>      _winSettings;
        std::unique_ptr<GuiWinHelp>          _winHelp;
        std::unique_ptr<GuiWinImguiDemo>     _winImguiDemo;
        std::unique_ptr<GuiWinImplotDemo>    _winImplotDemo;
        std::unique_ptr<GuiWinImguiMetrics>  _winImguiMetrics;
        std::unique_ptr<GuiWinImplotMetrics> _winImplotMetrics;
        std::unique_ptr<GuiWinImguiStyles>   _winImguiStyles;
        //std::unique_ptr<GuiWinImguiAbout>   _winImguiAbout;
        std::vector<GuiWin *> _appWindows;

        // Receivers
        struct AppReceiver
        {
            AppReceiver(std::shared_ptr<Database> _database, std::shared_ptr<Receiver> _receiver, std::unique_ptr<GuiWinReceiver> _rxWin) :
                database{_database}, receiver{_receiver}, rxWin{std::move(_rxWin)}, dataWin{}
            {}
            std::shared_ptr<Database>                  database;
            std::shared_ptr<Receiver>                  receiver;
            std::unique_ptr<GuiWinReceiver>            rxWin;
            std::vector< std::unique_ptr<GuiWinData> > dataWin;
        };
        std::vector< std::unique_ptr<AppReceiver> > _receivers;
        void                 _CreateReceiver();
        void                 _ProcessDataReceiverCb(const AppReceiver *appReceiver, const Data &data);
        void                 _DataWinButtonsReceiverCb(AppReceiver *appReceiver);
        void                 _TitleChangeReceiverCb(AppReceiver *appReceiver);
        void                 _ClearReceiverCb(AppReceiver *appReceiver);
        void                 _StoreRxWinSettings(std::unique_ptr<AppReceiver> &appReceiver);
        void                 _OpenPreviousDataWin(std::unique_ptr<AppReceiver> &appReceiver);

        std::mutex           _debugMutex;
        DEBUG_CFG_t          _debugCfg;
        DEBUG_CFG_t          _debugCfgOrig;

        double               _statsLast;
        struct timespec      _statsTime;
        float                _statsCpu;
        float                _statsRss;
};

/* ****************************************************************************************************************** */
#endif // __GUI_APP_H__
