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
#include <chrono>

#include <sys/time.h>
#include <sys/resource.h>

#include "platform.hpp"
#include "config.h"

#include "gui_inc.hpp"

#include "gui_win_app.hpp"
#include "gui_win_data.hpp"
#include "gui_win_experiment.hpp"
#include "gui_win_play.hpp"

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

GuiApp::GuiApp(const std::vector<std::string> &argv, const GuiAppEarlyLog &earlyLog,
    const std::vector<std::string> &versionInfos) :
    _statsLast     { },
    _statsTime     { },
    _statsCpu      { },
    _statsRss      { },
    _debugWinOpen  { false },
    _debugWinDim   { true },
    _versionInfos  { versionInfos },
    _h4xx0rMode    { false }
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

    // TODO: Parse command line arguments
    // -c <configfile>
    // ...

    // Load settings
    _settingsFile = Platform::ConfigFile("cfggui.conf");
    _settings.LoadConf(_settingsFile);

    // Create application windows
    _appWindows.resize(_NUM_APP_WIN);
    _appWindows[APP_WIN_ABOUT]            = std::make_unique<GuiWinAppAbout>();
    _appWindows[APP_WIN_SETTINGS]         = std::make_unique<GuiWinAppSettings>(std::bind(&GuiApp::_DrawSettingsEditor, this));
    _appWindows[APP_WIN_HELP]             = std::make_unique<GuiWinAppHelp>();
#ifndef IMGUI_DISABLE_DEMO_WINDOWS
    _appWindows[APP_WIN_IMGUI_DEMO]       = std::make_unique<GuiWinAppImguiDemo>();
    _appWindows[APP_WIN_IMPLOT_DEMO]      = std::make_unique<GuiWinAppImplotDemo>();
#endif
#ifndef IMGUI_DISABLE_METRICS_WINDOW
    _appWindows[APP_WIN_IMGUI_METRICS]    = std::make_unique<GuiWinAppImguiMetrics>();
#endif
    _appWindows[APP_WIN_IMPLOT_METRICS]   = std::make_unique<GuiWinAppImplotMetrics>();
    _appWindows[APP_WIN_IMGUI_STYLES]     = std::make_unique<GuiWinAppImguiStyles>();
    _appWindows[APP_WIN_IMPLOT_STYLES]    = std::make_unique<GuiWinAppImplotStyles>();
    _appWindows[APP_WIN_EXPERIMENT]       = std::make_unique<GuiWinExperiment>();
    _appWindows[APP_WIN_PLAY]             = std::make_unique<GuiWinPlay>();
    _appWindows[APP_WIN_LEGEND]           = std::make_unique<GuiWinAppLegend>();

    // Load settings
    GuiSettings::GetValue("App.debugWindow", _debugWinOpen, false);
    GuiSettings::GetValue("App.settingsWindow",   *_appWindows[APP_WIN_SETTINGS]->WinOpenFlag(), false);
    GuiSettings::GetValue("App.aboutWindow",      *_appWindows[APP_WIN_ABOUT]->WinOpenFlag(), false);
    GuiSettings::GetValue("App.helpWindow",       *_appWindows[APP_WIN_HELP]->WinOpenFlag(), false);
    GuiSettings::GetValue("App.playWindow",       *_appWindows[APP_WIN_PLAY]->WinOpenFlag(), false);
    GuiSettings::GetValue("App.legendWindow",     *_appWindows[APP_WIN_LEGEND]->WinOpenFlag(), false);
    GuiSettings::GetValue("App.experimentWindow", *_appWindows[APP_WIN_EXPERIMENT]->WinOpenFlag(), false);
    _debugLog.SetSettings(GuiSettings::GetValue("App.debugLog"));

    // Load previous receiver and logfile windows
    const std::vector<std::string> receiverWinNames = GuiSettings::GetValueList("App.receiverWindows", ",", MAX_SAVED_WINDOWS);
    if (!receiverWinNames.empty())
    {
        for (auto &winName: receiverWinNames)
        {
            _CreateInputWindow("Receiver", _receiverWindows, winName);
        }
    }
    const std::vector<std::string> logfileWinNames = GuiSettings::GetValueList("App.logfileWindows", ",", MAX_SAVED_WINDOWS);
    if (!logfileWinNames.empty())
    {
        for (auto &winName: logfileWinNames)
        {
            _CreateInputWindow("Logfile", _logfileWindows, winName);
        }
    }

    const std::vector<std::string> ntripWinNames = GuiSettings::GetValueList("App.ntripWindows", ",", MAX_SAVED_WINDOWS);
    if (!ntripWinNames.empty())
    {
        for (auto &winName: ntripWinNames)
        {
            _CreateNtripWindow(winName);
        }
    }
    _UpdateNtripWindows();

    // Say hello
    NOTICE("cfggui " CONFIG_VERSION " (" CONFIG_GITHASH ")");
    GuiNotify::Notice("Hello, hello", "Good morning, good evening!");
}

// ---------------------------------------------------------------------------------------------------------------------

GuiApp::~GuiApp()
{
    DEBUG("GuiApp::~GuiApp()");
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::Shutdown()
{
    // Reconfigure debug output to terminal only
    debugSetup(&_debugCfgOrig);

    DEBUG("GuiApp::Shutdown()");

    // Remember some settings
    GuiSettings::SetValue("App.debugWindow",      _debugWinOpen);
    GuiSettings::SetValue("App.settingsWindow",   _appWindows[APP_WIN_SETTINGS]->WinIsOpen());
    GuiSettings::SetValue("App.aboutWindow",      _appWindows[APP_WIN_ABOUT]->WinIsOpen());
    GuiSettings::SetValue("App.helpWindow",       _appWindows[APP_WIN_HELP]->WinIsOpen());
    GuiSettings::SetValue("App.playWindow",       _appWindows[APP_WIN_PLAY]->WinIsOpen());
    GuiSettings::SetValue("App.legendWindow",     _appWindows[APP_WIN_LEGEND]->WinIsOpen());
    GuiSettings::SetValue("App.experimentWindow", _appWindows[APP_WIN_EXPERIMENT]->WinIsOpen());
    GuiSettings::SetValue("App.debugLog", _debugLog.GetSettings());

    std::vector<std::string> openReceiverWinNames;
    for (auto &win: _receiverWindows)
    {
        openReceiverWinNames.push_back(win->WinName());
    }
    GuiSettings::SetValueList("App.receiverWindows", openReceiverWinNames, ",", MAX_SAVED_WINDOWS);

    std::vector<std::string> openLogfileWinNames;
    for (auto &win: _logfileWindows)
    {
        openLogfileWinNames.push_back(win->WinName());
    }
    GuiSettings::SetValueList("App.logfileWindows", openLogfileWinNames, ",", MAX_SAVED_WINDOWS);

    std::vector<std::string> openNtripWinNames;
    for (auto &win: _ntripWindows)
    {
        openNtripWinNames.push_back(win->WinName());
    }
    GuiSettings::SetValueList("App.ntripWindows", openNtripWinNames, ",", MAX_SAVED_WINDOWS);

    // Destroy child windows, so that they can save their settings before we save the file below
    _appWindows.clear();
    _receiverWindows.clear();
    _logfileWindows.clear();

    // Save settings file
    if (_settingsFile.size() > 0)
    {
        _settings.SaveConf(_settingsFile);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiApp::PrepFrame()
{
    return _settings.UpdateFonts();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::NewFrame()
{
    _settings.UpdateSizes();
}

// ---------------------------------------------------------------------------------------------------------------------

ImVec4 GuiApp::BackgroundColour()
{
    if (_h4xx0rMode)
    {
        return ImVec4(0.0f, 0.0f, 0.0f, GUI_COLOUR4(APP_BACKGROUND).w);
    }
    else
    {
        return GUI_COLOUR4(APP_BACKGROUND);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_DrawSettingsEditor()
{
    _settings.DrawSettingsEditor();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::Loop(const uint32_t &frame, const double &now)
{
    // Run windows loops
    for (auto &win: _appWindows)
    {
        win->Loop(frame, now);
    }
    for (auto &win: _ntripWindows)
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

    _notifications.Loop(now);

    if (_h4xx0rMode)
    {
        _LoopMatrix(frame, now);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::DrawFrame()
{
    _MainMenu();

    // Draw app windows
    for (auto &win: _appWindows)
    {
        if (win->WinIsOpen())
        {
            win->DrawWindow();
        }
    }

    // Draw ntrip windows, remove and destroy closed ones
    for (auto iter = _ntripWindows.begin(); iter != _ntripWindows.end(); )
    {
        auto &ntripWin = *iter;
        if (ntripWin->WinIsOpen())
        {
            ntripWin->DrawWindow();
            iter++;
        }
        else
        {
            iter = _ntripWindows.erase(iter);
        }
    }

    // Draw receiver windows, remove and destroy closed ones
    for (auto iter = _receiverWindows.begin(); iter != _receiverWindows.end(); )
    {
        auto &receiverWin = *iter;
        if (receiverWin->WinIsOpen())
        {
            receiverWin->DrawWindow();
            receiverWin->DrawDataWindows();
            iter++;
        }
        else
        {
            iter = _receiverWindows.erase(iter);
            _UpdateNtripWindows();
        }
    }

    // Draw logfile windows, remove and destroy closed ones
    for (auto iter = _logfileWindows.begin(); iter != _logfileWindows.end(); )
    {
        auto &logfileWin = *iter;
        if (logfileWin->WinIsOpen())
        {
            logfileWin->DrawWindow();
            logfileWin->DrawDataWindows();
            iter++;
        }
        else
        {
            iter = _logfileWindows.erase(iter);
        }
    }

    _perfData[Perf_e::CPU].Add(_statsCpu);

    // Debug window
    if (_debugWinOpen)
    {
        _DrawDebugWin();
    }

    if (_h4xx0rMode)
    {
        _DrawMatrix();
    }

    // Notifications
    _notifications.Draw();
}

// ---------------------------------------------------------------------------------------------------------------------

template<typename T> void GuiApp::_CreateInputWindow(
    const std::string &baseName, std::vector< std::unique_ptr<T> > &inputWindows, const std::string &prevWinName)
{
    std::vector<std::string> existingWinNames;
    for (auto &win: inputWindows)
    {
        existingWinNames.push_back(win->WinName());
    }

    int winNumber = 0;
    // Open a previous window
    if (!prevWinName.empty())
    {
        if (std::find(existingWinNames.begin(), existingWinNames.end(), prevWinName) == existingWinNames.end())
        {
            try { winNumber = std::stoi(prevWinName.substr(baseName.size())); } catch (...) { }
        }
    }
    // Find next free
    else
    {
        int n = 1;
        while (n < 1000)
        {
            const std::string newWinName = baseName + std::to_string(n); // BaseName1, BaseName2, ...
            if (std::find(existingWinNames.begin(), existingWinNames.end(), newWinName) == existingWinNames.end())
            {
                winNumber = n;
                break;
            }
            n++;
        }
    }

    // Create it
    if (winNumber > 0)
    {
        try
        {
            auto win = std::make_unique<T>(baseName + std::to_string(winNumber));
            win->WinOpen();
            win->WinSetTitle(baseName + " " + std::to_string(winNumber));
            win->OpenPreviousDataWin();
            inputWindows.push_back(std::move(win));
            std::sort(inputWindows.begin(), inputWindows.end(),
                [](const std::unique_ptr<T> &a, const std::unique_ptr<T> &b)
                {
                    return a->WinName() < b->WinName();
                });
        }
        catch (std::exception &e)
        {
            ERROR("new %s%d: %s", baseName.c_str(), winNumber, e.what());
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_CreateNtripWindow(const std::string &prevWinName)
{
    const std::string baseName = "NtripClient";
    std::vector<std::string> existingWinNames;
    for (auto &win: _ntripWindows)
    {
        existingWinNames.push_back(win->WinName());
    }

    int winNumber = 0;
    // Open a previous window
    if (!prevWinName.empty())
    {
        if (std::find(existingWinNames.begin(), existingWinNames.end(), prevWinName) == existingWinNames.end())
        {
            try { winNumber = std::stoi(prevWinName.substr(baseName.size())); } catch (...) { }
        }
    }
    // Find next free
    else
    {
        int n = 1;
        while (n < 1000)
        {
            const std::string newWinName = baseName + std::to_string(n); // BaseName1, BaseName2, ...
            if (std::find(existingWinNames.begin(), existingWinNames.end(), newWinName) == existingWinNames.end())
            {
                winNumber = n;
                break;
            }
            n++;
        }
    }

    // Create it
    if (winNumber > 0)
    {
        auto win = std::make_unique<GuiWinNtrip>(baseName + std::to_string(winNumber));
        win->WinOpen();
        win->WinSetTitle("NTRIP client " + std::to_string(winNumber));
        _ntripWindows.emplace_back(std::move(win));

        std::sort(_ntripWindows.begin(), _ntripWindows.end(),
            [](const auto &a, const auto &b)
            {
                return a->WinName() < b->WinName();
            });
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_UpdateNtripWindows()
{
    for (auto &ntripWin: _ntripWindows)
    {
        ntripWin->RemoveReceivers();
        for (auto &receiverWin: _receiverWindows)
        {
            ntripWin->AddReceiver(receiverWin->GetReceiver());
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_MainMenu()
{
    bool h4xx0rPopup = false;
    if (ImGui::BeginMainMenuBar())
    {
        // Store menu bar height FIXME: is there a better way than doing this here for every frame?
        _menuBarHeight = ImGui::GetWindowSize().y;

        ImGui::PushStyleColor(ImGuiCol_PopupBg, GuiSettings::style->Colors[ImGuiCol_MenuBarBg]);

        if (ImGui::BeginMenu(ICON_FK_HEART " Receiver"))
        {
            if (ImGui::MenuItem("New"))
            {
                _CreateInputWindow("Receiver", _receiverWindows);
                _UpdateNtripWindows();
            }
            if (!_receiverWindows.empty())
            {
                ImGui::Separator();
                for (auto &receiverWin: _receiverWindows)
                {
                    if (ImGui::BeginMenu(receiverWin->WinTitle().c_str()))
                    {
                        receiverWin->DataWinMenu();
                        ImGui::EndMenu();
                    }
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(ICON_FK_FOLDER " Logfile"))
        {
            if (ImGui::MenuItem("New"))
            {
                _CreateInputWindow("Logfile", _logfileWindows);
            }
            if (!_logfileWindows.empty())
            {
                ImGui::Separator();
                for (auto &logfileWin: _logfileWindows)
                {
                    if (ImGui::BeginMenu(logfileWin->WinTitle().c_str()))
                    {
                        logfileWin->DataWinMenu();
                        ImGui::EndMenu();
                    }
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(ICON_FK_COG " Tools"))
        {
            const int toolMenuEntries[] =
            {
                APP_WIN_ABOUT, APP_WIN_SETTINGS, APP_WIN_HELP, APP_WIN_PLAY
            };
            for (int ix = 0; ix < NUMOF(toolMenuEntries); ix++)
            {
                auto &win = _appWindows[toolMenuEntries[ix]];
                if (ImGui::MenuItem(win->WinName().c_str()))
                {
                    win->WinOpen();
                }
            }
            ImGui::Separator();

            if (ImGui::BeginMenu("NTRIP"))
            {
                if (ImGui::MenuItem("New client"))
                {
                    _CreateNtripWindow();
                    _UpdateNtripWindows();
                }
                if (!_ntripWindows.empty())
                {
                    ImGui::Separator();
                    for (auto &win: _ntripWindows)
                    {
                        if (ImGui::MenuItem(win->WinTitle().c_str()))
                        {
                            win->WinOpen();
                        }
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::Separator();

            ImGui::MenuItem("Debug", NULL, &_debugWinOpen);

            bool h4xx0r = _h4xx0rMode;
            if (ImGui::MenuItem("H4xx0r", NULL, &h4xx0r))
            {
                if (h4xx0r)
                {
                    h4xx0rPopup = true;
                }
                else
                {
                    _h4xx0rMode = _ConfigMatrix(false);
                }
            }
            ImGui::EndMenu();
        }

        ImGui::PopStyleColor();

        // Update (some) performance info
        const double now = ImGui::GetTime();
        const double delta = now - _statsLast;
        if (delta >= 1.0)
        {
            _statsLast = now;
            struct rusage usage;
            if (getrusage(RUSAGE_SELF, &usage) == 0)
            {
                _statsRss = (float)usage.ru_maxrss / 1024.0f;
            }
            struct timespec time;
            double cpu = 0.0;
            if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time) == 0)
            {
                cpu = ((double)(time.tv_sec - _statsTime.tv_sec))
                    + ((double)(time.tv_nsec - _statsTime.tv_nsec) * 1e-9);
                _statsTime = time;
            }
            _statsCpu = cpu / delta;
        }

        // Draw performance info
        char text[200];
        ImGuiIO &io = ImGui::GetIO();
        snprintf(text, sizeof(text),
            ICON_FK_TACHOMETER " %2.0f | " ICON_FK_CLOCK_O " %.1f | " ICON_FK_FILM " %d | " ICON_FK_THERMOMETER_HALF " %.0fMB %2.0f%% ", // <-- FIXME: CalcTextSize() misses one char?!
            io.Framerate, // io.Framerate > FLT_EPSILON ? 1000.0f / io.Framerate : 0.0f,
            //io.DeltaTime > FLT_EPSILON ? 1.0 / io.DeltaTime : 0.0, io.DeltaTime * 1e3,
            ImGui::GetTime(), ImGui::GetFrameCount(),
            _statsRss, _statsCpu * 1e2);
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

    if (h4xx0rPopup)
    {
        ImGui::OpenPopup("H4xx0rConfig");
    }
    if (ImGui::BeginPopup("H4xx0rConfig"))
    {
        _DrawMatrixConfig();
        ImGui::Separator();
        if (ImGui::Button("Start!"))
        {
            _h4xx0rMode = _ConfigMatrix(true);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
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
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x, _menuBarHeight), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(500,200), ImVec2(io.DisplaySize.x, io.DisplaySize.y - _menuBarHeight));

    // Style window (no border, no round corners, no alpha if focused/active, bg colour same as menubar)
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, _debugWinDim ? 0.75f * GuiSettings::style->Alpha : 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, GuiSettings::style->Colors[ImGuiCol_MenuBarBg]);

    // Start window
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
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
                const ImVec2 buttonSize { GuiSettings::charSize.x * 20, 0 };

                const struct { const char *label; int win; bool newline; } buttons[] =
                {
                    { "ImGui demo",     APP_WIN_IMGUI_DEMO,       true  },
                    { "ImGui metrics",  APP_WIN_IMGUI_METRICS,    false },
                    { "ImGui styles",   APP_WIN_IMGUI_STYLES,     false },
                    { "ImPlot demo",    APP_WIN_IMPLOT_DEMO,      true  },
                    { "ImPlot metrics", APP_WIN_IMPLOT_METRICS,   false },
                    { "Experiments",    APP_WIN_EXPERIMENT,       true  },
                };
                for (int ix = 0; ix < NUMOF(buttons); ix++)
                {
                    if (!buttons[ix].newline)
                    {
                        ImGui::SameLine();
                    }
                    if (ImGui::Button(buttons[ix].label, buttonSize) && _appWindows[buttons[ix].win])
                    {
                        if (!_appWindows[buttons[ix].win]->WinIsOpen())
                        {
                            _appWindows[buttons[ix].win]->WinOpen();
                        }
                        else
                        {
                            _appWindows[buttons[ix].win]->WinFocus();
                        }
                    }
                }

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Perf"))
            {
                const ImVec2 sizeAvail = ImGui::GetContentRegionAvail();
                const ImVec2 plotSize1 { sizeAvail.x, (sizeAvail.y * 2 / 3) - GuiSettings::style->ItemSpacing.y };
                const ImVec2 plotSize2 { ( (sizeAvail.x - ((_NUM_PERF - 1) * GuiSettings::style->ItemSpacing.x)) / _NUM_PERF), sizeAvail.y - plotSize1.y - GuiSettings::style->ItemSpacing.y };

                const struct { const char *title; const char *label; enum Perf_e perf; } plots[] =
                {
                    { "##Newframe",     "Newframe",      Perf_e::NEWFRAME },
                    { "##Loop",         "Loop",          Perf_e::LOOP },
                    { "##Draw",         "Draw",          Perf_e::DRAW },
                    { "##RenderImGui",  "Render ImGui",  Perf_e::RENDER_IM },
                    { "##RenderOpenGL", "Render OpenGL", Perf_e::RENDER_IM },
                    { "##Total",        "Total",         Perf_e::TOTAL },
                    { "##CPU",          "CPU load",      Perf_e::CPU },
                };

                if (ImPlot::BeginPlot("##Performance", plotSize1, ImPlotFlags_Crosshairs | ImPlotFlags_NoMenus))
                {
                    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100, ImGuiCond_Once);
                    ImPlot::SetupAxisLimits(ImAxis_X1, 0, _perfData[0].data.size(), ImGuiCond_Always);
                    ImPlot::SetupLegend(ImPlotLocation_South, ImPlotLegendFlags_Horizontal);
                    ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoTickLabels);
                    ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_LogScale);
                    ImPlot::SetupFinish();
                    for (int plotIx = 0; plotIx < NUMOF(plots); plotIx++)
                    {
                        PerfData *pd = &_perfData[plots[plotIx].perf];
                        // PlotLineG, PlotStairsG
                        ImPlot::PlotLineG(plots[plotIx].label, [](void *arg, int ix)
                            {
                                const PerfData *perf = (PerfData *)arg;
                                return ImPlotPoint( ix, perf->data[ (perf->ix + ix) % perf->data.size() ] );
                            }, pd, pd->data.size());
                    }
                    ImPlot::EndPlot();
                }

                const ImPlotRange range; // (0.0, 2.0);
                for (int plotIx = 0; plotIx < NUMOF(plots); plotIx++)
                {
                    if (ImPlot::BeginPlot(plots[plotIx].title, plotSize2, ImPlotFlags_Crosshairs | ImPlotFlags_NoMenus | ImPlotFlags_NoLegend))
                    {
                        ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks);
                        ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks);
                        ImPlot::SetupFinish();
                        PerfData &pd = _perfData[plots[plotIx].perf];
                        ImPlot::SetNextFillStyle(ImPlot::GetColormapColor(plotIx));
                        ImPlot::PlotHistogram(plots[plotIx].label, pd.data.data(), pd.data.size(), ImPlotBin_Sturges, false, false, range, true, 0.8);
                        ImPlot::EndPlot();
                    }
                    if (plotIx < (NUMOF(plots) - 1))
                    {
                        ImGui::SameLine();
                    }
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Versions"))
            {
                ImGui::PushTextWrapPos(0.0f);
                for (const auto &info: _versionInfos)
                {
                    ImGui::TextUnformatted(info.c_str());
                }
                ImGui::PopTextWrapPos();
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


// ---------------------------------------------------------------------------------------------------------------------

GuiApp::PerfData::PerfData() :
    ix{0}
{
    for (int i = 0; i < (int)data.size(); i++)
    {
        data[i] = NAN;
    }
}

void GuiApp::PerfData::Tic()
{
    t0 = std::chrono::system_clock::now();
}

void GuiApp::PerfData::Toc()
{
    const auto t1 = std::chrono::system_clock::now();
    const std::chrono::duration<float, std::milli> dt = t1 - t0;
    Add(dt.count());
}

void GuiApp::PerfData::Add(const float value)
{
    data[ix] = value;
    ix++;
    ix %= data.size();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::PerfTic(const enum Perf_e perf)
{
    if (perf < _NUM_PERF)
    {
        _perfData[perf].Tic();
    }
}

void GuiApp::PerfToc(const enum Perf_e perf)
{
    if (perf < _NUM_PERF)
    {
        _perfData[perf].Toc();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiApp::_ConfigMatrix(const bool enable)
{
    DEBUG("H4xx0r mode: %s", enable ? "enable" : "disable");
    if (enable)
    {
        if (!_matrix.Init(_matrixOpts))
        {
            return false;
        }
        _matrixLastRender = ImGui::GetTime();
    }
    else
    {
        _matrix.Destroy();
    }
    return enable;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_LoopMatrix(const uint32_t &frame, const double &now)
{
    UNUSED(frame);
    const double rate = 50.0; // 1/s
    int nIter = (now - _matrixLastRender + (0.5 / rate)) * rate;
    _matrixLastRender = now;
    nIter = CLIP(nIter, 1, 10);
    while (nIter > 0)
    {
        _matrix.Animate();
        nIter--;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_DrawMatrix()
{
    ImGuiViewport *viewPort = ImGui::GetMainViewport();
    if (_matrixFb.Begin(viewPort->Size.x, viewPort->Size.y))
    {
        _matrix.Render(viewPort->Size.x, viewPort->Size.y);

        _matrixFb.End();
        void *tex = _matrixFb.GetTexture();
        if (tex)
        {
            FfVec2f pos = viewPort->Pos;
            FfVec2f size = viewPort->Size;
            ImDrawList *draw = ImGui::GetBackgroundDrawList();
            draw->AddImage(tex, pos, pos + size, ImVec2(0,1), ImVec2(1,0));
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_DrawMatrixConfig()
{
    const struct { enum GlMatrix::Options::Mode_e mode; const char *label; } modes[] =
    {
        { GlMatrix::Options::MATRIX,      "MATRIX"      },
        { GlMatrix::Options::DNA,         "DNA"         },
        { GlMatrix::Options::BINARY,      "BINARY"      },
        { GlMatrix::Options::HEXADECIMAL, "HEXADECIMAL" },
        { GlMatrix::Options::DECIMAL,     "DECIMAL"     },
    };
    const char *modeStr = "?";
    for (int ix = 0; ix < NUMOF(modes); ix++)
    {
        if (modes[ix].mode == _matrixOpts.mode)
        {
            modeStr = modes[ix].label;
            break;
        }
    }
    if (ImGui::BeginCombo("Mode", modeStr))
    {
        for (int ix = 0; ix < NUMOF(modes); ix++)
        {
            if (ImGui::Selectable(modes[ix].label, modes[ix].mode == _matrixOpts.mode))
            {
                _matrixOpts.mode = modes[ix].mode;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SliderFloat("Speed",        &_matrixOpts.speed, 0.1f, 5.0f);
    ImGui::SliderFloat("Density",      &_matrixOpts.density, 1.0f, 200.0f, "%.0f");
    ImGui::Checkbox(   "Clock",        &_matrixOpts.doClock);
    ImGui::Checkbox(   "Fog",          &_matrixOpts.doFog);
    ImGui::Checkbox(   "Waves",        &_matrixOpts.doWaves);
    ImGui::Checkbox(   "Rotate",       &_matrixOpts.doRotate);
    ImGui::Checkbox(   "Debug grid",   &_matrixOpts.debugGrid);
    ImGui::Checkbox(   "Debug frames", &_matrixOpts.debugFrames);
    if (ImGui::Button("Defaults"))
    {
        _matrixOpts = GlMatrix::Options();
    }
}

/* ****************************************************************************************************************** */
