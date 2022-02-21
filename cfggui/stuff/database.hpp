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

#ifndef __DATABASE_HPP__
#define __DATABASE_HPP__

#include <mutex>
#include <vector>
#include <string>
#include <functional>

#include "ubloxcfg.h"
#include "ff_parser.h"
#include "ff_epoch.h"
#include "ff_cpp.hpp"

/* ****************************************************************************************************************** */

class Database
{
    public:
        Database(const int size);

        void Clear();
        void AddEpoch(const EPOCH_t *raw);
        void AddMsg(const PARSER_MSG_t *msg);

        enum PosIx_e { _X_   = 0, _Y_   = 1, _Z_      = 2,
                       _LAT_ = 0, _LON_ = 1, _HEIGHT_ = 2,
                       _E_   = 0, _N_   = 1, _U_      = 2, _NUM_POS_ = 3 };

        //! One entry in the database --> ProcEpochs(), LatestEpoch(), [], GetEpoch()
        struct Epoch
        {
            Epoch(const EPOCH_t *_raw);
            bool    valid;
            EPOCH_t raw;
            double  ts;                 // Timestamp [s]
            double  enuRef[_NUM_POS_];  // ENU relative to reference position
            double  enuMean[_NUM_POS_]; // ENU relative to mean position
        };
        struct Stats
        {
            Stats();
            void Begin();
            void Add(const double &val);
            void End();
            double min;
            double max;
            double mean;
            double std;
            int    count;
            private:
                double _sum;
                double _sum2;
        };
        struct ErrEll
        {
            ErrEll();
            double a;
            double b;
            double omega;
        };

        // Statistics of all data --> GetStats()
        struct EpochStats
        {
            EpochStats();
            Stats  llh[_NUM_POS_];
            Stats  enuRef[_NUM_POS_];  //!< ENU relative to reference position
            Stats  enuMean[_NUM_POS_]; //!< ENU relative to mean position
            ErrEll enErrEll;
        };
        EpochStats           GetStats();

        // Epoch data access
        void                 ProcEpochs(std::function<bool(const int ix, const Epoch &)> cb, const bool backwards = false);
        const Epoch         *LatestEpoch();

        void                 BeginGetEpoch();        //!< Lock mutex
        int                  NumEpochs();
        const Epoch         &operator[](const int ix); //!< Access epochs
        const Epoch         &GetEpoch(const int ix); //!< Access epochs
        void                 EndGetEpoch();          //!< Unlock mutex

        // Reference position
        enum RefPos_e { REFPOS_MEAN, REFPOS_LAST, REFPOS_USER };
        enum RefPos_e        GetRefPos();
        enum RefPos_e        GetRefPos(double llh[_NUM_POS_], double xyz[_NUM_POS_]);
        void                 SetRefPosMean();
        void                 SetRefPosLast();
        void                 SetRefPosLlh(double llh[_NUM_POS_]);
        void                 SetRefPosXyz(double xyz[_NUM_POS_]);

        // Database status
        int GetSize();
        int GetUsage();

    protected:

        int                  _size;
        std::vector<Epoch>   _epochs;
        int                  _epochIx;
        int                  _epochIxLast;
        int                  _epochsValidCnt;
        uint32_t             _epochsT0;
        EpochStats           _stats;
        std::mutex           _mutex;
        enum RefPos_e        _refPos;
        double               _refPosXyz[3];
        double               _refPosLlh[3];
        void                 _ProcEpochs(std::function<bool(const int ix, const Epoch &)> cb, const bool backwards = false);
        void                 _Sync();

    private:
};

/* ****************************************************************************************************************** */
#endif // __DATABASE_HPP__
