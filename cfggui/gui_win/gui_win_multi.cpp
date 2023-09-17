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

#include <algorithm>

#include "gui_inc.hpp"

#include "gui_win_multi.hpp"

/* ****************************************************************************************************************** */

GuiWinMulti::GuiWinMulti(const std::string &name) :
    GuiWin(name),
    _guiDataSerial { 0 },
    _latestTow     { NAN }
{
    _winSize = { 70, 30 };
    DEBUG("GuiWinMulti(%s)", _winName.c_str());

    _map.SetSettings( GuiSettings::GetValue(_winName + ".map") );
    _map.EnableFollowButton();
}

GuiWinMulti::~GuiWinMulti()
{
    DEBUG("~GuiWinMulti(%s)", _winName.c_str());
    GuiSettings::SetValue( _winName + ".map", _map.GetSettings() );
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinMulti::_Clear(const bool refresh)
{
    _points.clear();
    _pointKeys.clear();
    _latestTow = NAN;
    if (refresh)
    {
        _guiDataSerial = 0;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinMulti::Loop(const uint32_t &frame, const double &now)
{
    UNUSED(frame);
    UNUSED(now);
    // Update our lists when available receivers or logfiles changed
    if (_guiDataSerial != GuiData::serial)
    {
        _guiDataSerial = GuiData::serial;
        _allReceivers = GuiData::Receivers();
        _allLogfiles  = GuiData::Logfiles();
        for (auto iter = _usedReceivers.begin(); iter != _usedReceivers.end(); )
        {
            if (std::find(_allReceivers.begin(), _allReceivers.end(), *iter) == _allReceivers.end())
            {
                iter = _usedReceivers.erase(iter);
                _Clear(true);
            }
            else
            {
                iter++;
            }
        }
        for (auto iter = _usedLogfiles.begin(); iter != _usedLogfiles.end(); )
        {
            if (std::find(_allLogfiles.begin(), _allLogfiles.end(), *iter) == _allLogfiles.end())
            {
                iter = _usedLogfiles.erase(iter);
                _Clear(true);
            }
            else
            {
                iter++;
            }
        }
    }

    // Check if something changed
    bool changed = false;
    for (auto &receiver: _usedReceivers)
    {
        if (receiver->GetDatabase()->Changed(this))
        {
            changed = true;
        }
    }
    for (auto &logfile: _usedLogfiles)
    {
        if (logfile->GetDatabase()->Changed(this))
        {
            changed = true;
        }
    }

    if (changed)
    {
        _Clear();

        // All databases
        GuiData::DatabaseList databases;
        GuiData::InputList inputs;
        inputs.insert(inputs.end(), _usedReceivers.begin(), _usedReceivers.end());
        inputs.insert(inputs.end(), _usedLogfiles.begin(), _usedLogfiles.end());
        for (auto &input: inputs)
        {
            databases.push_back(input->GetDatabase());
            auto latestRow = input->LatestRow();
            if (!std::isnan(latestRow.time_gps_tow))
            {
                if (std::isnan(_latestTow) || (latestRow.time_gps_tow > _latestTow))
                {
                    _latestTow = latestRow.time_gps_tow;
                }
            }
        }

        // Collect all points, index by timestamp with some tolerance
        for (auto &database: databases)
        {
            database->ProcRows([&](const Database::Row &row)
            {
                if (row.pos_avail)
                {
                    const uint64_t key = std::isnan(row.time_posix) ? 0 : (uint64_t)((row.time_posix * 0.5e2) + 0.5) * 10;
                    auto entry = _points.find(key);
                    if (entry == _points.end())
                    {
                        auto iter = _points.emplace(key, std::vector<Point>());
                        entry = iter.first;
                    }
                    entry->second.emplace_back(row.pos_llh_lat, row.pos_llh_lon, row.fix_colour);
                }
                return true;
            }, false);
        }

        // Ordered list of all keys (timestamps)
        for (auto &entry: _points)
        {
            _pointKeys.push_back(entry.first);
        }
        std::sort(_pointKeys.begin(), _pointKeys.end(), [](const uint64_t a, const uint64_t b){ return a < b; });
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinMulti::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    _DrawControls();

    Gui::VerticalSeparator();

    if (ImGui::BeginChild("##Data"))
    {
        if (ImGui::BeginTabBar("Views"))
        {
            if (ImGui::BeginTabItem("Map"))
            {
                _DrawMap();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Scatter"))
            {
                ImGui::TextUnformatted("Not yet...");
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Signals"))
            {
                ImGui::TextUnformatted("Not yet...");
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Satellites"))
            {
                ImGui::TextUnformatted("Not yet...");
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::EndChild();

    _DrawWindowEnd();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinMulti::_DrawControls()
{
    if (ImGui::BeginChild("##Controls", ImVec2(GuiSettings::charSize.x * 15, 0)))
    {
        // --- Receivers -----
        for (auto &receiver: _allReceivers)
        {
            auto iter = std::find(_usedReceivers.begin(), _usedReceivers.end(), receiver);
            bool enabled = iter != _usedReceivers.end();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0)); // smaller radio and checkbox
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
            if (ImGui::Checkbox(receiver->GetName().c_str(), &enabled))
            {
                if (enabled)
                {
                    _usedReceivers.push_back(receiver);
                }
                else
                {
                    _usedReceivers.erase(iter);
                    _Clear();
                }
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();

            auto latestRow = receiver->LatestRow();
            if (!std::isnan(latestRow.time_gps_tow) && !std::isnan(_latestTow))
            {
                ImGui::Text("t: %+10.3f", latestRow.time_gps_tow - _latestTow);
            }
            else
            {
                ImGui::TextUnformatted("t: ?");
            }


            ImGui::Separator();
        }

        // ----- Logfiles -----

        bool canPlayAll  = false;
        bool canPauseAll = false;
        bool canStepAll  = false;

        for (auto &logfile: _allLogfiles)
        {
            ImGui::PushID(std::addressof(logfile));

            auto iter = std::find(_usedLogfiles.begin(), _usedLogfiles.end(), logfile);
            bool enabled = iter != _usedLogfiles.end();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0)); // smaller radio and checkbox
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
            if (ImGui::Checkbox(logfile->GetName().c_str(), &enabled))
            {
                if (enabled)
                {
                    _usedLogfiles.push_back(logfile);
                }
                else
                {
                    _usedLogfiles.erase(iter);
                    _Clear();
                }
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();

            auto latestRow = logfile->LatestRow();
            double dt = NAN;
            if (!std::isnan(latestRow.time_gps_tow) && !std::isnan(_latestTow))
            {
                dt = latestRow.time_gps_tow - _latestTow;
                ImGui::Text("t: %+10.3f", dt);
            }
            else
            {
                ImGui::TextUnformatted("t: ?");
            }

            const bool canPlay    = logfile->CanPlay();
            const bool canPause   = logfile->CanPause();
            const bool canStep    = logfile->CanStep();
            const bool canCatchup = (!std::isnan(dt) && dt < -0.001);

            if (canPlay)
            {
                canPlayAll = true;
            }
            if (canPause)
            {
                canPauseAll = true;
            }
            if (canStep)
            {
                canStepAll = true;
            }

            ImGui::BeginDisabled(!enabled);

            ImGui::BeginDisabled(!canPlay && !canPause);
            if (canPause)
            {
                if (ImGui::Button(ICON_FK_PAUSE "##PlayPause", GuiSettings::iconSize))
                {
                    logfile->Pause();
                }
            }
            else
            {
                if (ImGui::Button(ICON_FK_PLAY "##PlayPause", GuiSettings::iconSize))
                {
                    logfile->Play();
                }
            }
            Gui::ItemTooltip(canPause ? "Pause" : "Play");
            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::BeginDisabled(!canStep);
            ImGui::PushButtonRepeat(true);
            if (ImGui::Button(ICON_FK_FORWARD "##StepEpoch", GuiSettings::iconSize))
            {
                logfile->StepEpoch();
            }
            ImGui::PopButtonRepeat();
            ImGui::EndDisabled();
            Gui::ItemTooltip("Step epoch");

            ImGui::SameLine();

            ImGui::BeginDisabled(!canStep);
            ImGui::PushButtonRepeat(true);
            if (ImGui::Button(ICON_FK_STEP_FORWARD "##StepMsg", GuiSettings::iconSize))
            {
                logfile->StepMsg();
            }
            ImGui::PopButtonRepeat();
            ImGui::EndDisabled();
            Gui::ItemTooltip("Step message");

            ImGui::SameLine();

            ImGui::BeginDisabled(!canCatchup);
            ImGui::PushButtonRepeat(true);
            if (ImGui::Button(ICON_FK_FAST_FORWARD "##Catchup", GuiSettings::iconSize))
            {
                WARNING("catch up... not implemented...");
            }
            ImGui::PopButtonRepeat();
            ImGui::EndDisabled();
            Gui::ItemTooltip("Catch up with leading log");

            ImGui::EndDisabled();

            ImGui::Separator();
            ImGui::PopID();
        }

        // ---- Controls for all selected inputs ----

        ImGui::Separator();
        ImGui::BeginDisabled(!canPlayAll && !canPauseAll);
        if (canPauseAll)
        {
            if (ImGui::Button(ICON_FK_PAUSE "##PauseAll", GuiSettings::iconSize))
            {
                for (auto &logfile: _usedLogfiles)
                {
                    logfile->Pause();
                }
            }
        }
        else
        {
            if (ImGui::Button(ICON_FK_PLAY "##PlayAll", GuiSettings::iconSize))
            {
                for (auto &logfile: _usedLogfiles)
                {
                    logfile->Play();
                }
            }
        }
        ImGui::EndDisabled();
        Gui::ItemTooltip(canPauseAll ? "Pause all" : "Play all");

        ImGui::SameLine();
        ImGui::BeginDisabled(!canStepAll);
        if (ImGui::Button(ICON_FK_FORWARD "##StepEpochAll", GuiSettings::iconSize))
        {
            for (auto &logfile: _usedLogfiles)
            {
                logfile->StepEpoch();
            }
        }
        Gui::ItemTooltip("Step epoch all");
        ImGui::SameLine();
        if (ImGui::Button(ICON_FK_STEP_FORWARD "##StepMsgAll", GuiSettings::iconSize))
        {
            for (auto &logfile: _usedLogfiles)
            {
                logfile->StepMsg();
            }
        }
        ImGui::EndDisabled();
        Gui::ItemTooltip("Step message all");
    }

    ImGui::EndChild();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinMulti::_DrawMap()
{
    if (!_map.BeginDraw())
    {
        return;
    }

    ImDrawList *draw = ImGui::GetWindowDrawList();
    for (const auto key: _pointKeys)
    {
        const auto &points = _points[key];
        for (auto iter = points.cbegin(); iter != points.cend(); iter++)
        {
            const auto &point = *iter;
            const FfVec2f xy = _map.LonLatToScreen(point.lat, point.lon);
            if (iter != points.cbegin())
            {
                const auto &point0 = *(iter - 1);
                const FfVec2f xy0 = _map.LonLatToScreen(point0.lat, point0.lon);
                draw->AddLine(xy0, xy, GUI_COLOUR(PLOT_MAP_BASELINE));
            }

            draw->AddRectFilled(xy - FfVec2f(2,2), xy + FfVec2f(2,2), point.colour);
        }
    }

    if (!_pointKeys.empty() && _map.FollowEnabled())
    {
        const auto& latest = _points[_pointKeys.back()];
        if (!latest.empty())
        {
            _map.SetPos(latest[0].lat, latest[0].lon);
        }
    }

    _map.EndDraw();
}

/* ****************************************************************************************************************** */
