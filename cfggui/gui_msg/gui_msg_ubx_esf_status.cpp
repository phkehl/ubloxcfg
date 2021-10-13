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

#include <cstring>
#include <algorithm>

#include "ff_ubx.h"

#include "imgui.h"
#include "implot.h"
#include "IconsForkAwesome.h"

#include "gui_msg_ubx_esf_meas.hpp"
#include "gui_msg_ubx_esf_status.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxEsfStatus::GuiMsgUbxEsfStatus(std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile) :
    GuiMsg(receiver, logfile), _valid{false}
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxEsfStatus::Clear()
{
    _valid = false;
    _sensors.clear();
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ const std::vector<GuiMsg::StatusFlags> GuiMsgUbxEsfStatus::_fusionModeFlags =
{
    { UBX_ESF_STATUS_V2_FUSIONMODE_INIT,      "Init",         GUI_COLOUR(TEXT_WARNING) },
    { UBX_ESF_STATUS_V2_FUSIONMODE_FUSION,    "Fusion",       GUI_COLOUR(TEXT_OK)      },
    { UBX_ESF_STATUS_V2_FUSIONMODE_SUSPENDED, "Suspended",    GUI_COLOUR(TEXT_ERROR)   },
    { UBX_ESF_STATUS_V2_FUSIONMODE_DISABLED,  "Disabled",     GUI_COLOUR(TEXT_WARNING) },
};

/*static*/ const std::vector<GuiMsg::StatusFlags> GuiMsgUbxEsfStatus::_statusFlags012 =
{
    { UBX_ESF_STATUS_V2_INITSTATUS1_WTINITSTATUS_OFF,          "Off",          GUI_COLOUR(C_NONE) },
    { UBX_ESF_STATUS_V2_INITSTATUS1_WTINITSTATUS_INITALIZING,  "Initialising", GUI_COLOUR(TEXT_WARNING) },
    { UBX_ESF_STATUS_V2_INITSTATUS1_WTINITSTATUS_INITIALIZED,  "Initialised",  GUI_COLOUR(TEXT_OK) },
};

/*static*/ const std::vector<GuiMsg::StatusFlags> GuiMsgUbxEsfStatus::_statusFlags0123 =
{
    { UBX_ESF_STATUS_V2_INITSTATUS1_MNTALGSTATUS_OFF,          "Off",           GUI_COLOUR(C_NONE) },
    { UBX_ESF_STATUS_V2_INITSTATUS1_MNTALGSTATUS_INITALIZING,  "Initialising",  GUI_COLOUR(TEXT_WARNING) },
    { UBX_ESF_STATUS_V2_INITSTATUS1_MNTALGSTATUS_INITIALIZED1, "Initialised",   GUI_COLOUR(TEXT_OK) },
    { UBX_ESF_STATUS_V2_INITSTATUS1_MNTALGSTATUS_INITIALIZED2, "Initialised!",  GUI_COLOUR(TEXT_OK) },
};

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxEsfStatus::Update(const std::shared_ptr<Ff::ParserMsg> &msg)
{
    Clear();
    if ( (UBX_ESF_STATUS_VERSION_GET(msg->data) != UBX_ESF_STATUS_V2_VERSION) || (UBX_ESF_STATUS_V2_SIZE(msg->data) != msg->size) )
    {
        return;
    }

    UBX_ESF_STATUS_V2_GROUP0_t status;
    std::memcpy(&status, &msg->data[UBX_HEAD_SIZE], sizeof(status));

    _valid         = true;
    _iTow          = (double)status.iTOW * UBX_ESF_STATUS_V2_ITOW_SCALE;
    _wtInitStatus  = UBX_ESF_STATUS_V2_INITSTATUS1_WTINITSTATUS_GET(status.initStatus1);
    _mntAlgStatus  = UBX_ESF_STATUS_V2_INITSTATUS1_MNTALGSTATUS_GET(status.initStatus1);
    _insInitStatus = UBX_ESF_STATUS_V2_INITSTATUS1_INSINITSTATUS_GET(status.initStatus1);
    _imuInitStatus = UBX_ESF_STATUS_V2_INITSTATUS2_IMUINITSTATUS_GET(status.initStatus2);
    _fusionMode    = status.fusionMode;

    for (int sensIx = 0, offs = UBX_HEAD_SIZE + (int)sizeof(UBX_ESF_STATUS_V2_GROUP0_t); sensIx < (int)status.numSens;
            sensIx++, offs += (int)sizeof(UBX_ESF_STATUS_V2_GROUP1_t))
    {
        _sensors.emplace_back(Sensor(&msg->data[offs]));
    }

    std::sort(_sensors.begin(), _sensors.end(), [](const Sensor &a, const Sensor &b) { return a.type < b.type; });
}

// ---------------------------------------------------------------------------------------------------------------------

GuiMsgUbxEsfStatus::Sensor::Sensor(uint8_t *groupData)
{
    UBX_ESF_STATUS_V2_GROUP1_t sensor;
    std::memcpy(&sensor, groupData, sizeof(sensor));

    const int sensorType = UBX_ESF_STATUS_V2_SENSSTATUS1_TYPE_GET(sensor.sensStatus1);
    if (sensorType < (int)GuiMsgUbxEsfMeas::MEAS_DEFS.size() && GuiMsgUbxEsfMeas::MEAS_DEFS[sensorType].name)
    {
        type = Ff::Sprintf("%s##%p", GuiMsgUbxEsfMeas::MEAS_DEFS[sensorType].name, groupData);
    }
    else
    {
        type = Ff::Sprintf("Unknown (type %d)##%p", sensorType, groupData);
    }

    used = CHKBITS(sensor.sensStatus1, UBX_ESF_STATUS_V2_SENSSTATUS1_USED) ? "yes" : "no";

    ready = CHKBITS(sensor.sensStatus1, UBX_ESF_STATUS_V2_SENSSTATUS1_READY) ? "yes" : "no";

    constexpr const char * const calibStatusStrs[] = { "not calibrated", "calibrating", "calibrated", "calibrated!" };
    const int cIx = UBX_ESF_STATUS_V2_SENSSTATUS2_CALIBSTATUS_GET(sensor.sensStatus2);
    calibStatus = cIx < NUMOF(calibStatusStrs) ? calibStatusStrs[cIx] : "?";

    constexpr const char  * const timeStatusStrs[] = { "no data", "first byte", "event input", "time tag" };
    const int tIx = UBX_ESF_STATUS_V2_SENSSTATUS2_TIMESTATUS_GET(sensor.sensStatus2);
    timeStatus = tIx < NUMOF(timeStatusStrs) ? timeStatusStrs[tIx] : "?";

    freq = Ff::Sprintf("%d", sensor.freq);

    faults = "";
    if (CHKBITS(sensor.faults, UBX_ESF_STATUS_V2_FAULTS_BADMEAS))
    {
        faults += "meas ";
    }
    if (CHKBITS(sensor.faults, UBX_ESF_STATUS_V2_FAULTS_BADTTAG))
    {
        faults += "ttag ";
    }
    if (CHKBITS(sensor.faults, UBX_ESF_STATUS_V2_FAULTS_MISSINGMEAS))
    {
        faults += "missing ";
    }
    if (CHKBITS(sensor.faults, UBX_ESF_STATUS_V2_FAULTS_NOISYMEAS))
    {
        faults += "noisy ";
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxEsfStatus::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const ImVec2 &sizeAvail)
{
    if (!_valid)
    {
        return false;
    }
    if ( (UBX_ESF_STATUS_VERSION_GET(msg->data) != UBX_ESF_STATUS_V2_VERSION) || (UBX_ESF_STATUS_V2_SIZE(msg->data) != msg->size) )
    {
        return false;
    }

    UNUSED(msg);

    UBX_ESF_STATUS_V2_GROUP0_t status;
    std::memcpy(&status, &msg->data[UBX_HEAD_SIZE], sizeof(status));

    const float topHeight = 6 * (_winSettings->charSize.y + _winSettings->style.ItemSpacing.y);
    const ImVec2 topSize { 0.0f, topHeight };
    const ImVec2 bottomSize { sizeAvail.x, sizeAvail.y - topSize.y -_winSettings->style.ItemSpacing.y };
    const float dataOffs = 25 * _winSettings->charSize.x;

    ImGui::BeginChild("##Status", topSize);
    {
        ImGui::TextUnformatted("GPS time");
        ImGui::SameLine(dataOffs);
        ImGui::Text("%10.3f", _iTow);
        _RenderStatusFlag(_fusionModeFlags, _fusionMode ,   "Fusion mode",        dataOffs);
        _RenderStatusFlag(_statusFlags0123, _imuInitStatus, "IMU status",         dataOffs);
        _RenderStatusFlag(_statusFlags012,  _wtInitStatus,  "Wheel-ticks status", dataOffs);
        _RenderStatusFlag(_statusFlags0123, _insInitStatus, "INS status",         dataOffs);
        _RenderStatusFlag(_statusFlags0123, _mntAlgStatus,  "Mount align status", dataOffs);
    }
    ImGui::EndChild();

    const struct { const char *label; ImGuiTableColumnFlags flags; } columns[] =
    {
        { .label = "Sensor",        .flags = ImGuiTableColumnFlags_NoReorder },
        { .label = "Used",          .flags = 0 },
        { .label = "Ready",         .flags = 0 },
        { .label = "Calibration",   .flags = 0 },
        { .label = "Time",          .flags = 0 },
        { .label = "Freq",          .flags = 0 },
        { .label = "Faults",        .flags = 0 },
    };

    if (ImGui::BeginTable("Sensors", NUMOF(columns), TABLE_FLAGS, bottomSize))
    {
        ImGui::TableSetupScrollFreeze(1, 1);
        for (int ix = 0; ix < NUMOF(columns); ix++)
        {
            ImGui::TableSetupColumn(columns[ix].label, columns[ix].flags);
        }
        ImGui::TableHeadersRow();

        for (const auto &sensor: _sensors)
        {
            ImGui::TableNextRow();
            int colIx = 0;

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::Selectable(sensor.type.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::TextUnformatted(sensor.used.c_str());

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::TextUnformatted(sensor.ready.c_str());

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::TextUnformatted(sensor.calibStatus.c_str());

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::TextUnformatted(sensor.timeStatus.c_str());

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::TextUnformatted(sensor.freq.c_str());

            ImGui::TableSetColumnIndex(colIx++);
            ImGui::TextUnformatted(sensor.faults.c_str());
        }

        ImGui::EndTable();
    }

    return true;
}

/* ****************************************************************************************************************** */
