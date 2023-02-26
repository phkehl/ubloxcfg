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

#ifndef __GUI_WIN_DATA_SCATTER_HPP__
#define __GUI_WIN_DATA_SCATTER_HPP__

#include "gui_win_data.hpp"

/* ***** Scatter plot *********************************************************************************************** */

class GuiWinDataScatter : public GuiWinData
{
    public:
        GuiWinDataScatter(const std::string &name, std::shared_ptr<Database> database);
       ~GuiWinDataScatter();

    private:

        void _ProcessData(const InputData &data) final;
        void _ClearData() final;
        void _DrawContent() final;
        void _DrawToolbar() final;

        Database::Info _dbinfo;

        // Plot
        enum { NUM_SIGMA = 6 };
        static const float        SIGMA_SCALES[NUM_SIGMA];
        static const char * const SIGMA_LABELS[NUM_SIGMA];
        enum { NUM_HIST = 50 };
        int     _histogramN[NUM_HIST];
        int     _histogramE[NUM_HIST];
        int     _histNumPoints;
        bool    _showingErrorEll;
        bool    _triggerSnapRadius;
        double  _refPosLlhDragStart[3];
        double  _refPosXyzDragStart[3];

        // Config
        float  _plotRadius;
        bool   _showStats;
        bool   _showHistogram;
        bool   _showAccEst;
        bool   _sigmaEnabled[NUM_SIGMA];
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_SCATTER_HPP__
