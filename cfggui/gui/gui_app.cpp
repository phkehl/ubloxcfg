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

#include "gui_win_data_config.hpp"
#include "gui_win_data_fwupdate.hpp"
#include "gui_win_data_inf.hpp"
#include "gui_win_data_legend.hpp"
#include "gui_win_data_log.hpp"
#include "gui_win_data_map.hpp"
#include "gui_win_data_messages.hpp"
#include "gui_win_data_plot.hpp"
#include "gui_win_data_scatter.hpp"
#include "gui_win_data_signals.hpp"
#include "gui_win_data_satellites.hpp"
#include "gui_win_data_stats.hpp"

#include "platform.hpp"

#include "config.h"

#include "gui_app.hpp"

/* ****************************************************************************************************************** */

static void _debugLog(const DEBUG_LEVEL_t level, const char *str, const DEBUG_CFG_t *cfg)
{
    GuiApp *app = static_cast<GuiApp *>(cfg->arg);
    // Add to log window
    app->DebugLog(level, str);
    // Also add to console
    if ( (cfg->level >= DEBUG_LEVEL_DEBUG) || (level >= DEBUG_LEVEL_WARNING) )
    {
        fputs(str, stderr);
    }
}

GuiAppInitialLog::GuiAppInitialLog() :
    log{}
{
}

void GuiAppInitialLog::Add(DEBUG_LEVEL_t level, std::string line)
{
    log.push_back( { level, line } );
}

void GuiAppInitialLog::Clear()
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

GuiApp::GuiApp(const std::vector<std::string> &argv, const GuiAppInitialLog &initLog) :
    _settingsFile{}, _settings{nullptr},
    _appWindows{}, _receivers{},
    _debugMutex{},
    _statsLast{}, _statsTime{}, _statsCpu{}, _statsRss{}
{
    UNUSED(argv);
    UNUSED(initLog);

    DEBUG("GuiApp()");
    gGuiAppInstance = this;

    _settingsFile = Platform::ConfigFile("cfggui.conf");

    _settings = std::make_shared<GuiSettings>(_settingsFile);

    // Create application windows

    _winDebug        = std::make_unique<GuiWinDebug>();
    _appWindows.push_back(_winDebug.get());

    _winAbout        = std::make_unique<GuiWinAbout>();
    _appWindows.push_back(_winAbout.get());

    _winSettings     = std::make_unique<GuiWinSettings>();
    _appWindows.push_back(_winSettings.get());

    _winHelp         = std::make_unique<GuiWinHelp>();
    _appWindows.push_back(_winHelp.get());

#ifndef IMGUI_DISABLE_DEMO_WINDOWS
    _winImguiDemo    = std::make_unique<GuiWinImguiDemo>();
    _appWindows.push_back(_winImguiDemo.get());

    _winImplotDemo   = std::make_unique<GuiWinImplotDemo>();
    _appWindows.push_back(_winImplotDemo.get());
#endif

#ifndef IMGUI_DISABLE_METRICS_WINDOW
    _winImguiMetrics = std::make_unique<GuiWinImguiMetrics>();
    _appWindows.push_back(_winImguiMetrics.get());
#endif
    _winImplotMetrics = std::make_unique<GuiWinImplotMetrics>();
    _appWindows.push_back(_winImplotMetrics.get());

    _winImguiStyles  = std::make_unique<GuiWinImguiStyles>();
    _appWindows.push_back(_winImguiStyles.get());

    _winExperiment   = std::make_unique<GuiWinExperiment>();
    _appWindows.push_back(_winExperiment.get());

    //_winImguiAbout   = std::make_unique<GuiWinImguiAbout>();
    //_appWindows.push_back(_winImguiAbout.get());

    // Add initial log
    for (auto &entry: initLog.log)
    {
        DebugLog(entry.first, entry.second.c_str());
    }

    // Redirect debug output to debug window
    debugGetCfg(&_debugCfgOrig);
    _debugCfg.level  = DEBUG_LEVEL_DEBUG;
    _debugCfg.colour = _debugCfgOrig.colour;
    _debugCfg.mark   = NULL;
    _debugCfg.func   = _debugLog;
    _debugCfg.arg    = this;
    debugSetup(&_debugCfg);

    NOTICE("cfggui " CONFIG_VERSION " (" CONFIG_GITHASH ")");

    _CreateReceiver();
}

// ---------------------------------------------------------------------------------------------------------------------

GuiApp::~GuiApp()
{
    // Return debug output to terminal
    debugSetup(&_debugCfgOrig);

    DEBUG("GuiApp::~GuiApp()");

    // Remember which receiver windows were open
    std::vector<std::string> openWinNames;
    for (auto &receiver: _receivers)
    {
        openWinNames.push_back(receiver->rxWin->GetName());
    }
    _settings->SetValue("ReceiverWindows", Ff::StrJoin(openWinNames, ","));

    // Remove the callbacks before destructing the AppReceiver objects and the objects they contain
    for (auto &receiver: _receivers)
    {
        _StoreRxWinSettings(receiver);
        receiver->receiver->SetDataCb(nullptr);
        receiver->rxWin->SetCallbacks(nullptr, nullptr, nullptr);
    }

    _receivers.clear();

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

void GuiApp::DebugLog(const DEBUG_LEVEL_t level, const char *str)
{
    std::lock_guard<std::mutex> guard(_debugMutex);

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

    _winDebug->AddLog(tmp, colour);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::Loop()
{
    const uint32_t frame = ImGui::GetFrameCount();
    const double now = ImGui::GetTime();

    // Run windows loops
    for (auto win: _appWindows)
    {
        win->Loop(frame, now);
    }

    // Loop windows
    for (auto iter = _receivers.begin(); iter != _receivers.end(); )
    {
        auto &receiver = *iter;

        // Close/destroy receiver windows that are no longer needed and remove them from the list
        if (!receiver->rxWin->IsOpen())
        {
            _StoreRxWinSettings(receiver);
            iter = _receivers.erase(iter);
        }
        // Run loop
        else
        {
            receiver->rxWin->Loop(frame, now);
            for (auto &dataWin: receiver->dataWin)
            {
                dataWin->Loop(frame, now);
            }
            iter++;
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::DrawFrame()
{
    _MainMenu();

    // Draw app windows
    for (auto win: _appWindows)
    {
        if (win->IsOpen())
        {
            win->DrawWindow();
        }
    }

    // Draw receiver windows
    for (auto &receiver: _receivers)
    {
        if (receiver->rxWin->IsOpen())
        {
            receiver->rxWin->DrawWindow();

            // Draw data windows, remove closed ones
            for (auto iter = receiver->dataWin.begin(); iter != receiver->dataWin.end(); )
            {
                auto &dataWin = *iter;

                // Close/destroy receiver windows that are no longer needed and remove them from the list
                if (!dataWin->IsOpen())
                {
                    iter = receiver->dataWin.erase(iter);
                }
                // Draw
                else
                {
                    dataWin->DrawWindow();
                    iter++;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_CreateReceiver()
{
    // Find next free receiver name
    std::string name;
    int n = 1;
    while (n < 1000)
    {
        name = std::string("Receiver") + std::to_string(n);

        bool nameUnused = true;
        for (auto &receiver: _receivers)
        {
            if (name == receiver->rxWin->GetName())
            {
                nameUnused = false;
                break;
            }
        }
        if (nameUnused)
        {
            break;
        }
        n++;
    }
    if (n >= 1000)
    {
        return;
    }

    // Create receiver database, receiver object and receiver window
    try
    {
        auto database = std::make_shared<Database>(10000);
        auto receiver = std::make_shared<Receiver>(name, database);
        auto rxWin    = std::make_unique<GuiWinReceiver>(name, receiver, database);
        rxWin->SetTitle(std::string("Receiver ") + std::to_string(n));

        auto appReceiver = std::make_unique<AppReceiver>(database, receiver, std::move(rxWin));

        // Note:
        // - We are using plain AppReceiver pointer (because I don't know how to do it otherwise. Can we pass a const &AppReceiver?)
        // - We carefully remove the callbacks in ~GuiApp()
        appReceiver->receiver->SetDataCb( std::bind(&GuiApp::_ProcessDataReceiverCb,    this, appReceiver.get(), std::placeholders::_1) );
        appReceiver->rxWin->SetCallbacks( std::bind(&GuiApp::_DataWinButtonsReceiverCb, this, appReceiver.get()),
                                          std::bind(&GuiApp::_TitleChangeReceiverCb,    this, appReceiver.get()),
                                          std::bind(&GuiApp::_ClearReceiverCb,          this, appReceiver.get()));

        // Open previous data windows
        _OpenPreviousDataWin(appReceiver);

        // Add to list
        _receivers.push_back( std::move(appReceiver) );

        // Keep list ordered by name
        std::sort(_receivers.begin(), _receivers.end(),
            [](const std::unique_ptr<AppReceiver> &a, const std::unique_ptr<AppReceiver> &b)
            {
                return a->rxWin->GetName() < b->rxWin->GetName();
            });

    }
    //catch (std::bad_alloc &e)
    //{
    //    ERROR("new receiver: %s", e.what());
    //}
    catch (std::exception &e)
    {
        ERROR("new receiver: %s", e.what());
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_ProcessDataReceiverCb(const GuiApp::AppReceiver *appReceiver, const Data &data)
{
    appReceiver->rxWin->ProcessData(data);
    for (auto &dataWin: appReceiver->dataWin)
    {
        dataWin->ProcessData(data);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

#define _CREATE_RET  std::unique_ptr<GuiWinData>
#define _CREATE_ARGS const std::string &name, std::shared_ptr<Receiver> receiver, std::shared_ptr<Database> database
struct DataWindow
{
    const char   *name;
    const char   *title;
    const char   *button;
    bool          receiver;
    bool          logfile;
    std::function< _CREATE_RET ( _CREATE_ARGS ) > create;
};

static const DataWindow kDataWindows[] =
{
    {
        .name   = "Log",       .title  = "Log" ,              .button = ICON_FK_LIST_UL "##Log",          .receiver = true, .logfile = false,
        .create = [](_CREATE_ARGS) -> _CREATE_RET { return std::make_unique<GuiWinDataLog>(name, receiver, nullptr, database); }
    },
    {
        .name   = "Messages",  .title  = "Messages" ,         .button = ICON_FK_SORT_ALPHA_ASC "##Msgs",  .receiver = true, .logfile = false,
        .create = [](_CREATE_ARGS) -> _CREATE_RET { return std::make_unique<GuiWinDataMessages>(name, receiver, nullptr, database); }
    },
    {
        .name   = "Inf",       .title  = "Inf messages" ,     .button = ICON_FK_FILE_TEXT_O "##Inf",      .receiver = true, .logfile = false,
        .create = [](_CREATE_ARGS) -> _CREATE_RET { return std::make_unique<GuiWinDataInf>(name, receiver, nullptr, database); }
    },
    {
        .name   = "Scatter",   .title  = "Scatter plot" ,     .button = ICON_FK_CROSSHAIRS "##Scatter",   .receiver = true, .logfile = false,
        .create = [](_CREATE_ARGS) -> _CREATE_RET { return std::make_unique<GuiWinDataScatter>(name, receiver, nullptr, database); }
    },
    {
        .name   = "Signals",   .title  = "Signals" ,          .button = ICON_FK_BAR_CHART "##Signals",    .receiver = true, .logfile = false,
        .create = [](_CREATE_ARGS) -> _CREATE_RET { return std::make_unique<GuiWinDataSignals>(name, receiver, nullptr, database); }
    },
    {
        .name   = "Config",    .title  = "Configuration" ,    .button = ICON_FK_PAW "##Config",           .receiver = true, .logfile = false,
        .create = [](_CREATE_ARGS) -> _CREATE_RET { return std::make_unique<GuiWinDataConfig>(name, receiver, nullptr, database); }
    },
    {
        .name   = "Legend",    .title  = "Legend" ,           .button = ICON_FK_QUESTION "##Legend",      .receiver = true, .logfile = false,
        .create = [](_CREATE_ARGS) -> _CREATE_RET { return std::make_unique<GuiWinDataLegend>(name, receiver, nullptr, database); }
    },
    {
        .name   = "Plots",     .title  = "Plots" ,            .button = ICON_FK_LINE_CHART "##Plots",     .receiver = true, .logfile = false,
        .create = [](_CREATE_ARGS) -> _CREATE_RET { return std::make_unique<GuiWinDataPlot>(name, receiver, nullptr, database); }
    },
    {
        .name   = "Map",       .title  = "Map" ,              .button = ICON_FK_MAP "##Map",              .receiver = true, .logfile = false,
        .create = [](_CREATE_ARGS) -> _CREATE_RET { return std::make_unique<GuiWinDataMap>(name, receiver, nullptr, database); }
    },
    {
        .name   = "Satellites",.title  = "Satellites" ,       .button = ICON_FK_ROCKET "##Satellites",    .receiver = true, .logfile = false,
        .create = [](_CREATE_ARGS) -> _CREATE_RET { return std::make_unique<GuiWinDataSatellites>(name, receiver, nullptr, database); }
    },
    {
        .name   = "Stats",     .title  = "Statistics" ,       .button = ICON_FK_TABLE "##Stats",          .receiver = true, .logfile = false,
        .create = [](_CREATE_ARGS) -> _CREATE_RET { return std::make_unique<GuiWinDataStats>(name, receiver, nullptr, database); }
    },
    {
        .name   = "Fwupdate",  .title  = "Firmware update" ,  .button = ICON_FK_DOWNLOAD "##Fwupdate",    .receiver = true, .logfile = false,
        .create = [](_CREATE_ARGS) -> _CREATE_RET { return std::make_unique<GuiWinDataFwupdate>(name, receiver, nullptr, database); }
    },
};

void GuiApp::_DataWinButtonsReceiverCb(GuiApp::AppReceiver *appReceiver)
{
    (void)appReceiver;

    for (int ix = 0; ix < NUMOF(kDataWindows); ix++)
    {
        const DataWindow *dw = &kDataWindows[ix];
        if (dw->receiver)
        {
            if (ImGui::Button(dw->button, _settings->iconButtonSize))
            {

                std::string name;
                int n = 1;
                while (n < 1000)
                {
                    name = appReceiver->rxWin->GetName() + dw->name + std::to_string(n);
                    bool nameUnused = true;
                    for (auto &dataWin: appReceiver->dataWin)
                    {
                        if (name == dataWin->GetName())
                        {
                            nameUnused = false;
                            break;
                        }
                    }
                    if (nameUnused)
                    {
                        DEBUG("%s: add %s", appReceiver->rxWin->GetName().c_str(), name.c_str());
                        try
                        {
                            auto win = dw->create(name, appReceiver->receiver, appReceiver->database);
                            win->Open();
                            win->SetTitle(appReceiver->rxWin->GetTitle() + std::string(" - ") + dw->title + std::string(" ") + std::to_string(n));
                            appReceiver->dataWin.push_back( std::move(win) );
                        }
                        catch (std::exception &e)
                        {
                            ERROR("Failed creating %s window: %s", dw->title, e.what());
                        }
                        break;
                    }
                    n++;
                }
            }
            Gui::ItemTooltip(dw->title);
        }
        ImGui::SameLine();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_OpenPreviousDataWin(std::unique_ptr<GuiApp::AppReceiver> &appReceiver)
{
    std::string openDataWindows = "";
    _settings->GetValue(appReceiver->rxWin->GetName(), openDataWindows, "");
    if (openDataWindows.empty())
    {
        return;
    }

    std::vector<std::string> dataWinNames = Ff::StrSplit(openDataWindows, ",");
    if (dataWinNames.empty())
    {
        return;
    }

    std::string rxWinName = appReceiver->rxWin->GetName(); // "Receiver1"
    for (const auto &dataWinName: dataWinNames) // "Receiver1Scatter1", "Receiver1Map1", ...
    {
        try
        {
            std::string name = dataWinName.substr(rxWinName.size()); // ""Receiver1Map1" -> "Map1"
            for (int ix = 0; ix < NUMOF(kDataWindows); ix++)
            {
                const DataWindow *dw = &kDataWindows[ix];
                const int nameLen = strlen(dw->name);
                if (std::strncmp(name.c_str(), dw->name, nameLen) != 0) // "Map"(1) == "Map", "Scatter", ... ?
                {
                    continue;
                }
                std::string dataWinName2 = rxWinName + name; // "Receiver1Map1"
                auto win = dw->create(dataWinName2, appReceiver->receiver, appReceiver->database);
                win->Open();
                win->SetTitle(appReceiver->rxWin->GetTitle() + std::string(" - ") + dw->title + std::string(" ") + name.substr(nameLen));
                appReceiver->dataWin.push_back( std::move(win) );
                break;
            }
        }
        catch (std::exception &e)
        {
            WARNING("Failed opening window %s", dataWinName.c_str());
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_TitleChangeReceiverCb(AppReceiver *appReceiver)
{
    // "Receiver X - child" or "Receiver X: version - child"
    std::string mainTitle = appReceiver->rxWin->GetTitle();
    for (auto &dataWin: appReceiver->dataWin)
    {
        std::string childTitle = dataWin->GetTitle();
        const auto pos = childTitle.find(" - ");
        if (pos != std::string::npos)
        {
            childTitle.replace(0, pos, mainTitle);
            dataWin->SetTitle(childTitle);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_ClearReceiverCb(AppReceiver *appReceiver)
{
    for (auto &dataWin: appReceiver->dataWin)
    {
        dataWin->ClearData();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_StoreRxWinSettings(std::unique_ptr<GuiApp::AppReceiver> &appReceiver)
{
    // Remember which windows were open
    std::vector<std::string> openWinNames;
    for (auto &dataWin: appReceiver->dataWin)
    {
        openWinNames.push_back(dataWin->GetName());
    }
    _settings->SetValue(appReceiver->rxWin->GetName(), Ff::StrJoin(openWinNames, ","));
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiApp::_MainMenu()
{
    if (ImGui::BeginMainMenuBar())
    {
        _settings->menuBarHeight = ImGui::GetWindowSize().y;

        if (ImGui::BeginMenu("Receiver"))
        {
            if (ImGui::MenuItem("New"))
            {
                _CreateReceiver();
            }
            if (_receivers.size() > 0)
            {
                ImGui::Separator();

                for (auto &receiver: _receivers)
                {
                    char name[256];
                    snprintf(name, sizeof(name), "%s##%p", receiver->rxWin->GetTitle().c_str(), &receiver);
                    if (ImGui::MenuItem(name))
                    {
                        receiver->rxWin->Focus();
                    }
                }
            }
            ImGui::EndMenu();
        }
        // if (ImGui::BeginMenu("File"))
        // {
        //     ImGui::MenuItem("(dummy menu)", NULL, false, false);
        //     if (ImGui::MenuItem("New")) {}
        //     if (ImGui::MenuItem("Open", "Ctrl+O")) {}
        //     if (ImGui::MenuItem("Save", "Ctrl+S")) {}
        //     if (ImGui::MenuItem("Save As..")) {}
        //     ImGui::Separator();
        //     if (ImGui::MenuItem("Quit", "Alt+F4")) {}
        //     ImGui::EndMenu();
        // }
        // if (ImGui::BeginMenu("Edit"))
        // {
        //     if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
        //     if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
        //     ImGui::Separator();
        //     if (ImGui::MenuItem("Cut", "CTRL+X")) {}
        //     if (ImGui::MenuItem("Copy", "CTRL+C")) {}
        //     if (ImGui::MenuItem("Paste", "CTRL+V")) {}
        //     ImGui::EndMenu();
        // }
        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::MenuItem("About",       NULL, _winAbout->GetOpenFlag());
            ImGui::MenuItem("Settings",    NULL, _winSettings->GetOpenFlag());
            ImGui::MenuItem("Help",        NULL, _winHelp->GetOpenFlag());
            ImGui::MenuItem(/*ICON_FK_BUG " " */"Debug",       NULL, _winDebug->GetOpenFlag());
            if (ImGui::BeginMenu("Dear ImGui"))
            {
#ifndef IMGUI_DISABLE_DEMO_WINDOWS
                ImGui::MenuItem("ImGui demo",  NULL, _winImguiDemo->GetOpenFlag());
                ImGui::MenuItem("ImPlot demo", NULL, _winImplotDemo->GetOpenFlag());
#endif
#ifndef IMGUI_DISABLE_METRICS_WINDOW
                ImGui::MenuItem("ImGui metrics", NULL, _winImguiMetrics->GetOpenFlag());
#endif
                ImGui::MenuItem("ImPlot metrics", NULL, _winImplotMetrics->GetOpenFlag());

                ImGui::MenuItem("Styles",  NULL, _winImguiStyles->GetOpenFlag());
                //ImGui::MenuItem("About",   NULL, _winImguiAbout->GetOpenFlag());
                if (ImGui::BeginMenu("Colours"))
                {
                    float sz = ImGui::GetTextLineHeight();
                    for (int i = 0; i < ImGuiCol_COUNT; i++)
                    {
                        const char* name = ImGui::GetStyleColorName((ImGuiCol)i);
                        ImVec2 p = ImGui::GetCursorScreenPos();
                        ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x+sz, p.y+sz), ImGui::GetColorU32((ImGuiCol)i));
                        ImGui::Dummy(ImVec2(sz, sz));
                        ImGui::SameLine();
                        ImGui::MenuItem(name);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

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
            _winExperiment->Open();
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

/* ****************************************************************************************************************** */
