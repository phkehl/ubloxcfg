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
#include "gui_msg_ubx_mon_hw3.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxMonHw3::GuiMsgUbxMonHw3(std::shared_ptr<InputReceiver> receiver, std::shared_ptr<InputLogfile> logfile) :
    GuiMsg(receiver, logfile)
{
    _table.AddColumn("Pin");
    _table.AddColumn("Direction");
    _table.AddColumn("State");
    _table.AddColumn("Peripheral");
    _table.AddColumn("IRQ");
    _table.AddColumn("Pull");
    _table.AddColumn("Virtual");
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ const std::vector<GuiMsg::StatusFlags> GuiMsgUbxMonHw3::_rtcFlags =
{
    { true,      "calibrated",         GUI_COLOUR(TEXT_OK)      },
    { false,     "uncalibrated",       GUI_COLOUR(TEXT_WARNING) },
};

/*static*/ const std::vector<GuiMsg::StatusFlags> GuiMsgUbxMonHw3::_xtalFlags =
{
    { true,      "absent",             GUI_COLOUR(TEXT_OK) },
    { false,     "ok",                 GUI_COLOUR_NONE  },
};

/*static*/ const std::vector<GuiMsg::StatusFlags> GuiMsgUbxMonHw3::_safebootFlags =
{
    { true,      "active",             GUI_COLOUR(TEXT_WARNING) },
    { false,     "inactive",           GUI_COLOUR_NONE },

};

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxMonHw3::Update(const std::shared_ptr<Ff::ParserMsg> &msg)
{
    if ( (UBX_MON_HW3_VERSION_GET(msg->data) != UBX_MON_HW3_V0_VERSION) || (UBX_MON_HW3_V0_SIZE(msg->data) != msg->size) )
    {
        return;
    }
    UBX_MON_HW3_V0_GROUP0_t hw;
    std::memcpy(&hw, &msg->data[UBX_HEAD_SIZE], sizeof(hw));

    _table.ClearRows();
    for (int pinIx = 0, offs = UBX_HEAD_SIZE + (int)sizeof(UBX_MON_HW3_V0_GROUP0_t); pinIx < (int)hw.nPins;
            pinIx++, offs += (int)sizeof(UBX_MON_HW3_V0_GROUP1_t))
    {
        UBX_MON_HW3_V0_GROUP1_t pin;
        std::memcpy(&pin, &msg->data[offs], sizeof(pin));

        const uint8_t pinBank = pin.pinId & 0xff;
        const uint8_t pinNo   = (pin.pinId >> 8) & 0xff;
        _table.AddCellTextF("%d %2d##%d", pinBank, pinNo, pinIx);
        _table.AddCellText(CHKBITS(pin.pinMask, UBX_MON_HW3_V0_PINMASK_DIRECTION) ? "output" : "input");
        _table.AddCellText(CHKBITS(pin.pinMask, UBX_MON_HW3_V0_PINMASK_VALUE) ? "high" : "low");
        if (CHKBITS(pin.pinMask, UBX_MON_HW3_V0_PINMASK_PERIPHPIO))
        {
            _table.AddCellText("PIO");
        }
        else
        {
            const uint8_t periph = UBX_MON_HW3_V0_PINMASK_PINBANK_GET(pin.pinMask);
            const char periphChars[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' };
            _table.AddCellTextF("%c", periph < NUMOF(periphChars) ? periphChars[periph] : '?');
        }
        _table.AddCellText(CHKBITS(pin.pinMask, UBX_MON_HW3_V0_PINMASK_PIOIRQ) ? "yes" : "no");
        const bool pullLo = CHKBITS(pin.pinMask, UBX_MON_HW3_V0_PINMASK_PIOPULLLOW);
        const bool pullHi = CHKBITS(pin.pinMask, UBX_MON_HW3_V0_PINMASK_PIOPULLHIGH);
        _table.AddCellText(pullLo && pullHi ? "low high" : (pullLo ? "low" : (pullHi ? "high" : "")));
        if (CHKBITS(pin.pinMask, UBX_MON_HW3_V0_PINMASK_VPMANAGER))
        {
            _table.AddCellTextF("%u - %s", pin.VP,
                (pin.VP < NUMOF(GuiMsgUbxMonHw::_virtFuncs)) && GuiMsgUbxMonHw::_virtFuncs[pin.VP] ?
                GuiMsgUbxMonHw::_virtFuncs[pin.VP] : "?");
        }
        else
        {
            _table.AddCellEmpty();
        }
        _table.SetRowUid(pinIx + 1);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxMonHw3::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail)
{
    if ( (UBX_MON_HW3_VERSION_GET(msg->data) != UBX_MON_HW3_V0_VERSION) || (UBX_MON_HW3_V0_SIZE(msg->data) != msg->size) )
    {
        return false;
    }
    UBX_MON_HW3_V0_GROUP0_t hw;
    std::memcpy(&hw, &msg->data[UBX_HEAD_SIZE], sizeof(hw));

    const ImVec2 topSize = _CalcTopSize(4);
    const float dataOffs = 20 * GuiSettings::charSize.x;

    ImGui::BeginChild("##Status", topSize);
    {
        _RenderStatusFlag(_rtcFlags,       CHKBITS(hw.flags, UBX_MON_HW3_V0_FLAGS_RTCCALIB),   "RTC mode",      dataOffs);
        _RenderStatusFlag(_xtalFlags,      CHKBITS(hw.flags, UBX_MON_HW3_V0_FLAGS_XTALABSENT), "RTC XTAL",      dataOffs);
        _RenderStatusFlag(_safebootFlags,  CHKBITS(hw.flags, UBX_MON_HW3_V0_FLAGS_SAFEBOOT),   "Safeboot mode", dataOffs);
        char str[sizeof(hw.hwVersion) + 1];
        std::memcpy(str, hw.hwVersion, sizeof(hw.hwVersion));
        str[sizeof(str) - 1] = '\0';
        _RenderStatusText("Hardware version", str, dataOffs);
    }
    ImGui::EndChild();

    _table.DrawTable(sizeAvail - topSize);

    return true;
}

/* ****************************************************************************************************************** */
