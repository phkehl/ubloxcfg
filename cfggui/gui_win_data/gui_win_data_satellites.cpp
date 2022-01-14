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

#include <cmath>
#include <cstring>

#include "gui_inc.hpp"

#include "gui_win_data_signals.hpp"
#include "gui_win_data_satellites.hpp"

/* ****************************************************************************************************************** */

GuiWinDataSatellites::GuiWinDataSatellites(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database),
    _countAll  { "All" },
    _countGps  { "GPS" },
    _countGlo  { "GLO" },
    _countBds  { "BDS" },
    _countGal  { "GAL" },
    _countSbas { "SBAS" },
    _countQzss { "QZSS" },
    _selSats   { }
{
    _winSize = { 80, 25 };

    ClearData();

    _table.AddColumn("SV");
    _table.AddColumn("Azim", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("Elev", 0.0f, GuiWidgetTable::ColumnFlags::ALIGN_RIGHT);
    _table.AddColumn("Orbit");
    _table.AddColumn("Signal L1");
    _table.AddColumn("Signal L2");
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

GuiWinDataSatellites::SatInfo::SatInfo(const EPOCH_SATINFO_t *_satInfo)
    : satInfo{*_satInfo}, sigL1{}, sigL2{}
{
    visible = (satInfo.orbUsed > EPOCH_SATORB_NONE) && (satInfo.elev >= 0);
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataSatellites::Count::Count(const char *_name)
    : num{0}, used{0}, visible{0}, labelSky{}, labelList{}
{
    std::strncpy(name, _name, sizeof(name) - 1);
};

void GuiWinDataSatellites::Count::Reset()
{
    num = 0;
    used = 0;
    visible = 0;
    std::strncpy(labelSky, name, sizeof(labelSky) - 1);
    std::strncpy(labelList, name, sizeof(labelList) - 1);
}

void GuiWinDataSatellites::Count::Update()
{
    std::snprintf(labelSky, sizeof(labelSky), "%s (%d/%d)###%s", name, used, visible, name);
    std::snprintf(labelList, sizeof(labelList), "%s (%d/%d)###%s", name, used, num, name);
}

void GuiWinDataSatellites::Count::Add(const SatInfo &sat)
{
    num++;
    if ( (sat.sigL1.valid && (sat.sigL1.crUsed || sat.sigL1.prUsed || sat.sigL1.doUsed)) ||
         (sat.sigL2.valid && (sat.sigL2.crUsed || sat.sigL2.prUsed || sat.sigL2.doUsed)) )
    {
        used++;
    }
    if (sat.visible)
    {
        visible++;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::_UpdateSatellites()
{
    _satInfo.clear();
    _countAll.Reset();
    _countGps.Reset();
    _countGlo.Reset();
    _countBds.Reset();
    _countGal.Reset();
    _countSbas.Reset();
    _countQzss.Reset();
    if (!_latestEpoch)
    {
        return;
    }
    for (int ix = 0; ix < _latestEpoch->epoch.numSatellites; ix++)
    {
        SatInfo info(&_latestEpoch->epoch.satellites[ix]);
        const double a = (360.0 + 90 - (float)info.satInfo.azim) * (M_PI / 180.0);
        info.dX = std::cos(a);
        info.dY = -std::sin(a);
        info.fR = 1.0f - ((float)info.satInfo.elev * (1.0f/90.f));
        _satInfo.push_back(info);
    }
    // Add signal levels
    for (int ix = 0; ix < _latestEpoch->epoch.numSignals; ix++)
    {
        const EPOCH_SIGINFO_t *sig = &_latestEpoch->epoch.signals[ix];
        for (auto &sat: _satInfo)
        {
            if ( (sat.satInfo.sv == sig->sv) && (sat.satInfo.gnss == sig->gnss) )
            {
                switch (sig->band)
                {
                    case EPOCH_BAND_L1: sat.sigL1 = *sig; break;
                    case EPOCH_BAND_L2: sat.sigL2 = *sig; break;
                    default: break;
                }
                break;
            }
        }
    }

    // Counts
    for (auto &sat: _satInfo)
    {
        _countAll.Add(sat);
        switch (sat.satInfo.gnss)
        {
            case EPOCH_GNSS_GPS:  _countGps.Add(sat);  break;
            case EPOCH_GNSS_GLO:  _countGlo.Add(sat);  break;
            case EPOCH_GNSS_BDS:  _countBds.Add(sat);  break;
            case EPOCH_GNSS_GAL:  _countGal.Add(sat);  break;
            case EPOCH_GNSS_SBAS: _countSbas.Add(sat); break;
            case EPOCH_GNSS_QZSS: _countQzss.Add(sat); break;
            case EPOCH_GNSS_UNKNOWN: break;
        }
    }

    // Tab labels
    _countAll.Update();
    _countGps.Update();
    _countGlo.Update();
    _countBds.Update();
    _countGal.Update();
    _countSbas.Update();
    _countQzss.Update();

    // Populate table
    _table.ClearRows();
    for (auto &sat: _satInfo)
    {
        _table.AddCellText(sat.satInfo.svStr);

        if (sat.satInfo.orbUsed > EPOCH_SATORB_NONE)
        {
            _table.AddCellTextF("%d", sat.satInfo.azim);
            _table.AddCellTextF("%d", sat.satInfo.elev);
        }
        else
        {
            _table.AddCellText("-");
            _table.AddCellText("-");
        }
        _table.AddCellTextF("%-5s %c%c%c%c", sat.satInfo.orbUsedStr,
            CHKBITS(sat.satInfo.orbAvail, BIT(EPOCH_SATORB_ALM))   ? 'A' : '-',
            CHKBITS(sat.satInfo.orbAvail, BIT(EPOCH_SATORB_EPH))   ? 'E' : '-',
            CHKBITS(sat.satInfo.orbAvail, BIT(EPOCH_SATORB_PRED))  ? 'P' : '-',
            CHKBITS(sat.satInfo.orbAvail, BIT(EPOCH_SATORB_OTHER)) ? 'O' : '-');

        if ( sat.sigL1.valid && (sat.sigL1.use > EPOCH_SIGUSE_SEARCH) )
        {
            _table.AddCellCb(std::bind(&GuiWinDataSatellites::_DrawSignalLevelCb, this, std::placeholders::_1), &sat.sigL1);
        }
        else
        {
            _table.AddCellText("-");
        }
        if ( sat.sigL2.valid && (sat.sigL2.use > EPOCH_SIGUSE_SEARCH) )
        {
            _table.AddCellCb(std::bind(&GuiWinDataSatellites::_DrawSignalLevelCb, this, std::placeholders::_1), &sat.sigL2);
        }
        else
        {
            _table.AddCellText("-");
        }

        _table.SetRowFilter(sat.satInfo.gnss);
        if ( (sat.sigL1.valid && sat.sigL1.anyUsed) || (sat.sigL1.valid && sat.sigL1.anyUsed) )
        {
            _table.SetRowColour(GUI_COLOUR(SIGNAL_USED_TEXT));
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::_DrawContent()
{
    bool doSky = false;
    bool doList = false;
    EPOCH_GNSS_t filter = EPOCH_GNSS_UNKNOWN;

    if (ImGui::BeginTabBar("##tabs1", ImGuiTabBarFlags_FittingPolicyScroll))
    {
        if (ImGui::BeginTabItem("Sky"))
        {
            doSky = true;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("List"))
        {
            doList = true;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    Gui::VerticalSeparator();

    if (ImGui::BeginTabBar("##tabs2", ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_TabListPopupButton/*ImGuiTabBarFlags_FittingPolicyScroll*/))
    {
        if (ImGui::BeginTabItem(doSky ? _countAll.labelSky  : _countAll.labelList )) { filter = EPOCH_GNSS_UNKNOWN; ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem(doSky ? _countGps.labelSky  : _countGps.labelList )) { filter = EPOCH_GNSS_GPS;     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem(doSky ? _countGlo.labelSky  : _countGlo.labelList )) { filter = EPOCH_GNSS_GLO;     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem(doSky ? _countGal.labelSky  : _countGal.labelList )) { filter = EPOCH_GNSS_GAL;     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem(doSky ? _countBds.labelSky  : _countBds.labelList )) { filter = EPOCH_GNSS_BDS;     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem(doSky ? _countSbas.labelSky : _countSbas.labelList)) { filter = EPOCH_GNSS_SBAS;    ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem(doSky ? _countQzss.labelSky : _countQzss.labelList)) { filter = EPOCH_GNSS_QZSS;    ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }

    if (doSky)
    {
        _DrawSky(filter);
    }
    if (doList)
    {
        _DrawList(filter);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::_DrawSky(const EPOCH_GNSS_t filter)
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
        for (const auto &sat: _satInfo)
        {
            if ( !sat.visible || ( (filter != EPOCH_GNSS_UNKNOWN) && (sat.satInfo.gnss != filter) ) )
            {
                continue;
            }

            // Circle
            const float r = sat.fR * radiusPx;
            const FfVec2f svPos = canvasCent + FfVec2f(r * sat.dX, r * sat.dY);
            draw->AddCircleFilled(svPos, svR, GUI_COLOUR(SKY_VIEW_SAT));

            // Tooltip
            ImGui::SetCursorScreenPos(svPos + buttonOffs);
            ImGui::InvisibleButton(sat.satInfo.svStr, buttonSize);
            if (Gui::ItemTooltipBegin())
            {
                ImGui::Text("%s: azim %d, elev %d, orb %s, L1 %.0f %s, L2 %.0f %s",
                    sat.satInfo.svStr, sat.satInfo.azim, sat.satInfo.elev, sat.satInfo.orbUsedStr,
                    sat.sigL1.valid ? sat.sigL1.cno : 0.0, sat.sigL1.valid ? sat.sigL1.signalStr : "",
                    sat.sigL2.valid ? sat.sigL2.cno : 0.0, sat.sigL2.valid ? sat.sigL2.signalStr : "");
                Gui::ItemTooltipEnd();
            }

            // Signal level bars
            if (sat.sigL1.valid && (sat.sigL1.use > EPOCH_SIGUSE_NONE))
            {
                const FfVec2f barPos = svPos + barOffs1;
                const int colIx = sat.sigL1.cno > 55 ? (EPOCH_SIGCNOHIST_NUM - 1) : (sat.sigL1.cno > 0 ? (sat.sigL1.cno / 5) : 0 );
                draw->AddRectFilled(barPos, barPos + FfVec2f(sat.sigL1.cno * barScale, 3),
                    sat.sigL1.prUsed || sat.sigL1.crUsed || sat.sigL1.doUsed ? GUI_COLOUR(SIGNAL_00_05 + colIx) : GUI_COLOUR(SIGNAL_UNUSED));
            }
            if (sat.sigL2.valid && (sat.sigL2.use > EPOCH_SIGUSE_NONE))
            {
                const FfVec2f barPos = svPos + barOffs2;
                const int colIx = sat.sigL2.cno > 55 ? (EPOCH_SIGCNOHIST_NUM - 1) : (sat.sigL2.cno > 0 ? (sat.sigL2.cno / 5) : 0 );
                draw->AddRectFilled(barPos, barPos + FfVec2f(sat.sigL2.cno * barScale, 3),
                    sat.sigL2.prUsed || sat.sigL2.crUsed || sat.sigL2.doUsed ? GUI_COLOUR(SIGNAL_00_05 + colIx) : GUI_COLOUR(SIGNAL_UNUSED));
            }

            // Label
            ImGui::SetCursorScreenPos(svPos + textOffs);
            ImGui::TextUnformatted(sat.satInfo.svStr);
        }
    }

    draw->PopClipRect();

    ImGui::EndChild();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::_DrawList(const EPOCH_GNSS_t filter)
{
    _table.SetTableRowFilter(filter != EPOCH_GNSS_UNKNOWN ? filter : 0);
    _table.DrawTable();
}

void GuiWinDataSatellites::_DrawSignalLevelCb(void *arg) // same as GuiWinDataSignals::_DrawSignalLevel() !
{
    GuiWinDataSignals::DrawSignalLevelWithBar((const EPOCH_SIGINFO_t *)arg, GuiSettings::charSize);
}

/* ****************************************************************************************************************** */
