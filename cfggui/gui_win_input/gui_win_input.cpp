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

#include "imgui.h"
#include "implot.h"
#include "IconsForkAwesome.h"

#include "ff_stuff.h"
#include "ff_ubx.h"
#include "ff_trafo.h"
#include "ff_cpp.hpp"

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

#include "gui_win_input.hpp"

/* ****************************************************************************************************************** */

GuiWinInput::GuiWinInput(const std::string &name) :
    GuiWin(name),
    _database  { std::make_shared<Database>(10000) },
    // _callback  { nullptr },
    _logWidget { 1000, false },
    _rxVerStr  { "" }
{
    DEBUG("GuiWinInput(%s)", _winName.c_str());

    // OpenPreviousDataWin(); // cannot do this here, as it relies on some virtual methods only (fully) implemented in the derived classes
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinInput::~GuiWinInput()
{
    DEBUG("~GuiWinInput(%s)", _winName.c_str());

    // Remember which data windows were open
    std::vector<std::string> openWinNames;
    for (auto &dataWin: _dataWindows)
    {
        openWinNames.push_back(dataWin->GetName());
    }
    const std::string dataWinNames = Ff::StrJoin(openWinNames, ",");
    _winSettings->SetValue(_winName, dataWinNames);
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

void GuiWinInput::_ProcessData(const Data &data)
{
    switch (data.type)
    {
        case Data::Type::DATA_MSG:
            if (data.msg->name == "UBX-MON-VER")
            {
                char str[100];
                if (ubxMonVerToVerStr(str, sizeof(str), data.msg->data, data.msg->size))
                {
                    _rxVerStr = str;
                    _UpdateTitle();
                }
            }
            break;
        case Data::Type::INFO_NOTICE:
            _logWidget.AddLine(data.info->c_str(), GUI_COLOUR(INF_NOTICE));
            break;
        case Data::Type::INFO_WARN:
            _logWidget.AddLine(data.info->c_str(), GUI_COLOUR(INF_WARNING));
            break;
        case Data::Type::INFO_ERROR:
            _logWidget.AddLine(data.info->c_str(), GUI_COLOUR(INF_ERROR));
            break;
        case Data::Type::EVENT_STOP:
            _rxVerStr = "";
            _UpdateTitle();
            _epoch = nullptr;
            break;
        case Data::Type::DATA_EPOCH:
            if (data.epoch->epoch.valid)
            {
                _epoch = data.epoch;
                if (_fixStr != _epoch->epoch.fixStr)
                {
                    _fixStr  = _epoch->epoch.fixStr;
                    //_fixTime = _epoch->...timeofepoch... TODO
                }
            }
        default:
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

    _DrawDataWinButtons();

    Gui::VerticalSeparator();

    _DrawActionButtons();

    ImGui::Separator();

    const EPOCH_t *epoch = _epoch && _epoch->epoch.valid ? &_epoch->epoch : nullptr;
    const float statusHeight = ImGui::GetTextLineHeightWithSpacing() * 9;
    const float maxHeight = ImGui::GetContentRegionAvail().y;

    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
    ImGui::TextUnformatted("Navigation status");
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (ImGui::BeginChild("##StatusLeft", ImVec2(_winSettings->charSize.x * 40, statusHeight), false,
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

    _DrawWindowEnd();

    // Draw data windows, destroy and remove closed ones
    for (auto iter = _dataWindows.begin(); iter != _dataWindows.end(); )
    {
        auto &dataWin = *iter;

        if (dataWin->IsOpen())
        {
            dataWin->DrawWindow();
            iter++;
        }
        else
        {
            iter = _dataWindows.erase(iter);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

static const struct
{
    const char   *name;
    const char   *title;
    const char   *button;
    std::function< std::unique_ptr<GuiWinData> ( const std::string &name, std::shared_ptr<Database> database ) > create;
} kDataWindows[] =
{
    {
        .name   = "Log",       .title  = "Log" ,              .button = ICON_FK_LIST_UL "##Log",
        .create = [](const std::string &name, std::shared_ptr<Database> database)
            -> std::unique_ptr<GuiWinData> { return std::make_unique<GuiWinDataLog>(name, database); }
    },
    {
        .name   = "Messages",  .title  = "Messages" ,         .button = ICON_FK_SORT_ALPHA_ASC "##Msgs",
        .create = [](const std::string &name, std::shared_ptr<Database> database)
            -> std::unique_ptr<GuiWinData> { return std::make_unique<GuiWinDataMessages>(name, database); }
    },
    {
        .name   = "Inf",       .title  = "Inf messages" ,     .button = ICON_FK_FILE_TEXT_O "##Inf",
        .create = [](const std::string &name, std::shared_ptr<Database> database)
            -> std::unique_ptr<GuiWinData> { return std::make_unique<GuiWinDataInf>(name, database); }
    },
    {
        .name   = "Scatter",   .title  = "Scatter plot" ,     .button = ICON_FK_CROSSHAIRS "##Scatter",
        .create = [](const std::string &name, std::shared_ptr<Database> database)
            -> std::unique_ptr<GuiWinData> { return std::make_unique<GuiWinDataScatter>(name, database); }
    },
    {
        .name   = "Signals",   .title  = "Signals" ,          .button = ICON_FK_BAR_CHART "##Signals",
        .create = [](const std::string &name, std::shared_ptr<Database> database)
            -> std::unique_ptr<GuiWinData> { return std::make_unique<GuiWinDataSignals>(name, database); }
    },
    {
        .name   = "Config",    .title  = "Configuration" ,    .button = ICON_FK_PAW "##Config",
        .create = [](const std::string &name, std::shared_ptr<Database> database)
            -> std::unique_ptr<GuiWinData> { return std::make_unique<GuiWinDataConfig>(name, database); }
    },
    {
        .name   = "Legend",    .title  = "Legend" ,           .button = ICON_FK_QUESTION "##Legend",
        .create = [](const std::string &name, std::shared_ptr<Database> database)
            -> std::unique_ptr<GuiWinData> { return std::make_unique<GuiWinDataLegend>(name, database); }
    },
    {
        .name   = "Plots",     .title  = "Plots" ,            .button = ICON_FK_LINE_CHART "##Plots",
        .create = [](const std::string &name, std::shared_ptr<Database> database)
            -> std::unique_ptr<GuiWinData> { return std::make_unique<GuiWinDataPlot>(name, database); }
    },
    {
        .name   = "Map",       .title  = "Map" ,              .button = ICON_FK_MAP "##Map",
        .create = [](const std::string &name, std::shared_ptr<Database> database)
            -> std::unique_ptr<GuiWinData> { return std::make_unique<GuiWinDataMap>(name, database); }
    },
    {
        .name   = "Satellites",.title  = "Satellites" ,       .button = ICON_FK_ROCKET "##Satellites",
        .create = [](const std::string &name, std::shared_ptr<Database> database)
            -> std::unique_ptr<GuiWinData> { return std::make_unique<GuiWinDataSatellites>(name, database); }
    },
    {
        .name   = "Stats",     .title  = "Statistics" ,       .button = ICON_FK_TABLE "##Stats",
        .create = [](const std::string &name, std::shared_ptr<Database> database)
            -> std::unique_ptr<GuiWinData> { return std::make_unique<GuiWinDataStats>(name, database); }
    },
    {
        .name   = "Fwupdate",  .title  = "Firmware update" ,  .button = ICON_FK_DOWNLOAD "##Fwupdate",
        .create = [](const std::string &name, std::shared_ptr<Database> database)
            -> std::unique_ptr<GuiWinData> { return std::make_unique<GuiWinDataFwupdate>(name, database); }
    },
};

void GuiWinInput::_DrawDataWinButtons()
{
    for (int ix = 0; ix < NUMOF(kDataWindows); ix++)
    {
        if (ImGui::Button(kDataWindows[ix].button, _winSettings->iconButtonSize))
        {
            const std::string baseName = GetName() + kDataWindows[ix].name; // Receiver1Map, Logfile4Stats, ...
            int winNumber = 1;
            while (winNumber < 1000)
            {
                const std::string winName = baseName + std::to_string(winNumber);
                bool nameUnused = true;
                for (auto &dataWin: _dataWindows)
                {
                    if (winName == dataWin->GetName())
                    {
                        nameUnused = false;
                        break;
                    }
                }
                if (nameUnused)
                {
                    try
                    {
                        std::unique_ptr<GuiWinData> dataWin = kDataWindows[ix].create(winName, _database);
                        dataWin->Open();
                        dataWin->SetTitle(GetTitle() + std::string(" - ") + kDataWindows[ix].title + std::string(" ") + std::to_string(winNumber));
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
        Gui::ItemTooltip(kDataWindows[ix].title);

        if (ix < (NUMOF(kDataWindows) - 1))
        {
            ImGui::SameLine();
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::OpenPreviousDataWin()
{
    const std::string winName = GetName(); // "Receiver1", "Logfile3", ...
    std::string openDataWindows = "";
    _winSettings->GetValue(winName, openDataWindows, "");
    if (openDataWindows.empty())
    {
        return;
    }

     std::vector<std::string> dataWinNames = Ff::StrSplit(openDataWindows, ",");
    if (dataWinNames.empty())
    {
        return;
    }

    for (const auto &dataWinName: dataWinNames) // "Receiver1Scatter1", "Logfile3Map1", ...
    {
        try
        {
            std::string name = dataWinName.substr(winName.size()); // ""Receiver1Map1" -> "Map1"
            for (int ix = 0; ix < NUMOF(kDataWindows); ix++)
            {
                auto *dw = &kDataWindows[ix];
                const int nameLen = strlen(dw->name);
                if (std::strncmp(name.c_str(), dw->name, nameLen) != 0) // "Map"(1) == "Map", "Scatter", ... ?
                {
                    continue;
                }
                std::string dataWinName2 = winName + name; // "Receiver1Map1"
                auto win = dw->create(dataWinName2, _database);
                win->Open();
                win->SetTitle(winName + std::string(" - ") + dw->title + std::string(" ") + name.substr(nameLen));
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
    if (ImGui::Button(ICON_FK_ERASER "##Clear", _winSettings->iconButtonSize))
    {
        _ClearData();
    }
    Gui::ItemTooltip("Clear all data");
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::_DrawNavStatusLeft(const EPOCH_t *epoch)
{
    const float dataOffs = _winSettings->charSize.x * 13;

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
            ImGui::Text("%.1f", _epoch->epoch.uptime);
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
            ImGui::TextUnformatted("n/a");
            ImGui::PopStyleColor();
        }
    }

    ImGui::Selectable("Fix type");
    if (epoch && epoch->haveFix)
    {
        ImGui::SameLine(dataOffs);
        ImGui::PushStyleColor(ImGuiCol_Text, _winSettings->GetFixColour(epoch));
        ImGui::TextUnformatted(epoch->fixStr);
        ImGui::PopStyleColor();
        // ImGui::SameLine();
        // ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
        // ImGui::Text("%.1f", epoch-> - _fixTime); // TODO
        // ImGui::PopStyleColor();
    }

    ImGui::Selectable("Latitude");
    if (epoch && epoch->havePos)
    {
        int d, m;
        double s;
        deg2dms(rad2deg(epoch->llh[Database::_LAT_]), &d, &m, &s);
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
        deg2dms(rad2deg(epoch->llh[Database::_LON_]), &d, &m, &s);
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
        ImGui::Text("%.2f m", epoch->llh[Database::_HEIGHT_]);
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
    const float dataOffs = _winSettings->charSize.x * 12;

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

    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
    ImGui::Text("Signal levels vs. signals tracked/used:");
    ImGui::PopStyleColor();

    const ImVec2 canvasOffs = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    const ImVec2 canvasMax  = canvasOffs + canvasSize;
    //DEBUG("%f %f %f %f", canvasOffs.x, canvasOffs.y, canvasSize.x, canvasSize.y);
    const auto charSize = _winSettings->charSize;
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
    //           10      20     30

    // padding between bars and width of bars
    const float padx = 2.0f;
    const float width = (canvasSize.x - ((float)(EPOCH_SIGCNOHIST_NUM - 1) * padx)) / (float)EPOCH_SIGCNOHIST_NUM;

    // bottom space for x axis labelling
    const float pady = 1.0f + 1.0f + 4.0f + charSize.y;

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

    // x-axis colours
    y += 2.0f;
    for (int ix = 0; ix < EPOCH_SIGCNOHIST_NUM; ix++)
    {
        draw->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + 4.0f), GUI_COLOUR(SIGNAL_00_05 + ix));
        x += width + padx;
    }
    y += 4.0;

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
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::_DrawLog()
{
    _logWidget.DrawWidget();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInput::_UpdateTitle()
{
    std::string mainTitle = GetTitle(); // "Receiver X" or "Receiver X: version"

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

        SetTitle(mainTitle);
    }

    // "Receiver X - child" or "Receiver X: version - child"
    for (auto &dataWin: _dataWindows)
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

/* ****************************************************************************************************************** */
