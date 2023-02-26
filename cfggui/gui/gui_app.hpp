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

#ifndef __GUI_APP_HPP__
#define __GUI_APP_HPP__

#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <array>
#include <chrono>

#include <time.h>

#include "imgui.h"

#include "platform.hpp"
#include "gui_settings.hpp"
#include "gui_win_input_receiver.hpp"
#include "gui_win_input_logfile.hpp"
#include "gui_notify.hpp"
#include "gui_win_ntrip.hpp"
#include "gui_win_multi.hpp"
#include "glmatrix.hpp"
#include "opengl.hpp"
#include "gui_widget_tabbar.hpp"
#include "gui_data.hpp"

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
        GuiApp(const GuiAppEarlyLog &earlyLog, const std::vector<std::string> &versions);
       ~GuiApp();

        // Main application loop, do what needs to be done regularly
        void Loop(const uint32_t &frame, const double &now);

        // Draw one app frame, call all windows drawing methods
        void DrawFrame();

        void ConfirmClose(bool &display, bool &done);
        ImVec4 BackgroundColour();

        // Performance monitor
        enum Perf_e { NEWFRAME = 0, LOOP, DRAW, RENDER_IM, RENDER_GL, CPU, TOTAL, _NUM_PERF };
        void PerfTic(const enum Perf_e perf);
        void PerfToc(const enum Perf_e perf);

    private:

        void _MainMenu();

        // Application windows
        enum
        {
            APP_WIN_ABOUT = 0, APP_WIN_SETTINGS, APP_WIN_HELP, APP_WIN_IMGUI_DEMO, APP_WIN_IMPLOT_DEMO,
            APP_WIN_IMGUI_METRICS, APP_WIN_IMPLOT_METRICS, APP_WIN_IMGUI_STYLES, APP_WIN_IMPLOT_STYLES,
            APP_WIN_EXPERIMENT, APP_WIN_PLAY, APP_WIN_LEGEND,
            _NUM_APP_WIN
        };
        std::vector< std::unique_ptr<GuiWin> > _appWindows;
        GuiNotify _notifications;

        // Receiver and logfile windows
        std::vector< std::unique_ptr<GuiWinInputReceiver> > _receiverWindows;
        std::vector< std::unique_ptr<GuiWinInputLogfile>  > _logfileWindows;
        static constexpr int MAX_SAVED_WINDOWS = 20;
        template<typename T> void _CreateInputWindow(
            const std::string &baseName, std::vector< std::unique_ptr<T> > &inputWindows, const std::string &prevWinName);
        void _CreateReceiverWindow(const std::string &prevWinName = "");
        void _CreateLogfileWindow(const std::string &prevWinName = "");

        // NTRIP clients
        std::vector< std::unique_ptr<GuiWinNtrip> > _ntripWindows;
        void _CreateNtripWindow(const std::string &prevWinName = "");

        // Multiview windows
        std::vector< std::unique_ptr<GuiWinMulti> > _multiWindows;
        void _CreateMultiWindow(const std::string &prevWinName = "");

        // Debugging
        std::mutex           _debugMutex;
        DEBUG_CFG_t          _debugCfg;
        DEBUG_CFG_t          _debugCfgOrig;
        double               _statsLast;
        struct timespec      _statsTime;
        float                _statsCpu;
        Platform::MemUsage   _memUsage;
        bool                 _debugWinOpen;
        bool                 _debugWinDim;
        GuiWidgetLog         _debugLog;
        float                _menuBarHeight;
        GuiWidgetTabbar      _debugTabbar;
        void _DrawDebugWin();
        static void _DebugLogCb(const DEBUG_LEVEL_t level, const char *str, const DEBUG_CFG_t *cfg);
        static void _DebugLogAdd(const DEBUG_LEVEL_t level, const char *str, GuiWidgetLog *logWidget);

        // H4xx0r mode
        bool                 _h4xx0rMode;
        GlMatrix             _matrix;
        GlMatrix::Options    _matrixOpts;
        OpenGL::FrameBuffer  _matrixFb;
        double               _matrixLastRender;
        bool _ConfigMatrix(const bool enable);
        void _LoopMatrix(const uint32_t &frame, const double &now);
        void _DrawMatrix();
        void _DrawMatrixConfig();

        using PerfMeas_t = std::array<float, 250>;
        struct PerfData
        {
            PerfData();
            PerfMeas_t data;
            int        ix;
            std::chrono::system_clock::time_point t0;
            void Tic();
            void Toc();
            void Add(const float value);
        };
        std::array<PerfData, _NUM_PERF> _perfData;

        //GuiEvents _events;
};

/* ****************************************************************************************************************** */
#endif // __GUI_APP_HPP__
