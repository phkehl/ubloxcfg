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

#ifndef __GUI_WIN_DATA_SCATTER_H__
#define __GUI_WIN_DATA_SCATTER_H__

#include "gui_win_data.hpp"

/* ***** Scatter plot *********************************************************************************************** */

class GuiWinDataScatter : public GuiWinData
{
    public:
        GuiWinDataScatter(const std::string &name,
            std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile, std::shared_ptr<Database> database);
       ~GuiWinDataScatter();

      //void                 Loop(const uint32_t &frame, const double &now) override;
      //void                 ProcessData(const Data &data) override;
        void                 ClearData() override;
        void                 DrawWindow() override;

    protected:
        float                _plotRadius;
        bool                 _havePoints;
        bool                 _showStats;
        double               _refPosLlhDragStart[3];
        double               _refPosXyzDragStart[3];
        enum { NUM_SIGMA = 6 };
        static const float   _sigmaScales[NUM_SIGMA];
        static const char * const _sigmaLabels[NUM_SIGMA];
        bool                 _sigmaEnabled[NUM_SIGMA];
        enum { NUM_HIST = 50 };
        int                  _histogramN[NUM_HIST];
        int                  _histogramE[NUM_HIST];
        int                  _histNumPoints;
        bool                 _showHistogram;
        bool                 _showAccEst;
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_SCATTER_H__
