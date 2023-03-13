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

#ifndef __GUI_WIN_DATA_SIGNALS_HPP__
#define __GUI_WIN_DATA_SIGNALS_HPP__

#include <utility>
#include <vector>

#include "ff_epoch.h"

#include "gui_widget_table.hpp"
#include "gui_widget_tabbar.hpp"

#include "gui_win_data.hpp"

/* ***** Signals **************************************************************************************************** */

class GuiWinDataSignals : public GuiWinData
{
    public:
        GuiWinDataSignals(const std::string &name, std::shared_ptr<Database> database);

        static void DrawSignalLevelCb(void *arg);

    private:

        void _ProcessData(const InputData &data) final;
        void _DrawToolbar() final;
        void _DrawContent() final;
        void _ClearData() final;

        enum GnssIx : int { ALL = 0, GPS, GAL, BDS, GLO, OTHER, _NUM };
        static constexpr GnssIx GNSS_IXS[_NUM] = { ALL, GPS, GAL, BDS, GLO, OTHER, };
        static constexpr const char* GNSS_NAMES[_NUM] = { "All", "GPS", "GAL", "BDS", "GLO", "Other" };

        struct Count
        {
            Count();
            void Reset();
            void Add(const EPOCH_SIGINFO_t &sig);
            void Update();
            int  num_[GnssIx::_NUM];
            int  used_[GnssIx::_NUM];
            std::string label_[GnssIx::_NUM];
        };

        struct CnoBin {
            CnoBin(const float r0, const float r1, const float a0, const float a1);
            void Reset();
            void Add(const GnssIx gnssIx, const float cno);
            const std::string &Tooltip(const GnssIx gnssIx);

            // CNo statistics
            uint32_t count_[GnssIx::_NUM];
            float    min_[GnssIx::_NUM];
            float    max_[GnssIx::_NUM];
            float    mean_[GnssIx::_NUM];

            // Bin region in polar coordinates
            float r0_; //!< Min radius [unit] (0..1)
            float r1_; //!< Max radius [unit] (0..1)
            float a0_; //!< Min angle [rad] (0..2pi)
            float a1_; //!< Max angle [rad] (0..2pi)

            FfVec2f xy_; //!< Position of bin centre [unit, unit] (0..1, 0..1)
            std::string uid_;
            private:
                std::string _tooltip[GnssIx::_NUM];
        };

        struct CnoSky {
            CnoSky(const int n_az = 24 /* 360 / 24 = 15 deg*/, const int n_el = 9 /* 90 / 9 = 10 deg*/);
            void Reset();
            void AddCno(const EPOCH_SIGINFO_t &sig, const EPOCH_SATINFO_t &sat);
            std::size_t AzimElevToBin(const int azim, const int elev) const;
            int                   n_az_;  //!< Number of divisions in azimuth
            int                   n_el_;  //!< Number of divisions in elevation
            float                 s_az_;  //!< Size ("width") of bin in azimuth [deg]
            float                 s_el_;  //!< Size ("width") of bin in elevation [deg]
            std::vector<CnoBin>   bins_;  //!< Bins
            std::vector<float>    rs_;    //!< Precalculated radii for grid
            std::vector<FfVec2f>  axys_;  //!< Precalculated cos() and sin() of angles for grid
        };

        Count           _count;
        CnoSky          _cnoSky;
        bool            _onlyTracked;
        GuiWidgetTable  _table;
        GuiWidgetTabbar _tabbar1;
        GuiWidgetTabbar _tabbar2;

        void _UpdateSignals();
        void _DrawSky(const GnssIx gnssIx);
        void _DrawList(const GnssIx gnssIx);

        static constexpr int SKY_NUM_BINS_AZ = 24; // 15 deg
        static constexpr int SKY_NUM_BINS_EL =  9; // 10 deg
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_SIGNALS_HPP__
