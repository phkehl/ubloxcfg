// flipflip's (WGS84) transformation functions
//
// Copyright (c) 2020 Philippe Kehl (flipflip at oinkzwurgl dot org),
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

#ifndef __FF_TRAFO_H__
#define __FF_TRAFO_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ****************************************************************************************************************** */

double rad2deg(const double rad);

double deg2rad(const double deg);

void llh2xyz_vec(const double llh[3], double xyz[3]);
void llh2xyz_deg(const double lat, const double lon, const double height, double *x, double *y, double *z);
void llh2xyz_rad(const double lat, const double lon, const double height, double *x, double *y, double *z);

void xyz2llh_vec(const double xyz[3], double llh[3]);
void xyz2llh_deg(const double x, const double y, const double z, double *lat, double *lon, double *height);
void xyz2llh_rad(const double x, const double y, const double z, double *lat, double *lon, double *height);

void xyz2enu_vec(const double xyz[3], const double xyzRef[3], const double llhRef[3], double enu[3]);
void enu2xyz_vec(const double enu[3], const double xyzRef[3], const double llhRef[3], double xyz[3]);

/* ****************************************************************************************************************** */
#ifdef __cplusplus
}
#endif
#endif // __FF_TRAFO_H__
