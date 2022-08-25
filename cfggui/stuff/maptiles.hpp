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

#ifndef __MAPTILES_HPP__
#define __MAPTILES_HPP__

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>
#include <atomic>
#include <condition_variable>
#include <unordered_map>
#include <cstdint>
#include <cinttypes>
#include <cmath>

#include "ff_utils.hpp"
#include "ff_thread.hpp"
#include "opengl.hpp"

#include "mapparams.hpp"

/* ****************************************************************************************************************** */

class MapTiles
{
    public:

        MapTiles(const MapParams &mapParams);
       ~MapTiles();

        void *GetTileTex(const int tx, const int ty, const int tz);

        // Lat/lon [rad] to tile coordinates y/x (real!)
        static Ff::Vec2<double> LonLatToTileXy(const Ff::Vec2<double> &lonLat, const int tz);
        static Ff::Vec2<float>  LonLatToTileXy(const Ff::Vec2<float>  &lonLat, const int tz);
        // Tile coordinates y/x (real!) to lat/lon [rad]
        static Ff::Vec2<double> TileXyToLonLat(const Ff::Vec2<double> &tXy, const int tz);
        static Ff::Vec2<float>  TileXyToLonLat(const Ff::Vec2<float>  &tXy, const int tz);

        static constexpr double WGS84_A = 6378137.0;                   // Semi-major axis
        static constexpr double WGS84_C = 2 * M_PI * WGS84_A;          // Circumference at equator
        static constexpr int    MAX_TILES_IN_QUEUE = 250;

        int NumTilesInQueue(); // Number of tiles in queue for (down)loading

        struct DebugStats
        {
            DebugStats();
            int numTiles;
            int numAvail;
            int numLoad;
            int numFail;
            int numOutside;
        };
        static const DebugStats &GetStats();

    protected:

        MapParams   _mapParams;
        std::size_t _mapParamsUid;

        // A tile coordinate, the request for the download thread as well as the key for the tile storage unordered_map
        struct Coord
        {
            Coord(const std::size_t _uid, const int _x, const int _y, const int _z);
            std::size_t uid;
            int         x; // x coord (0..(z**2)-1)
            int         y; // y coord (0..(z**2)-1)
            int         z; // z coord (0..z)
            bool operator==(const Coord &other) const;
        };

        struct CoordHash
        {
            std::size_t operator() (const Coord &coord) const;
        };

        // Tile download threads
        std::vector<std::unique_ptr<Ff::Thread>> _threads;
        std::mutex          _requestMutex;
        std::queue<Coord>   _requestQueue;
        std::atomic<int>    _subDomainIx;
        void _Thread(Ff::Thread *thread);
        bool _DownloadTile(const std::string &url, const std::string &path, uint32_t timeout, const std::string &referer);
        static size_t _DownloadWriteCb(char *ptr, size_t size, size_t nmemb, void *userdata);

        // Tile storage
        struct Tile
        {
            enum State_e { LOADING, AVAILABLE, FAILED, OUTSIDE };
            Tile(const State_e _state);
            Tile() : Tile(LOADING) {}
            enum State_e           state;
            uint32_t               lastUsed;
            std::string            path;
            OpenGL::ImageTexture   tex;
        };
        static std::unordered_map<Coord, Tile, CoordHash> _tiles;
        static std::mutex _tilesMutex;

        // Built-in tiles
        static const uint8_t TILE_LOAD_PNG[973];  // "tileload.png"
        static const uint8_t TILE_FAIL_PNG[2276]; // "tilefail.png"
        static const uint8_t TILE_NOPE_PNG[786];  // "tilenope.png"
        static const uint8_t TILE_TEST_PNG[914];  // "tiletest.png"
        static OpenGL::ImageTexture _tileLoadTex;
        static OpenGL::ImageTexture _tileFailTex;
        static OpenGL::ImageTexture _tileNopeTex;
        static OpenGL::ImageTexture _tileTestTex;
        void *_builtInTile; // Set to one of the above if the current map uses one single fixed tile (for debugging...)

        // Housekeeping (expiring old tiles, cleaning up when the last instance disappears)
        static std::string          _debugText;
        static uint32_t             _lastHousekeeping;
        static constexpr double     HOUSEKEEPING_INT    =   1234; // [ms]
        static constexpr uint32_t   TILE_MAX_AGE        =  60000; // 1 min
        static constexpr uint32_t   TILE_MAX_AGE_FAILED = 300000; // 5 min
        static int                  _numInstances;
        static DebugStats           _debugStats;
        void _Cleanup(const uint32_t now);
};

/* ****************************************************************************************************************** */
#endif // __MAPTILES_HPP__
