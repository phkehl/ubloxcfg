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

#include <memory>
#include <algorithm>

#include "ff_stuff.h"
#include "ff_ubx.h"
#include "ff_trafo.h"
#include "ff_cpp.hpp"
#include "ff_utils.hpp"

#include "gui_inc.hpp"

#include "gui_win_data_config.hpp"
#include "gui_win_data_fwupdate.hpp"
#include "gui_win_data_inf.hpp"
#include "gui_win_data_log.hpp"
#include "gui_win_data_map.hpp"
#include "gui_win_data_messages.hpp"
#include "gui_win_data_plot.hpp"
#include "gui_win_data_scatter.hpp"
#include "gui_win_data_signals.hpp"
#include "gui_win_data_satellites.hpp"
#include "gui_win_data_stats.hpp"
#include "gui_win_data_custom.hpp"
#include "gui_win_data_epoch.hpp"
#include "gui_win_data_3d.hpp"

#include "gui_win_input.hpp"

/* ****************************************************************************************************************** */

GuiWinInput::GuiWinInput(const std::string &name) :
    GuiWin(name),
    _database        { std::make_shared<Database>(name) },
    _logWidget       { 1000 },
    _dataWinCaps     { DataWinDef::Cap_e::ALL },
    _autoHideDatawin { true }
{
    DEBUG("GuiWinInput(%s)", _winName.c_str());

    _winSize = { 100, 35 };

    // Prevent other (data win, other input win) windows from docking into center of the input window, i.e. other
    // windows can only split this a input window but not "overlap" (add a tab)
    //_winClass = std::make_unique<ImGuiWindowClass>();
    //_winClass->DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingInCentralNode; // FIXME; doesn't seem to work
    //_winClass->DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoDockingOverMe; // FIXME: but this seems to do the trick..
    // FIXME: but then again, we would also have to prevent the input window from docking over data windows..
    // so we just disable docking altogether for this window
    _winFlags |= ImGuiWindowFlags_NoDocking;

    // Load saved settings
    GuiSettings::GetValue(_winName + ".autoHideDatawin", _autoHideDatawin, true);
    // s.a. OpenPreviousDataWin(), called from GuiApp
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinInput::~GuiWinInput()
{
    DEBUG("~GuiWinInput(%s)", _winName.c_str());

    // Remember which data windows were open
    std::vector<std::string> openWinNames;
    for (auto &dataWin: _dataWindows)
    {
        openWinNames.push_back(dataWin->WinName());
    }
    GuiSettings::SetValueList(_winName + ".dataWindows", openWinNames, ",", MAX_SAVED_WINDOWS);
    GuiSettings::SetValue(_winName + ".autoHideDatawin", _autoHideDatawin);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::Loop(const uint32_t &frame, const double &now)
{
    for (auto &dataWin: _dataWindows)
    {
        dataWin->Loop(frame, now);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::_ProcessData(const InputData &data)
{
    switch (data.type)
    {
        case InputData::EVENT_START:
        case InputData::EVENT_STOP:
            _rxVerStr = "";
            _UpdateTitle();
            _epoch = nullptr;
            break;
        case InputData::DATA_MSG:
            break;
        case InputData::RXVERSTR:
            _rxVerStr = data.info;
            _UpdateTitle();
            break;
        case InputData::DATA_EPOCH:
            if (data.epoch->epoch.valid)
            {
                _epoch = data.epoch;
                if (_fixStr != _epoch->epoch.fixStr)
                {
                    _fixStr  = _epoch->epoch.fixStr;
                    //_fixTime = _epoch->...timeofepoch... TODO
                }
            }
            break;
        case InputData::INFO_NOTICE:
            _logWidget.AddLine(data.info.c_str(), GUI_COLOUR(INF_NOTICE));
            break;
        case InputData::INFO_WARNING:
            _logWidget.AddLine(data.info.c_str(), GUI_COLOUR(INF_WARNING));
            break;
        case InputData::INFO_ERROR:
            _logWidget.AddLine(data.info.c_str(), GUI_COLOUR(INF_ERROR));
            break;
    }

    for (auto &dataWin: _dataWindows)
    {
        dataWin->ProcessData(data);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::_ClearData()
{
    _database->Clear();
    _logWidget.Clear();
    _fixStr = nullptr;
    _epoch = nullptr;
    for (auto &dataWin: _dataWindows)
    {
        dataWin->ClearData();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::_AddDataWindow(std::unique_ptr<GuiWinData> dataWin)
{
    _dataWindows.push_back( std::move(dataWin) );
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }
    bool summonDataWindows = false;

    // Options, other actions
    if (ImGui::Button(ICON_FK_COG "##Options"))
    {
        ImGui::OpenPopup("Options");
    }
    Gui::ItemTooltip("Options");
    if (ImGui::BeginPopup("Options"))
    {
        ImGui::Checkbox("Autohide data windows", &_autoHideDatawin);
        Gui::ItemTooltip("Automatically hide all data windows if this window is collapsed\n"
                         "respectively invisible while docked into another window.");
        if (ImGui::Button("Summon data windows"))
        {
            summonDataWindows = true;
        }
        ImGui::EndPopup();
    }
    Gui::VerticalSeparator();

    _DrawDataWinButtons();

    Gui::VerticalSeparator();

    _DrawActionButtons();

    ImGui::Separator();

    const EPOCH_t *epoch = _epoch && _epoch->epoch.valid ? &_epoch->epoch : nullptr;
    const float statusHeight = ImGui::GetTextLineHeightWithSpacing() * 10;
    const float maxHeight = ImGui::GetContentRegionAvail().y;

    if (ImGui::BeginChild("##StatusLeft", ImVec2(GuiSettings::charSize.x * 40, statusHeight), false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ))
    {
        _DrawNavStatusLeft(epoch);
    }
    ImGui::EndChild();

    Gui::VerticalSeparator();

    if (ImGui::BeginChild("##StatusRight", ImVec2(0, MIN(statusHeight, maxHeight)), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ))
    {
        _DrawNavStatusRight(epoch);
    }
    ImGui::EndChild();

    ImGui::Separator();

    _DrawControls(); // Remaining stuff implemented in derived classes

    _DrawLog();

    if (summonDataWindows)
    {
        _SummonDataWindows();
    }

    _DrawWindowEnd();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::DrawDataWindows(const bool closeFocusedWin)
{
    if (_autoHideDatawin && !_winDrawn)
    {
        return;
    }

    // Draw data windows, destroy and remove closed ones
    for (auto iter = _dataWindows.begin(); iter != _dataWindows.end(); )
    {
        auto &dataWin = *iter;

        if (dataWin->WinIsOpen())
        {
            dataWin->DrawWindow();
            if (closeFocusedWin && dataWin->WinIsFocused())
            {
                dataWin->WinClose();
            }
            iter++;
        }
        else
        {
            iter = _dataWindows.erase(iter);
        }
    }
}

void GuiWinInput::DataWinMenu()
{
    if (ImGui::MenuItem("Main"))
    {
        WinFocus();
    }
    if (!_dataWindows.empty())
    {
        ImGui::Separator();
        const int offs = WinTitle().size() + 3;
        for (auto &win: _dataWindows)
        {
            if (ImGui::MenuItem(win->WinTitle().substr(offs).c_str()))
            {
                win->WinOpen();
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

#define _MK_CREATE(_cls_) \
    [](const std::string &name, std::shared_ptr<Database> database) -> std::unique_ptr<GuiWinData> { return std::make_unique<_cls_>(name, database); }

/*static*/ const std::vector<GuiWinInput::DataWinDef> GuiWinInput::_dataWinDefs =
{
    { "Log",        "Log",             ICON_FK_LIST_UL        "##Log",        DataWinDef::Cap_e::ALL,    _MK_CREATE(GuiWinDataLog)        },
    { "Messages",   "Messages",        ICON_FK_SORT_ALPHA_ASC "##Messages",   DataWinDef::Cap_e::ALL,    _MK_CREATE(GuiWinDataMessages)   },
    { "Inf",        "Inf messages",    ICON_FK_FILE_TEXT_O    "##Inf",        DataWinDef::Cap_e::ALL,    _MK_CREATE(GuiWinDataInf)        },
    { "Scatter",    "Scatter plot",    ICON_FK_CROSSHAIRS     "##Scatter",    DataWinDef::Cap_e::ALL,    _MK_CREATE(GuiWinDataScatter)    },
    { "Signals",    "Signals",         ICON_FK_SIGNAL         "##Signals",    DataWinDef::Cap_e::ALL,    _MK_CREATE(GuiWinDataSignals)    },
    { "Config",     "Configuration",   ICON_FK_PAW            "##Config",     DataWinDef::Cap_e::ACTIVE, _MK_CREATE(GuiWinDataConfig)     },
    { "Plots",      "Plots",           ICON_FK_LINE_CHART     "##Plots",      DataWinDef::Cap_e::ALL,    _MK_CREATE(GuiWinDataPlot)       },
    { "Map",        "Map",             ICON_FK_MAP            "##Map",        DataWinDef::Cap_e::ALL,    _MK_CREATE(GuiWinDataMap)        },
    { "Satellites", "Satellites",      ICON_FK_ROCKET         "##Satellites", DataWinDef::Cap_e::ALL,    _MK_CREATE(GuiWinDataSatellites) },
    { "Stats",      "Statistics",      ICON_FK_TABLE          "##Stats",      DataWinDef::Cap_e::ALL,    _MK_CREATE(GuiWinDataStats)      },
    { "Epoch",      "Epoch details",   ICON_FK_TH             "##Epoch",      DataWinDef::Cap_e::ALL,    _MK_CREATE(GuiWinDataEpoch)      },
    { "Fwupdate",   "Firmware update", ICON_FK_DOWNLOAD       "##Fwupdate",   DataWinDef::Cap_e::ACTIVE, _MK_CREATE(GuiWinDataFwupdate)   },
    { "Custom",     "Custom message",  ICON_FK_TERMINAL       "##Custom",     DataWinDef::Cap_e::ALL,    _MK_CREATE(GuiWinDataCustom)     },
    { "Threed",     "3d view",         ICON_FK_SPINNER        "##3dView",     DataWinDef::Cap_e::ALL,    _MK_CREATE(GuiWinData3d)     },
};

void GuiWinInput::_DrawDataWinButtons()
{
    for (auto &def: _dataWinDefs)
    {
        ImGui::BeginDisabled(!CHKBITS_ANY(def.reqs, _dataWinCaps));
        if (ImGui::Button(def.button, GuiSettings::iconSize))
        {
            const std::string baseName = WinName() + def.name; // Receiver1Map, Logfile4Stats, ...
            int winNumber = 1;
            while (winNumber < 1000)
            {
                const std::string winName = baseName + std::to_string(winNumber);
                bool nameUnused = true;
                for (auto &dataWin: _dataWindows)
                {
                    if (winName == dataWin->WinName())
                    {
                        nameUnused = false;
                        break;
                    }
                }
                if (nameUnused)
                {
                    try
                    {
                        std::unique_ptr<GuiWinData> dataWin = def.create(winName, _database);
                        dataWin->WinOpen();
                        dataWin->WinSetTitle(WinTitle() + std::string(" - ") + def.title + std::string(" ") + std::to_string(winNumber));
                        _AddDataWindow(std::move(dataWin));
                    }
                    catch (std::exception &e)
                    {
                        ERROR("new %s%d: %s", baseName.c_str(), winNumber, e.what());
                    }
                    break;
                }
                winNumber++;
            }
        }
        ImGui::EndDisabled();
        Gui::ItemTooltip(def.title);

        ImGui::SameLine(); // FIXME: for all but the last one..
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::OpenPreviousDataWin()
{
    const std::string winName = WinName(); // "Receiver1", "Logfile3", ...
    const std::vector<std::string> dataWinNames = GuiSettings::GetValueList(winName + ".dataWindows", ",", MAX_SAVED_WINDOWS);
    for (const auto &dataWinName: dataWinNames) // "Receiver1Scatter1", "Logfile3Map1", ...
    {
        try
        {
            std::string name = dataWinName.substr(winName.size()); // ""Receiver1Map1" -> "Map1"
            for (auto &def: _dataWinDefs)
            {
                const int nameLen = strlen(def.name);
                if (std::strncmp(name.c_str(), def.name, nameLen) != 0) // "Map"(1) == "Map", "Scatter", ... ?
                {
                    continue;
                }
                std::string dataWinName2 = winName + name; // "Receiver1Map1"
                auto win = def.create(dataWinName2, _database);
                win->WinOpen();
                win->WinSetTitle(WinTitle() + std::string(" - ") + def.title + std::string(" ") + name.substr(nameLen));
                _AddDataWindow(std::move(win));
                break;
            }
        }
        catch (std::exception &e)
        {
            WARNING("new %s: %s", dataWinName.c_str(), e.what());
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::_DrawActionButtons()
{
    // Clear
    if (ImGui::Button(ICON_FK_ERASER "##Clear", GuiSettings::iconSize))
    {
        _ClearData();
    }
    Gui::ItemTooltip("Clear all data");

    ImGui::SameLine();

    // Database status
    const char * const dbIcons[] =
    {
        ICON_FK_BATTERY_EMPTY           /* "##DbStatus" */, //  0% ..  20%
        ICON_FK_BATTERY_QUARTER         /* "##DbStatus" */, // 20% ..  40%
        ICON_FK_BATTERY_HALF            /* "##DbStatus" */, // 40% ..  60%
        ICON_FK_BATTERY_THREE_QUARTERS  /* "##DbStatus" */, // 60% ..  80%
        ICON_FK_BATTERY_FULL            /* "##DbStatus" */, // 80% .. 100%
    };
    const int dbSize  = _database->MaxSize();
    const int dbUsage = _database->Size();
    const float dbFull = (float)dbUsage / (float)dbSize;
    const int dbIconIx = CLIP(dbFull, 0.0f, 1.0f) * (float)(NUMOF(dbIcons) - 1);
    const ImVec2 cursor = ImGui::GetCursorPos();
    ImGui::InvisibleButton("DbStatus", GuiSettings::iconSize, ImGuiButtonFlags_None);
    //ImGui::Button(dbIcons[dbIconIx], GuiSettings::iconSize);
    if (Gui::ItemTooltipBegin())
    {
        ImGui::Text("Database %d/%d epochs, %.1f%% full", dbUsage, dbSize, dbFull * 1e2f);
        Gui::ItemTooltipEnd();
    }
    ImGui::SetCursorPos(cursor);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(dbIcons[dbIconIx]);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::_DrawNavStatusLeft(const EPOCH_t *epoch)
{
    const float dataOffs = GuiSettings::charSize.x * 13;

    // Sequence / status
    ImGui::Selectable("Seq, uptime");
    ImGui::SameLine(dataOffs);
    if (!epoch)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(FIX_INVALID));
        ImGui::TextUnformatted("no data");
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::Text("%d", _epoch->seq);
        ImGui::SameLine();
        if (_epoch->epoch.haveUptime)
        {
            ImGui::TextUnformatted( _epoch->epoch.uptimeStr);
        }
        else
        {
            Gui::TextDim("n/a");
        }
    }

    ImGui::Selectable("Fix type");
    if (epoch && epoch->haveFix)
    {
        ImGui::SameLine(dataOffs);
        ImGui::PushStyleColor(ImGuiCol_Text, GuiSettings::FixColour(epoch->fix, epoch->fixOk));
        ImGui::TextUnformatted(epoch->fixStr);
        ImGui::PopStyleColor();
        // ImGui::SameLine();
        // Gui::TextDim("%.1f", epoch-> - _fixTime); // TODO
    }

    ImGui::Selectable("Latitude");
    if (epoch && epoch->havePos)
    {
        int d, m;
        double s;
        deg2dms(rad2deg(epoch->llh[0]), &d, &m, &s);
        ImGui::SameLine(dataOffs);
        if (!epoch->fixOk) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
        ImGui::Text(" %2d° %2d' %9.6f\" %c", ABS(d), m, s, d < 0 ? 'S' : 'N');
        if (!epoch->fixOk) { ImGui::PopStyleColor(); }
    }
    ImGui::Selectable("Longitude");
    if (epoch && epoch->havePos)
    {
        int d, m;
        double s;
        deg2dms(rad2deg(epoch->llh[1]), &d, &m, &s);
        ImGui::SameLine(dataOffs);
        if (!epoch->fixOk) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
        ImGui::Text("%3d° %2d' %9.6f\" %c", ABS(d), m, s, d < 0 ? 'W' : 'E');
        if (!epoch->fixOk) { ImGui::PopStyleColor(); }
    }
    ImGui::Selectable("Height");
    if (epoch && epoch->havePos)
    {
        ImGui::SameLine(dataOffs);
        if (!epoch->fixOk) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
        ImGui::Text("%.2f m", epoch->llh[2]);
        if (epoch->haveMsl)
        {
            ImGui::SameLine();
            ImGui::Text("(%.1f MSL)", epoch->heightMsl);
        }

        if (!epoch->fixOk) { ImGui::PopStyleColor(); }
    }
    ImGui::Selectable("Accuracy");
    if (epoch && epoch->havePos)
    {
        ImGui::SameLine(dataOffs);
        if (!epoch->fixOk) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
        if (epoch->horizAcc > 1000.0)
        {
            ImGui::Text("H %.1f, V %.1f [km]", epoch->horizAcc * 1e-3, epoch->vertAcc * 1e-3);
        }
        else if (epoch->horizAcc > 10.0)
        {
            ImGui::Text("H %.1f, V %.1f [m]", epoch->horizAcc, epoch->vertAcc);
        }
        else
        {
            ImGui::Text("H %.3f, V %.3f [m]", epoch->horizAcc, epoch->vertAcc);
        }
        if (!epoch->fixOk) { ImGui::PopStyleColor(); }
    }
    ImGui::Selectable("Velocity");
    if (epoch && epoch->haveVel)
    {
        ImGui::SameLine(dataOffs);
        if (!epoch->fixOk) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
        ImGui::Text("%.2f [m/s] (%.1f [km/h])", epoch->vel3d, epoch->vel3d * (3600.0/1000.0));
        if (!epoch->fixOk) { ImGui::PopStyleColor(); }
    }
    ImGui::Selectable("Rel. pos.");
    if (epoch && epoch->haveRelPos)
    {
        ImGui::SameLine(dataOffs);
        if (!epoch->fixOk) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
        if (epoch->relLen > 1000.0)
        {
            ImGui::Text("N %.1f, E %.1f, D %.1f [km]",
                epoch->relNed[0] * 1e-3, epoch->relNed[1] * 1e-3, epoch->relNed[2] * 1e-3);
        }
        else if (epoch->relLen > 100.0)
        {
            ImGui::Text("N %.0f, E %.0f, D %.0f [m]", epoch->relNed[0], epoch->relNed[1], epoch->relNed[2]);
        }
        else
        {
            ImGui::Text("N %.1f, E %.1f, D %.1f [m]", epoch->relNed[0], epoch->relNed[1], epoch->relNed[2]);
        }
        if (!epoch->fixOk) { ImGui::PopStyleColor(); }
    }

    ImGui::Selectable("GPS time");
    if (epoch)
    {
        ImGui::SameLine(dataOffs);
        if (!epoch->haveGpsWeek) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
        ImGui::Text("%04d", epoch->gpsWeek);
        if (!epoch->haveGpsWeek) { ImGui::PopStyleColor(); }
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::TextUnformatted(":");
        ImGui::SameLine(0.0f, 0.0f);
        if (!epoch->haveGpsTow) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
        ImGui::Text(epoch->gpsTowAcc < 0.001 ? "%013.6f" : "%010.3f", epoch->gpsTow);
        if (!epoch->haveGpsTow) { ImGui::PopStyleColor(); }
    }

    ImGui::Selectable("Date/time");
    if (epoch)
    {
        ImGui::SameLine(dataOffs);
        if (!epoch->haveDate) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
        ImGui::Text("%04d-%02d-%02d", epoch->year, epoch->month, epoch->day);
        if (!epoch->haveDate) { ImGui::PopStyleColor(); }
        ImGui::SameLine();
        if (!epoch->haveTime) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
        ImGui::Text("%02d:%02d", epoch->hour, epoch->minute);
        if (!epoch->haveTime) { ImGui::PopStyleColor(); }
        ImGui::SameLine(0.0f, 0.0f);
        if (!epoch->leapSecKnown) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
        ImGui::Text(":%06.3f", epoch->second < 0.001 ? 0.0 : epoch->second);
        if (!epoch->leapSecKnown) { ImGui::PopStyleColor(); }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::_DrawNavStatusRight(const EPOCH_t *epoch)
{
    const float dataOffs = GuiSettings::charSize.x * 12;

    ImGui::Selectable("Sat. used");
    if (epoch)
    {
        ImGui::SameLine(dataOffs);
        ImGui::Text("%2d (%2dG %2dR %2dB %2dE %2dS %2dQ)",
            epoch->numSatUsed, epoch->numSatUsedGps, epoch->numSatUsedGlo, epoch->numSatUsedBds, epoch->numSatUsedGal,
            epoch->numSatUsedSbas, epoch->numSatUsedQzss);
    }
    ImGui::Selectable("Sig. used");
    if (epoch)
    {
        ImGui::SameLine(dataOffs);
        ImGui::Text("%2d (%2dG %2dR %2dB %2dE %2dS %2dQ)",
            epoch->numSigUsed, epoch->numSigUsedGps, epoch->numSigUsedGlo, epoch->numSigUsedBds, epoch->numSigUsedGal,
            epoch->numSigUsedSbas, epoch->numSigUsedQzss);
    }

    ImGui::Separator();

    const FfVec2f canvasOffs = ImGui::GetCursorScreenPos();
    const FfVec2f canvasSize = ImGui::GetContentRegionAvail();
    const FfVec2f canvasMax  = canvasOffs + canvasSize;
    //DEBUG("%f %f %f %f", canvasOffs.x, canvasOffs.y, canvasSize.x, canvasSize.y);
    const auto charSize = GuiSettings::charSize;
    if (canvasSize.y < (charSize.y * 5))
    {
        return;
    }

    // if (epoch)
    // {
    //     ImGui::Text("%d %d %d %d %d %d %d %d %d %d %d %d", epoch->sigCnoHistTrk[0], epoch->sigCnoHistTrk[1], epoch->sigCnoHistTrk[2], epoch->sigCnoHistTrk[3], epoch->sigCnoHistTrk[4], epoch->sigCnoHistTrk[5], epoch->sigCnoHistTrk[6], epoch->sigCnoHistTrk[7], epoch->sigCnoHistTrk[8], epoch->sigCnoHistTrk[9], epoch->sigCnoHistTrk[10], epoch->sigCnoHistTrk[11]);
    //     ImGui::Text("%d %d %d %d %d %d %d %d %d %d %d %d", epoch->sigCnoHistNav[0], epoch->sigCnoHistNav[1], epoch->sigCnoHistNav[2], epoch->sigCnoHistNav[3], epoch->sigCnoHistNav[4], epoch->sigCnoHistNav[5], epoch->sigCnoHistNav[6], epoch->sigCnoHistNav[7], epoch->sigCnoHistNav[8], epoch->sigCnoHistNav[9], epoch->sigCnoHistNav[10], epoch->sigCnoHistNav[11]);
    // }

    ImDrawList *draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(canvasOffs, canvasOffs + canvasSize);

    //
    //                +++
    //            +++ +++
    //        +++ +++ +++ +++
    //    +++ +++ +++ +++ +++     +++
    //   ---------------------------------
    //    === === === === === === === ...
    //   ---------------------------------
    //           10      20     30

    // padding between bars and width of bars
    const float padx = 2.0f;
    const float width = (canvasSize.x - ((float)(EPOCH_SIGCNOHIST_NUM - 1) * padx)) / (float)EPOCH_SIGCNOHIST_NUM;

    // bottom space for x axis labelling
    const float pady = 1.0f + charSize.y + 1.0f + 1.0f + 1.0 + charSize.y + 1.0;

    // scale for signal count (height of bars)
    //const float scale = (epoch->numSigUsed > 15 ? (2.0f / epoch->numSigUsed) : 0.1f) * canvasSize.y;
    const float scale = 1.0f / 25.0f * (canvasSize.y - pady);

    float x = canvasOffs.x;
    float y = canvasOffs.y + canvasSize.y - pady;

    // draw bars for signals
    if (epoch)
    {
        for (int ix = 0; ix < EPOCH_SIGCNOHIST_NUM; ix++)
        {
            // tracked signals
            if (epoch->sigCnoHistTrk[ix] > 0)
            {
                const float h = (float)epoch->sigCnoHistTrk[ix] * scale;
                draw->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y - h), GUI_COLOUR(SIGNAL_UNUSED));
            }
            // signals used
            if (epoch->sigCnoHistNav[ix] > 0)
            {
                const float h = (float)epoch->sigCnoHistNav[ix] * scale;
                draw->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y - h), GUI_COLOUR(SIGNAL_00_05 + ix));
            }

            x += width + padx;
        }
    }

    x = canvasOffs.x;
    y = canvasOffs.y;

    // y grid
    {
        const float dy = (canvasSize.y - pady) / 5;
        draw->AddLine(ImVec2(x, y), ImVec2(canvasMax.x, y), GUI_COLOUR(PLOT_GRID_MINOR));
        y += dy;

        draw->AddLine(ImVec2(x, y), ImVec2(canvasMax.x, y), GUI_COLOUR(PLOT_GRID_MAJOR));
        ImGui::SetCursorScreenPos(ImVec2(x, y + 1.0f));
        y += dy;
        draw->AddLine(ImVec2(x, y), ImVec2(canvasMax.x, y), GUI_COLOUR(PLOT_GRID_MINOR));
        y += dy;
        ImGui::Text("20");

        draw->AddLine(ImVec2(x, y), ImVec2(canvasMax.x, y), GUI_COLOUR(PLOT_GRID_MAJOR));
        ImGui::SetCursorScreenPos(ImVec2(x, y + 1.0f));
        y += dy;
        draw->AddLine(ImVec2(x, y), ImVec2(canvasMax.x, y), GUI_COLOUR(PLOT_GRID_MINOR));
        y += dy;
        ImGui::Text("10");
    }

    // x-axis horizontal line
    x = canvasOffs.x;
    y = canvasMax.y - pady + 1.0f;
    draw->AddLine(ImVec2(x, y), ImVec2(canvasMax.x, y), GUI_COLOUR(PLOT_GRID_MAJOR));

    // x-axis colours and counts
    y += 2.0f;
    for (int ix = 0; ix < EPOCH_SIGCNOHIST_NUM; ix++)
    {
        draw->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + 1.0f + charSize.y), GUI_COLOUR(SIGNAL_00_05 + ix));
        if (epoch && (width > (2.0f * charSize.x)))
        {
            const int n = epoch->sigCnoHistNav[ix];
            ImGui::SetCursorScreenPos(ImVec2(x + (0.5f * width) - (n < 10 ? 0.5f * charSize.x : charSize.x), y));

            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BLACK));
            ImGui::Text("%d", n);
            ImGui::PopStyleColor();
        }

        x += width + padx;
    }
    y += 1.0 + charSize.y;


    x = canvasOffs.x;
    y += 1.0;
    draw->AddLine(ImVec2(x, y), ImVec2(canvasMax.x, y), GUI_COLOUR(PLOT_GRID_MAJOR));

    // x-axis labels
    x = canvasOffs.x + width + padx + width + padx - charSize.x;
    y += 1.0f;

    for (int ix = 2; ix < EPOCH_SIGCNOHIST_NUM; ix += 2)
    {
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::Text("%d", ix * 5);
        x += width + padx + width + padx;
    }

    draw->PopClipRect();

    ImGui::SetCursorScreenPos(canvasOffs);
    ImGui::InvisibleButton("##SigLevPlotTooltop", canvasSize);
    Gui::ItemTooltip("Signal levels (x axis) vs. number of signals used and tracked (y axis)");
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::_DrawLog()
{
    _logWidget.DrawLog(); // only log, no controls
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::_UpdateTitle()
{
    std::string mainTitle = WinTitle(); // "Receiver X" or "Receiver X: version"

    {
        const auto pos = mainTitle.find(':');
        if (pos != std::string::npos)
        {
            mainTitle.erase(pos, std::string::npos);
        }

        if (!_rxVerStr.empty())
        {
            mainTitle += std::string(": ") + _rxVerStr;
        }

        WinSetTitle(mainTitle);
    }

    // "Receiver X - child" or "Receiver X: version - child"
    for (auto &dataWin: _dataWindows)
    {
        std::string childTitle = dataWin->WinTitle();
        const auto pos = childTitle.find(" - ");
        if (pos != std::string::npos)
        {
            childTitle.replace(0, pos, mainTitle);
            dataWin->WinSetTitle(childTitle);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::_SummonDataWindows()
{
    const FfVec2f sep = GuiSettings::charSize * 2;
    FfVec2f pos = ImGui::GetWindowPos();
    pos.y += ImGui::GetWindowSize().y + GuiSettings::style->ItemSpacing.y;
    for (auto &win: _dataWindows)
    {
        //if (!win->WinIsDocked())
        {
            win->WinMoveTo(pos);
            win->WinResize();
            win->WinFocus();
            pos += sep;
        }
    }
    WinFocus();
}

/* ****************************************************************************************************************** */
