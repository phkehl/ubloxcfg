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

#ifndef __GUI_WIN_DATA_SATELLITES_HPP__
#define __GUI_WIN_DATA_SATELLITES_HPP__

#include "gui_widget_table.hpp"

#include "gui_win_data.hpp"

/* ***** Sky view *************************************************************************************************** */

class GuiWinDataSatellites : public GuiWinData
{
    public:
        GuiWinDataSatellites(const std::string &name, std::shared_ptr<Database> database);

    protected:

        void _ProcessData(const Data &data) final;
        void _DrawContent() final;
        void _ClearData() final;

        struct SatInfo
        {
            SatInfo(const EPOCH_SATINFO_t *_satInfo);
            const EPOCH_SATINFO_t *satInfo;
            bool                   visible;
            float                  dX;
            float                  dY;
            float                  fR;
            const EPOCH_SIGINFO_t *sigL1;
            const EPOCH_SIGINFO_t *sigL2;
        };
        std::vector<SatInfo> _satInfo;
        struct Count
        {
            Count(const char *_name);
            void Reset();
            void Add(const SatInfo &sat);
            void Update();
            int  num;
            int  used;
            int  visible;
            char name[10];
            char labelSky[30];
            char labelList[30];
        };
        Count _countAll;
        Count _countGps;
        Count _countGlo;
        Count _countBds;
        Count _countGal;
        Count _countSbas;
        Count _countQzss;
        GuiWidgetTable _table;
        std::map<uint32_t, bool> _selSats;

        void _UpdateSatellites();
        void _DrawSky(const EPOCH_GNSS_t filter);
        void _DrawList(const EPOCH_GNSS_t filter);
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_SATELLITES_HPP__
