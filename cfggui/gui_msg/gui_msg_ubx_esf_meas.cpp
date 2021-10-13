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

/* ****************************************************************************************************************** */

GuiMsgUbxEsfMeas::GuiMsgUbxEsfMeas(std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile) :
    GuiMsg(receiver, logfile)
{
    Clear();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxEsfMeas::Clear()
{
    _measInfos.clear();
    _selected.clear();
}

// ---------------------------------------------------------------------------------------------------------------------

// Sources:
// - u-center, see tools/make_fake_ubxesfmeas.c and tools/make_fake_ubxesfstatus.c
// - ZED-F9R integration manual (UBX-20039643)
/*static*/ const std::vector<GuiMsgUbxEsfMeas::MeasDef> GuiMsgUbxEsfMeas::MEAS_DEFS =
{
    /*  0 */ { "No data",                GuiMsgUbxEsfMeas::MeasDef::UNSIGNED, "", 0.0,  "" },
    /*  1 */ { "Front wheel angle",      GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%.3f", 1e-3, "deg" },
    /*  2 */ { "Rear Wheel angle",       GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%.3f", 1e-3, "deg" },
    /*  3 */ { "Pitch",                  GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%.3f", 1e-3, "deg" },
    /*  4 */ { "Steering wheel angle",   GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%.3f", 1e-3, "deg" },
    /*  5 */ { "Gyroscope Z",            GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%.4f", 2.44140625e-04 /* 2**-12 */, "deg/s" },
    /*  6 */ { "Wheel tick FL",          GuiMsgUbxEsfMeas::MeasDef::TICK,     "%f",   1.0,  "ticks" },
    /*  7 */ { "Wheel tick FR",          GuiMsgUbxEsfMeas::MeasDef::TICK,     "%f",   1.0,  "ticks" },
    /*  8 */ { "Wheel tick RL",          GuiMsgUbxEsfMeas::MeasDef::TICK,     "%f",   1.0,  "ticks" },
    /*  9 */ { "Wheel tick RR",          GuiMsgUbxEsfMeas::MeasDef::TICK,     "%f",   1.0,  "ticks" },
    /* 10 */ { "Single tick",            GuiMsgUbxEsfMeas::MeasDef::TICK,     "%f",   1.0,  "ticks" },
    /* 11 */ { "Speed",                  GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%.3f", 1e-3, "m/s" },
    /* 12 */ { "Gyroscope temp.",        GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%.2f", 1e-2, "C" },
    /* 13 */ { "Gyroscope Y",            GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%.4f", 2.44140625e-04 /* 2**-12 */, "deg/s" },
    /* 14 */ { "Gyroscope X",            GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%.4f", 2.44140625e-04 /* 2**-12 */, "deg/s" },
    /* 15 */ { nullptr,                  GuiMsgUbxEsfMeas::MeasDef::UNSIGNED, "",     0.0, nullptr },
    /* 16 */ { "Accelerometer X",        GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%.4f", 9.765625e-4 /* 2**-10 */, "m/s^2" },
    /* 17 */ { "Accelerometer Y",        GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%.4f", 9.765625e-4 /* 2**-10 */, "m/s^2" },
    /* 18 */ { "Accelerometer Z",        GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%.4f", 9.765625e-4 /* 2**-10 */, "m/s^2" },
};

// ---------------------------------------------------------------------------------------------------------------------

GuiMsgUbxEsfMeas::MeasInfo::MeasInfo() :
    nMeas{0}
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxEsfMeas::Update(const std::shared_ptr<Ff::ParserMsg> &msg)
{
    if (UBX_ESF_MEAS_V0_SIZE(msg->data) != msg->size)
    {
        return;
    }

    UBX_ESF_MEAS_V0_GROUP0_t head;
    std::memcpy(&head, &msg->data[UBX_HEAD_SIZE], sizeof(head));
    const int numMeas = UBX_ESF_MEAS_V0_FLAGS_NUMMEAS_GET(head.flags);
    const int timeMarkSent = UBX_ESF_MEAS_V0_FLAGS_TIMEMARKSENT_GET(head.flags);
    const bool calibTtagValid = CHKBITS(head.flags, UBX_ESF_MEAS_V0_FLAGS_CALIBTTAGVALID);

    uint32_t calibTtag = 0;
    if (calibTtagValid)
    {
        UBX_ESF_MEAS_V0_GROUP2_t ttag;
        std::memcpy(&ttag, &msg->data[UBX_HEAD_SIZE + sizeof(UBX_ESF_MEAS_V0_GROUP0_t) + (numMeas * sizeof(UBX_ESF_MEAS_V0_GROUP1_t))] , sizeof(ttag));
        calibTtag = ttag.calibTtag;
    }

    for (int measIx = 0, offs = UBX_HEAD_SIZE + (int)sizeof(UBX_ESF_MEAS_V0_GROUP0_t); measIx < numMeas;
            measIx++, offs += (int)sizeof(UBX_ESF_MEAS_V0_GROUP1_t))
    {
        UBX_ESF_MEAS_V0_GROUP1_t meas;
        std::memcpy(&meas, &msg->data[offs], sizeof(meas));
        const int      dataType  = UBX_ESF_MEAS_V0_DATA_DATATYPE_GET(meas.data);
        const uint32_t dataField = UBX_ESF_MEAS_V0_DATA_DATAFIELD_GET(meas.data);

        auto *measDef = (dataType < (int)MEAS_DEFS.size()) && MEAS_DEFS[dataType].name ? &MEAS_DEFS[dataType] : nullptr;

        // UID for this entry
        const std::string uid = measDef ? measDef->name : std::to_string(dataType);

        // Find entry, create new one if necessary
        MeasInfo *info = nullptr;
        auto entry = _measInfos.find(uid);
        if (entry != _measInfos.end())
        {
            info = &entry->second;
        }
        else
        {
            auto foo = _measInfos.insert({ uid, MeasInfo() });
            info = &foo.first->second;
        }

        // Populate/update info
        info->nMeas++;
        info->lastTs   = msg->ts;
        info->name     = Ff::Sprintf("%s (%d)", measDef ? measDef->name : "Unknown", dataType);
        info->rawHex   = Ff::Sprintf("0x%06x", dataField);
        info->ttagSens = Ff::Sprintf("%.3f", (double)head.timeTag * UBX_ESF_MEAS_V0_CALIBTTAG_SCALE);
        info->ttagRx   = calibTtagValid ? Ff::Sprintf("%.3f", (double)calibTtag * UBX_ESF_MEAS_V0_CALIBTTAG_SCALE) : "";
        info->provider = std::to_string(head.id);

        if (measDef)
        {
            switch (measDef->type)
            {
                case MeasDef::SIGNED:
                {
                    // sign extend à la https://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend
                    struct { signed int x: 24; } s;
                    s.x = dataField;
                    const int32_t v = s.x;
                    info->value = Ff::Sprintf(measDef->fmt, (double)v * measDef->scale);
                    break;
                }
                case MeasDef::UNSIGNED:
                    info->value = Ff::Sprintf(measDef->fmt, (double)dataField * measDef->scale);
                    break;
                case MeasDef::TICK:
                {
                    const int32_t v = (int32_t)(dataField & 0x007fffff) * (CHKBITS(dataField, 0x00800000) ? -1 : 1);
                    info->value = Ff::Sprintf(measDef->fmt, (double)v * measDef->scale);
                    break;
                }
            }
            if (measDef->unit)
            {
                info->value += " ";
                info->value += measDef->unit;
            }
        }

        UNUSED(timeMarkSent); // TODO
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxEsfMeas::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const ImVec2 &sizeAvail)
{
    UNUSED(msg);
    const uint32_t now = TIME();

    const struct { const char *label; ImGuiTableColumnFlags flags; } columns[] =
    {
        { .label = "Measurement",  .flags = ImGuiTableColumnFlags_NoReorder },
        { .label = "Value",        .flags = 0 },
        { .label = "Raw ",         .flags = 0 },
        { .label = "Ttag sensor",  .flags = 0 },
        { .label = "Ttag rx",      .flags = 0 },
        //{ .label = "Source",       .flags = 0 }, // TODO: u-center shows "1" for source. What's that? Perhaps a u-center internal thing...
        { .label = "Provider",     .flags = 0 },
        { .label = "Count",        .flags = 0 },
        { .label = "Age",          .flags = 0 },
    };

    if (ImGui::BeginTable("meas", NUMOF(columns), TABLE_FLAGS, sizeAvail))
    {
        ImGui::TableSetupScrollFreeze(4, 1);
        for (int ix = 0; ix < NUMOF(columns); ix++)
        {
            ImGui::TableSetupColumn(columns[ix].label, columns[ix].flags);
        }
        ImGui::TableHeadersRow();

        for (auto &entry: _measInfos)
        {
            int ix = 0;
            auto &info = entry.second;
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(ix++);
            if (ImGui::Selectable(info.name.c_str(), _selected == entry.first, ImGuiSelectableFlags_SpanAllColumns))
            {
                _selected = (_selected == entry.first ? 0 : entry.first);
            }

            ImGui::TableSetColumnIndex(ix++);
            ImGui::TextUnformatted(info.value.c_str());

            ImGui::TableSetColumnIndex(ix++);
            ImGui::TextUnformatted(info.rawHex.c_str());

            ImGui::TableSetColumnIndex(ix++);
            ImGui::TextUnformatted(info.ttagSens.c_str());

            ImGui::TableSetColumnIndex(ix++);
            ImGui::TextUnformatted(info.ttagRx.c_str());

            // ImGui::TableSetColumnIndex(ix++);
            // ImGui::TextUnformatted(info.source.c_str());

            ImGui::TableSetColumnIndex(ix++);
            ImGui::TextUnformatted(info.provider.c_str());

            ImGui::TableSetColumnIndex(ix++);
            ImGui::Text("%u", info.nMeas);

            ImGui::TableSetColumnIndex(ix++);
            ImGui::Text("%.1f", (now - info.lastTs) * 1e-3);
        }

        ImGui::EndTable();
    }

    return true;
}

/* ****************************************************************************************************************** */
