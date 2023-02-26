// flipflip's (WGS84) transformation functions
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

#include <math.h>
#include <stdlib.h>

#ifdef _WIN32
#  define NOGDI
#  include <windows.h>
#endif

#include "ff_trafo.h"

/* ****************************************************************************************************************** */

double rad2deg(const double rad)
{
    return rad * 180.0 * (1.0 / M_PI);
}

double deg2rad(const double deg)
{
    return deg * (1.0 / 180.0) * M_PI;
}

// ---------------------------------------------------------------------------------------------------------------------

void deg2dms(const double deg, int *d, int *m, double *s)
{
    double tmp = deg;
    *d = floor(tmp);
    tmp -= *d;
    tmp *= 60.0;
    *m = floor(tmp);
    tmp -= *m;
    tmp *= 60.0;
    *s = tmp;
}

// ---------------------------------------------------------------------------------------------------------------------

#define WGS84_A  6378137.000
#define WGS84_E2 0.00669438

enum { _LAT_ = 0, _LON_ = 1, _HEIGHT_ = 2, _X_ = 0, _Y_ = 1, _Z_ = 2, _EAST_ = 0, _NORTH_ = 1, _UP_ = 2 };

// ---------------------------------------------------------------------------------------------------------------------

inline void llh2xyz_deg(const double lat, const double lon, const double height, double *x, double *y, double *z)
{
    llh2xyz_rad(deg2rad(lat), deg2rad(lon), height, x, y, z);
}

inline void llh2xyz_rad(const double lat, const double lon, const double height, double *x, double *y, double *z)
{
    double llh[3] = { lat, lon, height };
    double xyz[3];
    llh2xyz_vec(llh, xyz);
    *x = xyz[_X_];
    *y = xyz[_Y_];
    *z = xyz[_Z_];
}

void llh2xyz_vec(const double llh[3], double xyz[3])
{
    const double sinLat = sin(llh[_LAT_]);
    const double cosLat = cos(llh[_LAT_]);
    const double sinLon = sin(llh[_LON_]);
    const double cosLon = cos(llh[_LON_]);
    double n = WGS84_A / sqrt(1.0 - (WGS84_E2 * sinLat * sinLat));
    const double nPlusHeight = n + llh[_HEIGHT_];
    xyz[0] = nPlusHeight * cosLat * cosLon;
    xyz[1] = nPlusHeight * cosLat * sinLon;
    xyz[2] = ((n * (1.0 - WGS84_E2)) + llh[_HEIGHT_]) * sinLat;
}

// ---------------------------------------------------------------------------------------------------------------------

inline void xyz2llh_deg(const double x, const double y, const double z, double *lat, double *lon, double *height)
{
    xyz2llh_rad(x, y, z, lat, lon, height);
    *lat = rad2deg(*lat);
    *lon = rad2deg(*lon);
}

inline void xyz2llh_rad(const double x, const double y, const double z, double *lat, double *lon, double *height)
{
    double xyz[3] = { x, y, z };
    double llh[3];
    xyz2llh_vec(xyz, llh);
    *lat = llh[_LAT_];
    *lon = llh[_LON_];
    *height = llh[_HEIGHT_];
}

void xyz2llh_vec(const double xyz[3], double llh[3])
{
    // Poles
    if ( (fabs(xyz[_X_]) < 5e-4) && (fabs(xyz[_Y_]) < 5e-4) )
    {
        llh[_LAT_] = xyz[_Z_] < 0.0 ? (-M_PI / 2.0) : (M_PI / 2.0);
        llh[_LON_] = 0.0;
        llh[_HEIGHT_] = fabs(xyz[_Z_]) - 6356752.314213634 /* = sqrt(WGS84_A * WGS84_A * (1.0 - WGS84_E2)) */;
    }

    const double p = sqrt( (xyz[_X_] * xyz[_X_]) + (xyz[_Y_] * xyz[_Y_]) );
    llh[_LAT_] = atan( xyz[_Z_] / (1.0 - WGS84_E2) / p );
    const double sinLat = sin(llh[_LAT_]);
    const double n = WGS84_A / sqrt( 1.0 - (WGS84_E2 * (sinLat * sinLat)) );
    int iter = 50;
    double h = 0.0;
    while (iter > 0)
    {
        iter--;
        const double h0 = h;
        const double b0 = llh[_LAT_];
        const double fac = 1.0 - (WGS84_E2 * n / (n + h));
        llh[_LAT_] = atan(xyz[_Z_] / p / fac);
        h = p / cos(llh[_LAT_]) - n;
        if ( (fabs(h0 - h) < 1e-6) && (fabs(b0 - llh[_LAT_]) < 1e-11) )
        {
            break;
        }
    }
    llh[_LON_] = atan2(xyz[_Y_], xyz[_X_]);
    llh[_HEIGHT_] = h;
}

// ---------------------------------------------------------------------------------------------------------------------

void xyz2enu_vec(const double xyz[3], const double xyzRef[3], const double llhRef[3], double enu[3])
{
    // Error vector in cartesian
    //double d[3] = { xyzRef[_X_] - xyz[_X_], xyzRef[_Y_] - xyz[_Y_], xyzRef[_Z_] - xyz[_Z_] };
    double d[3] = { xyz[_X_] - xyzRef[_X_], xyz[_Y_] - xyzRef[_Y_], xyz[_Z_] - xyzRef[_Z_] };

    // Reference point on ellipsoid
    double _llhRef[3];
    if (llhRef == NULL)
    {
        xyz2llh_vec(xyz, _llhRef);
        llhRef = _llhRef;
    }

    // Transform to local tangetial plane
    // https://gssc.esa.int/navipedia/index.php/Transformations_between_ECEF_and_ENU_coordinates
    // https://en.wikipedia.org/wiki/Geographic_coordinate_conversion#From_ECEF_to_ENU
    //
    // | e |   |     -sin(lon)            cos(lon)          0      |   | Xp - Xr |
    // | n | = | -cos(lon)*sin(lat)  -sin(lon)*sin(lat)   cos(lat) | * | Yp - Yr |
    // | u |   |  cos(lon)*cos(lat)   sin(lon)*cos(lat)   sin(lat) |   | Zp - Zr |

    const double sinLat = sin(llhRef[_LAT_]);
    const double cosLat = cos(llhRef[_LAT_]);
    const double sinLon = sin(llhRef[_LON_]);
    const double cosLon = cos(llhRef[_LON_]);

    enu[_EAST_]  = (-sinLon          * d[_X_]) +  (cosLon          * d[_Y_]) /* + (0   * d[_Z_])*/;
    enu[_NORTH_] = (-cosLon * sinLat * d[_X_]) + (-sinLon * sinLat * d[_Y_]) + (cosLat * d[_Z_]);
    enu[_UP_]    =  (cosLon * cosLat * d[_X_]) +  (sinLon * cosLat * d[_Y_]) + (sinLat * d[_Z_]);
}

void enu2xyz_vec(const double enu[3], const double xyzRef[3], const double llhRef[3], double xyz[3])
{
    double _llhRef[3];
    if (llhRef == NULL)
    {
        xyz2llh_vec(xyz, _llhRef);
        llhRef = _llhRef;
    }

    // | Xp |   | -sin(lon)  -cos(lon)*sin(lat)   cos(lon)*cos(lat) |   | e |   | Xr |
    // | Yp | = |  cos(lon)  -sin(lon)*sin(lat)   sin(lon)*cos(lat) | * | n | + | Yr |
    // | Zp |   |     0           cos(lat)            sin(lat)      |   | u |   | Zr |

    const double sinLat = sin(llhRef[_LAT_]);
    const double cosLat = cos(llhRef[_LAT_]);
    const double sinLon = sin(llhRef[_LON_]);
    const double cosLon = cos(llhRef[_LON_]);

    xyz[_X_] = (-sinLon * enu[_EAST_]) + (-cosLon * sinLat * enu[_NORTH_]) + (cosLon * cosLat * enu[_UP_]) + xyzRef[_X_];
    xyz[_Y_] =  (cosLon * enu[_EAST_]) + (-sinLon * sinLat * enu[_NORTH_]) + (sinLon * cosLat * enu[_UP_]) + xyzRef[_Y_];
    xyz[_Z_] =  /*(0 * enu[_EAST_]) */ +           (cosLat * enu[_NORTH_]) +          (sinLat * enu[_UP_]) + xyzRef[_Z_];
}


void xyz2ned_vec(const double ned[3], const double llhRef[3], double xyz[3])
{
    const double sinLat = sin(llhRef[_LAT_]);
    const double cosLat = cos(llhRef[_LAT_]);
    const double sinLon = sin(llhRef[_LON_]);
    const double cosLon = cos(llhRef[_LON_]);
    xyz[_X_] = (-sinLat * cosLon * ned[0]) + (-sinLon * ned[1]) + (-cosLat * cosLon * ned[2]);
    xyz[_Y_] = (-sinLat * sinLon * ned[0]) +  (cosLon * ned[1]) + (-cosLat * sinLon * ned[2]);
    xyz[_Z_] =  (cosLat          * ned[0])                      + (-sinLat * ned[2]);
}

/* ****************************************************************************************************************** */

// gcc -o trafo_test ff_trafo.c -DFF_TRAFO_TEST -lm && ./trafo_test
#ifdef FF_TRAFO_TEST

#include <stdio.h>
#include <float.h>

#define TEST(descr, predicate) do { numTests++; \
        if (predicate) \
        { \
            numPass++; \
            if (verbosity > 0) { printf("%03d PASS %-30s %s [%s:%d]\n", numTests, descr, # predicate, __FILE__, __LINE__); } \
        } \
        else \
        { \
            numFail++; \
            printf("%03d FAIL %-30s %s [%s:%d]\n", numTests, descr, # predicate, __FILE__, __LINE__); \
        } \
    } while (0)

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    int numTests = 0;
    int numPass = 0;
    int numFail = 0;
    int verbosity = 1;

    {
        TEST("deg2rad(0.0)", fabs(deg2rad(0.0) - 0.0) < DBL_EPSILON);
        TEST("deg2rad(90.0)", fabs(deg2rad(90.0) - (M_PI/2.0) < DBL_EPSILON));
        TEST("deg2rad(180.0)", fabs(deg2rad(180.0) - M_PI < DBL_EPSILON));
        TEST("rad2deg(0.0)", fabs(rad2deg(0.0) - 0.0) < DBL_EPSILON);
        TEST("rad2deg(M_PI/2.0)", fabs(rad2deg(M_PI/2.0) - 90.0) < DBL_EPSILON);
        TEST("rad2deg(M_PI)", fabs(rad2deg(M_PI) - 180.0) < DBL_EPSILON);
    }

    {
        double x, y, z;
        llh2xyz_deg(47.3, 8.5, 550.0, &x, &y, &z);
        TEST("llh2xyz(47.3, 8.5, 550.0)", (fabs(x - 4286008.1) < 0.1) && (fabs(y - 640548.2) < 0.1) && (fabs(z - 4664851.1) < 0.1));
    }

    {
        double lat, lon, height;
        xyz2llh_deg(4286008.1, 640548.2, 4664851.1, &lat, &lon, &height);
        TEST("xyz2llh(4286008.1, 640548.2, 4664851.1)", (fabs(lat - 47.3) < 0.1) && (fabs(lon - 8.5) < 0.1) && (fabs(height - 550.0) < 0.1));
    }

    {
        const double xyzRef[3] = { 4286008.1,      640548.2,      4664851.1 };
        const double xyzTst[3] = { 4285916.105624, 640610.928009, 4664926.528159 };
        double enu[3];
        xyz2enu_vec(xyzRef, xyzRef, NULL, enu);
        TEST("xyz2enu(ref, ref)", (fabs(enu[0]) < DBL_EPSILON) && (fabs(enu[1]) < DBL_EPSILON) && (fabs(enu[2]) < DBL_EPSILON));
        xyz2enu_vec(xyzTst, xyzRef, NULL, enu);
        TEST("xyz2enu(tst, ref)", (fabs(enu[0] + 75.6) < 0.1) && (fabs(enu[1] + 111.2) < 0.1) && (fabs(enu[2] + 123.4) < 0.1));
    }

    printf("%d tests: %d passed, %d failed\n", numTests, numPass, numFail);
    return(numFail > 0 ? 1 : 0);
}

#endif // FF_TRAFO_TEST

/* ****************************************************************************************************************** */
// eof
