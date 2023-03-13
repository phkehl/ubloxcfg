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

#include <cmath>
#include <cstring>

#include "gui_inc.hpp"

#include "gui_win_data_signals.hpp"
#include "gui_win_data_satellites.hpp"

/* ****************************************************************************************************************** */

GuiWinDataSatellites::GuiWinDataSatellites(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database),
    _tabbar1   { WinName() + "Tabbar1" },
    _tabbar2   { WinName() + "Tabbar2", ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_TabListPopupButton }
{
    _winSize = { 80, 25 };

    ClearData();

    _table.AddColumn("SV", 0.0f, GuiWidgetTable::ColumnFlags::SORTABLE);
    _table.AddColumn("Azim", 0.0f, GuiWidgetTable::ColumnFlags::SORTABLE | GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("Elev", 0.0f, GuiWidgetTable::ColumnFlags::SORTABLE | GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("Orbit", 0.0f, GuiWidgetTable::ColumnFlags::SORTABLE);
    _table.AddColumn("Signal L1", 0.0f, GuiWidgetTable::ColumnFlags::SORTABLE);
    _table.AddColumn("Signal L2", 0.0f, GuiWidgetTable::ColumnFlags::SORTABLE);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::_ProcessData(const InputData &data)
{
    if (data.type == InputData::DATA_EPOCH)
    {
        _UpdateSatellites();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::_ClearData()
{
    _UpdateSatellites();
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataSatellites::Sat::Sat(const EPOCH_SATINFO_t &satInfo, const EPOCH_t &epoch) :
    satInfo_ { satInfo },
    visible_ { (satInfo.orbUsed > EPOCH_SATORB_NONE) && (satInfo.elev >= 0) },
    gnssIx_  { GnssIx::OTHER }
{
    std::memset(&sigL1_, 0, sizeof(sigL1_));
    std::memset(&sigL2_, 0, sizeof(sigL2_));

    const float r = 1.0f - ((float)satInfo_.elev * (1.0f / 90.f));
    const float a = (270.0f + (float)satInfo_.azim) * (M_PI / 180.0);
    xy_ = FfVec2f(std::cos(a), std::sin(a)) * r;

    // Add signal levels
    for (int ix = 0; (ix < epoch.numSignals) && !(sigL1_.valid && sigL2_.valid); ix++)
    {
        const EPOCH_SIGINFO_t &sig = epoch.signals[ix];
        if ( (satInfo_.sv == sig.sv) && (satInfo_.gnss == sig.gnss) )
        {
            switch (sig.band)
            {
                case EPOCH_BAND_L1: sigL1_ = sig; break;
                case EPOCH_BAND_L2: sigL2_ = sig; break;
                case EPOCH_BAND_L5:
                case EPOCH_BAND_UNKNOWN:
                    break;
            }
        }
    }

    switch (satInfo_.gnss)
    {
        case EPOCH_GNSS_GPS: gnssIx_ = GnssIx::GPS; break;
        case EPOCH_GNSS_GLO: gnssIx_ = GnssIx::GLO; break;
        case EPOCH_GNSS_BDS: gnssIx_ = GnssIx::BDS; break;
        case EPOCH_GNSS_GAL: gnssIx_ = GnssIx::GAL; break;
        case EPOCH_GNSS_SBAS:
        case EPOCH_GNSS_QZSS:
        case EPOCH_GNSS_UNKNOWN:
            break;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataSatellites::Count::Count()
{
    Reset();
};

void GuiWinDataSatellites::Count::Reset()
{
    for (GnssIx ix: GNSS_IXS)
    {
        num_[ix] = 0;
        used_[ix] = 0;
        visible_[ix] = 0;
        labelSky_[ix] = Ff::Sprintf("%s###%s", GNSS_NAMES[ix], GNSS_NAMES[ix]);
        labelList_[ix] = Ff::Sprintf("%s###%s", GNSS_NAMES[ix], GNSS_NAMES[ix]);
    }
}

void GuiWinDataSatellites::Count::Update()
{
    for (GnssIx ix: GNSS_IXS)
    {
        labelSky_[ix]  = Ff::Sprintf("%s (%d/%d)###%s", GNSS_NAMES[ix], used_[ix], visible_[ix], GNSS_NAMES[ix]);
        labelList_[ix] = Ff::Sprintf("%s (%d/%d)###%s", GNSS_NAMES[ix], used_[ix], num_[ix],     GNSS_NAMES[ix]);
    }
}

void GuiWinDataSatellites::Count::Add(const Sat &sat)
{
    num_[GnssIx::ALL]++;
    num_[sat.gnssIx_]++;
    if ( (sat.sigL1_.valid && (sat.sigL1_.crUsed || sat.sigL1_.prUsed || sat.sigL1_.doUsed)) ||
         (sat.sigL2_.valid && (sat.sigL2_.crUsed || sat.sigL2_.prUsed || sat.sigL2_.doUsed)) )
    {
        used_[GnssIx::ALL]++;
        used_[sat.gnssIx_]++;
    }
    if (sat.visible_)
    {
        visible_[GnssIx::ALL]++;
        visible_[sat.gnssIx_]++;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::_UpdateSatellites()
{
    _table.ClearRows();
    _sats.clear();
    _count.Reset();
    if (!_latestEpoch)
    {
        return;
    }
    for (int ix = 0; ix < _latestEpoch->epoch.numSatellites; ix++)
    {
        _sats.emplace_back(_latestEpoch->epoch.satellites[ix], _latestEpoch->epoch);
    }

    // Update counts, Populate table
    for (auto &sat: _sats)
    {
        _count.Add(sat);

        const uint32_t thisSat = (sat.satInfo_.gnss << 16) | (sat.satInfo_.sv << 8);
        _table.AddCellText(sat.satInfo_.svStr, thisSat);

        if (sat.satInfo_.orbUsed > EPOCH_SATORB_NONE)
        {
            _table.AddCellTextF("%d", sat.satInfo_.azim);
            _table.SetCellSort(sat.satInfo_.azim);
            _table.AddCellTextF("%d", sat.satInfo_.elev);
            _table.SetCellSort(sat.satInfo_.elev + 100);
        }
        else
        {
            _table.AddCellText("-", 1);
            _table.AddCellText("-", 1);
        }
        _table.AddCellTextF("%-5s %c%c%c%c", sat.satInfo_.orbUsedStr,
            CHKBITS(sat.satInfo_.orbAvail, BIT(EPOCH_SATORB_ALM))   ? 'A' : '-',
            CHKBITS(sat.satInfo_.orbAvail, BIT(EPOCH_SATORB_EPH))   ? 'E' : '-',
            CHKBITS(sat.satInfo_.orbAvail, BIT(EPOCH_SATORB_PRED))  ? 'P' : '-',
            CHKBITS(sat.satInfo_.orbAvail, BIT(EPOCH_SATORB_OTHER)) ? 'O' : '-');
        _table.SetCellSort(sat.satInfo_.orbUsed);
        if ( sat.sigL1_.valid && (sat.sigL1_.use > EPOCH_SIGUSE_SEARCH) )
        {
            _table.AddCellCb(&GuiWinDataSignals::DrawSignalLevelCb, &sat.sigL1_);
            _table.SetCellSort(sat.sigL1_.cno * 100);
        }
        else
        {
            _table.AddCellText("-", 1);
        }
        if ( sat.sigL2_.valid && (sat.sigL2_.use > EPOCH_SIGUSE_SEARCH) )
        {
            _table.AddCellCb(&GuiWinDataSignals::DrawSignalLevelCb, &sat.sigL2_);
            _table.SetCellSort(sat.sigL2_.cno * 100);
        }
        else
        {
            _table.AddCellText("-", 1);
        }

        _table.SetRowFilter(sat.satInfo_.gnss);
        if ( (sat.sigL1_.valid && sat.sigL1_.anyUsed) || (sat.sigL1_.valid && sat.sigL1_.anyUsed) )
        {
            _table.SetRowColour(GUI_COLOUR(SIGNAL_USED_TEXT));
        }
        _table.SetRowUid(thisSat);
    }

    // Update tab labels
    _count.Update();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::_DrawContent()
{
    bool doSky = false;
    bool doList = false;
    GnssIx gnssIx = GnssIx::ALL;

    if (_tabbar1.Begin())
    {
        if (_tabbar1.Item("Sky"))  { doSky = true; };
        if (_tabbar1.Item("List")) { doList = true; };
        _tabbar1.End();
    }

    Gui::VerticalSeparator();

    if (_tabbar2.Begin())
    {
        for (GnssIx ix: GNSS_IXS)
        {
            if (_tabbar2.Item(doSky ? _count.labelSky_[ix] : _count.labelList_[ix]))
            {
                gnssIx = ix;
            }
        }
        _tabbar2.End();
    }

    if (doSky)
    {
        _DrawSky(gnssIx);
    }
    if (doList)
    {
        _DrawList(gnssIx);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::_DrawSky(const GnssIx gnssIx)
{
    ImGui::BeginChild("##Plot", ImVec2(0.0f, 0.0f), false,
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Canvas
    const FfVec2f canvasOffs = ImGui::GetCursorScreenPos();
    const FfVec2f canvasSize = ImGui::GetContentRegionAvail();
    const FfVec2f canvasCent {
        canvasOffs.x + std::floor(canvasSize.x * 0.5f), canvasOffs.y + std::floor(canvasSize.y * 0.5f) };

    ImDrawList *draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(canvasOffs, canvasOffs + canvasSize);

    // const ImVec2 cursorOrigin = ImGui::GetCursorPos();
    // const ImVec2 cursorMax = ImGui::GetWindowContentRegionMax();

    const float radiusPx = (MAX(canvasSize.x, canvasSize.y) / 2.0f) - (2 * GuiSettings::charSize.x);

    // Draw grid
    {
        const int step = 30;
        const float f = 0.707106781f; // cos(pi/4)
        for (int elev = 0; elev < 90; elev += step)
        {
            const float r1 = radiusPx * (1.0f - (float)elev/90.f);
            draw->AddCircle(canvasCent, r1, GUI_COLOUR(PLOT_GRID_MAJOR), 0);
            const float r2 = radiusPx * (1.0f - (float)(elev + (step/2))/90.f);
            draw->AddCircle(canvasCent, r2, GUI_COLOUR(PLOT_GRID_MINOR), 0);
        }
        draw->AddLine(canvasCent - FfVec2f(radiusPx, 0), canvasCent + FfVec2f(radiusPx, 0), GUI_COLOUR(PLOT_GRID_MAJOR));
        draw->AddLine(canvasCent - FfVec2f(0, radiusPx), canvasCent + FfVec2f(0, radiusPx), GUI_COLOUR(PLOT_GRID_MAJOR));
        draw->AddLine(canvasCent - FfVec2f(f * radiusPx, f * radiusPx), canvasCent + FfVec2f(f * radiusPx, f * radiusPx), GUI_COLOUR(PLOT_GRID_MINOR));
        draw->AddLine(canvasCent - FfVec2f(f * radiusPx, -f * radiusPx), canvasCent + FfVec2f(f * radiusPx, f * -radiusPx), GUI_COLOUR(PLOT_GRID_MINOR));
    }

    // Draw satellites
    {
        const FfVec2f textOffs(-1.5f * GuiSettings::charSize.x, -0.5f * GuiSettings::charSize.y);
        const float svR = 2.0f * GuiSettings::charSize.x;
        const FfVec2f barOffs1(-svR, (0.5f * GuiSettings::charSize.y) );
        const FfVec2f barOffs2(-svR, (0.5f * GuiSettings::charSize.y) + 3 + 2);
        const FfVec2f buttonSize(2 * svR, 2 * svR);
        const FfVec2f buttonOffs(-svR, -svR);
        const float barScale = svR / 25.0f;
        for (const auto &sat: _sats)
        {
            if ( !sat.visible_ || ( (gnssIx != GnssIx::ALL) && (sat.gnssIx_ != gnssIx) ) )
            {
                continue;
            }

            // Circle
            const FfVec2f svPos = canvasCent + (sat.xy_ * radiusPx);
            draw->AddCircleFilled(svPos, svR, GUI_COLOUR(SKY_VIEW_SAT));

            // Tooltip
            ImGui::SetCursorScreenPos(svPos + buttonOffs);
            ImGui::InvisibleButton(sat.satInfo_.svStr, buttonSize);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s: azim %d, elev %d, orb %s, L1 %.0f %s, L2 %.0f %s",
                    sat.satInfo_.svStr, sat.satInfo_.azim, sat.satInfo_.elev, sat.satInfo_.orbUsedStr,
                    sat.sigL1_.valid ? sat.sigL1_.cno : 0.0, sat.sigL1_.valid ? sat.sigL1_.signalStr : "",
                    sat.sigL2_.valid ? sat.sigL2_.cno : 0.0, sat.sigL2_.valid ? sat.sigL2_.signalStr : "");
                ImGui::EndTooltip();
            }

            // Signal level bars
            if (sat.sigL1_.valid && (sat.sigL1_.use > EPOCH_SIGUSE_NONE))
            {
                const FfVec2f barPos = svPos + barOffs1;
                draw->AddRectFilled(barPos, barPos + FfVec2f(sat.sigL1_.cno * barScale, 3),
                    sat.sigL1_.prUsed || sat.sigL1_.crUsed || sat.sigL1_.doUsed ? GuiSettings::CnoColour(sat.sigL1_.cno) : GUI_COLOUR(SIGNAL_UNUSED));
            }
            if (sat.sigL2_.valid && (sat.sigL2_.use > EPOCH_SIGUSE_NONE))
            {
                const FfVec2f barPos = svPos + barOffs2;
                draw->AddRectFilled(barPos, barPos + FfVec2f(sat.sigL2_.cno * barScale, 3),
                    sat.sigL2_.prUsed || sat.sigL2_.crUsed || sat.sigL2_.doUsed ? GuiSettings::CnoColour(sat.sigL2_.cno) : GUI_COLOUR(SIGNAL_UNUSED));
            }

            // Label
            ImGui::SetCursorScreenPos(svPos + textOffs);
            ImGui::TextUnformatted(sat.satInfo_.svStr);
        }
    }

    draw->PopClipRect();

    ImGui::EndChild();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::_DrawList(const GnssIx gnssIx)
{
    _table.SetTableRowFilter(gnssIx != GnssIx::ALL ? gnssIx : 0);
    _table.DrawTable();
}


/* ****************************************************************************************************************** */
