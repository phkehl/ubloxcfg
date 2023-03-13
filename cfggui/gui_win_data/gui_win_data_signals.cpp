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

#include "gui_inc.hpp"
#include "implot.h"

#include "gui_win_data_signals.hpp"

/* ****************************************************************************************************************** */

GuiWinDataSignals::GuiWinDataSignals(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database),
    _onlyTracked { true },
    _tabbar1     { WinName() + "Tabbar1" },
    _tabbar2     { WinName() + "Tabbar2", ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_TabListPopupButton }
{
    _winSize = { 115, 40 };

    ClearData();

    _table.AddColumn("SV", 0.0f, GuiWidgetTable::ColumnFlags::SORTABLE | GuiWidgetTable::ColumnFlags::HIDE_SAME);
    _table.AddColumn("Signal", 0.0f, GuiWidgetTable::ColumnFlags::SORTABLE);
    _table.AddColumn("Bd.", 0.0f, GuiWidgetTable::ColumnFlags::SORTABLE);
    _table.AddColumn("Level", 0.0f, GuiWidgetTable::ColumnFlags::SORTABLE);
    _table.AddColumn("Use", 0.0f, GuiWidgetTable::ColumnFlags::SORTABLE);
    _table.AddColumn("Iono");
    _table.AddColumn("Health");
    _table.AddColumn("Used");
    _table.AddColumn("PR res.", 0.0f, GuiWidgetTable::ColumnFlags::SORTABLE);
    _table.AddColumn("Corrections");
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSignals::_ProcessData(const InputData &data)
{
    if (data.type == InputData::DATA_EPOCH)
    {
        _UpdateSignals();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSignals::_ClearData()
{
    _UpdateSignals();
    _cnoSky.Reset();
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataSignals::Count::Count()
{
    Reset();
};

void GuiWinDataSignals::Count::Reset()
{
    for (GnssIx ix: GNSS_IXS)
    {
        num_[ix] = 0;
        used_[ix] = 0;
        label_[ix] = Ff::Sprintf("%s###%s", GNSS_NAMES[ix], GNSS_NAMES[ix]);
    }
}

void GuiWinDataSignals::Count::Update()
{
    for (GnssIx ix: GNSS_IXS)
    {
        label_[ix] = Ff::Sprintf("%s (%d/%d)###%s", GNSS_NAMES[ix], used_[ix], num_[ix], GNSS_NAMES[ix]);
    }
}

void GuiWinDataSignals::Count::Add(const EPOCH_SIGINFO_t &sig)
{
    GnssIx ix = OTHER;
    switch (sig.gnss)
    {
        case EPOCH_GNSS_GPS: ix = GPS; break;
        case EPOCH_GNSS_GLO: ix = GLO; break;
        case EPOCH_GNSS_BDS: ix = BDS; break;
        case EPOCH_GNSS_GAL: ix = GAL; break;
        case EPOCH_GNSS_SBAS:
        case EPOCH_GNSS_QZSS:
        case EPOCH_GNSS_UNKNOWN:
            break;
    }
    num_[GnssIx::ALL]++;
    num_[ix]++;
    if ( sig.prUsed || sig.crUsed || sig.doUsed )
    {
        used_[GnssIx::ALL]++;
        used_[ix]++;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataSignals::CnoBin::CnoBin(const float r0, const float r1, const float a0, const float a1) :
    r0_        { r0 },
    r1_        { r1 },
    a0_        { a0 },
    a1_        { a1 },
    uid_       { Ff::Sprintf("%016lx", reinterpret_cast<std::uintptr_t>(this)) }
{
    Reset();
    const float ar = 0.5f * (r0_ + r1_);
    const float aa = 0.5f * (a0_ + a1_);
    xy_.x = ar * std::cos(aa);
    xy_.y = ar * std::sin(aa);
}

void GuiWinDataSignals::CnoBin::Add(const GnssIx ix, const float cno)
{
    if (std::isnan(max_[ix]) || (cno > max_[ix])) { max_[ix] = cno; }
    if (std::isnan(min_[ix]) || (cno < min_[ix])) { min_[ix] = cno; }
    count_[ix]++;
    if (std::isnan(mean_[ix]))
    {
        mean_[ix] = cno;
    }
    else
    {
        mean_[ix] += (cno - mean_[ix]) / (float)count_[ix];
    }
    _tooltip[ix].clear();
}

void GuiWinDataSignals::CnoBin::Reset()
{
    for (GnssIx ix: GNSS_IXS)
    {
        count_[ix] =  0;
        min_[ix]   =  NAN;
        max_[ix]   =  NAN;
        mean_[ix]  =  NAN;
        _tooltip[ix].clear();
    }
}

const std::string &GuiWinDataSignals::CnoBin::Tooltip(const GnssIx gnssIx)
{
    if (_tooltip[gnssIx].empty())
    {
        _tooltip[gnssIx] = Ff::Sprintf("%.1f dBHz (min %.1f, max %.1f, n=%u)",
            mean_[gnssIx], min_[gnssIx], max_[gnssIx], count_[gnssIx]);
    }
    return _tooltip[gnssIx];
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataSignals::CnoSky::CnoSky(const int n_az, const int n_el) :
    n_az_ { std::clamp(n_az, 4, 36) },
    n_el_ { std::clamp(n_el, 4, 18) },
    s_az_ { 360.0f / (float)n_az_ },
    s_el_ {  90.0f / (float)n_el_ }
{
    // Bins arrangement:
    // - clockwise (0 deg = east, 90 deg = south, 180 deg = west, 270 deg = north
    // - from out (0 deg elevation) to in (90 deg elevation)
    //                    (270)                         .
    //                                                  .
    //                , - ~ ~ ~ - ,                     .
    //            , '   \ __4__ /    ' ,                .
    //          ,      , \     / ',  5  ,               .
    //         ,   3 ,    \ 10/    '     ,              .
    //        ,     ,   9  \ / 11   r1    r0            .
    // (180)  ,-------------o--------,----,--- a0   (0) .
    //        ,     ,   8  / \  6   ,#####,             .
    //         ,     ',   / 7 \    ,##0##,              .
    //          ,   2  ' /,___,\ ,######,               .
    //            ,     /   1   \####, '                .
    //              ' - , _ _ _ ,\ '         a = angle  .
    //                            \          r = radius .
    //                             a1                   .
    //                    (90)                          .

    // Calculate bins and grid
    // - Elevation circles
    for (int el = 0; el < n_el_; el++)
    {
        const float r0 = (1.0f - ((el       * (90.0f / (float)n_el_) / 90.0f)));
        const float r1 = (1.0f - (((el + 1) * (90.0f / (float)n_el_) / 90.0f)));
        rs_.push_back(r0); // Radii for grid circles

        // - Azimuth lines
        for (int az = 0; az < n_az_; az++)
        {
            const float a0 = (float)az       * (360.0f / (float)n_az_) * (M_PI / 180.f);
            const float a1 = (float)(az + 1) * (360.0f / (float)n_az_) * (M_PI / 180.f);
            if (el == 0) { axys_.emplace_back(std::cos(a0), std::sin(a0)); } // x/y (from angles) for grid lines

            // Add bin
            bins_.emplace_back(r0, r1, a0, a1);
        }
    }
}

void GuiWinDataSignals::CnoSky::Reset()
{
    for (auto &bin: bins_)
    {
        bin.Reset();
    }
}

std::size_t GuiWinDataSignals::CnoSky::AzimElevToBin(const int azim, const int elev) const
{
    // Determine the bin this should go into
    const int elevIx = elev / s_el_; // "circle"
    const int azimIx = ((azim + 270) % 360) / s_az_; // "sector"
    return (elevIx * n_az_) + azimIx;
}

void GuiWinDataSignals::CnoSky::AddCno(const EPOCH_SIGINFO_t &sig, const EPOCH_SATINFO_t &sat)
{
    // Skip unusable data
    if ( (sat.orbUsed <= EPOCH_SATORB_NONE) || (sat.azim < 0) || (sat.azim > 360) || (sat.elev < 0) || (sat.elev > 90) || (sig.cno < 0.1f) )
    {
        return;
    }

    GnssIx gnssIx = OTHER;
    switch (sig.gnss)
    {
        case EPOCH_GNSS_GPS: gnssIx = GPS; break;
        case EPOCH_GNSS_GLO: gnssIx = GLO; break;
        case EPOCH_GNSS_BDS: gnssIx = BDS; break;
        case EPOCH_GNSS_GAL: gnssIx = GAL; break;
        case EPOCH_GNSS_SBAS:
        case EPOCH_GNSS_QZSS:
        case EPOCH_GNSS_UNKNOWN:
            break;
    }

    // const int elevIx = sat.elev / s_el_; // "circle"
    // const int azimIx = ((sat.azim + 270) % 360) / s_az_; // "sector"
    // const std::size_t binIx = (elevIx * n_az_) + azimIx;
    const std::size_t binIx = AzimElevToBin(sat.azim, sat.elev);

    // Update statistics
    if (binIx < bins_.size())
    {
        bins_[binIx].Add(gnssIx, sig.cno);
        bins_[binIx].Add(GnssIx::ALL, sig.cno);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSignals::_UpdateSignals()
{
    _table.ClearRows();
    _count.Reset();
    if (!_latestEpoch)
    {
        return;
    }

    // Populate full list of signals, considering filters
    const int numSig = _latestEpoch->epoch.numSignals;
    for (int ix = 0; ix < numSig; ix++)
    {
        const EPOCH_SIGINFO_t &sig = _latestEpoch->epoch.signals[ix];

        // Skip untracked signals in table
        if (_onlyTracked && (sig.use < EPOCH_SIGUSE_ACQUIRED))
        {
            continue;
        }

        // Populate table
        const uint32_t thisSat = (sig.gnss << 16) | (sig.sv << 8); // <use><gnss><sat><sig>
        const uint32_t rowUid = thisSat | sig.signal;
        const uint32_t rowSort = rowUid | (sig.use > EPOCH_SIGUSE_SEARCH ? 0x01000000 : 0xff000000);// default sort key: sort by gnss/sat/sig, and used signals first, searching signals at the end

        _table.AddCellText(sig.svStr, thisSat);
        _table.AddCellText(sig.signalStr, sig.signal);
        _table.AddCellText(sig.bandStr, sig.band);
        if ( sig.valid && (sig.use > EPOCH_SIGUSE_SEARCH) )
        {
            _table.AddCellCb(&GuiWinDataSignals::DrawSignalLevelCb, (void * /* nasty! :-( */)&sig);
        }
        else
        {
            _table.AddCellText("-");
        }
        _table.SetCellSort(sig.valid ? 1 + (sig.cno * 100) : 0xffffffff);

        _table.AddCellText(sig.useStr, sig.use);
        _table.AddCellText(sig.ionoStr);
        _table.AddCellText(sig.healthStr);
        _table.AddCellTextF("%s %s %s",
            sig.prUsed ? "PR" : "--",
            sig.crUsed ? "CR" : "--",
            sig.doUsed ? "DO" : "--");

        const uint32_t prResSort = (sig.prUsed ? std::abs(sig.prRes) * 1000.0f : 9999999);
        if (sig.prUsed /*sig->prRes != 0.0*/)
        {
            _table.AddCellTextF("%6.1f", sig.prRes);
            _table.SetCellSort(prResSort);
        }
        else
        {
            _table.AddCellText("-", prResSort);
        }
        _table.AddCellTextF("%-9s %s %s %s",
            sig.corrStr,
            sig.prCorrUsed ? "PR" : "--",
            sig.crCorrUsed ? "CR" : "--",
            sig.doCorrUsed ? "DO" : "--");

        if (sig.use <= EPOCH_SIGUSE_SEARCH)
        {
            _table.SetRowColour(GUI_COLOUR(SIGNAL_UNUSED_TEXT));
        }
        else if (sig.anyUsed)
        {
            _table.SetRowColour(GUI_COLOUR(SIGNAL_USED_TEXT));
        }

        _table.SetRowSort(rowSort);
        _table.SetRowUid(rowUid);

        _table.SetRowFilter(sig.gnss);

        // Update skyplot
        if (sig.satIx != EPOCH_NO_SV)
        {
            _cnoSky.AddCno(sig, _latestEpoch->epoch.satellites[sig.satIx]);
        }

        // Update counts
        _count.Add(sig);
    }

    // Update tab labels
    _count.Update();
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ void GuiWinDataSignals::DrawSignalLevelCb(void *arg)
{
    const EPOCH_SIGINFO_t *sig = (const EPOCH_SIGINFO_t *)arg;

    ImGui::Text("%-4s %4.1f", sig->signalStr, sig->cno);
    ImGui::SameLine();

    ImDrawList *draw = ImGui::GetWindowDrawList();

    const float barMaxLen = 100.0f; // [px]
    const float barMaxCno = 55.0f;  // [dbHz]
    const FfVec2f bar0 = ImGui::GetCursorScreenPos();
    const FfVec2f barSize = ImVec2(barMaxLen, GuiSettings::charSize.y);
    ImGui::Dummy(barSize);
    const float barLen = sig->cno * barMaxLen / barMaxCno;
    const FfVec2f bar1 = bar0 + FfVec2f(barLen, barSize.y);
    draw->AddRectFilled(bar0, bar1, sig->anyUsed ? GuiSettings::CnoColour(sig->cno) : GUI_COLOUR(SIGNAL_UNUSED));
    const float offs42 = 42.0 * barMaxLen / barMaxCno;
    draw->AddLine(bar0 + FfVec2f(offs42, 0), bar0 + FfVec2f(offs42, barSize.y), GUI_COLOUR(C_GREY));
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSignals::_DrawToolbar()
{
    ImGui::SameLine();


    if (Gui::ToggleButton(ICON_FK_CIRCLE_O "###OnlyTracked", ICON_FK_DOT_CIRCLE_O "###OnlyTracked", &_onlyTracked,
        "Showing only tracked signals", "Showing all signals", GuiSettings::iconSize))
    {
        _UpdateSignals();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSignals::_DrawContent()
{
    bool doList = false;
    bool doSky = false;
    GnssIx gnssIx = GnssIx::ALL;

    if (_tabbar1.Begin())
    {
        if (_tabbar1.Item("List")) { doList = true; };
        if (_tabbar1.Item("Sky"))  { doSky = true; };
        _tabbar1.End();
    }

    Gui::VerticalSeparator();
    if (_tabbar2.Begin())
    {
        for (GnssIx ix: GNSS_IXS)
        {
            if (_tabbar2.Item(_count.label_[ix]))
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
    else if (doList)
    {
        _DrawList(gnssIx);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSignals::_DrawList(const GnssIx gnssIx)
{
    _table.SetTableRowFilter(gnssIx != GnssIx::ALL ? gnssIx : 0);
    _table.DrawTable();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSignals::_DrawSky(const GnssIx gnssIx)
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

    const float radiusPx = (MAX(canvasSize.x, canvasSize.y) / 2.0f) - (2 * GuiSettings::charSize.x);

    // Draw grid
    {
        // Elevation circles
        int ix = 0;
        const int rm = _cnoSky.rs_.size() / 3;
        for (const float r: _cnoSky.rs_)
        {
            draw->AddCircle(canvasCent, radiusPx * r,
                (ix++ % rm) == 0 ? GUI_COLOUR(PLOT_GRID_MAJOR) : GUI_COLOUR(PLOT_GRID_MINOR));
        }
        // Azimuth lines
        ix = 0;
        const int ra = _cnoSky.axys_.size() / 4;
        for (const auto &xy: _cnoSky.axys_)
        {
            draw->AddLine(canvasCent, canvasCent + (xy * radiusPx),
                (ix++ % ra) == 0 ? GUI_COLOUR(PLOT_GRID_MAJOR) :  GUI_COLOUR(PLOT_GRID_MINOR));
        }
    }

    // Draw bins
    for (auto &bin: _cnoSky.bins_)
    {
        if (bin.mean_[gnssIx] > 0.0f)
        {
            draw->PathClear();
            draw->PathArcTo(canvasCent, radiusPx * bin.r0_, bin.a0_, bin.a1_);
            draw->PathArcTo(canvasCent, radiusPx * bin.r1_, bin.a1_, bin.a0_);
            draw->PathFillConvex(GuiSettings::CnoColour(bin.mean_[gnssIx]));
        }
    }

    // Tooltips and bin highlight
    if (ImGui::IsWindowHovered())
    {
        // Is circle hovered?
        const FfVec2f mouse = ImGui::GetMousePos();
        const FfVec2f dxy = mouse - canvasCent;
        const float dxy2 = (dxy.x * dxy.x) + (dxy.y * dxy.y);
        const float r2 = radiusPx * radiusPx;
        if (dxy2 < r2)
        {
            // Determine which bin is hovered
            const float a = std::atan2(dxy.y, dxy.x);
            const int azim = ((int)std::floor(a * (180.0f / M_PI) + 0.5f) + (a < 0.0f ? 360 : 0) + 90) % 360;
            const int elev = 90 - (int)std::floor((90.0f * std::sqrt(dxy2) / std::sqrt(r2)) + 0.5f);
            const auto binIx = _cnoSky.AzimElevToBin(azim, elev);
            if (binIx < _cnoSky.bins_.size())
            {
                auto &bin = _cnoSky.bins_[binIx];
                if (bin.count_[gnssIx] > 0)
                {
                    ImGui::SetCursorScreenPos(mouse);
                    ImGui::Dummy(ImVec2(1,1));
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(bin.Tooltip(gnssIx).c_str());
                    ImGui::EndTooltip();

                    draw->PathClear();
                    draw->PathArcTo(canvasCent, radiusPx * bin.r0_, bin.a0_, bin.a1_);
                    draw->PathArcTo(canvasCent, radiusPx * bin.r1_, bin.a1_, bin.a0_);
                    draw->PathStroke(GUI_COLOUR(C_GREY), ImDrawFlags_Closed, 3);
                }
            }
        }
    }

    draw->PopClipRect();

    ImGui::EndChild();
}

/* ****************************************************************************************************************** */
