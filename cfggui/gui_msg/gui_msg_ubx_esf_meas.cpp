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

#include <cstring>
#include <algorithm>

#include "ff_ubx.h"

#include "gui_inc.hpp"

#include "gui_msg_ubx_esf_meas.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxEsfMeas::GuiMsgUbxEsfMeas(std::shared_ptr<InputReceiver> receiver, std::shared_ptr<InputLogfile> logfile) :
    GuiMsg(receiver, logfile),
    _resetPlotRange { true },
    _autoPlotRange  { true }
{
    _table.AddColumn("Measurement");
    _table.AddColumn("Value", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("Raw");
    _table.AddColumn("Ttag sensor", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("Ttag rx", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    //_table.AddColumn("Source"); // TODO: u-center shows "1" for source. What's that? Perhaps a);    _table.AddColumn("Provider");
    _table.AddColumn("Provider");
    _table.AddColumn("Count", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("Age", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);

    Clear();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxEsfMeas::Clear()
{
    _measInfos.clear();
    _table.ClearRows();
    _resetPlotRange = true;
}

// ---------------------------------------------------------------------------------------------------------------------

// Sources:
// - u-center, see tools/make_fake_ubxesfmeas.c and tools/make_fake_ubxesfstatus.c
// - ZED-F9R integration manual (UBX-20039643)
/*static*/ const std::vector<GuiMsgUbxEsfMeas::MeasDef> GuiMsgUbxEsfMeas::MEAS_DEFS =
{
    /*  0 */ { "No data",                GuiMsgUbxEsfMeas::MeasDef::UNSIGNED, "", 0.0,  "" },
    /*  1 */ { "Front wheel angle",      GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%+.3f",  1e-3, "deg" },
    /*  2 */ { "Rear Wheel angle",       GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%+.3f",  1e-3, "deg" },
    /*  3 */ { "Pitch",                  GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%+.3f",  1e-3, "deg" },
    /*  4 */ { "Steering wheel angle",   GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%+.3f",  1e-3, "deg" },
    /*  5 */ { "Gyroscope Z",            GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%+.4f",  0x1p-12 /*perl -e ' printf "%a", 2**-12' */, "deg/s" },
    /*  6 */ { "Wheel tick FL",          GuiMsgUbxEsfMeas::MeasDef::TICK,     "%.0f",   1.0,  "ticks" },
    /*  7 */ { "Wheel tick FR",          GuiMsgUbxEsfMeas::MeasDef::TICK,     "%.0f",   1.0,  "ticks" },
    /*  8 */ { "Wheel tick RL",          GuiMsgUbxEsfMeas::MeasDef::TICK,     "%.0f",   1.0,  "ticks" },
    /*  9 */ { "Wheel tick RR",          GuiMsgUbxEsfMeas::MeasDef::TICK,     "%.0f",   1.0,  "ticks" },
    /* 10 */ { "Single tick",            GuiMsgUbxEsfMeas::MeasDef::TICK,     "%.0f",   1.0,  "ticks" },
    /* 11 */ { "Speed",                  GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%+.3f",  1e-3, "m/s" },
    /* 12 */ { "Gyroscope temp.",        GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%.2f",   1e-2, "C" },
    /* 13 */ { "Gyroscope Y",            GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%+.4f",  0x1p-12 /*perl -e ' printf "%a", 2**-12' */, "deg/s" },
    /* 14 */ { "Gyroscope X",            GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%+.4f",  0x1p-12 /*perl -e ' printf "%a", 2**-12' */, "deg/s" },
    /* 15 */ { nullptr,                  GuiMsgUbxEsfMeas::MeasDef::UNSIGNED, "",       0.0, nullptr },
    /* 16 */ { "Accelerometer X",        GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%+8.4f", 0x1p-10 /*perl -e ' printf "%a", 2**-10' */, "m/s^2" },
    /* 17 */ { "Accelerometer Y",        GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%+8.4f", 0x1p-10 /*perl -e ' printf "%a", 2**-10' */, "m/s^2" },
    /* 18 */ { "Accelerometer Z",        GuiMsgUbxEsfMeas::MeasDef::SIGNED,   "%+8.4f", 0x1p-10 /*perl -e ' printf "%a", 2**-10' */, "m/s^2" },
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

        // (Unique) ID name for this entry
        const uint32_t uid = dataType;
        const std::string name = Ff::Sprintf("%s (%d)##%u", measDef ? measDef->name : "Unknown", dataType, uid);

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
        info->name     = name;
        info->rawHex   = Ff::Sprintf("0x%06x", dataField);
        info->ttagSens = Ff::Sprintf("%.3f", (double)head.timeTag * UBX_ESF_MEAS_V0_CALIBTTAG_SCALE);
        info->ttagRx   = calibTtagValid ? Ff::Sprintf("%.3f", (double)calibTtag * UBX_ESF_MEAS_V0_CALIBTTAG_SCALE) : "";
        info->provider = std::to_string(head.id);

        if (measDef)
        {
            double value = 0.0f;
            switch (measDef->type)
            {
                case MeasDef::SIGNED:
                {
                    // sign extend à la https://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend
                    struct { signed int x: 24; } s;
                    s.x = dataField;
                    const int32_t v = s.x;
                    value = (double)v * measDef->scale;
                    info->value = Ff::Sprintf(measDef->fmt, value);
                    break;
                }
                case MeasDef::UNSIGNED:
                    value = (double)dataField * measDef->scale;
                    info->value = Ff::Sprintf(measDef->fmt, value);
                    break;
                case MeasDef::TICK:
                {
                    const int32_t v = (int32_t)(dataField & 0x007fffff) * (CHKBITS(dataField, 0x00800000) ? -1 : 1);
                    value = (double)v * measDef->scale;
                    info->value = Ff::Sprintf(measDef->fmt, value);
                    break;
                }
            }
            if (measDef->unit)
            {
                info->value += " ";
                info->value += measDef->unit;
            }

            info->plotData.push_back(value);
            while (info->plotData.size() > MAX_PLOT_DATA)
            {
                info->plotData.pop_front();
            }
        }

        UNUSED(timeMarkSent); // TODO
    }

    _table.ClearRows();

    for (auto &entry: _measInfos)
    {
        const uint32_t uid = entry.first;
        auto &info = entry.second;
        _table.AddCellText(info.name, uid);
        _table.AddCellText(info.value);
        _table.AddCellText(info.rawHex);
        _table.AddCellText(info.ttagSens);
        _table.AddCellText(info.ttagRx);
        // _table.AddCellText(info.source);
        _table.AddCellText(info.provider);
        _table.AddCellTextF("%u", info.nMeas);
        if (_receiver)
        {
            _table.AddCellCb([](void *arg)
                {
                    const float dt = (TIME() - ((MeasInfo *)arg)->lastTs) * 1e-3f;
                    if (dt < 1000.0f)
                    {
                        ImGui::Text("%5.1f", dt);
                    }
                    else
                    {
                        ImGui::TextUnformatted("oo");
                    }
                }, &info);
        }
        else
        {
            _table.AddCellEmpty();
        }
        _table.SetRowUid(uid);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxEsfMeas::Buttons()
{
    ImGui::SameLine();

    ImGui::BeginDisabled(_measInfos.empty() || _autoPlotRange);
    if (ImGui::Button(ICON_FK_ARROWS_ALT "###ResetPlotRange"))
    {
        _resetPlotRange = true;
    }
    Gui::ItemTooltip("Reset plot range");
    ImGui::EndDisabled();

    ImGui::SameLine();
    Gui::ToggleButton(ICON_FK_CROP "###AutoPlotRange", NULL, &_autoPlotRange,
        "Automatically adjusting plot range", "Not automatically adjusting plot range", GuiSettings::iconSize);

}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxEsfMeas::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail)
{
    UNUSED(msg);

    const float tableReqHeight = (_table.GetNumRows() + 1) * (GuiSettings::charSize.y + GuiSettings::style->ItemSpacing.y + 2.0f);
    FfVec2f tableSize { sizeAvail.x, (0.5f * sizeAvail.y) - GuiSettings::style->ItemSpacing.y };
    if (tableSize.y > tableReqHeight)
    {
        tableSize.y = tableReqHeight;
    }

    _table.DrawTable(tableSize);

    if (_resetPlotRange || _autoPlotRange)
    {
        ImPlot::SetNextAxesToFit();
    }
    if (ImPlot::BeginPlot("##meas", ImVec2(sizeAvail.x, sizeAvail.y - tableSize.y - GuiSettings::style->ItemSpacing.y),
            ImPlotFlags_Crosshairs | ImPlotFlags_NoFrame))
    {
        ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoTickLabels);
        if (_resetPlotRange || _autoPlotRange)
        {
            ImPlot::SetupAxesLimits(ImAxis_X1, 0, MAX_PLOT_DATA, ImGuiCond_Always);
        }
        ImPlot::SetupFinish();

        for (auto &entry: _measInfos)
        {
            if (!_table.IsSelected(entry.first))
            {
                continue;
            }
            auto &info = entry.second;

            ImPlot::PlotLineG(info.name.c_str(), [](int ix, void *arg)
                {
                    const std::deque<double> *pd = (const std::deque<double> *)arg;
                    return ImPlotPoint( ix, pd->at(ix) );
                }, &info.plotData, info.plotData.size());
        }

        ImPlot::EndPlot();
    }

    _resetPlotRange = false;

    return true;
}

/* ****************************************************************************************************************** */
