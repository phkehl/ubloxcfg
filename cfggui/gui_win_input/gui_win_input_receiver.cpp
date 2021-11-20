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

#include "ff_port.h"

#include "gui_inc.hpp"

#include "platform.hpp"

#include "gui_win_input_receiver.hpp"

/* ****************************************************************************************************************** */

GuiWinInputReceiver::GuiWinInputReceiver(const std::string &name) :
    GuiWinInput(name),
    _port{}, _baudrate{0},
    _stopSent{false}, _triggerConnect{false}, _focusPortInput{false},
    _recordFileDialog{_winName + "RecordFileDialog"},
    _recordHandle{nullptr}, _recordSize{0}, _recordMessage{""}
{
    DEBUG("GuiWinInputReceiver(%s)", _winName.c_str());

    _winSize   = { 80, 25 };
    SetTitle("Receiver X");

    _receiver = std::make_shared<Receiver>(name, _database);
    _receiver->SetDataCb( std::bind(&GuiWinInputReceiver::_ProcessData, this, std::placeholders::_1) );

    if (_recentPorts.empty())
    {
        _recentPorts = _winSettings->GetValueMult("ReceiverRecentPorts", MAX_RECENT_PORTS);
    }
    // Populate some examples if none were loaded
    if (_recentPorts.empty())
    {
        _recentPorts.push_back("/dev/ttyACM0");
        _recentPorts.push_back("ser:///dev/ttyUSB0");
        _recentPorts.push_back("tcp://192.168.1.1:12345");
        _recentPorts.push_back("telnet://192.168.1.2:23456");
    }
    else
    {
        _port = _recentPorts[0];
    }

    _ClearData();
};

// ---------------------------------------------------------------------------------------------------------------------

GuiWinInputReceiver::~GuiWinInputReceiver()
{
    DEBUG("~GuiWinInputReceiver(%s)", _winName.c_str());
    _winSettings->SetValueMult("ReceiverRecentPorts", _recentPorts, MAX_RECENT_PORTS);
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWinInputReceiver::IsOpen()
{
    // Keep window open as long as receiver is still connected (during disconnect)
    return _winOpen || !_receiver->IsIdle();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputReceiver::Loop(const uint32_t &frame, const double &now)
{
    GuiWinInput::Loop(frame, now);

    // User requested to close window
    if (!_winOpen)
    {
        if (!_stopSent && !_receiver->IsIdle())
        {
            _receiver->Stop();
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
            _recordMessage = Ff::Sprintf("Stop recording\n(%s,\n%.3f MiB, %.2f KiB/s)",
                _recordFilePath.c_str(), (double)_recordSize * (1.0 / 1024.0 / 1024.0), _recordKiBs);
            _recordLastMsgTime = now;
            _recordButtonColor = _recordButtonColor == 0 ? (ImU32)GUI_COLOUR(C_RED) : 0;
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

void GuiWinInputReceiver::_ProcessData(const Data &data)
{
    GuiWinInput::_ProcessData(data);

    switch (data.type)
    {
        case Data::Type::DATA_MSG:
            if (_recordHandle)
            {
                try
                {
                    _recordHandle->write((const char *)data.msg->data, data.msg->size);
                    _recordSize += data.msg->size;
                }
                catch (...)
                {
                    _logWidget.AddLine(Ff::Sprintf("Failed writing %s: %s", _recordFilePath.c_str(), std::strerror(errno)), GUI_COLOUR(DEBUG_ERROR));
                    _recordHandle = nullptr;
                }
            }

            break;
        case Data::Type::DATA_EPOCH:
            if (data.epoch->epoch.valid)
            {
                _epochTs = ImGui::GetTime();
            }
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputReceiver::_ClearData()
{
    GuiWinInput::_ClearData();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputReceiver::_AddDataWindow(std::unique_ptr<GuiWinData> dataWin)
{
    dataWin->SetReceiver(_receiver);
    _dataWindows.push_back(std::move(dataWin));
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputReceiver::_DrawActionButtons()
{
    GuiWinInput::_DrawActionButtons();

    Gui::VerticalSeparator();

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
        { NULL,                      RX_RESET_GNSS_RESTART,  "Restart navigation" },
        { NULL, RX_RESET_NONE, NULL },
        { NULL,                      RX_RESET_SAFEBOOT,      "Safeboot" },
    };

    const bool disable = !_receiver->IsReady();
    ImGui::BeginDisabled(disable);

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

    ImGui::EndDisabled();

    Gui::VerticalSeparator();

    ImGui::BeginDisabled(disable);

    if (!_recordHandle)
    {
        if (ImGui::Button(ICON_FK_CIRCLE "###Record", _winSettings->iconButtonSize))
        {
            if (!_recordFileDialog.IsInit())
            {
                _recordFilePath = "";
                _recordFileDialog.InitDialog(GuiWinFileDialog::FILE_SAVE);
                _recordFileDialog.SetFilename( Ff::Strftime(0, "log_%Y%m%d_%H%M.ubx") );
                _recordFileDialog.SetTitle(_winTitle + " - Record logfile...");
            }
            else
            {
                _recordFileDialog.Focus();
            }
        }
        Gui::ItemTooltip("Record logfile");
    }
    else
    {
        if (_recordButtonColor != 0) { ImGui::PushStyleColor(ImGuiCol_Text, _recordButtonColor); }
        if (ImGui::Button(ICON_FK_STOP "###Record", _winSettings->iconButtonSize))
        {
            _recordHandle = nullptr;
            _logWidget.AddLine("Recording stopped", GUI_COLOUR(DEBUG_NOTICE));
        }
        if (_recordButtonColor != 0) { ImGui::PopStyleColor(); }
        Gui::ItemTooltip(_recordMessage.c_str());
    }

    ImGui::EndDisabled();

    if (_recordFileDialog.IsInit())
    {
        if (_recordFileDialog.DrawDialog())
        {
            _recordFilePath = _recordFileDialog.GetPath();
            if (!_recordFilePath.empty())
            {
                _recordSize = 0;
                _recordLastSize = 0;
                _recordLastMsgTime = 0.0;
                _recordLastSizeTime = 0.0;
                _recordKiBs = 0.0;
                try
                {
                    _recordHandle = std::make_unique<std::ofstream>();
                    _recordHandle->exceptions(std::ifstream::failbit | std::ifstream::badbit);
                    _recordHandle->open(_recordFilePath, std::ofstream::binary);
                    _logWidget.AddLine(Ff::Sprintf("Recording to %s", _recordFilePath.c_str()), GUI_COLOUR(DEBUG_NOTICE));
                    ImGui::CloseCurrentPopup();
                }
                catch (std::exception &e)
                {
                    _logWidget.AddLine(Ff::Sprintf("Failed recording to %s: %s", _recordFilePath.c_str(), e.what()), GUI_COLOUR(DEBUG_ERROR));
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputReceiver::_DrawControls()
{
    // Connect/disconnect/abort button
    {
        // Idle (i.e. disconnected)
        if (_receiver->IsIdle())
        {
            const bool disabled = _port.length() < 5;
            ImGui::BeginDisabled(disabled);
            if (ImGui::Button(ICON_FK_PLAY "###StartStop", _winSettings->iconButtonSize) || (!disabled && _triggerConnect))
            {
                _receiver->Start(_port, _baudrate);
            }
            ImGui::EndDisabled();
            Gui::ItemTooltip("Connect receiver");
        }
        // Ready (i.e. conncted and ready to receive commands)
        else if (_receiver->IsReady())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTGREEN));
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
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_GREEN));
            if (ImGui::Button(ICON_FK_EJECT "###StartStop", _winSettings->iconButtonSize))
            {
                _receiver->Stop();
            }
            ImGui::PopStyleColor();
            Gui::ItemTooltip("Force disconnect receiver");
        }
    }

    ImGui::SameLine();

    // Baudrate
    {
        const bool disabled = _receiver->IsBusy();
        ImGui::BeginDisabled(disabled);
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
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    // Port input
    {
        const bool disabled = !_receiver->IsIdle();
        ImGui::BeginDisabled(disabled);
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
        ImGui::PopItemWidth();
        //ImGui::SetItemDefaultFocus();
        Gui::ItemTooltip("Port, e.g.:\n[ser://]<device>\ntcp://<addr>:<port>\ntelnet://<addr>:<port>");
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    // Ports button/menu
    {
        const bool disabled = !_receiver->IsIdle();
        ImGui::BeginDisabled(disabled);
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
        ImGui::EndDisabled();
    }

    ImGui::Separator();
}

// ---------------------------------------------------------------------------------------------------------------------

/* static */ std::vector<std::string> GuiWinInputReceiver::_recentPorts = {};

void GuiWinInputReceiver::_AddRecentPort(const std::string &port)
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
    while ((int)_recentPorts.size() > MAX_RECENT_PORTS)
    {
        _recentPorts.pop_back();
    }
}

/* ****************************************************************************************************************** */
