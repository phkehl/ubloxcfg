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

#ifndef __DATABASE_HPP__
#define __DATABASE_HPP__

#include <cmath>
#include <mutex>
#include <deque>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

#include "ubloxcfg.h"
#include "ff_parser.h"
#include "ff_epoch.h"
#include "ff_cpp.hpp"

#include "gui_settings.hpp"

/* ****************************************************************************************************************** */

// PN: no plot, no statistics (for types other than float or double)
// P0: no statistics (must be float or double)
// P1: statistics pass 1 (must be float or double)
// P2: statistics pass 2 (must be float or double)
#define DATABASE_COLUMNS(_PN_, _P0_, _P1_, _P2_) \
    /*  Field                Type     Init   Fmt       Label */ \
    _PN_(time_ts,            uint32_t, 0,    NULL,     NULL /* reception/processing time */         ) /* TIME() */ \
    _P0_(time_monotonic,     double,  NAN,   "%.3f",   "Monotonic time [s]"                         ) \
    _P0_(time_gps_tow,       double,  NAN,   "%.3f",   "GPS time of week [s]"                       ) \
    _P1_(time_gps_tow_acc,   double,  NAN,   "%.3e",   "GPS time of week accuracy estimate [s]"     ) \
    _P0_(time_posix,         double,  NAN,   "%.3f",   "POSIX time [s]"                             ) \
    \
    _PN_(fix_type,           EPOCH_FIX_t, EPOCH_FIX_UNKNOWN, NULL, NULL                             ) \
    _PN_(fix_ok,             bool,    false, NULL,     NULL                                         ) \
    _P0_(fix_type_val,       float,   NAN,   NULL,     "Position fix [-]"                           ) \
    _P0_(fix_ok_val,         float,   NAN,   NULL,     "Position fix OK [-]"                        ) \
    _PN_(fix_colour,         ImU32, GUI_COLOUR(FIX_INVALID), NULL, NULL                             ) \
    _PN_(fix_str,            const char *, "?", NULL,  NULL                                         ) \
    _P1_(fix_interval,       float,   NAN,   "%.3f",   "Fix interval [s]"                           ) \
    _P1_(fix_rate,           float,   NAN,   "%.1f",   "Fix rate [Hz]"                              ) \
    _P1_(fix_latency,        float,   NAN,   "%.3f",   "Fix latency [s]"                            ) \
    _P1_(fix_mean_interval,  float,   NAN,   "%.3f",   "Fix average interval (10 fixes window) [s]" ) \
    _P1_(fix_mean_rate,      float,   NAN,   "%.1f",   "Fix average rate (10 fixes window) [Hz]"    ) \
    _P1_(fix_mean_latency,   float,   NAN,   "%.3f",   "Fix average latency (10 fixes window) [s]"  ) \
    \
    _PN_(pos_avail,          bool,    false, NULL,     NULL /* pos_* below have values */           ) \
    _P1_(pos_ecef_x,         double,  NAN,   "%.3f",   "Position ECEF X [m]"                        ) /* pos_ecef_* must be adjacent! */ \
    _P1_(pos_ecef_y,         double,  NAN,   "%.3f",   "Position ECEF Y [m]"                        ) \
    _P1_(pos_ecef_z,         double,  NAN,   "%.3f",   "Position ECEF Z [m]"                        ) \
    _P1_(pos_llh_lat_deg,    double,  NAN,   "%.12f",  "Position LLH latitude [deg]"                ) \
    _P1_(pos_llh_lon_deg,    double,  NAN,   "%.12f",  "Position LLH longitude [deg]"               ) \
    _P1_(pos_llh_lat,        double,  NAN,   NULL,     NULL                                         ) /* pos_llh_* must be adjacent! */ \
    _P1_(pos_llh_lon,        double,  NAN,   NULL,     NULL                                         ) \
    _P1_(pos_llh_height,     double,  NAN,   "%.3f",   "Position LLH height [m]"                    ) \
    _P2_(pos_enu_ref_east,   double,  NAN,   "%.3f",   "Position ENU (ref) East [m]"                ) /* pos_enu_ref_* must be adjacent! */ \
    _P2_(pos_enu_ref_north,  double,  NAN,   "%.3f",   "Position ENU (ref) North [m]"               ) \
    _P2_(pos_enu_ref_up,     double,  NAN,   "%.3f",   "Position ENU (ref) up [m]"                  ) \
    _P2_(pos_enu_mean_east,  double,  NAN,   "%.3f",   "Position ENU (mean) East [m]"               ) /* pos_enu_mean_* must be adjacent! */ \
    _P2_(pos_enu_mean_north, double,  NAN,   "%.3f",   "Position ENU (mean) North [m]"              ) \
    _P2_(pos_enu_mean_up,    double,  NAN,   "%.3f",   "Position ENU (mean) up [m]"                 ) \
    _P1_(pos_acc_3d,         double,  NAN,   "%.3f",   "Position accuracy estimate (3d) [m]"        ) \
    _P1_(pos_acc_horiz,      double,  NAN,   "%.3f",   "Position accuracy estimate (horiz.) [m]"    ) \
    _P1_(pos_acc_vert,       double,  NAN,   "%.3f",   "Position accuracy estimate (vert.) [m]"     ) \
    \
    _PN_(vel_avail,          bool,    false, NULL,     NULL /* vel_* below have values */           ) \
    _P1_(vel_enu_east,       double,  NAN,   "%.3f",   "Velocity ENU East [m/s]"                    ) \
    _P1_(vel_enu_north,      double,  NAN,   "%.3f",   "Velocity ENU North [m/s]"                   ) \
    _P1_(vel_enu_down,       double,  NAN,   "%.3f",   "Velocity ENU down [m/s]"                    ) \
    _P1_(vel_3d,             double,  NAN,   "%.3f",   "Velocity (3d) [m/s]"                        ) \
    _P1_(vel_horiz,          double,  NAN,   "%.3f",   "Velocity (horiz) [m/s]"                     ) \
    \
    _P1_(clock_bias,         double,  NAN,   "%.3e",   "Receiver clock bias [s]"                    ) \
    _P1_(clock_drift,        double,  NAN,   "%.3e",   "Receiver clock drift [s/s]"                 ) \
    \
    _PN_(relpos_avail,       bool,    false, NULL,     NULL                                         ) \
    _P1_(relpos_dist,        double,  NAN,   "%.3f",   "Relative position distance [m]"             ) \
    _P1_(relpos_ned_north,   double,  NAN,   "%.3f",   "Relative position North [m]"                ) /* relpos_ned_* must be adjacent! */ \
    _P1_(relpos_ned_east,    double,  NAN,   "%.3f",   "Relative position East [m]"                 ) \
    _P1_(relpos_ned_down,    double,  NAN,   "%.3f",   "Relative position Down [m]"                 ) \
    \
    _P1_(dop_pdop,           float,   NAN,   "%.2f",   "Position DOP [-]"                           ) \
    \
    _PN_(sol_numsig_avail,   bool,    false, NULL,     NULL                                         ) \
    _P1_(sol_numsig_tot,     float,   NAN,   "%.1f",   "Number of signals used (total) [-]"         ) \
    _P1_(sol_numsig_gps,     float,   NAN,   "%.1f",   "Number of signals used (GPS) [-]"           ) \
    _P1_(sol_numsig_glo,     float,   NAN,   "%.1f",   "Number of signals used (GLONASS) [-]"       ) \
    _P1_(sol_numsig_gal,     float,   NAN,   "%.1f",   "Number of signals used (Galileo) [-]"       ) \
    _P1_(sol_numsig_bds,     float,   NAN,   "%.1f",   "Number of signals used (BeiDou) [-]"        ) \
    _P1_(sol_numsig_sbas,    float,   NAN,   "%.1f",   "Number of signals used (SBAS) [-]"          ) \
    _P1_(sol_numsig_qzss,    float,   NAN,   "%.1f",   "Number of signals used (QZSS) [-]"          ) \
    _PN_(sol_numsat_avail,   bool,    false, NULL,     NULL                                         ) \
    _P1_(sol_numsat_tot,     float,   NAN,   "%.1f",   "Number of satellites used (total) [-]"      ) \
    _P1_(sol_numsat_gps,     float,   NAN,   "%.1f",   "Number of satellites used (GPS) [-]"        ) \
    _P1_(sol_numsat_glo,     float,   NAN,   "%.1f",   "Number of satellites used (GLONASS) [-]"    ) \
    _P1_(sol_numsat_gal,     float,   NAN,   "%.1f",   "Number of satellites used (Galileo) [-]"    ) \
    _P1_(sol_numsat_bds,     float,   NAN,   "%.1f",   "Number of satellites used (BeiDou) [-]"     ) \
    _P1_(sol_numsat_sbas,    float,   NAN,   "%.1f",   "Number of satellites used (SBAS) [-]"       ) \
    _P1_(sol_numsat_qzss,    float,   NAN,   "%.1f",   "Number of satellites used (QZSS) [-]"       ) \
    \
    _P1_(cno_nav_00,         float,   NAN,   "%.1f",   "Number of signals used (0-4 dBHz) [-]"      ) /* cno_nav_* must be adjacent! */ \
    _P1_(cno_nav_05,         float,   NAN,   "%.1f",   "Number of signals used (5-9 dBHz) [-]"      ) \
    _P1_(cno_nav_10,         float,   NAN,   "%.1f",   "Number of signals used (10-14 dBHz) [-]"    ) \
    _P1_(cno_nav_15,         float,   NAN,   "%.1f",   "Number of signals used (15-19 dBHz) [-]"    ) \
    _P1_(cno_nav_20,         float,   NAN,   "%.1f",   "Number of signals used (20-24 dBHz) [-]"    ) \
    _P1_(cno_nav_25,         float,   NAN,   "%.1f",   "Number of signals used (25-29 dBHz) [-]"    ) \
    _P1_(cno_nav_30,         float,   NAN,   "%.1f",   "Number of signals used (30-34 dBHz) [-]"    ) \
    _P1_(cno_nav_35,         float,   NAN,   "%.1f",   "Number of signals used (35-39 dBHz) [-]"    ) \
    _P1_(cno_nav_40,         float,   NAN,   "%.1f",   "Number of signals used (40-44 dBHz) [-]"    ) \
    _P1_(cno_nav_45,         float,   NAN,   "%.1f",   "Number of signals used (45-49 dBHz) [-]"    ) \
    _P1_(cno_nav_50,         float,   NAN,   "%.1f",   "Number of signals used (50-54 dBHz) [-]"    ) \
    _P1_(cno_nav_55,         float,   NAN,   "%.1f",   "Number of signals used (55- dBHz) [-]"      ) \
    _P1_(cno_trk_00,         float,   NAN,   "%.1f",   "Number of signals tracked (0-4 dBHz) [-]"   ) /* cno_trk_* must be adjacent! */ \
    _P1_(cno_trk_05,         float,   NAN,   "%.1f",   "Number of signals tracked (5-9 dBHz) [-]"   ) \
    _P1_(cno_trk_10,         float,   NAN,   "%.1f",   "Number of signals tracked (10-14 dBHz) [-]" ) \
    _P1_(cno_trk_15,         float,   NAN,   "%.1f",   "Number of signals tracked (15-19 dBHz) [-]" ) \
    _P1_(cno_trk_20,         float,   NAN,   "%.1f",   "Number of signals tracked (20-24 dBHz) [-]" ) \
    _P1_(cno_trk_25,         float,   NAN,   "%.1f",   "Number of signals tracked (25-29 dBHz) [-]" ) \
    _P1_(cno_trk_30,         float,   NAN,   "%.1f",   "Number of signals tracked (30-34 dBHz) [-]" ) \
    _P1_(cno_trk_35,         float,   NAN,   "%.1f",   "Number of signals tracked (35-39 dBHz) [-]" ) \
    _P1_(cno_trk_40,         float,   NAN,   "%.1f",   "Number of signals tracked (40-44 dBHz) [-]" ) \
    _P1_(cno_trk_45,         float,   NAN,   "%.1f",   "Number of signals tracked (45-49 dBHz) [-]" ) \
    _P1_(cno_trk_50,         float,   NAN,   "%.1f",   "Number of signals tracked (50-54 dBHz) [-]" ) \
    _P1_(cno_trk_55,         float,   NAN,   "%.1f",   "Number of signals tracked (55- dBHz) [-]"   ) \
    \
    _P1_(cno_avg_top5_l1,    float,   NAN,   "%.1f",   "Average of 5 strongest signals (L1) [dBHz]" ) \
    _P1_(cno_avg_top5_l2,    float,   NAN,   "%.1f",   "Average of 5 strongest signals (L2) [dBHz]" ) \
    _P1_(cno_avg_top5_l5,    float,   NAN,   "%.1f",   "Average of 5 strongest signals (L5) [dBHz]" ) \
    _P1_(cno_avg_top10_l1,   float,   NAN,   "%.1f",   "Average of 10 strongest signals (L1) [dBHz]" ) \
    _P1_(cno_avg_top10_l2,   float,   NAN,   "%.1f",   "Average of 10 strongest signals (L2) [dBHz]" ) \
    _P1_(cno_avg_top10_l5,   float,   NAN,   "%.1f",   "Average of 10 strongest signals (L5) [dBHz]" ) \
    /* end*/


#define _DB_ROW_FIELDS(  _field_, _type_, _init_, _fmt_, _label_) _type_ _field_;
#define _DB_ROW_CTOR(    _field_, _type_, _init_, _fmt_, _label_) _field_ { _init_ },
#define _DB_STATS_FIELDS(_field_, _type_, _init_, _fmt_, _label_) Stats _field_;
#define _DB_FIELDS_ENUM( _field_, _type_, _init_, _fmt_, _label_) CONCAT(ix_, _field_),
#define _DB_SKIP(        _field_, _type_, _init_, _fmt_, _label_) /* nothing */
#define _DB_COUNT(       _field_, _type_, _init_, _fmt_, _label_) + 1
// undef'd at the end of this file

class Database
{
    public:

        Database(const std::string &name);
       ~Database();

        enum FieldIx { ix_none, DATABASE_COLUMNS(_DB_SKIP, _DB_FIELDS_ENUM, _DB_FIELDS_ENUM, _DB_FIELDS_ENUM) };
        struct FieldDef { const char *label; const char *name; const char *fmt; FieldIx field; };
        static constexpr int NUM_FIELDS = 0 DATABASE_COLUMNS(_DB_SKIP, _DB_COUNT, _DB_COUNT, _DB_COUNT);
        static const FieldDef FIELDS[NUM_FIELDS];

        //! One row in the database
        struct Row
        {
            Row() : DATABASE_COLUMNS(_DB_ROW_CTOR, _DB_ROW_CTOR, _DB_ROW_CTOR, _DB_ROW_CTOR) dummy{0} {}

            // All fields, e.g.:
            // uint32_t time_ts
            // EPOCH_FIX_T fix_type;
            // double pos_ecef_x;
            // etc.
            DATABASE_COLUMNS(_DB_ROW_FIELDS, _DB_ROW_FIELDS, _DB_ROW_FIELDS, _DB_ROW_FIELDS)
            int dummy;

            // Access fields by enum value (no "PN" fields, ony P0, P1, P2)
            // FieldIx: ix_fix_type_val, ix_pos_ecef_x, etc.
            double operator[](const FieldIx field) const;
        };

        //! Statistics for a database value
        struct Stats
        {
            Stats();
            double min;   //!< Minimum value (NAN if count < 1)
            double max;   //!< Maximum value (NAN if count < 1)
            double mean;  //!< Mean value (NAN if count < 1)
            double std;   //!< Standard deviation (NAN if count < 2)
            int    count; //!< Number of samples
        };

        //! Database info, statistics, other derived data
        struct Info
        {
            Info();
            //! Statistics for each column (that has statistics)
            struct {
                DATABASE_COLUMNS(_DB_SKIP, _DB_SKIP, _DB_STATS_FIELDS, _DB_STATS_FIELDS)
            } stats;

            static constexpr int NUM_STATS = 0 DATABASE_COLUMNS(_DB_SKIP, _DB_SKIP, _DB_COUNT, _DB_COUNT);
            struct {
                const char *label; // May be NULL
                const char *fmt;
                Stats      *stats;
            } stats_list[NUM_STATS];

            //! Error ellipse (East-North)
            struct {
                double a;
                double b;
                double omega;
            } err_ell;

            static constexpr int CNO_BINS_NUM = EPOCH_SIGCNOHIST_NUM;
            //! Histogram data for number of signals tracked
            struct {
                float mins[CNO_BINS_NUM];
                float maxs[CNO_BINS_NUM];
                float means[CNO_BINS_NUM];
                float stds[CNO_BINS_NUM];
            } cno_trk;
            //! Histogram data for number of signals used
            struct {
                float mins[CNO_BINS_NUM];
                float maxs[CNO_BINS_NUM];
                float means[CNO_BINS_NUM];
                float stds[CNO_BINS_NUM];
            } cno_nav;
            // Histogram bins
            static const float CNO_BINS_LO[CNO_BINS_NUM]; // lower bound
            static const float CNO_BINS_MI[CNO_BINS_NUM]; // bin centre
        };


        const std::string   &GetName();
        void                 Clear();
        Row                  AddEpoch(const EPOCH_t &epoch, const bool isRealTime);
        Info                 GetInfo();
        Row                  LatestRow();
        void                 ProcRows(std::function<bool(const Row &)> cb, const bool backwards = false);

        void                 BeginGetRows();                 //!< Lock mutex
        const Row           &operator[](const int ix) const; //!< Access rows
        const Row           &GetRow(const int ix) const;     //!< Access rows
        void                 EndGetRows();                   //!< Unlock mutex

        // Database status
        int                  Size();
        int                  MaxSize();
        bool                 Changed(const void *uid);

        // Reference position
        enum RefPos   { REFPOS_MEAN, REFPOS_LAST, REFPOS_USER };
        enum RefPos          GetRefPos();
        enum RefPos          GetRefPos(double llh[3], double xyz[3]);
        void                 SetRefPosMean();
        void                 SetRefPosLast();
        void                 SetRefPosLlh(double llh[3]);
        void                 SetRefPosXyz(double xyz[3]);

    private:

        std::string          _name;
        std::mutex           _mutex;
        enum RefPos          _refPos;
        double               _refPosXyz[3];
        double               _refPosLlh[3];
        void                 _ProcRows(std::function<bool(const Row &)> cb, const bool backwards = false);
        void                 _Sync();
        uint32_t             _serial;
        std::unordered_map<const void *, uint32_t> _changed;

        std::deque<Row>      _rows;    //!< Database rows of values (rows = epochs)
        double               _rowsT0;  //!< Start time for Row.time_monotonic
        Info                 _info;    //!< Info (statistics, error ellipse, ...)
};

#undef _DB_ROW_FIELDS
#undef _DB_ROW_CTOR
#undef _DB_STATS_FIELDS
#undef _DB_SKIP
#undef _DB_COUNT

/* ****************************************************************************************************************** */
#endif // __DATABASE_HPP__
