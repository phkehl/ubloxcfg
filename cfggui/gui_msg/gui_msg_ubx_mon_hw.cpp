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

#include "ff_ubx.h"

#include "gui_inc.hpp"

#include "gui_msg_ubx_mon_hw.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxMonHw::GuiMsgUbxMonHw(std::shared_ptr<InputReceiver> receiver, std::shared_ptr<InputLogfile> logfile) :
    GuiMsg(receiver, logfile)
{
    _table.AddColumn("Pin");
    _table.AddColumn("Function");
    _table.AddColumn("Type");
    _table.AddColumn("Level");
    _table.AddColumn("IRQ");
    _table.AddColumn("Pull");
    _table.AddColumn("Virtual");
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ const std::vector<GuiMsg::StatusFlags> GuiMsgUbxMonHw::_rtcFlags =
{
    { true,      "calibrated",         GUI_COLOUR(TEXT_OK)      },
    { false,     "uncalibrated",       GUI_COLOUR(TEXT_WARNING) },
};

/*static*/ const std::vector<GuiMsg::StatusFlags> GuiMsgUbxMonHw::_xtalFlags =
{
    { true,      "absent",             GUI_COLOUR(TEXT_OK) },
    { false,     "ok",                 GUI_COLOUR_NONE  },
};

/*static*/ const std::vector<GuiMsg::StatusFlags> GuiMsgUbxMonHw::_aStatusFlags =
{
    { UBX_MON_HW_V0_ASTATUS_INIT,     "init",           GUI_COLOUR_NONE },
    { UBX_MON_HW_V0_ASTATUS_UNKNOWN,  "unknown",        GUI_COLOUR_NONE },
    { UBX_MON_HW_V0_ASTATUS_OK,       "OK",             GUI_COLOUR(TEXT_OK) },
    { UBX_MON_HW_V0_ASTATUS_SHORT,    "short",          GUI_COLOUR(TEXT_WARNING) },
    { UBX_MON_HW_V0_ASTATUS_OPEN,     "open",           GUI_COLOUR(TEXT_WARNING)  },
};

/*static*/ const std::vector<GuiMsg::StatusFlags> GuiMsgUbxMonHw::_aPowerFlags =
{
    { UBX_MON_HW_V0_APOWER_OFF,       "off",            GUI_COLOUR_NONE },
    { UBX_MON_HW_V0_APOWER_ON,        "on",             GUI_COLOUR(TEXT_OK) },
    { UBX_MON_HW_V0_APOWER_UNKNOWN,   "unknown",        GUI_COLOUR_NONE },
};

/*static*/ const std::vector<GuiMsg::StatusFlags> GuiMsgUbxMonHw::_safebootFlags =
{
    { true,                           "active",         GUI_COLOUR(TEXT_WARNING) },
    { false,                          "inactive",       GUI_COLOUR_NONE },

};

/*static*/ const std::vector<GuiMsg::StatusFlags> GuiMsgUbxMonHw::_jammingFlags =
{
    { UBX_MON_HW_V0_FLAGS_JAMMINGSTATE_UNKNOWN,   "unknown",   GUI_COLOUR_NONE },
    { UBX_MON_HW_V0_FLAGS_JAMMINGSTATE_OK,        "ok",        GUI_COLOUR(TEXT_OK) },
    { UBX_MON_HW_V0_FLAGS_JAMMINGSTATE_WARNING,   "warning",   GUI_COLOUR(TEXT_WARNING) },
    { UBX_MON_HW_V0_FLAGS_JAMMINGSTATE_CRITICAL,  "critical",  GUI_COLOUR(TEXT_ERROR) },
};

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxMonHw::Update(const std::shared_ptr<Ff::ParserMsg> &msg)
{
    if (msg->size != UBX_MON_HW_V0_SIZE)
    {
        return;
    }
    UBX_MON_HW_V0_GROUP0_t hw;
    std::memcpy(&hw, &msg->data[UBX_HEAD_SIZE], sizeof(hw));

    _table.ClearRows();
    for (uint32_t pinIx = 0, pinMask = 0x00000001; pinIx < NUMOF(hw.VP); pinIx++, pinMask <<= 1)
    {
        _table.AddCellText(_pinNames[pinIx]);
        const uint8_t vPinIx = hw.VP[pinIx];
        if (CHKBITS(hw.pinSel, pinMask))
        {
            _table.AddCellText(_pioNames[pinIx]);
        }
        else
        {
            _table.AddCellText(CHKBITS(hw.pinBank, pinMask) ? _periphBfuncs[pinIx] : _periphAfuncs[pinIx]);
        }

        // (G)PIO
        if (CHKBITS(hw.pinSel, pinMask))
        {
            _table.AddCellText(CHKBITS(hw.pinDir, pinMask) ? "PIO_OUT" : "PIO_IN" );
        }
        // PERIPH function
        else
        {
            _table.AddCellText(CHKBITS(hw.pinBank, pinMask) ? "PERIPH_B" : "PERIPH_A" );
        }

        _table.AddCellText(CHKBITS(hw.pinVal, pinMask) ? "HIGH" : "LOW");
        _table.AddCellText(CHKBITS(hw.pinIrq, pinMask) ? "yes" : "no"); // FIXME: correct?
        const bool pullLo = CHKBITS(hw.pullL, pinMask);
        const bool pullHi = CHKBITS(hw.pullH, pinMask);
        _table.AddCellText(pullLo && pullHi ? "LOW HIGH" : (pullLo ? "LOW" : (pullHi ? "HIGH" : "")));
        if (CHKBITS(hw.usedMask, pinMask))
        {
            _table.AddCellTextF("%u - %s", vPinIx, (vPinIx < NUMOF(_virtFuncs)) && _virtFuncs[vPinIx] ? _virtFuncs[vPinIx] : "?");
        }
        else
        {
            _table.AddCellEmpty();
        }
        _table.SetRowUid(pinIx + 1);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxMonHw::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail)
{
    if (msg->size != UBX_MON_HW_V0_SIZE)
    {
        return false;
    }
    UBX_MON_HW_V0_GROUP0_t hw;
    std::memcpy(&hw, &msg->data[UBX_HEAD_SIZE], sizeof(hw));

    const ImVec2 topSize = _CalcTopSize(6);
    const ImVec2 topSize2 = ImVec2(0.5 * (sizeAvail.x - GuiSettings::style->ItemSpacing.x), topSize.y);
    const float dataOffs = 16 * GuiSettings::charSize.x;

    if (ImGui::BeginChild("##Status", topSize2))
    {
        _RenderStatusFlag(_rtcFlags,      CHKBITS(hw.flags, UBX_MON_HW_V0_FLAGS_RTCCALIB),   "RTC mode",       dataOffs);
        _RenderStatusFlag(_xtalFlags,     CHKBITS(hw.flags, UBX_MON_HW_V0_FLAGS_XTALABSENT), "RTC XTAL",       dataOffs);
        _RenderStatusFlag(_aStatusFlags,  hw.aStatus,                                        "Antenna status", dataOffs);
        _RenderStatusFlag(_aPowerFlags,   hw.aPower,                                         "Antenna power",  dataOffs);
        _RenderStatusFlag(_safebootFlags, CHKBITS(hw.flags, UBX_MON_HW_V0_FLAGS_SAFEBOOT),   "Safeboot mode",  dataOffs);
        _RenderStatusFlag(_jammingFlags,  UBX_MON_HW_V0_FLAGS_JAMMINGSTATE_GET(hw.flags),    "Jamming status", dataOffs);
    }
    ImGui::EndChild();

    //Gui::VerticalSeparator();
    ImGui::SameLine();

    if (ImGui::BeginChild("##Gauges", topSize2))
    {
        char str[100];

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Noise level");
        ImGui::SameLine(dataOffs);
        const float noise = (float)hw.noisePerMS / (float)UBX_MON_HW_V0_NOISEPERMS_MAX;
        snprintf(str, sizeof(str), "%u", hw.noisePerMS);
        ImGui::ProgressBar(noise, ImVec2(-1.0f,0.0f), str);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("AGC monitor");
        ImGui::SameLine(dataOffs);
        const float agc = (float)hw.agcCnt / (float)UBX_MON_HW_V0_AGCCNT_MAX;
        snprintf(str, sizeof(str), "%.1f%%", agc * 1e2f);
        ImGui::ProgressBar(agc, ImVec2(-1.0f,0.0f), str);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("CW jamming");
        ImGui::SameLine(dataOffs);
        const float jam = (float)hw.jamInd / (float)UBX_MON_HW_V0_JAMIND_MAX;
        snprintf(str, sizeof(str), "%.1f%%", jam * 1e2f);
        ImGui::ProgressBar(jam, ImVec2(-1.0f,0.0f), str);
    }
    ImGui::EndChild();

    _table.DrawTable(sizeAvail - topSize);

    return true;
}

/* ****************************************************************************************************************** */
