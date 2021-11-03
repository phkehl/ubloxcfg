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

#include "gui_win_data_signals.hpp"

#include "gui_win_data_inc.hpp"

/* ****************************************************************************************************************** */

GuiWinDataSignals::GuiWinDataSignals(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database),
    _countAll{"All"}, _countGps{"GPS"}, _countGlo{"GLO"}, _countBds{"BDS"}, _countGal{"GAL"}, _countSbas{"SBAS"}, _countQzss{"QZSS"},
    _minSigUse{EPOCH_SIGUSE_ACQUIRED}, _selSigs{}
{
    _winSize = { 115, 40 };

    ClearData();

    _table.AddColumn("SV", 40.0f);
    _table.AddColumn("Signal", 60.0f);
    _table.AddColumn("Bd.", 30.0f);
    _table.AddColumn("Level", 155.0f);
    _table.AddColumn("Use", 75.0f);
    _table.AddColumn("Iono", 80.0f);
    _table.AddColumn("Health", 80.0f);
    _table.AddColumn("Used", 70.0f);
    _table.AddColumn("PR res.", 60.0f);
    _table.AddColumn("Corrections", 140.0f);
}

// ---------------------------------------------------------------------------------------------------------------------

//void GuiWinDataSignals::Loop(const std::unique_ptr<Receiver> &receiver)
//{
//}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSignals::ProcessData(const Data &data)
{
    switch (data.type)
    {
        case Data::Type::DATA_EPOCH:
            if (data.epoch->epoch.numSignals > 0)
            {
                _epoch = data.epoch;
                _epochTs = ImGui::GetTime();
            }
            _UpdateSignals();
            // TODO: drop epoch if signals info gets too old
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinDataSignals::Loop(const uint32_t &frame, const double &now)
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

void GuiWinDataSignals::ClearData()
{
    _epoch = nullptr;
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
    std::strncpy(label, name, sizeof(label) - 1);
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
    _sigInfo.clear();
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

    // Populate full list of signals, considering filters
    const int numSig = _epoch->epoch.numSignals;
    for (int ix = 0; ix < numSig; ix++)
    {
        const EPOCH_SIGINFO_t *sig = &_epoch->epoch.signals[ix];
        if (sig->use < _minSigUse)
        {
            continue;
        }
        _sigInfo.push_back(sig);
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

    // Sort searching signals to the end
    if (_minSigUse <= EPOCH_SIGUSE_SEARCH)
    {
        std::sort(_sigInfo.begin(), _sigInfo.end(), [](const EPOCH_SIGINFO_t *a, const EPOCH_SIGINFO_t *b)
        {
            // First, sort by signal quality groups (none/searching <--> tracked/used)
            if ( (a->use > EPOCH_SIGUSE_SEARCH) && (b->use <= EPOCH_SIGUSE_SEARCH) )
            {
                return true; // a before b
            }
            if ( (a->use <= EPOCH_SIGUSE_SEARCH) && (b->use > EPOCH_SIGUSE_SEARCH) )
            {
                return false; // b before a
            }
            // Second, sort by order from EPOCH_t (GNSS, SV, signal)
            return a->_order < b->_order;
        });
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

void GuiWinDataSignals::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    // Controls
    {
        switch (_minSigUse)
        {
            case EPOCH_SIGUSE_UNKNOWN:
            case EPOCH_SIGUSE_NONE:
            case EPOCH_SIGUSE_SEARCH:
                if (ImGui::Button(ICON_FK_DOT_CIRCLE_O "###MinSigUse", _winSettings->iconButtonSize))
                {
                    _minSigUse = EPOCH_SIGUSE_ACQUIRED;
                    _UpdateSignals();
                }
                Gui::ItemTooltip("Showing all signals");
                break;
            case EPOCH_SIGUSE_ACQUIRED:
            case EPOCH_SIGUSE_UNUSED:
            case EPOCH_SIGUSE_CODELOCK:
            case EPOCH_SIGUSE_CARRLOCK:
                if (ImGui::Button(ICON_FK_CIRCLE_O "###MinSigUse", _winSettings->iconButtonSize))
                {
                    _minSigUse = EPOCH_SIGUSE_SEARCH;
                    _UpdateSignals();
                }
                Gui::ItemTooltip("Showing only tracked signals");
                break;
        }

        ImGui::SameLine();

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

    EPOCH_GNSS_t filter = EPOCH_GNSS_UNKNOWN;
    if (ImGui::BeginTabBar("##tabs2", ImGuiTabBarFlags_FittingPolicyScroll /*ImGuiTabBarFlags_FittingPolicyResizeDown*/))
    {
        if (ImGui::BeginTabItem(_countAll.label )) { filter = EPOCH_GNSS_UNKNOWN; ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem(_countGps.label )) { filter = EPOCH_GNSS_GPS;     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem(_countGlo.label )) { filter = EPOCH_GNSS_GLO;     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem(_countGal.label )) { filter = EPOCH_GNSS_GAL;     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem(_countBds.label )) { filter = EPOCH_GNSS_BDS;     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem(_countSbas.label)) { filter = EPOCH_GNSS_SBAS;    ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem(_countQzss.label)) { filter = EPOCH_GNSS_QZSS;    ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }

    _DrawSignals(filter);

    _DrawWindowEnd();
}

void GuiWinDataSignals::_DrawSignals(const EPOCH_GNSS_t filter)
{
    _table.BeginDraw();

    ImDrawList *draw = ImGui::GetWindowDrawList();
    const float lineHeight = ImGui::GetTextLineHeight();
    const ImVec2 barOffs(_winSettings->style.ItemSpacing.x + (4 * _winSettings->charSize.x), -_winSettings->style.ItemSpacing.y);

    uint32_t prevSat = 0xffffffff;
    for (auto *sig: _sigInfo)
    {
        if ( ( (filter != EPOCH_GNSS_UNKNOWN) && (sig->gnss != filter) ) )
        {
            continue;
        }

        const bool sigUsed = sig->prUsed || sig->crUsed || sig->doUsed;
        bool style = true;
        if (sig->use <= EPOCH_SIGUSE_SEARCH)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
        }
        else if (sigUsed)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_OK));
        }
        else
        {
            style = false;
        }

        const uint32_t thisSat = (sig->gnss << 8) | sig->sv;
        const uint32_t uid = (sig->signal << 16) | thisSat;
        auto selUid = _selSigs.find(uid);
        const bool selected = selUid == _selSigs.end() ? false : selUid->second;
        if (_table.ColSelectable(thisSat != prevSat ? sig->svStr : "", selected))
        {
            _selSigs[uid] = !selected;
        }
        prevSat = thisSat;

        _table.ColText(sig->signalStr);

        _table.ColText(sig->bandStr);
        ImGui::Text("%4.1f", sig->cno);
        const ImVec2 offs = ImGui::GetCursorScreenPos() + barOffs;
        const int colIx = sig->cno > 55 ? (EPOCH_SIGCNOHIST_NUM - 1) : (sig->cno > 0 ? (sig->cno / 5) : 0 );
        draw->AddRectFilled(offs, offs + ImVec2(sig->cno * 2, -lineHeight), sigUsed ? GUI_COLOUR(SIGNAL_00_05 + colIx) : GUI_COLOUR(SIGNAL_UNUSED));
        draw->AddLine(offs + ImVec2(42 * 2, 0), offs + ImVec2(42 * 2, -lineHeight), GUI_COLOUR(PLOT_GRID_MAJOR));
        ImGui::NextColumn();

        _table.ColTextF("%s", sig->useStr);

        _table.ColText(sig->ionoStr);

        _table.ColText(sig->healthStr);

        _table.ColTextF("%s %s %s",
            sig->prUsed ? "PR" : "--",
            sig->crUsed ? "CR" : "--",
            sig->doUsed ? "DO" : "--");

        if (sig->prUsed /*sig->prRes != 0.0*/)
        {
            _table.ColTextF("%6.1f", sig->prRes);
        }
        else
        {
            _table.ColSkip();
        }

        _table.ColTextF("%-9s %s %s %s",
            sig->corrStr,
            sig->prCorrUsed ? "PR" : "--",
            sig->crCorrUsed ? "CR" : "--",
            sig->doCorrUsed ? "DO" : "--");

        if (style)
        {
            ImGui::PopStyleColor();
        }

        ImGui::Separator();
    }

    _table.EndDraw();
}

/* ****************************************************************************************************************** */
