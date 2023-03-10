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

#include <cmath>
#include <cstring>
#include <cfloat>
#include <limits>

#include "ff_stuff.h"
#include "ff_debug.h"
#include "ff_trafo.h"
#include "ff_utils.hpp"

#include "database.hpp"

/* ****************************************************************************************************************** */

Database::Database(const std::string &name) :
    _name        { name },
    _refPos      { REFPOS_MEAN },
    _refPosXyz   { 0.0, 0.0, 0.0 },
    _refPosLlh   { 0.0, 0.0, 0.0 },
    _serial      { 0 }
{
    DEBUG("Database()");
    Clear();
}

Database::~Database()
{
    DEBUG("~Database()");
}

// ---------------------------------------------------------------------------------------------------------------------

const std::string &Database::GetName()
{
    return _name;
}

// ---------------------------------------------------------------------------------------------------------------------

void Database::Clear()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _rows.clear();
    _rowsT0 = 0.0;
    _info = Info();
    _serial++;
}

// ---------------------------------------------------------------------------------------------------------------------

int Database::Size()
{
    return _rows.size();
}

int Database::MaxSize()
{
    return GuiSettings::dbNumRows;
}

// ---------------------------------------------------------------------------------------------------------------------

Database::Info Database::GetInfo()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _info;
}

// ---------------------------------------------------------------------------------------------------------------------

enum Database::RefPos Database::GetRefPos()
{
    //std::lock_guard<std::mutex> lock(_mutex);
    return _refPos;
}

enum Database::RefPos Database::GetRefPos(double llh[3], double xyz[3])
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (llh)
    {
        std::memcpy(llh, _refPosLlh, sizeof(_refPosLlh));
    }
    if (xyz)
    {
        std::memcpy(xyz, _refPosXyz, sizeof(_refPosXyz));
    }
    return _refPos;
}

// ---------------------------------------------------------------------------------------------------------------------

void Database::SetRefPosMean()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _refPos = REFPOS_MEAN;
    _Sync();
}

void Database::SetRefPosLast()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _refPos = REFPOS_LAST;
    _Sync();
}

void Database::SetRefPosLlh(double llh[3])
{
    std::lock_guard<std::mutex> lock(_mutex);
    _refPos = REFPOS_USER;
    std::memcpy(_refPosLlh, llh, sizeof(_refPosLlh));
    llh2xyz_vec(_refPosLlh, _refPosXyz);
    _Sync();
}

void Database::SetRefPosXyz(double xyz[3])
{
    std::lock_guard<std::mutex> lock(_mutex);
    _refPos = REFPOS_USER;
    std::memcpy(_refPosXyz, xyz, sizeof(_refPosXyz));
    xyz2llh_vec(_refPosXyz, _refPosLlh);
    _Sync();
}

// ---------------------------------------------------------------------------------------------------------------------

void Database::ProcRows(std::function<bool(const Row &)> cb, const bool backwards)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _ProcRows(cb, backwards);
}

void Database::_ProcRows(std::function<bool(const Row &)> cb, const bool backwards)
{
    bool abort = false;
    if (!backwards)
    {
        for (auto entry = _rows.cbegin(); !abort && (entry != _rows.cend()); entry++)
        {
            if (!cb(*entry))
            {
                abort = true;
            }
        }
    }
    else
    {
        for (auto entry = _rows.crbegin(); !abort && (entry != _rows.crend()); entry++)
        {
            if (!cb(*entry))
            {
                abort = true;
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

Database::Row Database::LatestRow()
{
    return _rows.empty() ? Row() : _rows.back();
}

// ---------------------------------------------------------------------------------------------------------------------

void Database::BeginGetRows()
{
    _mutex.lock();
}

const Database::Row &Database::GetRow(const int ix) const
{
    return _rows[ix];
}

const Database::Row &Database::operator[](const int ix) const
{
    return _rows[ix];
}

void Database::EndGetRows()
{
    _mutex.unlock();
}

// ---------------------------------------------------------------------------------------------------------------------

void Database::AddEpoch(const EPOCH_t &raw)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_rows.empty()) {
        _rowsT0 = (double)raw.ts * 1e-3;
    }

    Row row;

    row.time_ts = raw.ts;
    row.time_monotonic = ((double)raw.ts * 1e-3) - _rowsT0;
    if (raw.haveGpsTow) {
        row.time_gps_tow     = raw.gpsTow;
        row.time_gps_tow_acc = raw.gpsTowAcc;
    }
    if (raw.havePosixTime) {
        row.time_posix = raw.posixTime;
    }

    if (raw.haveFix) {
        row.fix_type     = raw.fix;
        row.fix_ok       = raw.fixOk;
        row.fix_type_val = (float)raw.fix;
        row.fix_ok_val   = (raw.fixOk ? 1.0f : 0.0f);
        row.fix_colour   = GuiSettings::FixColour(row.fix_type, row.fix_ok);
        row.fix_str      = raw.fixStr;

        // Find last row with fix, calculate mean interval and rate
        auto lastFixRow1  = _rows.rend();
        auto lastFixRow10 = _rows.rend();
        int numFixes = 0;
        float latencySum = 0.0f;
        int latencyNum = 0;
        for (auto cand = _rows.rbegin(); (cand != _rows.rend()) && (numFixes <= 9); cand++)
        {
            if (cand->fix_type != EPOCH_FIX_UNKNOWN)
            {
                numFixes++;
                switch (numFixes)
                {
                    case  1: lastFixRow1  = cand; break;
                    case 10: lastFixRow10 = cand; break;
                }
                if (!std::isnan(cand->fix_latency))
                {
                    latencySum += cand->fix_latency;
                    latencyNum++;
                }
            }
        }
        if (lastFixRow1 != _rows.rend())
        {
            row.fix_interval = (row.time_monotonic - lastFixRow1->time_monotonic);
            if (row.fix_interval > 0.0f)
            {
                row.fix_rate = 1.0f / row.fix_interval;
            }
            if (lastFixRow10 != _rows.rend())
            {
                row.fix_mean_interval = (row.time_monotonic - lastFixRow10->time_monotonic) / 10.0f;
            }
            if (row.fix_interval > 0.0f)
            {
                row.fix_mean_rate = 1.0f / row.fix_mean_interval;
            }
        }
        if (raw.haveLatency)
        {
            row.fix_latency = raw.latency;
            if (!std::isnan(raw.latency) && (latencyNum > 5))
            {
                row.fix_mean_latency = (latencySum + raw.latency) / (float)(latencyNum + 1);
            }
        }
    }

    if (raw.havePos) {
        row.pos_avail       = true;
        row.pos_ecef_x      = raw.xyz[0];
        row.pos_ecef_y      = raw.xyz[1];
        row.pos_ecef_z      = raw.xyz[2];
        row.pos_llh_lat_deg = rad2deg(raw.llh[0]);
        row.pos_llh_lon_deg = rad2deg(raw.llh[1]);
        row.pos_llh_lat     = raw.llh[0];
        row.pos_llh_lon     = raw.llh[1];
        row.pos_llh_height  = raw.llh[2];
        row.pos_acc_3d      = raw.posAcc;
        row.pos_acc_horiz   = raw.horizAcc;
        row.pos_acc_vert    = raw.vertAcc;
    }

    if (raw.haveVel) {
        row.vel_avail        = true;
        row.vel_enu_east     = raw.velNed[1];
        row.vel_enu_north    = raw.velNed[0];
        row.vel_enu_down     = -raw.velNed[2];
        row.vel_3d           = raw.vel3d;
        row.vel_horiz        = raw.vel2d;
    }

    if (raw.haveClock) {
        row.clock_bias       = raw.clockBias;
        row.clock_drift      = raw.clockDrift;
    }

    if (raw.haveRelPos) {
        row.relpos_avail     = true;
        row.relpos_dist      = raw.relLen;
        row.relpos_ned_north = raw.relNed[0];
        row.relpos_ned_east  = raw.relNed[1];
        row.relpos_ned_down  = raw.relNed[2];
    }

    if (raw.havePdop) {
        row.dop_pdop         = raw.pDOP;
    }

    if (raw.haveNumSig) {
        row.sol_numsig_avail = true;
        row.sol_numsig_tot   = raw.numSigUsed;
        row.sol_numsig_gps   = raw.numSigUsedGps;
        row.sol_numsig_glo   = raw.numSigUsedGlo;
        row.sol_numsig_gal   = raw.numSigUsedGal;
        row.sol_numsig_bds   = raw.numSigUsedBds;
        row.sol_numsig_sbas  = raw.numSigUsedSbas;
        row.sol_numsig_qzss  = raw.numSigUsedQzss;
    }

    if (raw.haveNumSat) {
        row.sol_numsat_avail = true;
        row.sol_numsat_tot   = raw.numSatUsed;
        row.sol_numsat_gps   = raw.numSatUsedGps;
        row.sol_numsat_glo   = raw.numSatUsedGlo;
        row.sol_numsat_gal   = raw.numSatUsedGal;
        row.sol_numsat_bds   = raw.numSatUsedBds;
        row.sol_numsat_sbas  = raw.numSatUsedSbas;
        row.sol_numsat_qzss  = raw.numSatUsedQzss;
    }

    if (raw.haveSigCnoHist) {
        static_assert(EPOCH_SIGCNOHIST_NUM == 12); // we'll have to update our code if epoch.h changes...
        row.cno_nav_00 = raw.sigCnoHistNav[0];
        row.cno_nav_05 = raw.sigCnoHistNav[1];
        row.cno_nav_10 = raw.sigCnoHistNav[2];
        row.cno_nav_15 = raw.sigCnoHistNav[3];
        row.cno_nav_20 = raw.sigCnoHistNav[4];
        row.cno_nav_25 = raw.sigCnoHistNav[5];
        row.cno_nav_30 = raw.sigCnoHistNav[6];
        row.cno_nav_35 = raw.sigCnoHistNav[7];
        row.cno_nav_40 = raw.sigCnoHistNav[8];
        row.cno_nav_45 = raw.sigCnoHistNav[9];
        row.cno_nav_50 = raw.sigCnoHistNav[10];
        row.cno_nav_55 = raw.sigCnoHistNav[11];
        row.cno_trk_00 = raw.sigCnoHistTrk[0];
        row.cno_trk_05 = raw.sigCnoHistTrk[1];
        row.cno_trk_10 = raw.sigCnoHistTrk[2];
        row.cno_trk_15 = raw.sigCnoHistTrk[3];
        row.cno_trk_20 = raw.sigCnoHistTrk[4];
        row.cno_trk_25 = raw.sigCnoHistTrk[5];
        row.cno_trk_30 = raw.sigCnoHistTrk[6];
        row.cno_trk_35 = raw.sigCnoHistTrk[7];
        row.cno_trk_40 = raw.sigCnoHistTrk[8];
        row.cno_trk_45 = raw.sigCnoHistTrk[9];
        row.cno_trk_50 = raw.sigCnoHistTrk[10];
        row.cno_trk_55 = raw.sigCnoHistTrk[11];
    }

    _rows.push_back(row);

    while ((int)_rows.size() > GuiSettings::dbNumRows) {
        _rows.pop_front();
    }

    _Sync();
}

// ---------------------------------------------------------------------------------------------------------------------

#define _DB_STATS_LIST(  _field_, _type_, _init_, _fmt_, _label_) { _label_, _fmt_, &stats._field_ },
#define _DB_SKIP(_field_, _type_, _init_, _fmt_, _label_) /* nothing */

Database::Info::Info() :
    stats_list { DATABASE_COLUMNS(_DB_SKIP, _DB_SKIP, _DB_STATS_LIST, _DB_STATS_LIST) },
    err_ell { NAN, NAN, NAN },
    cno_trk { { NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN },
              { NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN },
              { NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN },
              { NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN } },
    cno_nav { { NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN },
              { NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN },
              { NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN },
              { NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN } }
{
}

/*static*/ const float Database::Info::CNO_BINS_LO[CNO_BINS_NUM] =
{
    EPOCH_SIGCNOHIST_IX2CNO_L(0),
    EPOCH_SIGCNOHIST_IX2CNO_L(1),
    EPOCH_SIGCNOHIST_IX2CNO_L(2),
    EPOCH_SIGCNOHIST_IX2CNO_L(3),
    EPOCH_SIGCNOHIST_IX2CNO_L(4),
    EPOCH_SIGCNOHIST_IX2CNO_L(5),
    EPOCH_SIGCNOHIST_IX2CNO_L(6),
    EPOCH_SIGCNOHIST_IX2CNO_L(7),
    EPOCH_SIGCNOHIST_IX2CNO_L(8),
    EPOCH_SIGCNOHIST_IX2CNO_L(9),
    EPOCH_SIGCNOHIST_IX2CNO_L(10),
    EPOCH_SIGCNOHIST_IX2CNO_L(11),
};
/*static*/ const float Database::Info::CNO_BINS_MI[CNO_BINS_NUM] =
{
    (float)EPOCH_SIGCNOHIST_IX2CNO_L( 0) + (0.5f * (float)(EPOCH_SIGCNOHIST_IX2CNO_H( 0) - EPOCH_SIGCNOHIST_IX2CNO_L( 0) + 1)),
    (float)EPOCH_SIGCNOHIST_IX2CNO_L( 1) + (0.5f * (float)(EPOCH_SIGCNOHIST_IX2CNO_H( 1) - EPOCH_SIGCNOHIST_IX2CNO_L( 1) + 1)),
    (float)EPOCH_SIGCNOHIST_IX2CNO_L( 2) + (0.5f * (float)(EPOCH_SIGCNOHIST_IX2CNO_H( 2) - EPOCH_SIGCNOHIST_IX2CNO_L( 2) + 1)),
    (float)EPOCH_SIGCNOHIST_IX2CNO_L( 3) + (0.5f * (float)(EPOCH_SIGCNOHIST_IX2CNO_H( 3) - EPOCH_SIGCNOHIST_IX2CNO_L( 3) + 1)),
    (float)EPOCH_SIGCNOHIST_IX2CNO_L( 4) + (0.5f * (float)(EPOCH_SIGCNOHIST_IX2CNO_H( 4) - EPOCH_SIGCNOHIST_IX2CNO_L( 4) + 1)),
    (float)EPOCH_SIGCNOHIST_IX2CNO_L( 5) + (0.5f * (float)(EPOCH_SIGCNOHIST_IX2CNO_H( 5) - EPOCH_SIGCNOHIST_IX2CNO_L( 5) + 1)),
    (float)EPOCH_SIGCNOHIST_IX2CNO_L( 6) + (0.5f * (float)(EPOCH_SIGCNOHIST_IX2CNO_H( 6) - EPOCH_SIGCNOHIST_IX2CNO_L( 6) + 1)),
    (float)EPOCH_SIGCNOHIST_IX2CNO_L( 7) + (0.5f * (float)(EPOCH_SIGCNOHIST_IX2CNO_H( 7) - EPOCH_SIGCNOHIST_IX2CNO_L( 7) + 1)),
    (float)EPOCH_SIGCNOHIST_IX2CNO_L( 8) + (0.5f * (float)(EPOCH_SIGCNOHIST_IX2CNO_H( 8) - EPOCH_SIGCNOHIST_IX2CNO_L( 8) + 1)),
    (float)EPOCH_SIGCNOHIST_IX2CNO_L( 9) + (0.5f * (float)(EPOCH_SIGCNOHIST_IX2CNO_H( 9) - EPOCH_SIGCNOHIST_IX2CNO_L( 9) + 1)),
    (float)EPOCH_SIGCNOHIST_IX2CNO_L(10) + (0.5f * (float)(EPOCH_SIGCNOHIST_IX2CNO_H(10) - EPOCH_SIGCNOHIST_IX2CNO_L(10) + 1)),
    (float)EPOCH_SIGCNOHIST_IX2CNO_L(11) + (0.5f * (float)(EPOCH_SIGCNOHIST_IX2CNO_H(11) - EPOCH_SIGCNOHIST_IX2CNO_L(11) + 1)),
};

#define _DB_ST_BEGIN(_field_, _type_, _init_, _fmt_, _label_) \
    int    CONCAT(count_, _field_) = 0; \
    double CONCAT(min_,   _field_) = DBL_MAX; \
    double CONCAT(max_,   _field_) = -DBL_MAX; \
    double CONCAT(mean_,  _field_) = 0.0; \
    double CONCAT(sum_,   _field_) = 0.0; \
    double CONCAT(sum2_,  _field_) = 0.0; \


#define _DB_ST_ADD( _field_, _type_, _init_, _fmt_, _label_) \
    if (!std::isnan(row._field_)) { \
        CONCAT(count_, _field_)++; \
        if (row._field_ < CONCAT(min_, _field_)) { CONCAT(min_, _field_) = row._field_; } \
        if (row._field_ > CONCAT(max_, _field_)) { CONCAT(max_, _field_) = row._field_; } \
        CONCAT(mean_, _field_) += ((double)row._field_ - CONCAT(mean_, _field_)) / (double)CONCAT(count_, _field_); \
        CONCAT(sum_, _field_) += row._field_; \
        CONCAT(sum2_, _field_) += (row._field_ * row._field_); \
    }

#define _DB_ST_END(_field_, _type_, _init_, _fmt_, _label_) \
    if (CONCAT(count_, _field_) > 0) { \
        info.stats._field_.count = CONCAT(count_, _field_); \
        info.stats._field_.min   = CONCAT(min_,   _field_); \
        info.stats._field_.max   = CONCAT(max_,   _field_); \
        info.stats._field_.mean  = CONCAT(mean_,  _field_); \
    } \
    if (CONCAT(count_, _field_) > 1) { \
        const double var = (CONCAT(sum2_, _field_) - \
            ((CONCAT(sum_, _field_) * CONCAT(sum_, _field_)) / (double)CONCAT(count_, _field_))) / \
            (double)(CONCAT(count_, _field_) - 1); \
        if (var > 0.0) { info.stats._field_.std = std::sqrt(var); } \
    }

void Database::_Sync()
{
    Info info;

    // Calculate statistics (pass 1)
    {
        DATABASE_COLUMNS(_DB_SKIP, _DB_SKIP, _DB_ST_BEGIN, _DB_SKIP);
        for (auto &row: _rows) {
            DATABASE_COLUMNS(_DB_SKIP, _DB_SKIP, _DB_ST_ADD, _DB_SKIP);
        }
        DATABASE_COLUMNS(_DB_SKIP, _DB_SKIP, _DB_ST_END, _DB_SKIP);
    }

    // Update user's selected reference position for row.pos_enu_ref_*
    switch (_refPos)
    {
        // Use mean position from statistics calculated above
        case REFPOS_MEAN:
        {
            _refPosXyz[0] = info.stats.pos_ecef_x.mean;
            _refPosXyz[1] = info.stats.pos_ecef_y.mean;
            _refPosXyz[2] = info.stats.pos_ecef_z.mean;
            xyz2llh_vec(_refPosXyz, _refPosLlh);
            break;
        }
        // Use the last position
        case REFPOS_LAST:
        {
            _ProcRows([&](const Row &row) {
                if (row.pos_avail) {
                    _refPosXyz[0] = row.pos_ecef_x;
                    _refPosXyz[1] = row.pos_ecef_y;
                    _refPosXyz[2] = row.pos_ecef_z;
                    xyz2llh_vec(_refPosXyz, _refPosLlh);
                    return false;
                }
                return true;
            }, true);
            break;
        }
        case REFPOS_USER:
            // _refPosXyz/_refPosLlh set by user
            break;
    }

    // Calculate row.pos_enu_ref_* (ENU relative to user's selected reference position)
    for (auto &row: _rows) {
        if (!std::isnan(row.pos_ecef_x)) {
            xyz2enu_vec(&row.pos_ecef_x, _refPosXyz, _refPosLlh, &row.pos_enu_ref_east);
        }
    }

    // Calculate row.pos_enu_mean_* (ENU relative to mean position)
    const double ref_xyz[3] = { info.stats.pos_ecef_x.mean, info.stats.pos_ecef_y.mean, info.stats.pos_ecef_z.mean };
    const double ref_llh[3] = { info.stats.pos_llh_lat.mean, info.stats.pos_llh_lon.mean, info.stats.pos_llh_height.mean };
    for (auto &row: _rows) {
        if (!std::isnan(row.pos_ecef_x)) {
            xyz2enu_vec(&row.pos_ecef_x, ref_xyz, ref_llh, &row.pos_enu_mean_east);
        }
    }

    // Calculate statistics (pass 2)
    {
        DATABASE_COLUMNS(_DB_SKIP, _DB_SKIP, _DB_SKIP, _DB_ST_BEGIN);
        for (auto &row: _rows) {
            DATABASE_COLUMNS(_DB_SKIP, _DB_SKIP, _DB_SKIP, _DB_ST_ADD);
        }
        DATABASE_COLUMNS(_DB_SKIP, _DB_SKIP, _DB_SKIP, _DB_ST_END);
    }

    // Calculate East/North error ellipse
    if (info.stats.pos_enu_mean_east.count > 2) {
        double corr_en = 0.0;
        for (const auto &row: _rows) {
            if (row.pos_avail) {
                corr_en += (row.pos_enu_mean_east * row.pos_enu_mean_north);
            }
        }
        const auto &s_e = info.stats.pos_enu_mean_east;
        const auto &s_n = info.stats.pos_enu_mean_north;
        const double qxx = s_e.std * s_e.std;
        const double qyy = s_n.std * s_n.std;
        const double qxy = (corr_en / (double)s_e.count) - (s_e.mean * s_n.mean);
        const double tmp1 = 0.5 * (qxx - qyy);
        const double tmp2 = std::sqrt( (tmp1 * tmp1) + (qxy * qxy) );
        const double tmp3 = 0.5 * (qxx + qyy);
        const double tmp4 = tmp3 + tmp2;
        const double tmp5 = tmp3 - tmp2;
        info.err_ell.a = std::sqrt(std::max(tmp4, 0.0));
        info.err_ell.b = std::sqrt(std::max(tmp5, 0.0));
        info.err_ell.omega = 0.5 * std::atan2(2.0 * qxy, qxx - qyy);
    }


    // Signal level histogram
#  define _HELPER(_to_, _ix_, _from_) \
    _to_.mins[_ix_]   = _from_.min; \
    _to_.maxs[_ix_]   = _from_.max; \
    _to_.means[_ix_]  = _from_.mean; \
    _to_.stds[_ix_]   = _from_.std;
    _HELPER(info.cno_trk,  0, info.stats.cno_trk_00);
    _HELPER(info.cno_trk,  1, info.stats.cno_trk_05);
    _HELPER(info.cno_trk,  2, info.stats.cno_trk_10);
    _HELPER(info.cno_trk,  3, info.stats.cno_trk_15);
    _HELPER(info.cno_trk,  4, info.stats.cno_trk_20);
    _HELPER(info.cno_trk,  5, info.stats.cno_trk_25);
    _HELPER(info.cno_trk,  6, info.stats.cno_trk_30);
    _HELPER(info.cno_trk,  7, info.stats.cno_trk_35);
    _HELPER(info.cno_trk,  8, info.stats.cno_trk_40);
    _HELPER(info.cno_trk,  9, info.stats.cno_trk_45);
    _HELPER(info.cno_trk, 10, info.stats.cno_trk_50);
    _HELPER(info.cno_trk, 11, info.stats.cno_trk_55);
    _HELPER(info.cno_nav,  0, info.stats.cno_nav_00);
    _HELPER(info.cno_nav,  1, info.stats.cno_nav_05);
    _HELPER(info.cno_nav,  2, info.stats.cno_nav_10);
    _HELPER(info.cno_nav,  3, info.stats.cno_nav_15);
    _HELPER(info.cno_nav,  4, info.stats.cno_nav_20);
    _HELPER(info.cno_nav,  5, info.stats.cno_nav_25);
    _HELPER(info.cno_nav,  6, info.stats.cno_nav_30);
    _HELPER(info.cno_nav,  7, info.stats.cno_nav_35);
    _HELPER(info.cno_nav,  8, info.stats.cno_nav_40);
    _HELPER(info.cno_nav,  9, info.stats.cno_nav_45);
    _HELPER(info.cno_nav, 10, info.stats.cno_nav_50);
    _HELPER(info.cno_nav, 11, info.stats.cno_nav_55);
#  undef _HELPER

    std::swap(_info, info);
    _serial++;

}

#undef _DB_ST_BEGIN
#undef _DB_ST_ADD
#undef _DB_ST_END

// ---------------------------------------------------------------------------------------------------------------------

#define _DB_SWITCH_CASE( _field_, _type_, _init_, _fmt_, _label_) case CONCAT(ix_, _field_): return (double)_field_;

double Database::Row::operator[](const FieldIx field) const
{
    switch (field)
    {
        DATABASE_COLUMNS(_DB_SKIP, _DB_SWITCH_CASE, _DB_SWITCH_CASE, _DB_SWITCH_CASE)
        default: return NAN;
    }
}

#undef _DB_SWITCH_CASE

// ---------------------------------------------------------------------------------------------------------------------

#define _DB_FIELD_INFO( _field_, _type_, _init_, _fmt_, _label_) { _label_, # _field_, CONCAT(ix_, _field_) },

/*static*/ const Database::FieldDef Database::FIELDS[Database::NUM_FIELDS] =
{
    DATABASE_COLUMNS(_DB_SKIP, _DB_FIELD_INFO, _DB_FIELD_INFO, _DB_FIELD_INFO)
};

#undef _DB_FIELD_INFO

// ---------------------------------------------------------------------------------------------------------------------

bool Database::Changed(const void *uid)
{
    bool res = false;
    auto entry = _changed.find(uid);
    if (entry != _changed.end())
    {
        if (entry->second != _serial)
        {
            res = true;
            entry->second = _serial;
        }
    }
    else
    {
        _changed.emplace(uid, _serial);
        res = true;
    }
    return res;
}

/* ****************************************************************************************************************** */

Database::Stats::Stats() :
    min   { NAN },
    max   { NAN },
    mean  { NAN },
    std   { NAN },
    count { 0 }
{
}

/* ****************************************************************************************************************** */
