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

#ifndef __GUI_WIN_DATA_STATS_H__
#define __GUI_WIN_DATA_STATS_H__

#include "gui_win_data.hpp"

/* ***** Map ******************************************************************************************************** */

class GuiWinDataStats : public GuiWinData
{
    public:
        GuiWinDataStats(const std::string &name,
            std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile, std::shared_ptr<Database> database);

      //void                 Loop(const uint32_t &frame, const double &now) override;
        void                 ProcessData(const Data &data) override;
        void                 ClearData() override;
        void                 DrawWindow() override;

    protected:

        typedef std::function<const Database::Stats (const Database::EpochStats &)> StatsGetter;
        struct Row;
        typedef std::function<void (const Database::Stats &, Row &)> RowFormatter;

        struct Row
        {
            Row(const char *_label, StatsGetter _getter, RowFormatter _formatter);
            void Clear();
            std::string  label;
            std::string  count;
            std::string  min;
            std::string  mean;
            std::string  std;
            std::string  max;
            StatsGetter  getter;
            RowFormatter formatter;
        };

        std::vector<Row> _rows;
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_STATS_H__
