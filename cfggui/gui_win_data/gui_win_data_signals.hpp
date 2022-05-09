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

#ifndef __GUI_WIN_DATA_SIGNALS_HPP__
#define __GUI_WIN_DATA_SIGNALS_HPP__

#include "ff_epoch.h"

#include "gui_widget_table.hpp"
#include "gui_widget_tabbar.hpp"

#include "gui_win_data.hpp"

/* ***** Signals **************************************************************************************************** */

class GuiWinDataSignals : public GuiWinData
{
    public:
        GuiWinDataSignals(const std::string &name, std::shared_ptr<Database> database);

        static void DrawSignalLevelWithBar(const EPOCH_SIGINFO_t *sig, const ImVec2 charSize);

    protected:

        void _ProcessData(const InputData &data) final;
        void _DrawToolbar() final;
        void _DrawContent() final;
        void _ClearData() final;

        std::vector<EPOCH_SIGINFO_t> _sigInfo;
        struct Count
        {
            Count(const char *_name);
            void Reset();
            void Add(const EPOCH_SIGINFO_t *sig);
            void Update();
            int  num;
            int  used;
            char name[10];
            char label[30];
        };
        Count _countAll;
        Count _countGps;
        Count _countGlo;
        Count _countBds;
        Count _countGal;
        Count _countSbas;
        Count _countQzss;
        EPOCH_SIGUSE_t       _minSigUse;
        GuiWidgetTable       _table;
        GuiWidgetTabbar      _tabbar;
        std::map<uint32_t, bool> _selSigs;

        void _UpdateSignals();
        void _DrawSignalLevelCb(void *arg);
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_SIGNALS_HPP__
