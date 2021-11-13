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
#include <array>
#include <chrono>

#include <time.h>

#include "imgui.h"

#include "gui_settings.hpp"
#include "gui_win_input_receiver.hpp"
#include "gui_win_input_logfile.hpp"


struct GuiAppEarlyLog
{
    GuiAppEarlyLog();
    void Add(DEBUG_LEVEL_t level, std::string line);
    void Clear();
    std::vector< std::pair<DEBUG_LEVEL_t, std::string> > log;
};

class GuiApp
{
    public:
        GuiApp(const std::vector<std::string> &argv, const GuiAppEarlyLog &earlyLog);
       ~GuiApp();

        static GuiApp &GetInstance();

        // Main application loop, do what needs to be done regularly
        void Loop();

        // Called in between ImGui::Render() and ImGui::NewFrame()
        bool PrepFrame();
        // Called just after ImGui::NewFrame();
        void NewFrame();

        // Draw one app frame, call all windows drawing methods
        void DrawFrame();

        void ConfirmClose(bool &display, bool &done);
        ImVec4 BackgroundColour();

        // Get settings instance
        std::shared_ptr<GuiSettings> GetSettings();

        // Performance monitor
        enum Perf_e { NEWFRAME = 0, LOOP, DRAW, RENDER_IM, RENDER_GL, _NUM_PERF };
        void PerfTic(const enum Perf_e perf);
        void PerfToc(const enum Perf_e perf);

    private:

        void _MainMenu();

        std::string                          _settingsFile;
        std::shared_ptr<GuiSettings>         _settings;

        // Application windows
        enum
        {
            APP_WIN_ABOUT = 0, APP_WIN_SETTINGS, APP_WIN_HELP, APP_WIN_IMGUI_DEMO, APP_WIN_IMPLOT_DEMO,
            APP_WIN_IMGUI_METRICS, APP_WIN_IMPLOT_METRICS, APP_WIN_IMGUI_STYLES, APP_WIN_IMPLOT_STYLES,
            APP_WIN_EXPERIMENT, APP_WIN_PLAY,
            _NUM_APP_WIN
        };
        std::vector< std::unique_ptr<GuiWin> > _appWindows;

        // Receiver and logfile windows
        std::vector< std::unique_ptr<GuiWinInputReceiver> > _receiverWindows;
        std::vector< std::unique_ptr<GuiWinInputLogfile>  > _logfileWindows;
        template<typename T> void _CreateInputWindow(
            const std::string &baseName, std::vector< std::unique_ptr<T> > &inputWindows, const std::string &prevWinName = "");

        // Debugging
        std::mutex           _debugMutex;
        DEBUG_CFG_t          _debugCfg;
        DEBUG_CFG_t          _debugCfgOrig;
        double               _statsLast;
        struct timespec      _statsTime;
        float                _statsCpu;
        float                _statsRss;
        bool                 _debugWinOpen;
        bool                 _debugWinDim;
        GuiWidgetLog         _debugLog;
        void _DrawDebugWin();
        static void _DebugLogCb(const DEBUG_LEVEL_t level, const char *str, const DEBUG_CFG_t *cfg);
        static void _DebugLogAdd(const DEBUG_LEVEL_t level, const char *str, GuiWidgetLog *logWidget);

        using PerfMeas_t = std::array<float, 250>;
        struct PerfData
        {
            PerfData();
            PerfMeas_t data;
            int        ix;
            int        num;
            std::chrono::system_clock::time_point t0;
            void Tic();
            void Toc();
        };
        std::array<PerfData, _NUM_PERF> _perfData;
};

/* ****************************************************************************************************************** */
#endif // __GUI_APP_H__
