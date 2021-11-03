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

#include "gui_win_data_satellites.hpp"

#include "gui_win_data_inc.hpp"

/* ****************************************************************************************************************** */

GuiWinDataSatellites::GuiWinDataSatellites(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database),
    _countAll{"All"}, _countGps{"GPS"}, _countGlo{"GLO"}, _countBds{"BDS"}, _countGal{"GAL"}, _countSbas{"SBAS"}, _countQzss{"QZSS"},
    _selSats{}
{
    _winSize = { 80, 25 };

    ClearData();

    const float em = _winSettings->charSize.x;
    _table.AddColumn("SV", 5 * em);
    _table.AddColumn("Azim", 5 * em);
    _table.AddColumn("Elev", 5 * em);
    _table.AddColumn("Orbit", 11 * em);
    _table.AddColumn("Signal L1", 24 * em);
    _table.AddColumn("Signal L2", 24 * em);
}

// ---------------------------------------------------------------------------------------------------------------------

//void GuiWinDataSatellites::Loop(const std::unique_ptr<Receiver> &receiver)
//{
//}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::ProcessData(const Data &data)
{
    switch (data.type)
    {
        case Data::Type::DATA_EPOCH:
            if (data.epoch->epoch.numSatellites > 0)
            {
                _epoch = data.epoch;
                _epochTs = ImGui::GetTime();
            }
            _UpdateSatellites();
            // TODO: drop epoch if satellites info gets too old
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::Loop(const uint32_t &frame, const double &now)
{
    (void)frame;
    // Expire data
    if (_epoch && !_receiver->IsIdle())
    {
        _epochAge = now - _epochTs;
        if (_epochAge > 5.0)
        {
            ClearData();
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::ClearData()
{
    _epoch = nullptr;
    _UpdateSatellites();
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataSatellites::SatInfo::SatInfo(const EPOCH_SATINFO_t *_satInfo)
    : satInfo{_satInfo}, sigL1{nullptr}, sigL2{nullptr}
{
    visible = (satInfo->orbUsed > EPOCH_SATORB_NONE) && (satInfo->elev >= 0);
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
    if ( (sat.sigL1 && (sat.sigL1->crUsed || sat.sigL1->prUsed || sat.sigL1->doUsed)) ||
         (sat.sigL2 && (sat.sigL2->crUsed || sat.sigL2->prUsed || sat.sigL2->doUsed)) )
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
    if (!_epoch)
    {
        return;
    }
    for (int ix = 0; ix < _epoch->epoch.numSatellites; ix++)
    {
        SatInfo info(&_epoch->epoch.satellites[ix]);
        const double a = (360.0 + 90 - (float)info.satInfo->azim) * (M_PI / 180.0);
        info.dX = std::cos(a);
        info.dY = -std::sin(a);
        info.fR = 1.0f - ((float)info.satInfo->elev * (1.0f/90.f));
        _satInfo.push_back(info);
    }
    // Add signal levels
    for (int ix = 0; ix < _epoch->epoch.numSignals; ix++)
    {
        const EPOCH_SIGINFO_t *sig = &_epoch->epoch.signals[ix];
        for (auto &sat: _satInfo)
        {
            if ( (sat.satInfo->sv == sig->sv) && (sat.satInfo->gnss == sig->gnss) )
            {
                switch (sig->band)
                {
                    case EPOCH_BAND_L1: sat.sigL1 = sig; break;
                    case EPOCH_BAND_L2: sat.sigL2 = sig; break;
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
        switch (sat.satInfo->gnss)
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
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    // Controls
    {
        // Clear
        if (ImGui::Button(ICON_FK_ERASER "##Clear", _winSettings->iconButtonSize))
        {
            ClearData();
        }
        Gui::ItemTooltip("Clear all data");

        // Age
        if (_epoch && !_receiver->IsIdle())
        {
            ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - (3 * _winSettings->charSize.x) );
            ImGui::Text("%.1f", _epochAge);
        }
    }

    ImGui::Separator();

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

    if (ImGui::BeginTabBar("##tabs2", ImGuiTabBarFlags_FittingPolicyScroll /*ImGuiTabBarFlags_FittingPolicyResizeDown*/))
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

    _DrawWindowEnd();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::_DrawSky(const EPOCH_GNSS_t filter)
{
    ImGui::BeginChild("##Plot", ImVec2(0.0f, 0.0f), false,
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Canvas
    const ImVec2 canvasOffs = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    const ImVec2 canvasCent = ImVec2(
        canvasOffs.x + std::floor(canvasSize.x * 0.5), canvasOffs.y + std::floor(canvasSize.y * 0.5));

    ImDrawList *draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(canvasOffs, canvasOffs + canvasSize);

    // const ImVec2 cursorOrigin = ImGui::GetCursorPos();
    // const ImVec2 cursorMax = ImGui::GetWindowContentRegionMax();

    const float radiusPx = (MAX(canvasSize.x, canvasSize.y) / 2.0f) - (2 * _winSettings->charSize.x);

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
        draw->AddLine(canvasCent - ImVec2(radiusPx, 0), canvasCent + ImVec2(radiusPx, 0), GUI_COLOUR(PLOT_GRID_MAJOR));
        draw->AddLine(canvasCent - ImVec2(0, radiusPx), canvasCent + ImVec2(0, radiusPx), GUI_COLOUR(PLOT_GRID_MAJOR));
        draw->AddLine(canvasCent - ImVec2(f * radiusPx, f * radiusPx), canvasCent + ImVec2(f * radiusPx, f * radiusPx), GUI_COLOUR(PLOT_GRID_MINOR));
        draw->AddLine(canvasCent - ImVec2(f * radiusPx, -f * radiusPx), canvasCent + ImVec2(f * radiusPx, f * -radiusPx), GUI_COLOUR(PLOT_GRID_MINOR));
    }

    // Draw satellites
    {
        const ImVec2 textOffs(-1.5f * _winSettings->charSize.x, -0.5f * _winSettings->charSize.y);
        const float svR = 2.0f * _winSettings->charSize.x;
        const ImVec2 barOffs1(-svR, (0.5f * _winSettings->charSize.y) );
        const ImVec2 barOffs2(-svR, (0.5f * _winSettings->charSize.y) + 3 + 2);
        const ImVec2 buttonSize(2 * svR, 2 * svR);
        const ImVec2 buttonOffs(-svR, -svR);
        const float barScale = svR / 25.0f;
        for (const auto &sat: _satInfo)
        {
            if ( !sat.visible || ( (filter != EPOCH_GNSS_UNKNOWN) && (sat.satInfo->gnss != filter) ) )
            {
                continue;
            }

            // Circle
            const float r = sat.fR * radiusPx;
            const ImVec2 svPos = canvasCent + ImVec2(r * sat.dX, r * sat.dY);
            draw->AddCircleFilled(svPos, svR, GUI_COLOUR(SKY_VIEW_SAT));

            // Tooltip
            ImGui::SetCursorScreenPos(svPos + buttonOffs);
            ImGui::InvisibleButton(sat.satInfo->svStr, buttonSize);
            if (Gui::ItemTooltipBegin())
            {
                ImGui::Text("%s: azim %d, elev %d, orb %s, L1 %.0f %s, L2 %.0f %s",
                    sat.satInfo->svStr, sat.satInfo->azim, sat.satInfo->elev, sat.satInfo->orbUsedStr,
                    sat.sigL1 ? sat.sigL1->cno : 0.0, sat.sigL1 ? sat.sigL1->signalStr : "",
                    sat.sigL2 ? sat.sigL2->cno : 0.0, sat.sigL2 ? sat.sigL2->signalStr : "");
                Gui::ItemTooltipEnd();
            }

            // Signal level bars
            if (sat.sigL1 && (sat.sigL1->use > EPOCH_SIGUSE_NONE))
            {
                const ImVec2 barPos = svPos + barOffs1;
                const int colIx = sat.sigL1->cno > 55 ? (EPOCH_SIGCNOHIST_NUM - 1) : (sat.sigL1->cno > 0 ? (sat.sigL1->cno / 5) : 0 );
                draw->AddRectFilled(barPos, barPos + ImVec2(sat.sigL1->cno * barScale, 3),
                    sat.sigL1->prUsed || sat.sigL1->crUsed || sat.sigL1->doUsed ? GUI_COLOUR(SIGNAL_00_05 + colIx) : GUI_COLOUR(SIGNAL_UNUSED));
            }
            if (sat.sigL2 && (sat.sigL2->use > EPOCH_SIGUSE_NONE))
            {
                const ImVec2 barPos = svPos + barOffs2;
                const int colIx = sat.sigL2->cno > 55 ? (EPOCH_SIGCNOHIST_NUM - 1) : (sat.sigL2->cno > 0 ? (sat.sigL2->cno / 5) : 0 );
                draw->AddRectFilled(barPos, barPos + ImVec2(sat.sigL2->cno * barScale, 3),
                    sat.sigL2->prUsed || sat.sigL2->crUsed || sat.sigL2->doUsed ? GUI_COLOUR(SIGNAL_00_05 + colIx) : GUI_COLOUR(SIGNAL_UNUSED));
            }

            // Label
            ImGui::SetCursorScreenPos(svPos + textOffs);
            ImGui::TextUnformatted(sat.satInfo->svStr);
        }
    }

    draw->PopClipRect();

    ImGui::EndChild();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSatellites::_DrawList(const EPOCH_GNSS_t filter)
{
    const float lineHeight = ImGui::GetTextLineHeight();
    const ImVec2 barOffs(_winSettings->style.ItemSpacing.x + (9 * _winSettings->charSize.x), -_winSettings->style.ItemSpacing.y);

    _table.BeginDraw();

    ImDrawList *draw = ImGui::GetWindowDrawList();

    for (auto &sat: _satInfo)
    {
        if ( (filter != EPOCH_GNSS_UNKNOWN) && (sat.satInfo->gnss != filter) )
        {
            continue;
        }

        const auto L1 = sat.sigL1;
        const auto L2 = sat.sigL2;
        const bool L1used = L1 && (L1->use > EPOCH_SIGUSE_SEARCH) && (L1->prUsed || L1->crUsed || L1->doUsed);
        const bool L2used = L2 && (L2->use > EPOCH_SIGUSE_SEARCH) && (L2->prUsed || L2->crUsed || L2->doUsed);

        if (L1used || L2used)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTGREEN));
        }

        const uint32_t uid = (sat.satInfo->gnss << 8) | sat.satInfo->sv;
        auto selUid = _selSats.find(uid);
        const bool selected = selUid == _selSats.end() ? false : selUid->second;
        if (_table.ColSelectable(sat.satInfo->svStr, selected))
        {
            _selSats[uid] = !selected;
        }

        if (sat.satInfo->orbUsed > EPOCH_SATORB_NONE)
        {
            _table.ColTextF("%3d", sat.satInfo->azim);
            _table.ColTextF("%3d", sat.satInfo->elev);
        }
        else
        {
            _table.ColText("-");
            _table.ColText("-");
        }
        const auto avail = sat.satInfo->orbAvail;
        _table.ColTextF("%-5s %c%c%c%c", sat.satInfo->orbUsedStr,
            (avail & BIT(EPOCH_SATORB_ALM))   == BIT(EPOCH_SATORB_ALM)   ? 'A' : '-',
            (avail & BIT(EPOCH_SATORB_EPH))   == BIT(EPOCH_SATORB_EPH)   ? 'E' : '-',
            (avail & BIT(EPOCH_SATORB_PRED))  == BIT(EPOCH_SATORB_PRED)  ? 'P' : '-',
            (avail & BIT(EPOCH_SATORB_OTHER)) == BIT(EPOCH_SATORB_OTHER) ? 'O' : '-');

        if (L1used)
        {
            //_table.ColTextF("%-4s %.1f", L1->signalStr, L1->cno);
            ImGui::Text("%-4s %4.1f", L1->signalStr, L1->cno);
            const ImVec2 offs = ImGui::GetCursorScreenPos() + barOffs;
            const int colIx = L1->cno > 55 ? (EPOCH_SIGCNOHIST_NUM - 1) : (L1->cno > 0 ? (L1->cno / 5) : 0 );
            draw->AddRectFilled(offs, offs + ImVec2(L1->cno * 2, -lineHeight), L1used ?  GUI_COLOUR(SIGNAL_00_05 + colIx) : GUI_COLOUR(SIGNAL_UNUSED));
            draw->AddLine(offs + ImVec2(42 * 2, 0), offs + ImVec2(42 * 2, -lineHeight), GUI_COLOUR(C_GREY));
            ImGui::NextColumn();
        }
        else
        {
            _table.ColText("-");
        }

        if (L2used)
        {
            //_table.ColTextF("%-4s %.1f", L2->signalStr, L2->cno);
            ImGui::Text("%-4s %4.1f", L2->signalStr, L2->cno);
            const ImVec2 offs = ImGui::GetCursorScreenPos() + barOffs;
            const int colIx = L2->cno > 55 ? (EPOCH_SIGCNOHIST_NUM - 1) : (L2->cno > 0 ? (L2->cno / 5) : 0 );
            draw->AddRectFilled(offs, offs + ImVec2(L2->cno * 2, -lineHeight), L2used ?  GUI_COLOUR(SIGNAL_00_05 + colIx) : GUI_COLOUR(SIGNAL_UNUSED));
            draw->AddLine(offs + ImVec2(42 * 2, 0), offs + ImVec2(42 * 2, -lineHeight), GUI_COLOUR(C_GREY));
            ImGui::NextColumn();
        }
        else
        {
            _table.ColText("-");
        }

        if (L1used || L2used)
        {
            ImGui::PopStyleColor();
        }

        ImGui::Separator();
    }
    _table.EndDraw();
}

/* ****************************************************************************************************************** */
