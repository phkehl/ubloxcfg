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

Database::Database(const int size, const std::string &name) :
    _name        { name },
    _maxSize     { size },
    _refPos      { REFPOS_MEAN },
    _refPosXyz   { 0.0, 0.0, 0.0 },
    _refPosLlh   { 0.0, 0.0, 0.0 },
    _serial      { 0 }
{
    DEBUG("Database(%d)", _maxSize);
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

Database::EpochStats Database::GetStats()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _stats;
}

// ---------------------------------------------------------------------------------------------------------------------

enum Database::RefPos_e Database::GetRefPos()
{
    //std::lock_guard<std::mutex> lock(_mutex);
    return _refPos;
}

enum Database::RefPos_e Database::GetRefPos(double llh[3], double xyz[3])
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

void Database::SetRefPosLlh(double llh[_NUM_POS_])
{
    std::lock_guard<std::mutex> lock(_mutex);
    _refPos = REFPOS_USER;
    std::memcpy(_refPosLlh, llh, sizeof(_refPosLlh));
    llh2xyz_vec(_refPosLlh, _refPosXyz);
    _Sync();
}

void Database::SetRefPosXyz(double xyz[_NUM_POS_])
{
    std::lock_guard<std::mutex> lock(_mutex);
    _refPos = REFPOS_USER;
    std::memcpy(_refPosXyz, xyz, sizeof(_refPosXyz));
    xyz2llh_vec(_refPosXyz, _refPosLlh);
    _Sync();
}

// ---------------------------------------------------------------------------------------------------------------------

void Database::ProcEpochs(std::function<bool(const Database::Epoch &)> cb, const bool backwards)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _ProcEpochs(cb, backwards);
}

void Database::_ProcEpochs(std::function<bool(const Database::Epoch &)> cb, const bool backwards)
{
    bool abort = false;
    if (!backwards)
    {
        for (auto entry = _epochs.cbegin(); !abort && (entry != _epochs.cend()); entry++)
        {
            if (!cb(*entry))
            {
                abort = true;
            }
        }
    }
    else
    {
        for (auto entry = _epochs.crbegin(); !abort && (entry != _epochs.crend()); entry++)
        {
            if (!cb(*entry))
            {
                abort = true;
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

const Database::Epoch *Database::LatestEpoch()
{
    return _epochs.empty() ? nullptr : &_epochs.back();
}

// ---------------------------------------------------------------------------------------------------------------------

void Database::BeginGetEpoch()
{
    _mutex.lock();
}

const Database::Epoch &Database::GetEpoch(const int ix)
{
    return _epochs[ix];
}

const Database::Epoch &Database::operator[](const int ix)
{
    return _epochs[ix];
}

void Database::EndGetEpoch()
{
    _mutex.unlock();
}

// ---------------------------------------------------------------------------------------------------------------------

void Database::Clear()
{
    std::lock_guard<std::mutex> lock(_mutex);

    _epochs.clear();
    _epochsT0 = 0.0;
    _stats = EpochStats();
    _serial++;
}

// ---------------------------------------------------------------------------------------------------------------------

int Database::Size()
{
    return _epochs.size();
}

int Database::MaxSize()
{
    return _maxSize;
}

// ---------------------------------------------------------------------------------------------------------------------

void Database::AddEpoch(const EPOCH_t *raw)
{
    std::lock_guard<std::mutex> lock(_mutex);

    //DEBUG("Database::AddEpoch() %s", raw->str);

    auto epoch = Epoch(raw);

    // Timestamp
    if (_epochs.empty())
    {
        _epochsT0 = epoch.raw.ts;
        epoch.ts = 0.0;
    }
    else
    {
        epoch.ts = (double)(epoch.raw.ts - _epochsT0) * 1e-3;
    }

    _epochs.push_back(epoch);

    while ((int)_epochs.size() > _maxSize)
    {
        _epochs.pop_front();
    }

    _Sync();
}

// ---------------------------------------------------------------------------------------------------------------------

void Database::_Sync()
{
    // TODO: calculate stuff in xyz, transform to llh later

    // ---- Update (recalculate) statistics ------------------------------------

    EpochStats stats;
    stats.llh[_LAT_].Begin();
    stats.llh[_LON_].Begin();
    stats.llh[_HEIGHT_].Begin();
    for (const auto &e: _epochs)
    {
        if (e.valid && e.raw.havePos)
        {
            stats.llh[_LAT_].Add(e.raw.llh[_LAT_]);
            stats.llh[_LON_].Add(e.raw.llh[_LON_]);
            stats.llh[_HEIGHT_].Add(e.raw.llh[_HEIGHT_]);
        }
    }
    stats.llh[_LAT_].End();
    stats.llh[_LON_].End();
    stats.llh[_HEIGHT_].End();

    // ----- (Re-)calculate ENU vectors for scatter plot ----------------------

    // Reference position for ENU calculation below
    switch (_refPos)
    {
        case REFPOS_MEAN:
        {
            _refPosLlh[_LAT_] = stats.llh[_LAT_].mean;
            _refPosLlh[_LON_] = stats.llh[_LON_].mean;
            _refPosLlh[_HEIGHT_] = stats.llh[_HEIGHT_].mean;
            llh2xyz_vec(_refPosLlh, _refPosXyz);
            break;
        }
        case REFPOS_LAST:
        {
            _ProcEpochs([&](const Database::Epoch &epoch)
            {
                if (epoch.raw.havePos)
                {
                    std::memcpy(_refPosLlh, epoch.raw.llh, sizeof(_refPosLlh));
                    llh2xyz_vec(_refPosLlh, _refPosXyz);
                    return false;
                }
                return true;
            }, true);
            break;
        }
        case REFPOS_USER:
            // _refPosLlh and _refPosXyz set by user
            break;
    }

    // Calculate East/North/Up (relative to refPos)
    stats.enuRef[_E_].Begin();
    stats.enuRef[_N_].Begin();
    stats.enuRef[_U_].Begin();
    for (auto &e: _epochs)
    {
        if (e.valid && e.raw.havePos)
        {
            xyz2enu_vec(e.raw.xyz, _refPosXyz, _refPosLlh, e.enuRef); // epoch
            stats.enuRef[_E_].Add(e.enuRef[_E_]);
            stats.enuRef[_N_].Add(e.enuRef[_N_]);
            stats.enuRef[_U_].Add(e.enuRef[_U_]);
        }
    }
    stats.enuRef[_E_].End();
    stats.enuRef[_N_].End();
    stats.enuRef[_U_].End();

    // Calculate East/North/Up (relative to mean pos)
    stats.enuMean[_E_].Begin();
    stats.enuMean[_N_].Begin();
    stats.enuMean[_U_].Begin();
    double meanLlh[_NUM_POS_] = { stats.llh[_LAT_].mean, stats.llh[_LON_].mean, stats.llh[_HEIGHT_].mean };
    double meanXyz[_NUM_POS_];
    llh2xyz_vec(meanLlh, meanXyz);
    double corrEN = 0.0;
    for (auto &e: _epochs)
    {
        if (e.valid && e.raw.havePos)
        {
            xyz2enu_vec(e.raw.xyz, meanXyz, meanLlh, e.enuMean); // epoch
            stats.enuMean[_E_].Add(e.enuMean[_E_]);
            stats.enuMean[_N_].Add(e.enuMean[_N_]);
            stats.enuMean[_U_].Add(e.enuMean[_U_]);
            corrEN += (e.enuMean[_E_] * e.enuMean[_N_]);
        }
    }
    stats.enuMean[_E_].End();
    stats.enuMean[_N_].End();
    stats.enuMean[_U_].End();

    // Calculate East/North error ellipse
    if (stats.enuMean[_E_].count > 2)
    {
        double qxx = stats.enuMean[_E_].std * stats.enuMean[_E_].std;
        double qyy = stats.enuMean[_N_].std * stats.enuMean[_N_].std;
        double qxy = (corrEN / (double)stats.enuMean[_E_].count) - (stats.enuMean[_E_].mean * stats.enuMean[_N_].mean);
        double tmp = (qxx - qyy) / 2;
        double tmp1 = ((qxx + qyy) / 2) + std::sqrt( (tmp * tmp) + (qxy * qxy) );
        double tmp2 = ((qxx + qyy) / 2) - std::sqrt( (tmp * tmp) + (qxy * qxy) );
        tmp1 = MAX(tmp1, 0);
        tmp2 = MAX(tmp2, 0);
        stats.enErrEll.a = std::sqrt(tmp1);
        stats.enErrEll.b = std::sqrt(tmp2);
        stats.enErrEll.omega = std::atan2(2 * qxy, qxx - qyy) / 2;
    }

    // CNo histogram
    for (int ix = 0; ix < EPOCH_SIGCNOHIST_NUM; ix++)
    {
        Stats statsCnoNav;
        Stats statsCnoTrk;
        statsCnoNav.Begin();
        statsCnoTrk.Begin();
        for (const auto &e: _epochs)
        {
            statsCnoNav.Add(e.raw.sigCnoHistNav[ix]);
            statsCnoTrk.Add(e.raw.sigCnoHistTrk[ix]);
        }
        statsCnoNav.End();
        statsCnoTrk.End();

        const float lo = EPOCH_SIGCNOHIST_IX2CNO_L(ix);
        const float hi = EPOCH_SIGCNOHIST_IX2CNO_H(ix);
        const float cno = lo + (0.5 * (hi - lo + 1.0f));
        const std::string label = Ff::Sprintf("%.0f-%.0f", lo, hi);
        stats.cnoNav.Add(label, lo, cno, hi, statsCnoNav);
        stats.cnoTrk.Add(label, lo, cno, hi, statsCnoTrk);
    }

    std::swap(_stats, stats);
    _serial++;
}

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

Database::Epoch::Epoch(const EPOCH_t *_raw) :
    valid   { false },
    raw     { },
    ts      { 0 },
    enuRef  { 0, 0, 0 },
    enuMean { 0, 0, 0 }
{
    if ( (_raw != NULL) && (_raw->valid) )
    {
        raw = *_raw;
        valid = true;
    }
}

/* ****************************************************************************************************************** */

Database::Stats::Stats() :
    min   { 0.0 },
    max   { 0.0 },
    mean  { 0.0 },
    std   { 0.0 },
    count { 0 },
    _sum  { 0.0 },
    _sum2 { 0.0 }
{
}

void Database::Stats::Begin()
{
    min   = DBL_MAX;
    max   = -DBL_MAX;
    mean  = 0.0;
    std   = 0.0;
    _sum  = 0.0;
    _sum2 = 0.0;
}

void Database::Stats::Add(const double &val)
{
    count++;
    if (val < min) { min = val; }
    if (val > max) { max = val; }
    mean += (val - mean) / (double)count;
    _sum += val;
    _sum2 += (val * val);
}

void Database::Stats::End()
{
    if (count > 1)
    {
        const double var = (_sum2 - ((_sum * _sum) / (double)count)) / (double)(count - 1);
        if (var > 0.0)
        {
            std = std::sqrt(var);
        }
    }
    else if (count < 1)
    {
        min = 0.0;
        max = 0.0;
    }
}

/* ****************************************************************************************************************** */

Database::ErrEll::ErrEll() :
    a     { 0.0 },
    b     { 0.0 },
    omega { 0.0 }
{
}

/* ****************************************************************************************************************** */

Database::EpochStats::EpochStats()
{
}

/* ****************************************************************************************************************** */

Database::CnoStats::CnoStats() :
    count { 0 }
{
}

void Database::CnoStats::Clear()
{
    labels.clear();
    cnosLo.clear();
    cnosMi.clear();
    cnosHi.clear();
    means.clear();
    mins.clear();
    maxs.clear();
    stds.clear();
}

void Database::CnoStats::Add(const std::string &label, const float cnoLo, const float cnoMi, const float cnoHi, const Stats &stats)
{
    labels.push_back(label);
    cnosLo.push_back(cnoLo);
    cnosMi.push_back(cnoMi);
    cnosHi.push_back(cnoHi);
    means.push_back(stats.mean);
    mins.push_back(stats.min);
    maxs.push_back(stats.max);
    stds.push_back(stats.std);
    count++;
}

/* ****************************************************************************************************************** */
