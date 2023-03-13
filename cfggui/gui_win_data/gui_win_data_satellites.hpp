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

#ifndef __GUI_WIN_DATA_SATELLITES_HPP__
#define __GUI_WIN_DATA_SATELLITES_HPP__

#include "gui_widget_table.hpp"
#include "gui_widget_tabbar.hpp"

#include "gui_win_data.hpp"

/* ***** Sky view *************************************************************************************************** */

class GuiWinDataSatellites : public GuiWinData
{
    public:
        GuiWinDataSatellites(const std::string &name, std::shared_ptr<Database> database);

    protected:

        void _ProcessData(const InputData &data) final;
        void _DrawContent() final;
        void _ClearData() final;

        enum GnssIx : int { ALL = 0, GPS, GAL, BDS, GLO, OTHER, _NUM };
        static constexpr GnssIx GNSS_IXS[_NUM] = { ALL, GPS, GAL, BDS, GLO, OTHER };
        static constexpr const char* GNSS_NAMES[_NUM] = { "All", "GPS", "GAL", "BDS", "GLO", "Other" };

        struct Sat
        {
            Sat(const EPOCH_SATINFO_t &satInfo, const EPOCH_t &epoch);
            EPOCH_SATINFO_t satInfo_;
            bool            visible_;
            GnssIx          gnssIx_;
            FfVec2f         xy_;
            EPOCH_SIGINFO_t sigL1_;
            EPOCH_SIGINFO_t sigL2_;
        };

        struct Count
        {
            Count();
            void Reset();
            void Add(const Sat &sat);
            void Update();
            int               num_[GnssIx::_NUM];
            int               used_[GnssIx::_NUM];
            int               visible_[GnssIx::_NUM];
            std::string       labelSky_[GnssIx::_NUM];
            std::string       labelList_[GnssIx::_NUM];
        };

        std::vector<Sat> _sats;
        Count            _count;
        GuiWidgetTable   _table;
        GuiWidgetTabbar  _tabbar1;
        GuiWidgetTabbar  _tabbar2;

        void _UpdateSatellites();
        void _DrawSky(const GnssIx gnssIx);
        void _DrawList(const GnssIx gnssIx);
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_SATELLITES_HPP__
