/* ************************************************************************************************/ // clang-format off
// flipflip's cfggui
// Copyright (c) 2021 Philippe Kehl (flipflip at oinkzwurgl dot org)
// All rights reserved.

#ifndef __GUI_WIN_DATA_SPECTRUM_H__
#define __GUI_WIN_DATA_SPECTRUM_H__

#include "gui_win_data.hpp"

/* ***** Firmware update ******************************************************************************************** */

class GuiWinDataSpectrum : public GuiWinData
{
    public:
        GuiWinDataSpectrum(const std::string &name,
            std::shared_ptr<Receiver> receiver, std::shared_ptr<Logfile> logfile, std::shared_ptr<Database> database);

      //void                 Loop(const uint32_t &frame, const double &now) override;
        void                 ProcessData(const Data &data) override;
        void                 ClearData() override;
        void                 DrawWindow() override;

    protected:

        struct SpectData
        {
            SpectData();
            bool valid;
            std::vector<double> freq;
            std::vector<double> ampl;
            std::vector<double> min;
            std::vector<double> max;
            std::vector<double> mean;
            double center;
            double span;
            double res;
            double pga;
            double count;
        };
        struct Label
        {
            Label(const double _freq, const char *_label);
            double      freq;
            std::string title;
            std::string id;
        };

        std::vector<SpectData> _spects;
        ImPlotFlags          _plotFlags;
        std::vector<Label>   _labels;
        bool                 _resetPlotRange;

        void _Update(const uint8_t *msg, const int size);
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_SPECTRUM_H__
