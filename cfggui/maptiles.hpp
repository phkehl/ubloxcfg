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

#ifndef __MAPTILES_H__
#define __MAPTILES_H__

#include <cinttypes>
#include <cmath>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <unordered_map>

/* ****************************************************************************************************************** */

struct MapParams
{
    MapParams();
    std::string               name;
    std::string               title;
    std::string               attribution;
    std::string               link;
    int                       zoomMin;
    int                       zoomMax;
    int                       tileSizeX;
    int                       tileSizeY;
    std::string               downloadUrl;
    std::vector<std::string>  subDomains;
    uint32_t                  downloadTimeout;
    std::string               cachePath;
    std::string               referer;
    double                    minLat; // S
    double                    maxLat; // N
    double                    minLon; // W
    double                    maxLon; // E
    uint32_t                  threads;
};

class MapTiles
{
    public:

        MapTiles(const MapParams &mapParams);
       ~MapTiles();

        void                 Loop(const double &now);

        void                *GetTileTex(const int tx, const int ty, const int tz);
        static const std::string &GetDebugText();

        static double        LatToTy(const double lat, const int tz); // latitude [rad] to tile coordinate y (real!)
        static double        LonToTx(const double lon, const int tz); // longitude [rad] to tile coordinate x (real!)
        static double        TyToLat(const double ty,  const int tz); // tile y coordinate (real!) to latitude [rad]
        static double        TxToLon(const double tx,  const int tz); // tile x coordinate (real!) to longitude [rad]

        static constexpr double kMinLat =  -85.0511 * (M_PI / 180.0);
        static constexpr double kMaxLat =   85.0511 * (M_PI / 180.0);
        static constexpr double kMinLon = -180.0    * (M_PI / 180.0);
        static constexpr double kMaxLon =  180.0    * (M_PI / 180.0);
        static constexpr double kWgs84a = 6378137.0;                   // Semi-major axis
        static constexpr double kWgs84c = 2 * M_PI * kWgs84a;          // Circumference at equator
        static constexpr int    kMaxtilesInQueue = 250;

        int                   NumTilesInQueue();

    protected:

        MapParams            _mapParams;

        // Tile download thread
        struct TileRequest
        {
            TileRequest(const std::string &_key, const int _tx, const int _ty, const int _tz);
            std::string       key;
            int               tx;
            int               ty;
            int               tz;
        };
        struct TileData
        {
            TileData(const std::string &_key, const int _width, const int _height, const uint8_t *_data, const int size);
            std::string          key;
            int                  width;
            int                  height;
            std::vector<uint8_t> data;
        };
        std::vector<std::unique_ptr<std::thread>> _requestThreads;
        std::atomic<bool>       _requestThreadAbort;
        std::mutex              _requestMutex;
        std::condition_variable _requestSem;
        std::queue<TileRequest> _requestQueue;
        std::mutex              _dataMutex;
        std::queue<TileData>    _dataQueue;
        std::atomic<int>        _subDomainIx;
        void                 _ThreadWrapper(std::string name);
        void                 _Thread(std::string name);
        bool                 _DownloadTile(const std::string &url, const std::filesystem::path &path, uint32_t timeout, const std::string &referer);
};

/* ****************************************************************************************************************** */
#endif // __MAPTILES_H__
