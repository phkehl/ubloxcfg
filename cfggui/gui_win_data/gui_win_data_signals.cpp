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

#include "gui_inc.hpp"

#include "gui_win_data_signals.hpp"

/* ****************************************************************************************************************** */

GuiWinDataSignals::GuiWinDataSignals(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database),
    _countAll  { "All" },
    _countGps  { "GPS" },
    _countGlo  { "GLO" },
    _countBds  { "BDS" },
    _countGal  { "GAL" },
    _countSbas { "SBAS" },
    _countQzss { "QZSS" },
    _minSigUse { EPOCH_SIGUSE_ACQUIRED },
    _tabbar    { WinName(), ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_TabListPopupButton }
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
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWinDataSignals::Count::Count(const char *_name) :
    num{0}, used{0}, label{}
{
    std::strncpy(name, _name, sizeof(name) - 1);
};

void GuiWinDataSignals::Count::Reset()
{
    num = 0;
    used = 0;
    std::snprintf(label, sizeof(label), "%s###%s", name, name);
}

void GuiWinDataSignals::Count::Update()
{
    std::snprintf(label, sizeof(label), "%s (%d/%d)###%s", name, used, num, name);
}

void GuiWinDataSignals::Count::Add(const EPOCH_SIGINFO_t *sig)
{
    num++;
    if ( sig->prUsed || sig->crUsed || sig->doUsed )
    {
        used++;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSignals::_UpdateSignals()
{
    _table.ClearRows();
    _sigInfo.clear();
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

    // Populate full list of signals, considering filters
    const int numSig = _latestEpoch->epoch.numSignals;
    for (int ix = 0; ix < numSig; ix++)
    {
        const EPOCH_SIGINFO_t *sig = &_latestEpoch->epoch.signals[ix];
        if (sig->use < _minSigUse)
        {
            continue;
        }
        _sigInfo.push_back(*sig);
        _countAll.Add(sig);
        switch (sig->gnss)
        {
            case EPOCH_GNSS_UNKNOWN:                   break;
            case EPOCH_GNSS_GPS:  _countGps.Add(sig);  break;
            case EPOCH_GNSS_GLO:  _countGlo.Add(sig);  break;
            case EPOCH_GNSS_BDS:  _countBds.Add(sig);  break;
            case EPOCH_GNSS_GAL:  _countGal.Add(sig);  break;
            case EPOCH_GNSS_SBAS: _countSbas.Add(sig); break;
            case EPOCH_GNSS_QZSS: _countQzss.Add(sig); break;
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
    for (auto &sig: _sigInfo)
    {
        // <use><gnss><sat><sig>
        const uint32_t thisSat = (sig.gnss << 16) | (sig.sv << 8);
        const uint32_t rowUid = thisSat | sig.signal;
        const uint32_t rowSort = rowUid | (sig.use > EPOCH_SIGUSE_SEARCH ? 0x01000000 : 0xff000000);// default sort key: sort by gnss/sat/sig, and used signals first, searching signals at the end

        _table.AddCellText(sig.svStr, thisSat);
        _table.AddCellText(sig.signalStr, sig.signal);
        _table.AddCellText(sig.bandStr, sig.band);
        if ( sig.valid && (sig.use > EPOCH_SIGUSE_SEARCH) )
        {
            _table.AddCellCb(std::bind(&GuiWinDataSignals::_DrawSignalLevelCb, this, std::placeholders::_1), &sig);
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
    }
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ void GuiWinDataSignals::DrawSignalLevelWithBar(const EPOCH_SIGINFO_t *sig, const ImVec2 charSize)
{
    ImGui::Text("%-4s %4.1f", sig->signalStr, sig->cno);
    ImGui::SameLine();

    ImDrawList *draw = ImGui::GetWindowDrawList();

    const float barMaxLen = 100.0f; // [px]
    const float barMaxCno = 55.0f;  // [dbHz]
    const FfVec2f bar0 = ImGui::GetCursorScreenPos();
    const FfVec2f barSize = ImVec2(barMaxLen, charSize.y);
    ImGui::Dummy(barSize);
    const float barLen = sig->cno * barMaxLen / barMaxCno;
    const FfVec2f bar1 = bar0 + FfVec2f(barLen, barSize.y);
    const int colIx = sig->cno > 55.0f ? (EPOCH_SIGCNOHIST_NUM - 1) : (sig->cno > 0.0f ? (sig->cno / 5.0f) : 0);
    draw->AddRectFilled(bar0, bar1, sig->anyUsed ? GUI_COLOUR(SIGNAL_00_05 + colIx) : GUI_COLOUR(SIGNAL_UNUSED));
    const float offs42 = 42.0 * barMaxLen / barMaxCno;
    draw->AddLine(bar0 + FfVec2f(offs42, 0), bar0 + FfVec2f(offs42, barSize.y), GUI_COLOUR(C_GREY));
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSignals::_DrawSignalLevelCb(void *arg)
{
    DrawSignalLevelWithBar((const EPOCH_SIGINFO_t *)arg, GuiSettings::charSize);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSignals::_DrawToolbar()
{
    ImGui::SameLine();

    switch (_minSigUse)
    {
        case EPOCH_SIGUSE_UNKNOWN:
        case EPOCH_SIGUSE_NONE:
        case EPOCH_SIGUSE_SEARCH:
            if (ImGui::Button(ICON_FK_DOT_CIRCLE_O "###MinSigUse", GuiSettings::iconSize))
            {
                _minSigUse = EPOCH_SIGUSE_ACQUIRED;
                _UpdateSignals();
            }
            Gui::ItemTooltip("Showing all signals");
            break;
        case EPOCH_SIGUSE_ACQUIRED:
        case EPOCH_SIGUSE_UNUSABLE:
        case EPOCH_SIGUSE_CODELOCK:
        case EPOCH_SIGUSE_CARRLOCK:
            if (ImGui::Button(ICON_FK_CIRCLE_O "###MinSigUse", GuiSettings::iconSize))
            {
                _minSigUse = EPOCH_SIGUSE_SEARCH;
                _UpdateSignals();
            }
            Gui::ItemTooltip("Showing only tracked signals");
            break;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSignals::_DrawContent()
{
    EPOCH_GNSS_t filter = EPOCH_GNSS_UNKNOWN;

    if (_tabbar.Begin())
    {
        _tabbar.Item(_countAll.label,  [&filter]() { filter = EPOCH_GNSS_UNKNOWN; });
        _tabbar.Item(_countGps.label,  [&filter]() { filter = EPOCH_GNSS_GPS;     });
        _tabbar.Item(_countGlo.label,  [&filter]() { filter = EPOCH_GNSS_GLO;     });
        _tabbar.Item(_countGal.label,  [&filter]() { filter = EPOCH_GNSS_GAL;     });
        _tabbar.Item(_countBds.label,  [&filter]() { filter = EPOCH_GNSS_BDS;     });
        _tabbar.Item(_countSbas.label, [&filter]() { filter = EPOCH_GNSS_SBAS;    });
        _tabbar.Item(_countQzss.label, [&filter]() { filter = EPOCH_GNSS_QZSS;    });
        _tabbar.End();
    }

    _table.SetTableRowFilter(filter != EPOCH_GNSS_UNKNOWN ? filter : 0);
    _table.DrawTable();
}

/* ****************************************************************************************************************** */
