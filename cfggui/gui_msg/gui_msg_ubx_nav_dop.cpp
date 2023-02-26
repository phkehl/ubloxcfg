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
#include <cmath>

#include "ff_ubx.h"

#include "gui_inc.hpp"

#include "gui_msg_ubx_nav_dop.hpp"

/* ****************************************************************************************************************** */

GuiMsgUbxNavDop::GuiMsgUbxNavDop(std::shared_ptr<InputReceiver> receiver, std::shared_ptr<InputLogfile> logfile) :
    GuiMsg(receiver, logfile),
    _gDopFrac { 0.0f }
{
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiMsgUbxNavDop::Update(const std::shared_ptr<Ff::ParserMsg> &msg)
{
    if (msg->size != UBX_NAV_DOP_V0_SIZE)
    {
        return;
    }

    UBX_NAV_DOP_V0_GROUP0_t dop;
    std::memcpy(&dop, &msg->data[UBX_HEAD_SIZE], sizeof(dop));

    const float gDop = dop.gDOP * UBX_NAV_DOP_V0_XDOP_SCALE;
    _gDopStr  = Ff::Sprintf("%.2f", gDop);
    _gDopFrac = std::log10(CLIP(gDop, LOG_SCALE, MAX_DOP) * (1.0/LOG_SCALE)) / MAX_LOG;

    const float pDop = dop.pDOP * UBX_NAV_DOP_V0_XDOP_SCALE;
    _pDopStr  = Ff::Sprintf("%.2f", pDop);
    _pDopFrac = std::log10(CLIP(pDop, LOG_SCALE, MAX_DOP) * (1.0/LOG_SCALE)) / MAX_LOG;

    const float tDop = dop.tDOP * UBX_NAV_DOP_V0_XDOP_SCALE;
    _tDopStr  = Ff::Sprintf("%.2f", tDop);
    _tDopFrac = std::log10(CLIP(tDop, LOG_SCALE, MAX_DOP) * (1.0/LOG_SCALE)) / MAX_LOG;

    const float vDop = dop.vDOP * UBX_NAV_DOP_V0_XDOP_SCALE;
    _vDopStr  = Ff::Sprintf("%.2f", vDop);
    _vDopFrac = std::log10(CLIP(vDop, LOG_SCALE, MAX_DOP) * (1.0/LOG_SCALE)) / MAX_LOG;

    const float hDop = dop.hDOP * UBX_NAV_DOP_V0_XDOP_SCALE;
    _hDopStr  = Ff::Sprintf("%.2f", hDop);
    _hDopFrac = std::log10(CLIP(hDop, LOG_SCALE, MAX_DOP) * (1.0/LOG_SCALE)) / MAX_LOG;

    const float nDop = dop.nDOP * UBX_NAV_DOP_V0_XDOP_SCALE;
    _nDopStr  = Ff::Sprintf("%.2f", nDop);
    _nDopFrac = std::log10(CLIP(nDop, LOG_SCALE, MAX_DOP) * (1.0/LOG_SCALE)) / MAX_LOG;

    const float eDop = dop.eDOP * UBX_NAV_DOP_V0_XDOP_SCALE;
    _eDopStr  = Ff::Sprintf("%.2f", eDop);
    _eDopFrac = std::log10(CLIP(eDop, LOG_SCALE, MAX_DOP) * (1.0/LOG_SCALE)) / MAX_LOG;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiMsgUbxNavDop::Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail)
{
    UNUSED(sizeAvail);
    if (msg->size != UBX_NAV_DOP_V0_SIZE)
    {
        return false;
    }

    UBX_NAV_DOP_V0_GROUP0_t dop;
    std::memcpy(&dop, &msg->data[UBX_HEAD_SIZE], sizeof(dop));

    const float charWidth = GuiSettings::charSize.x;
    const float dataOffs = 16 * charWidth;
    const float width = ImGui::GetContentRegionAvail().x - dataOffs;

    ImGui::AlignTextToFramePadding();

    const ImVec2 size { -1.0f, 0.0f };

    ImGui::TextUnformatted("Geometric DOP");
    ImGui::SameLine(dataOffs);
    ImGui::ProgressBar(_gDopFrac, size, _gDopStr.c_str());

    ImGui::TextUnformatted("Position DOP");
    ImGui::SameLine(dataOffs);
    ImGui::ProgressBar(_pDopFrac, size, _pDopStr.c_str());

    ImGui::TextUnformatted("Time DOP");
    ImGui::SameLine(dataOffs);
    ImGui::ProgressBar(_tDopFrac, size, _tDopStr.c_str());

    ImGui::TextUnformatted("Vertical DOP");
    ImGui::SameLine(dataOffs);
    ImGui::ProgressBar(_vDopFrac, size, _vDopStr.c_str());

    ImGui::TextUnformatted("Horizontal DOP");
    ImGui::SameLine(dataOffs);
    ImGui::ProgressBar(_hDopFrac, size, _hDopStr.c_str());

    ImGui::TextUnformatted("Northing DOP");
    ImGui::SameLine(dataOffs);
    ImGui::ProgressBar(_nDopFrac, size, _nDopStr.c_str());

    ImGui::TextUnformatted("Easting DOP");
    ImGui::SameLine(dataOffs);
    ImGui::ProgressBar(_eDopFrac, size, _eDopStr.c_str());


    if (width > (charWidth * 30))
    {
        constexpr float o005 = std::log10( 0.5 * (1.0/LOG_SCALE)) / MAX_LOG;
        constexpr float o010 = std::log10( 1.0 * (1.0/LOG_SCALE)) / MAX_LOG;
        constexpr float o050 = std::log10( 5.0 * (1.0/LOG_SCALE)) / MAX_LOG;
        constexpr float o100 = std::log10(10.0 * (1.0/LOG_SCALE)) / MAX_LOG;
        constexpr float o500 = std::log10(50.0 * (1.0/LOG_SCALE)) / MAX_LOG;

        ImGui::NewLine();

        ImGui::SameLine(dataOffs - charWidth);
        ImGui::TextUnformatted("0.1");

        ImGui::SameLine(dataOffs + (o005 * width) - charWidth);
        ImGui::TextUnformatted("0.5");

        ImGui::SameLine(dataOffs + (o010 * width) - charWidth);
        ImGui::TextUnformatted("1.0");

        ImGui::SameLine(dataOffs + (o050 * width));
        ImGui::TextUnformatted("5");

        ImGui::SameLine(dataOffs + (o100 * width) - charWidth);
        ImGui::TextUnformatted("10");

        ImGui::SameLine(dataOffs + (o500 * width) - charWidth);
        ImGui::TextUnformatted("50");
    }

    return true;
}

/* ****************************************************************************************************************** */
