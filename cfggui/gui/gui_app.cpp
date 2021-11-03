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

#include <memory>
#include <mutex>
#include <algorithm>
#include <filesystem>
#include <cstring>

#include <sys/time.h>

#ifndef _WIN32
#  include <sys/resource.h>
#endif

#include "IconsForkAwesome.h"

#include "platform.hpp"

#include "config.h"

#include "gui_app.hpp"

/* ****************************************************************************************************************** */

GuiAppEarlyLog::GuiAppEarlyLog() :
    log{}
{
}

void GuiAppEarlyLog::Add(DEBUG_LEVEL_t level, std::string line)
{
    log.push_back( { level, line } );
}

void GuiAppEarlyLog::Clear()
{
    log.clear();
}

/* ****************************************************************************************************************** */

static GuiApp *gGuiAppInstance;

GuiApp &GuiApp::GetInstance()
{
    return *gGuiAppInstance;
}

/* ****************************************************************************************************************** */

GuiApp::GuiApp(const std::vector<std::string> &argv, const GuiAppEarlyLog &earlyLog) :
    _settingsFile{}, _settings{nullptr},
    _appWindows{},
    _debugMutex{},
    _statsLast{}, _statsTime{}, _statsCpu{}, _statsRss{},
    _debugWinOpen{false}
{
    UNUSED(argv);
    DEBUG("GuiApp()");
    gGuiAppInstance = this;

    // Add early log entries to log widget and re-configure logging
    for (const auto &entry: earlyLog.log)
    {
        _DebugLogAdd(entry.first, entry.second.c_str(), &_debugLog);
    }
    debugGetCfg(&_debugCfgOrig);
    _debugCfg.level  = DEBUG_LEVEL_DEBUG;
    _debugCfg.colour = _debugCfgOrig.colour;
    _debugCfg.mark   = NULL;
    _debugCfg.func   = _DebugLogCb;
    _debugCfg.arg    = &_debugLog;
    debugSetup(&_debugCfg);

    // Create settings instance, will be used by all windows
    _settingsFile = Platform::ConfigFile("cfggui.conf");
    _settings = std::make_shared<GuiSettings>(_settingsFile);

    // Create application windows
    _appWindows.resize(_NUM_APP_WIN);
    _appWindows[APP_WIN_ABOUT]            = std::make_unique<GuiWinAbout>();
    _appWindows[APP_WIN_SETTINGS]         = std::make_unique<GuiWinSettings>();
    _appWindows[APP_WIN_HELP]             = std::make_unique<GuiWinHelp>();
#ifndef IMGUI_DISABLE_DEMO_WINDOWS
    _appWindows[APP_WIN_IMGUI_DEMO]       = std::make_unique<GuiWinImguiDemo>();
    _appWindows[APP_WIN_IMPLOT_DEMO]      = std::make_unique<GuiWinImplotDemo>();
#endif
#ifndef IMGUI_DISABLE_METRICS_WINDOW
    _appWindows[APP_WIN_IMGUI_METRICS]    = std::make_unique<GuiWinImguiMetrics>();
#endif
    _appWindows[APP_WIN_IMPLOT_METRICS]   = std::make_unique<GuiWinImplotMetrics>();
    _appWindows[APP_WIN_IMGUI_STYLES]     = std::make_unique<GuiWinImguiStyles>();
    _appWindows[APP_WIN_IMPLOT_STYLES]    = std::make_unique<GuiWinImplotStyles>();
    _appWindows[APP_WIN_EXPERIMENT]       = std::make_unique<GuiWinExperiment>();

    // Load settings
    _settings->GetValue("DebugWinOpen", _debugWinOpen, false);

    // Create at least one receiver window, load previous windows
    _CreateReceiverWindow();

    // Hello, hello
    NOTICE("cfggui " CONFIG_VERSION " (" CONFIG_GITHASH ")");
}

// ---------------------------------------------------------------------------------------------------------------------

GuiApp::~GuiApp()
{
    // Reconfigure debug output to terminal only
    debugSetup(&_debugCfgOrig);

    DEBUG("GuiApp::~GuiApp()");

    // Remember some settings
    _settings->SetValue("DebugWinOpen", _debugWinOpen);

    std::vector<std::string> openReceiverWinNames;
    for (auto &receiver: _receiverWindows)
    {
        openReceiverWinNames.push_back(receiver->GetName());
    }
    _settings->SetValue("ReceiverWindows", Ff::StrJoin(openReceiverWinNames, ","));

    std::vector<std::string> openLogfileWinNames;
    for (auto &logfile: _logfileWindows)
    {
        openLogfileWinNames.push_back(logfile->GetName());
    }
    _settings->SetValue("LogfileWindows", Ff::StrJoin(openLogfileWinNames, ","));


    // Destroy child windows, so that they can save their settings before we save the file below
    _appWindows.clear();
    _receiverWindows.clear();
    _logfileWindows.clear();

    // Save settings file
    if (_settingsFile.size() > 0)
    {
        _settings->SaveConf(_settingsFile);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

std::shared_ptr<GuiSettings> GuiApp::GetSettings()
{
    return _settings;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiApp::PrepFrame()
{
    return _settings->_UpdateFont();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::NewFrame()
{
    _settings->_UpdateSizes();
}

// ---------------------------------------------------------------------------------------------------------------------

ImVec4 GuiApp::BackgroundColour()
{
    return ImGui::ColorConvertU32ToFloat4(_settings->colours[GuiSettings::APP_BACKGROUND]);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::Loop()
{
    const uint32_t frame = ImGui::GetFrameCount();
    const double now = ImGui::GetTime();

    // Run windows loops
    for (auto &win: _appWindows)
    {
        win->Loop(frame, now);
    }
    for (auto &win: _receiverWindows)
    {
        win->Loop(frame, now);
    }
    for (auto &win: _logfileWindows)
    {
        win->Loop(frame, now);
    }

    // Debug window
    if (_debugWinOpen)
    {
        _DrawDebugWin();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::DrawFrame()
{
    _MainMenu();

    // Draw app windows
    for (auto &win: _appWindows)
    {
        if (win->IsOpen())
        {
            win->DrawWindow();
        }
    }

    // Draw receiver windows, remove and destroy closed ones
    for (auto iter = _receiverWindows.begin(); iter != _receiverWindows.end(); )
    {
        auto &receiverWin = *iter;
        if (receiverWin->IsOpen())
        {
            receiverWin->DrawWindow();
            iter++;
        }
        else
        {
            iter = _receiverWindows.erase(iter);
        }
    }

    // Draw logfile windows, remove and destroy closed ones
    for (auto iter = _logfileWindows.begin(); iter != _logfileWindows.end(); )
    {
        auto &logfileWin = *iter;
        if (logfileWin->IsOpen())
        {
            logfileWin->DrawWindow();
            iter++;
        }
        else
        {
            iter = _logfileWindows.erase(iter);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

int GuiApp::_FindUnusedWindowNumber(const std::string &baseName, const std::vector<std::string> existingNames)
{
    int n = 1;
    while (n < 1000)
    {
        const std::string winName = baseName + std::to_string(n); // BaseName1, BaseName2, ...
        bool nameUnused = true;
        for (auto &existingName: existingNames)
        {
            if (winName == existingName)
            {
                nameUnused = false;
                break;
            }
        }
        if (nameUnused)
        {
            return n;
        }
        n++;
    }
    return 0;
}

void GuiApp::_CreateReceiverWindow()
{
    std::vector<std::string> winNames;
    for (auto &win: _receiverWindows)
    {
        winNames.push_back(win->GetName());
    }
    const std::string baseName = "Receiver";
    const int winNumber = _FindUnusedWindowNumber(baseName, winNames);
    if (winNumber > 0)
    {
        try
        {
            auto win = std::make_unique<GuiWinInputReceiver>(baseName + std::to_string(winNumber));
            win->Open();
            win->SetTitle(baseName + " " + std::to_string(winNumber));
            win->OpenPreviousDataWin();
            _receiverWindows.push_back(std::move(win));
            std::sort(_receiverWindows.begin(), _receiverWindows.end(),
                [](const std::unique_ptr<GuiWinInputReceiver> &a, const std::unique_ptr<GuiWinInputReceiver> &b)
                {
                    return a->GetName() < b->GetName();
                });
        }
        catch (std::exception &e)
        {
            ERROR("new %s%d: %s", baseName.c_str(), winNumber, e.what());
        }
    }
}

void GuiApp::_CreateLogfileWindow()
{
    std::vector<std::string> winNames;
    for (auto &win: _logfileWindows)
    {
        winNames.push_back(win->GetName());
    }
    const std::string baseName = "Logfile";
    const int winNumber = _FindUnusedWindowNumber(baseName, winNames);
    if (winNumber > 0)
    {
        try
        {
            auto win = std::make_unique<GuiWinInputLogfile>(baseName + std::to_string(winNumber));
            win->Open();
            win->SetTitle(baseName + " " + std::to_string(winNumber));
            win->OpenPreviousDataWin();
            _logfileWindows.push_back(std::move(win));
            std::sort(_logfileWindows.begin(), _logfileWindows.end(),
                [](const std::unique_ptr<GuiWinInputLogfile> &a, const std::unique_ptr<GuiWinInputLogfile> &b)
                {
                    return a->GetName() < b->GetName();
                });
        }
        catch (std::exception &e)
        {
            ERROR("new %s%d: %s", baseName.c_str(), winNumber, e.what());
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_MainMenu()
{
    if (ImGui::BeginMainMenuBar())
    {
        // Store menu bar height FIXME: is there a better way than doing this here for every frame?
        _settings->menuBarHeight = ImGui::GetWindowSize().y;

        ImGui::PushStyleColor(ImGuiCol_PopupBg, _settings->style.Colors[ImGuiCol_MenuBarBg]);

        if (ImGui::BeginMenu("Receiver"))
        {
            if (ImGui::MenuItem("New"))
            {
                _CreateReceiverWindow();
            }
            if (!_receiverWindows.empty())
            {
                ImGui::Separator();
                for (auto &receiverWin: _receiverWindows)
                {
                    if (ImGui::MenuItem(receiverWin->GetTitle().c_str()))
                    {
                        receiverWin->Focus();
                    }
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Logfile"))
        {
            if (ImGui::MenuItem("New"))
            {
                _CreateLogfileWindow();
            }
            if (!_logfileWindows.empty())
            {
                ImGui::Separator();
                for (auto &logfileWin: _logfileWindows)
                {
                    if (ImGui::MenuItem(logfileWin->GetTitle().c_str()))
                    {
                        logfileWin->Focus();
                    }
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::MenuItem("About",       NULL, _appWindows[APP_WIN_ABOUT]->GetOpenFlag());
            ImGui::MenuItem("Settings",    NULL, _appWindows[APP_WIN_SETTINGS]->GetOpenFlag());
            ImGui::MenuItem("Help",        NULL, _appWindows[APP_WIN_HELP]->GetOpenFlag());
            ImGui::MenuItem(/*ICON_FK_BUG " " */"Debug",       NULL, &_debugWinOpen);
            ImGui::EndMenu();
        }

        ImGui::PopStyleColor();

        // Update (some) performance info
        const double now = ImGui::GetTime();
        const double delta = now - _statsLast;
        if (delta >= 1.0)
        {
            _statsLast = now;
#ifndef _WIN32
            struct rusage usage;
            if (getrusage(RUSAGE_SELF, &usage) == 0)
            {
                _statsRss = (float)usage.ru_maxrss / 1024.0f;
            }
#endif
            struct timespec time;
            double cpu = 0.0;
            if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time) == 0)
            {
                cpu = ((double)(time.tv_sec - _statsTime.tv_sec))
                    + ((double)(time.tv_nsec - _statsTime.tv_nsec) * 1e-9);
                _statsTime = time;
            }
            _statsCpu = 1e2 * cpu / delta;
        }

        // Draw performance info
        char text[200];
        ImGuiIO &io = ImGui::GetIO();
        snprintf(text, sizeof(text),
            ICON_FK_TACHOMETER " %2.0f | " ICON_FK_CLOCK_O " %.1f | " ICON_FK_FILM " %d | " ICON_FK_THERMOMETER_HALF " %.0fMB %2.0f%% ", // <-- FIXME: CalcTextSize() misses one char?!
            io.Framerate, // io.Framerate > FLT_EPSILON ? 1000.0f / io.Framerate : 0.0f,
            //io.DeltaTime > FLT_EPSILON ? 1.0 / io.DeltaTime : 0.0, io.DeltaTime * 1e3,
            ImGui::GetTime(), ImGui::GetFrameCount(),
            _statsRss, _statsCpu);
        const float w = ImGui::CalcTextSize(text).x + ImGui::GetStyle().WindowPadding.x;
        ImGui::SameLine(ImGui::GetWindowWidth() - w);
        ImGui::PushStyleColor(ImGuiCol_Text, io.DeltaTime < 99e-3 ? GUI_COLOUR(C_BRIGHTWHITE) : GUI_COLOUR(C_WHITE));
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(
                ICON_FK_TACHOMETER       " = FPS, "
                ICON_FK_CLOCK_O          " = time, "
                ICON_FK_FILM             " = frames, "
                ICON_FK_THERMOMETER_HALF " = RSS [MB] / CPU [%]");
            ImGui::EndTooltip();
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            _debugWinOpen = !_debugWinOpen;
        }
        ImGui::EndMainMenuBar();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::ConfirmClose(bool &display, bool &done)
{
    const char * const popupName = "Really Close?###ConfirmClose";
    ImGui::OpenPopup(popupName);
    if (ImGui::BeginPopupModal(popupName, NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("All connections will be shut down.\nAll data will be lost.\n");
        ImGui::Separator();

        if (ImGui::Button(ICON_FK_CHECK " OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            done = true;
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button(ICON_FK_TIMES " Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            display = false;
        }
        ImGui::EndPopup();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_DrawDebugWin()
{
    // Always position window at NE corner, constrain min/max size
    auto &io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x, _settings->menuBarHeight), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(300,100), ImVec2(io.DisplaySize.x, io.DisplaySize.y - _settings->menuBarHeight));

    // Style window (no border, no round corners, no alpha if focused/active, bg colour same as menubar)
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, _debugWinDim ? 0.75f * _settings->style.Alpha : 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, _settings->style.Colors[ImGuiCol_MenuBarBg]);

    // Start window
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove;
    const bool isOpen = ImGui::Begin("Debug##CfgGuiDebugWin", &_debugWinOpen, flags);

    // Pop style stack
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();

    // Should it be dimmed, i.e. is it neither focussed nor active?
    _debugWinDim = !(ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) || ImGui::IsWindowHovered(ImGuiFocusedFlags_ChildWindows));

    // Draw window
    if (isOpen)
    {
        if (ImGui::BeginTabBar("tabs", ImGuiTabBarFlags_FittingPolicyScroll))
        {
            if (ImGui::BeginTabItem("Log"))
            {
                _debugLog.DrawWidget();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Tools"))
            {
                const ImVec2 buttonSize { _settings->charSize.x * 20, 0 };

                const struct { const char *label; int win; bool newline; } buttons[] =
                {
                    { "ImGui demo",     APP_WIN_IMGUI_DEMO,       false },
                    { "ImGui metrics",  APP_WIN_IMGUI_METRICS,    false },
                    { "ImGui styles",   APP_WIN_IMGUI_STYLES,     true  },
                    { "ImPlot demo",    APP_WIN_IMPLOT_DEMO,      false },
                    { "ImPlot metrics", APP_WIN_IMPLOT_METRICS,   true  },
                    { "Experiments",    APP_WIN_EXPERIMENT,       false },
                };
                for (int ix = 0; ix < NUMOF(buttons); ix++)
                {
                    if (ImGui::Button(buttons[ix].label, buttonSize) && _appWindows[buttons[ix].win])
                    {
                        if (!_appWindows[buttons[ix].win]->IsOpen())
                        {
                            _appWindows[buttons[ix].win]->Open();
                        }
                        else
                        {
                            _appWindows[buttons[ix].win]->Focus();
                        }
                    }
                    if (!buttons[ix].newline)
                    {
                        ImGui::SameLine();
                    }
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ void GuiApp::_DebugLogCb(const DEBUG_LEVEL_t level, const char *str, const DEBUG_CFG_t *cfg)
{
    // Add to log window
    _DebugLogAdd(level, str, static_cast<GuiWidgetLog *>(cfg->arg));

    // Also add to console
    if ( (cfg->level >= DEBUG_LEVEL_DEBUG) || (level >= DEBUG_LEVEL_WARNING) )
    {
        fputs(str, stderr);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ void GuiApp::_DebugLogAdd(const DEBUG_LEVEL_t level, const char *str, GuiWidgetLog *logWidget)
{
    // Skip colour escape output, Note: This relies on how ff_debug.h does things internally
    // (i.e. printing colour escapes and the data separately in individual calls to the print function)
    if (str[0] == '\e')
    {
        return;
    }

    // Prepend prefix to actual string so that we can filter on it
    const char *prefix = "?: ";
    ImU32 colour = 0;
    switch (level)
    {
        case DEBUG_LEVEL_TRACE:   prefix = "T: "; colour = GUI_COLOUR(DEBUG_TRACE);   break;
        case DEBUG_LEVEL_DEBUG:   prefix = "D: "; colour = GUI_COLOUR(DEBUG_DEBUG);   break;
        case DEBUG_LEVEL_PRINT:   prefix = "P: "; colour = GUI_COLOUR(DEBUG_PRINT);   break;
        case DEBUG_LEVEL_NOTICE:  prefix = "N: "; colour = GUI_COLOUR(DEBUG_NOTICE);  break;
        case DEBUG_LEVEL_WARNING: prefix = "W: "; colour = GUI_COLOUR(DEBUG_WARNING); break;
        case DEBUG_LEVEL_ERROR:   prefix = "E: "; colour = GUI_COLOUR(DEBUG_ERROR);   break;
    }

    // Copy and add prefix
    char tmp[1000];
    int len = snprintf(tmp, sizeof(tmp), "%s%s", prefix, str);

    // Remove trailing \n
    if ( (len > 0) && (tmp[len - 1] == '\n') )
    {
        tmp[len - 1] = '\0';
    }

    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    logWidget->AddLine(tmp, colour);
}

/* ****************************************************************************************************************** */
