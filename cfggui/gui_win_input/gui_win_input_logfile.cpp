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

#include "platform.hpp"
#include "gui_inc.hpp"

#include "gui_win_input_logfile.hpp"

/* ****************************************************************************************************************** */

GuiWinInputLogfile::GuiWinInputLogfile(const std::string &name) :
    GuiWinInput(name),
    _fileDialog{_winName + "OpenLogFileDialog"}
{
    DEBUG("GuiWinInputLogfile(%s)", _winName.c_str());

    WinSetTitle("Logfile X");

    _dataWinCaps = DataWinDef::Cap_e::PASSIVE;

    _logfile = std::make_shared<InputLogfile>(name, _database);
    _logfile->SetDataCb( std::bind(&GuiWinInputLogfile::_ProcessData, this, std::placeholders::_1) );

    GuiSettings::GetValue(_winName + ".playSpeed", _playSpeed, 1.0f);
    GuiSettings::GetValue(_winName + ".limitPlaySpeed", _limitPlaySpeed, true);
    _logfile->SetPlaySpeed(_playSpeed);
    _playSpeed = _logfile->GetPlaySpeed();

    GuiData::AddLogfile(_logfile);

    _ClearData();
};

// ---------------------------------------------------------------------------------------------------------------------

GuiWinInputLogfile::~GuiWinInputLogfile()
{
    DEBUG("~GuiWinInputLogfile(%s)", _winName.c_str());

    GuiData::RemoveLogfile(_logfile);

    GuiSettings::SetValue(_winName + ".playSpeed", _playSpeed);
    GuiSettings::SetValue(_winName + ".limitPlaySpeed", _limitPlaySpeed);

    if (_logfile)
    {
        _logfile->Close();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

std::shared_ptr<InputLogfile> GuiWinInputLogfile::GetLogfile()
{
    return _logfile;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputLogfile::Loop(const uint32_t &frame, const double &now)
{
    GuiWinInput::Loop(frame, now);
    UNUSED(frame);

    // Pump receiver events and dispatch
    _logfile->Loop(now);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputLogfile::_ProcessData(const InputData &data)
{
    GuiWinInput::_ProcessData(data);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputLogfile::_ClearData()
{
    GuiWinInput::_ClearData();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputLogfile::_AddDataWindow(std::unique_ptr<GuiWinData> dataWin)
{
    dataWin->SetLogfile(_logfile);
    GuiWinInput::_AddDataWindow(std::move(dataWin));
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputLogfile::_DrawActionButtons()
{
    GuiWinInput::_DrawActionButtons();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinInputLogfile::_DrawControls()
{
    const bool canOpen  = _logfile->CanOpen();
    const bool canClose = _logfile->CanClose();
    const bool canPlay  = _logfile->CanPlay();
    const bool canStop  = _logfile->CanStop();
    const bool canPause = _logfile->CanPause();
    const bool canStep  = _logfile->CanStep();
    const bool canSeek  = _logfile->CanSeek();

    // Open log
    {
        ImGui::BeginDisabled(!canOpen);

        if (ImGui::Button(ICON_FK_FOLDER_OPEN "##Open", GuiSettings::iconSize))
        {
            if (!_fileDialog.IsInit())
            {
                _fileDialog.InitDialog(GuiWinFileDialog::FILE_OPEN);
                _fileDialog.WinSetTitle(_winTitle + " - Open logfile...");
                _fileDialog.SetFileFilter("\\.(ubx|raw|ubz|ubx\\.gz)", true);
            }
            else
            {
                _fileDialog.WinFocus();
            }
        }
        Gui::ItemTooltip("Open logfile");

        ImGui::SameLine(0, 0);

        if (ImGui::BeginCombo("##RecentLogs", NULL, ImGuiComboFlags_HeightLarge | ImGuiComboFlags_NoPreview))
        {
            const std::string *selectedLog = nullptr;
            const auto &recent = GuiSettings::GetRecentItems(GuiSettings::RECENT_LOGFILES);
            if (recent.empty())
            {
                ImGui::TextUnformatted("No recent logs");
            }
            for (auto &log: recent)
            {
                if (ImGui::Selectable(log.c_str()))
                {
                    _seekProgress = -1.0f;
                    selectedLog = &log;
                }
            }
            if (!recent.empty())
            {
                ImGui::Separator();
                if (ImGui::Selectable("Clear recent logs"))
                {
                    GuiSettings::ClearRecentItems(GuiSettings::RECENT_LOGFILES);
                }
            }
            ImGui::EndCombo();
            if (selectedLog)
            {
                if (_logfile->Open(*selectedLog))
                {
                    GuiSettings::AddRecentItem(GuiSettings::RECENT_LOGFILES, *selectedLog);
                }
            }
        }

        ImGui::EndDisabled();

        // Handle file dialog, and log open
        if (_fileDialog.IsInit())
        {
            if (_fileDialog.DrawDialog())
            {
                const auto path = _fileDialog.GetPath();
                _seekProgress = -1.0f;
                if (!path.empty() && _logfile->Open(path))
                {
                    GuiSettings::AddRecentItem(GuiSettings::RECENT_LOGFILES, path);
                    _progressBarFmt = Platform::FileName(path) + " (%.3f%%)";
                }
            }
        }
    }

    ImGui::SameLine();

    // Close log
    {
        ImGui::BeginDisabled(!canClose);
        if (ImGui::Button(ICON_FK_EJECT "##Close", GuiSettings::iconSize))
        {
            _logfile->Close();
        }
        ImGui::EndDisabled();
        Gui::ItemTooltip("Close logfile");
    }

    ImGui::SameLine();

    // Play / pause
    {
        ImGui::BeginDisabled(!canPlay && !canPause);
        if (canPause)
        {
            if (ImGui::Button(ICON_FK_PAUSE "##PlayPause", GuiSettings::iconSize))
            {
                _logfile->Pause();
            }
            Gui::ItemTooltip("Pause");
        }
        else
        {
            if (ImGui::Button(ICON_FK_PLAY "##PlayPause", GuiSettings::iconSize))
            {
                _logfile->Play();
            }
            Gui::ItemTooltip("Play");
        }
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    // Stop
    {
        ImGui::BeginDisabled(!canStop);
        if (ImGui::Button(ICON_FK_STOP "##Stop", GuiSettings::iconSize))
        {
            _logfile->Stop();
            _ClearData();
        }
        Gui::ItemTooltip("Stop (rewind)");
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    // Step epoch
    {
        ImGui::BeginDisabled(!canStep);
        ImGui::PushButtonRepeat(true);
        if (ImGui::Button(ICON_FK_FORWARD "##StepEpoch", GuiSettings::iconSize))
        {
            _logfile->StepEpoch();
        }
        Gui::ItemTooltip("Step epoch");
        ImGui::PopButtonRepeat();
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    // Step message
    {
        ImGui::BeginDisabled(!canStep);
        ImGui::PushButtonRepeat(true);
        if (ImGui::Button(ICON_FK_STEP_FORWARD "##StepMsg", GuiSettings::iconSize))
        {
            _logfile->StepMsg();
        }
        Gui::ItemTooltip("Step message");
        ImGui::PopButtonRepeat();
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    // Play speed
    {
        if (ImGui::Checkbox("##SpeedLimit", &_limitPlaySpeed))
        {
            _logfile->SetPlaySpeed(_limitPlaySpeed ? _playSpeed : InputLogfile::PLAYSPEED_INF);
        }
        Gui::ItemTooltip("Limit play speed");

        ImGui::SameLine(0,0);

        ImGui::BeginDisabled(!_limitPlaySpeed);
        ImGui::SetNextItemWidth(GuiSettings::charSize.x * 6);
        if (ImGui::DragFloat("##Speed", &_playSpeed, 0.1f, InputLogfile::PLAYSPEED_MIN, InputLogfile::PLAYSPEED_MAX,
            _limitPlaySpeed ? "%.1f" : "inf", ImGuiSliderFlags_AlwaysClamp))
        /*if (!ImGui::IsItemActive()) { Gui::ItemTooltip("Play speed [epochs/s]"); }
        if (ImGui::IsItemDeactivatedAfterEdit())*/
        {
            _logfile->SetPlaySpeed(_playSpeed);
        }
        const char * const presets[] = { "0.1", "0.5", "1.0", "2.0", "5.0", "10.0", "20.0", "50.0", "100.0" };
        ImGui::EndDisabled();
        Gui::ItemTooltip("Play speed [epochs/s]");

        ImGui::SameLine(0, 0);

        if (ImGui::BeginCombo("##SpeedChoice", NULL, ImGuiComboFlags_HeightLarge | ImGuiComboFlags_NoPreview))
        {
            for (int ix = 0; ix < NUMOF(presets); ix++)
            {
                if (ImGui::Selectable(presets[ix]))
                {
                    _playSpeed = std::strtof(presets[ix], NULL);
                    _logfile->SetPlaySpeed(_playSpeed);
                    _limitPlaySpeed = true;
                }
            }
            ImGui::EndCombo();
        }
    }

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(_logfile->StateStr());

    // Progress indicator
    if (canClose) // i.e. log must be currently open
    {
        float progress = _seekProgress > -1.0f ? _seekProgress : _logfile->GetPlayPos() * 1e2f;
        ImGui::SetNextItemWidth(-1);
        ImGui::BeginDisabled(!canSeek);
        if (ImGui::SliderFloat("##PlayProgress", &progress, 0.0, 100.0, _progressBarFmt.c_str(), ImGuiSliderFlags_AlwaysClamp))
        {
            _seekProgress = progress; // Store away because...
        }
        if (!ImGui::IsItemActive()) { Gui::ItemTooltip("Play position (progress) [%filesize]"); }
        if (ImGui::IsItemDeactivated() && // ...this fires one frame later
            ( std::abs( _logfile->GetPlayPos() - _seekProgress ) > 1e-5 ) )
        {
            _logfile->Seek(_seekProgress * 1e-2f);
            _seekProgress = -1.0f;
        }
        ImGui::EndDisabled();
    }
    else
    {
        float progress = 0;
        ImGui::SetNextItemWidth(-1);
        ImGui::BeginDisabled(true);
        ImGui::SliderFloat("##PlayProgress", &progress, 0.0, 100.0, "");
        ImGui::EndDisabled();
        //ImGui::InvisibleButton("##PlayProgress", GuiSettings::iconSize);
    }
    ImGui::Separator();
}

/* ****************************************************************************************************************** */
