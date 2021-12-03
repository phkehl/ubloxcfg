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

#ifndef __MAPPARAMS_HPP__
#define __MAPPARAMS_HPP__

#include <string>
#include <vector>
#include <cstdint>
#include <cmath>

/* ****************************************************************************************************************** */

struct MapParams
{
    bool                      enabled;
    std::string               name;
    std::string               title;
    std::string               attribution;
    std::string               attrLink;
    std::string               license;
    std::string               licenseLink;
    std::string               policy;
    std::string               policyLink;
    int                       zoomMin;
    int                       zoomMax;
    int                       tileSizeX;
    int                       tileSizeY;
    std::string               downloadUrl;
    std::vector<std::string>  subDomains;
    uint32_t                  downloadTimeout;
    std::string               referer;
    std::string               cachePath;
    double                    minLat; // S [rad]
    double                    maxLat; // N [rad]
    double                    minLon; // W [rad]
    double                    maxLon; // E [rad]
    uint32_t                  threads;

    static constexpr double MIN_LAT =  -85.0511 * (M_PI / 180.0);
    static constexpr double MAX_LAT =   85.0511 * (M_PI / 180.0);
    static constexpr double MIN_LON = -180.0    * (M_PI / 180.0);
    static constexpr double MAX_LON =  180.0    * (M_PI / 180.0);

    static const std::vector<MapParams> BUILTIN_MAPS;
};

/* ****************************************************************************************************************** */
#endif // __MAPPARAMS_HPP__
