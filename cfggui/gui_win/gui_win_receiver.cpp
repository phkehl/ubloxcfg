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

#include <cstdint>
#include <cstring>
#include <functional>
#include <filesystem>

#include "ff_stuff.h"
#include "ff_ubx.h"
#include "ff_port.h"
#include "ff_trafo.h"
#include "ff_cpp.hpp"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "IconsForkAwesome.h"

#include "platform.hpp"
#include "database.hpp"

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

#include "gui_stuff.hpp"

#include "gui_win_receiver.hpp"

/* ****************************************************************************************************************** */

GuiWinReceiver::GuiWinReceiver(const std::string &name, std::shared_ptr<Receiver> receiver, std::shared_ptr<Database> database) :
    _rxVerStr{}, _receiver{receiver}, _database{database}, _dataWinButtonsCb{nullptr}, _titleChangeCb{nullptr},
    _port{}, _baudrate{0}, _log{1000, false},
    _stopSent{false}, _triggerConnect{false}, _focusPortInput{false},
    _recordHandle{nullptr}, _recordSize{0}, _recordFileName{""}, _recordDialogTo{-1.0}, _recordError{""}, _recordMessage{""}
{
    _winName       = name;
    _winIniPos     = POS_NW;
    _winOpen       = true;
    _winSize       = { 80, 25 };
    SetTitle("Receiver X");

    DEBUG("GuiWinReceiver(%s)", _winName.c_str());

    _InitRecentPorts();

    if (!_recentPorts.empty())
    {
        _port = _recentPorts[0];
    }
    _dataWinButtonsCb = nullptr;

    _ClearData();
};

// ---------------------------------------------------------------------------------------------------------------------

GuiWinReceiver::~GuiWinReceiver()
{
    DEBUG("~GuiWinReceiver(%s)", _winName.c_str());
    _winSettings->SetValue("RecentPorts", Ff::StrJoin(_recentPorts, ","));
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinReceiver::SetCallbacks(std::function<void()> dataWinButtonCb, std::function<void()> titleChangeCb,
    std::function<void()> clearCb)
{
    _dataWinButtonsCb = dataWinButtonCb;
    _titleChangeCb    = titleChangeCb;
    _clearCb          = clearCb;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWinReceiver::IsOpen()
{
    // Keep window open as long as receiver is still connected
    return _winOpen || !_receiver->IsIdle();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinReceiver::_UpdateTitle()
{
    std::string title = GetTitle(); // "Receiver X" or "Receiver X: version"

    const auto pos = title.find(':');
    if (pos != std::string::npos)
    {
        title.erase(pos, std::string::npos);
    }

    if (_rxVerStr.size() > 0)
    {
        title += std::string(": ") + _rxVerStr;
    }

    SetTitle(title);

    if (_titleChangeCb)
    {
        _titleChangeCb();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinReceiver::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    // Child window buttons
    if (_dataWinButtonsCb)
    {
        _dataWinButtonsCb();
    }

    Gui::VerticalSeparator();

    // Actions
    {
        // Clear
        if (ImGui::Button(ICON_FK_ERASER "##Clear", _winSettings->iconButtonSize))
        {
            _ClearData();
        }
        Gui::ItemTooltip("Clear all data");
    }

    Gui::VerticalSeparator();

    // Receiver actions
    _DrawCommandButtons();

    Gui::VerticalSeparator();

    _DrawRecordButton();

    ImGui::Separator();

    // Status
    _DrawRxStatus();

    ImGui::Separator();

    // Receiver connection
    _DrawConnectionWidget();

    _DrawWindowEnd();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinReceiver::_DrawConnectionWidget()
{
    // Connect/disconnect/abort button
    {
        // Idle (i.e. disconnected)
        if (_receiver->IsIdle())
        {
            const bool disabled = _port.length() < 5;
            if (disabled) { Gui::BeginDisabled(); }
            if (ImGui::Button(ICON_FK_PLAY "###StartStop", _winSettings->iconButtonSize) || (!disabled && _triggerConnect))
            {
                _receiver->Start(_port, _baudrate);
            }
            if (disabled) { Gui::EndDisabled(); }
            Gui::ItemTooltip("Connect receiver");
        }
        // Ready (i.e. conncted and ready to receive commands)
        else if (_receiver->IsReady())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, Gui::BrightGreen);
            if (ImGui::Button(ICON_FK_STOP "###StartStop", _winSettings->iconButtonSize))
            {
                _receiver->Stop();
            }
            ImGui::PopStyleColor();
            Gui::ItemTooltip("Disconnect receiver");
        }
        // Busy (i.e. connected and busy doing something)
        else if (_receiver->IsBusy())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, Gui::Green);
            if (ImGui::Button(ICON_FK_EJECT "###StartStop", _winSettings->iconButtonSize))
            {
                _receiver->Stop(true);
            }
            ImGui::PopStyleColor();
            Gui::ItemTooltip("Force disconnect receiver");
        }
    }

    ImGui::SameLine();

    // Baudrate
    {
        const bool disabled = _receiver->IsBusy();
        if (disabled) { Gui::BeginDisabled(); }
        if (ImGui::Button(ICON_FK_TACHOMETER "##Baudrate", _winSettings->iconButtonSize))
        {
            ImGui::OpenPopup("Baudrate");
        }
        Gui::ItemTooltip(_baudrate > 0 ?
            Ff::Sprintf("Baudrate (currently: %d)", (int)_baudrate).c_str() : "Baudrate (currently: autobaud)");
        if (ImGui::BeginPopup("Baudrate"))
        {
            const int baudrates[] = { PORT_BAUDRATES };
            ImGui::TextUnformatted("Baudrate");
            int baudrate = _baudrate;
            bool update = false;
            if (ImGui::RadioButton("Auto", &baudrate, 0))
            {
                update = true;
            }
            for (int ix = 0; ix < NUMOF(baudrates); ix++)
            {
                char tmp[10];
                snprintf(tmp, sizeof(tmp), "%6d", baudrates[ix]);
                if (ImGui::RadioButton(tmp, &baudrate, baudrates[ix]))
                {
                    update = true;
                }
            }
            _baudrate = baudrate;
            if (update && _receiver->IsReady())
            {
                _receiver->SetBaudrate(baudrate);
            }
            ImGui::EndPopup();
        }
        if (disabled) { Gui::EndDisabled(); }
    }

    ImGui::SameLine();

    // Port input
    {
        const bool disabled = !_receiver->IsIdle();
        if (disabled) { Gui::BeginDisabled(); }
        ImGui::PushItemWidth(-(_winSettings->iconButtonSize.x + _winSettings->style.ItemSpacing.x));
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;
        if (_focusPortInput)
        {
            ImGui::SetKeyboardFocusHere();
            _focusPortInput = false;
            flags |= ImGuiInputTextFlags_AutoSelectAll;
        }
        if (ImGui::InputTextWithHint("##Port", "Port", &_port, flags))
        {
            _triggerConnect = true;
            DEBUG("add recent port %s", _port.c_str());
            _AddRecentPort(_port);
        }
        else
        {
            _triggerConnect = false;
        }
        //ImGui::SetItemDefaultFocus();
        Gui::ItemTooltip("Port, e.g.:\n[ser://]<device>\ntcp://<addr>:<port>\ntelnet://<addr>:<port>");
        if (disabled) { Gui::EndDisabled(); }
    }

    ImGui::SameLine();

    // Ports button/menu
    {
        const bool disabled = !_receiver->IsIdle();
        if (disabled) { Gui::BeginDisabled(); }
        if (ImGui::Button(ICON_FK_STAR "##Recent", _winSettings->iconButtonSize))
        {
            ImGui::OpenPopup("Recent");
        }
        Gui::ItemTooltip("Recent and detected ports");
        if (ImGui::BeginPopup("Recent"))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
            ImGui::TextUnformatted("Recently used ports");
            ImGui::PopStyleColor();
            for (auto &port : _recentPorts)
            {
                if (ImGui::Selectable(port.c_str()))
                {
                    _port = port;
                    _baudrate = 0;
                    _focusPortInput = true;
                    _AddRecentPort(_port);
                }
            }

            ImGui::Separator();

            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
            ImGui::TextUnformatted("Detected ports");
            ImGui::PopStyleColor();

            ImGui::PushID(1234);
            auto ports = Platform::EnumeratePorts(true);
            for (auto &p: ports)
            {
                if (ImGui::Selectable(p.port.c_str()))
                {
                    _port = p.port;
                    _baudrate = 0;
                    _AddRecentPort(p.port);
                    _focusPortInput = true;
                }
                if (!p.desc.empty())
                {
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
                    ImGui::TextUnformatted(p.desc.c_str());
                    ImGui::PopStyleColor();
                }
            }
            ImGui::PopID();
            if (ports.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
                ImGui::TextUnformatted("No ports detected");
                ImGui::PopStyleColor();
            }

            ImGui::EndPopup();
        }
        if (disabled) { Gui::EndDisabled(); }
    }

    ImGui::Separator();

    // Log
    {
        _log.DrawWidget();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinReceiver::_DrawCommandButtons()
{
    const struct
    {
        const char *button;
        RX_RESET_t  reset;
        const char *tooltip;
    } resets[] =
    {
        { ICON_FK_THERMOMETER_FULL,  RX_RESET_HOT,           "Hotstart\nKeep all navigation data, like u-center" },
        { ICON_FK_THERMOMETER_HALF,  RX_RESET_WARM,          "Warmstart\nDelete ephemerides, like u-center" },
        { ICON_FK_THERMOMETER_EMPTY, RX_RESET_COLD,          "Coldstart\nDelete all navigation data, like u-center" },

        { NULL,                      RX_RESET_SOFT,          "Controlled software reset (0x01)" },
        { NULL,                      RX_RESET_HARD,          "Hardware reset (watchdog) after shutdown (0x04)" },
        { NULL, RX_RESET_NONE, NULL },
        { NULL,                      RX_RESET_DEFAULT,       "Revert config to default, keep nav data" },
        { NULL,                      RX_RESET_FACTORY,       "Revert config to default and coldstart" },
        { NULL, RX_RESET_NONE, NULL },
        { NULL,                      RX_RESET_GNSS_STOP,     "Stop navigation" },
        { NULL,                      RX_RESET_GNSS_START,    "Start navigation" },
        { NULL,                      RX_RESET_GNSS_RESTART,  "Restart navigation" }
    };

    //ImGui::PushID(_winUid);

    const bool disable = !_receiver->IsReady();
    if (disable) { Gui::BeginDisabled(); }

    // Individual buttons
    for (int ix = 0; ix < NUMOF(resets); ix++)
    {
        if (resets[ix].button != NULL)
        {
            if (ImGui::Button(resets[ix].button, _winSettings->iconButtonSize))
            {
                _receiver->Reset(resets[ix].reset);
            }
            Gui::ItemTooltip(resets[ix].tooltip);

            ImGui::SameLine();
        }
    }

    // Button with remaining reset commands
    if (ImGui::Button(ICON_FK_POWER_OFF "##Reset", _winSettings->iconButtonSize))
    {
        ImGui::OpenPopup("ResetCommands");
    }
    if (ImGui::BeginPopup("ResetCommands"))
    {
        for (int ix = 0; ix < NUMOF(resets); ix++)
        {
            if (resets[ix].button != NULL)
            {
                continue;
            }
            if (resets[ix].reset == RX_RESET_NONE)
            {
                ImGui::Separator();
            }
            else
            {
                if (ImGui::MenuItem(rxResetStr(resets[ix].reset)))
                {
                    _receiver->Reset(resets[ix].reset);
                }
                if ( (resets[ix].tooltip[0] != '\0'))
                {
                    Gui::ItemTooltip(resets[ix].tooltip);
                }
            }
        }
        ImGui::EndPopup();
    }
    Gui::ItemTooltip("Reset receiver");

    if (disable) { Gui::EndDisabled(); }
    //ImGui::PopID();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinReceiver::_DrawRecordButton()
{
    const bool disable = !_receiver->IsReady();
    if (disable) { Gui::BeginDisabled(); }

    if (!_recordHandle)
    {
        if (ImGui::Button(ICON_FK_CIRCLE "###Record", _winSettings->iconButtonSize))
        {
            ImGui::OpenPopup("RecordLog");
        }
        Gui::ItemTooltip("Record logfile");
    }
    else
    {
        if (_recordButtonColor != 0) { ImGui::PushStyleColor(ImGuiCol_Text, _recordButtonColor); }
        if (ImGui::Button(ICON_FK_STOP "###Record", _winSettings->iconButtonSize))
        {
            _recordHandle = nullptr;
            _log.AddLine("Recording stopped", Gui::White);
        }
        if (_recordButtonColor != 0) { ImGui::PopStyleColor(); }
        Gui::ItemTooltip(_recordMessage.c_str());
    }

    if (disable) { Gui::EndDisabled(); }

    if (ImGui::BeginPopup("RecordLog"))
    {
        const bool showMessage = (_recordDialogTo > 0.0f) && (ImGui::GetTime() < _recordDialogTo);
        if (showMessage) { Gui::BeginDisabled(); }
        ImGui::PushItemWidth(400.0f);
        ImGui::InputTextWithHint("##FileName", "File name", &_recordFileName);
        ImGui::PopItemWidth();
        if (showMessage) { Gui::EndDisabled(); }
        if (_recordDialogTo < 0.0f)
        {
            if (_recordFileName.empty()) { Gui::BeginDisabled(); }
            if (ImGui::Button(ICON_FK_CHECK " Record"))
            {
                _recordError.clear();
                _recordMessage.clear();
                _recordSize = 0;
                _recordLastSize = 0;
                _recordLastMsgTime = 0.0;
                _recordLastSizeTime = 0.0;
                _recordKiBs = 0.0;
                try
                {
                    _recordHandle = std::make_unique<std::ofstream>();
                    _recordHandle->exceptions(std::ifstream::failbit | std::ifstream::badbit);
                    _recordHandle->open(_recordFileName, std::ofstream::binary);
                    _log.AddLine(Ff::Sprintf("Recording to %s", _recordFileName.c_str()), Gui::White);
                    ImGui::CloseCurrentPopup();
                }
                catch (...)
                {
                    ERROR("Failed opening %s: %s", _recordFileName.c_str(), std::strerror(errno));
                    _recordError = Ff::Sprintf("Error: %s!", std::strerror(errno));
                    _recordDialogTo = ImGui::GetTime() + 2.0;
                }
            }
            ImGui::SameLine();
            if (_recordFileName.empty()) { Gui::EndDisabled(); }
            if (ImGui::Button(ICON_FK_TIMES " Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
        }
        else
        {
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, Gui::BrightRed);
            ImGui::TextUnformatted(_recordError.c_str());
            ImGui::PopStyleColor();
            if (!showMessage)
            {
                _recordDialogTo = -1.0;
            }
        }

        ImGui::EndPopup();
    }

}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinReceiver::_DrawRxStatus()
{
    const float dataOffs = _winSettings->charSize.x * 13;
    const EPOCH_t *epoch = _epoch && _epoch->epoch.valid ? &_epoch->epoch : nullptr;
    const float statusHeight = ImGui::GetTextLineHeightWithSpacing() * 10;

    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
    ImGui::TextUnformatted("Navigation status");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::BeginChild("##StatusLeft", ImVec2(_winSettings->charSize.x * 40, statusHeight), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );

    // Sequence / status
    ImGui::Selectable("Seq, uptime");
    ImGui::SameLine(dataOffs);
    if (_receiver->IsReady())
    {
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
    }
    else if (_receiver->IsBusy())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
        ImGui::TextUnformatted("Receiver busy");
        ImGui::PopStyleColor();
    }
    else if (_receiver->IsIdle())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
        ImGui::TextUnformatted("Receiver idle");
        ImGui::PopStyleColor();
    }

    ImGui::Selectable("Fix type");
    if (epoch && epoch->haveFix)
    {
        ImGui::SameLine(dataOffs);
        ImGui::PushStyleColor(ImGuiCol_Text, _winSettings->GetFixColour(epoch));
        ImGui::TextUnformatted(epoch->fixStr);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
        ImGui::Text("%.1f", ImGui::GetTime() - _fixTime);
        ImGui::PopStyleColor();
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


    ImGui::EndChild();
    Gui::VerticalSeparator();
    ImGui::BeginChild("##StatusRight", ImVec2(0, statusHeight), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );

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
    _DrawSvLevelHist(epoch);

    ImGui::EndChild();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinReceiver::_DrawSvLevelHist(const EPOCH_t *epoch)
{
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

void GuiWinReceiver::Loop(const uint32_t &frame, const double &now)
{
    (void)frame;

    // User requested to close window
    if (!_winOpen)
    {
        if (!_stopSent && !_receiver->IsIdle())
        {
            _receiver->Stop(true);
            _stopSent = true;
        }
    }

    // Expire data
    if (_epoch && !_receiver->IsIdle())
    {
        _epochAge = now - _epochTs;
        if (_epochAge > 5.0)
        {
            _epoch = nullptr;
        }
    }

    // Update recording
    if (_recordHandle)
    {
        constexpr double sizeDeltaTime = 2.0;
        constexpr double msgDeltaTime = 0.5;

        if ( (now - _recordLastMsgTime) > msgDeltaTime)
        {
            _recordMessage = Ff::Sprintf("Stop recording\n(%.3f MiB, %.2f KiB/s)",
                (double)_recordSize * (1.0 / 1024.0 / 1024.0), _recordKiBs);
            _recordLastMsgTime = now;
            _recordButtonColor = _recordButtonColor == 0 ? (ImU32)Gui::Red : 0;
        }
        if ( (now - _recordLastSizeTime) > sizeDeltaTime)
        {
            _recordKiBs = (double)(_recordSize - _recordLastSize) * (1.0 / (sizeDeltaTime * 1024.0));
            _recordLastSize = _recordSize;
            _recordLastSizeTime = now;
        }
    }

    // Pump receiver events and dispatch
    _receiver->Loop(now);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinReceiver::ProcessData(const Data &data)
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

            if (_recordHandle)
            {
                try
                {
                    _recordHandle->write((const char *)data.msg->data, data.msg->size);
                    _recordSize += data.msg->size;
                }
                catch (...)
                {
                    _log.AddLine(Ff::Sprintf("Failed writing %s: %s", _recordFileName.c_str(), std::strerror(errno)), Gui::BrightRed);
                    _recordHandle = nullptr;
                }
            }

            break;
        case Data::Type::INFO_NOTICE:
            _log.AddLine(data.info->c_str(), Gui::BrightWhite);
            break;
        case Data::Type::INFO_WARN:
            _log.AddLine(data.info->c_str(), Gui::Yellow);
            break;
        case Data::Type::INFO_ERROR:
            _log.AddLine(data.info->c_str(), Gui::BrightRed);
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
                _epochTs = ImGui::GetTime();
                if (_fixStr != _epoch->epoch.fixStr)
                {
                    _fixStr  = _epoch->epoch.fixStr;
                    _fixTime = _epochTs;
                }
            }
        default:
            break;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinReceiver::_ClearData()
{
    // Our own stuff
    _log.Clear();
    _database->Clear();
    _fixStr = nullptr;
    if (_clearCb)
    {
        _clearCb();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

/* static */ std::vector<std::string> GuiWinReceiver::_recentPorts = {};

void GuiWinReceiver::_AddRecentPort(const std::string &port)
{
    for (auto iter = _recentPorts.begin(); iter != _recentPorts.end(); iter++)
    {
        if (*iter == port)
        {
            _recentPorts.erase(iter);
            break;
        }
    }
    _recentPorts.insert(_recentPorts.begin(), 1, port);
    while ((int)_recentPorts.size() > 20)
    {
        _recentPorts.pop_back();
    }
}

void GuiWinReceiver::_InitRecentPorts()
{
    // Load from config file on first instance
    static bool loaded = false;
    if (!loaded)
    {
        std::string recentPorts;
        _winSettings->GetValue("RecentPorts", recentPorts, "");
        for (auto &port: Ff::StrSplit(recentPorts, ","))
        {
            if (!port.empty())
            {
                _recentPorts.push_back(port);
            }
        }

        // Populate some examples if none were loaded
        if (_recentPorts.empty())
        {
            _recentPorts.push_back("/dev/ttyACM0");
            _recentPorts.push_back("ser:///dev/ttyUSB0");
            _recentPorts.push_back("tcp://192.168.1.1:12345");
            _recentPorts.push_back("telnet://192.168.1.2:23456");
        }

        loaded = true;
    }
}

/* ****************************************************************************************************************** */
