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
#include <fstream>

#ifdef _WIN32
#  ifndef NOGDI
#    define NOGDI
#  endif
#endif

#include <curl/curl.h>
#include <GLFW/glfw3.h>

#ifndef _WIN32
#  include <sys/prctl.h>
#endif

#include "config.h"
#include "ff_debug.h"
#include "ff_stuff.h"
#include "ff_cpp.hpp"
#include "ff_trafo.h"
#include "stb_image.h"

#include "platform.hpp"

#include "maptiles.hpp"

/* ****************************************************************************************************************** */

MapParams::MapParams() :
    name{}, attribution{}, link{}, zoomMin{}, zoomMax{}, tileSizeX{}, tileSizeY{},
    downloadUrl{}, subDomains{}, downloadTimeout{}, cachePath{}, referer{},
    minLat{MapTiles::kMinLat}, maxLat{MapTiles::kMaxLat}, minLon{MapTiles::kMinLon}, maxLon{MapTiles::kMaxLon},
    threads{1}
{
}

/* ****************************************************************************************************************** */

// https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
// https://wiki.openstreetmap.org/wiki/Zoom_levels
// https://wiki.openstreetmap.org/wiki/Precision_of_coordinates

double MapTiles::LatToTy(const double lat, const int tz)
{
    return (1.0 - std::asinh(std::tan(lat)) / M_PI) / 2.0 * std::pow(2.0, tz);
}

double MapTiles::LonToTx(const double lon, const int tz)
{
    return (lon + M_PI) / (2.0 * M_PI) * pow(2, tz);
}

double MapTiles::TyToLat(const double ty,  const int tz)
{
    const double n = M_PI - (2.0 * M_PI) * (double)ty / std::pow(2.0, tz);
    return std::atan( 0.5 * (std::exp(n) - std::exp(-n)) );
}

double MapTiles::TxToLon(const double tx,  const int tz)
{
    return ( (double)tx / std::pow(2.0, tz) * (2.0 * M_PI) ) - M_PI;
}

/* ****************************************************************************************************************** */

// (OpenGL) texture
struct Texture
{
    Texture(const std::string &path);
    Texture(const uint8_t *pngData, const int pngSize);
    Texture(const int _width, const int _height, const std::vector<uint8_t> &_data);
    ~Texture();
    int          width;
    int          height;
    void        *id;
};

Texture::Texture(const std::string &path) :
    width{0}, height{0}, id{NULL}
{
    uint8_t *imageData = stbi_load(path.c_str(), &width, &height, NULL, 4);
    if (imageData == NULL)
    {
        WARNING("Failed loading texture (%s)", path.c_str());
        return;
    }
    GLuint imageTexture;
    glGenTextures(1, &imageTexture);
    glBindTexture(GL_TEXTURE_2D, imageTexture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    stbi_image_free(imageData);

    id = (void *)(uintptr_t)imageTexture;
    //DEBUG("MapTiles::Texture() %s %dx%d %p", path.c_str(), width, height, id);
}

Texture::Texture(const uint8_t *pngData, const int pngSize)
{
    uint8_t *imageData = stbi_load_from_memory(pngData, pngSize, &width, &height, NULL, 4);
    if (imageData == NULL)
    {
        WARNING("Failed loading texture");
        return;
    }

    GLuint imageTexture;
    glGenTextures(1, &imageTexture);
    glBindTexture(GL_TEXTURE_2D, imageTexture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    stbi_image_free(imageData);

    id = (void *)(uintptr_t)imageTexture;
}

Texture::Texture(const int _width, const int _height, const std::vector<uint8_t> &_data) :
    width{_width}, height{_height}, id{NULL}
{
    GLuint imageTexture;
    glGenTextures(1, &imageTexture);
    glBindTexture(GL_TEXTURE_2D, imageTexture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, _data.data());

    id = (void *)(uintptr_t)imageTexture;
    //DEBUG("MapTiles::Texture() %dx%d %p", width, height, id);
}

Texture::~Texture()
{
    //DEBUG("~MapTiles::Texture() %p", id);
    GLuint tex = (GLuint)(uintptr_t)id;
    glDeleteTextures(1, &tex);
}

// ---------------------------------------------------------------------------------------------------------------------

// Tile database
struct Tile
{
    Tile(const int _tx, const int _ty, const int _tz);
    int                    tx;
    int                    ty;
    int                    tz;
    enum State_e { LOADING, AVAILABLE, FAILED, OUTSIDE };
    enum State_e           state;
    uint32_t               lastUsed;
    std::unique_ptr<Texture> tex;
};

// ---------------------------------------------------------------------------------------------------------------------

std::unordered_map< std::string, Tile > _gTiles         {};
std::unique_ptr<Texture>                _gTileLoadTex   {nullptr};
std::unique_ptr<Texture>                _gTileFailTex   {nullptr};
std::unique_ptr<Texture>                _gTileNopeTex   {nullptr};
std::unique_ptr<Texture>                _gTileTestTex   {nullptr};

double                                  _gLastHousekeeping {0};
std::string                             _gDebugText {};
static constexpr double                 _kHousekeepingInterval = 1.0;
static constexpr uint32_t               _kTileTexMaxAge       =  60000;
static constexpr uint32_t               _kTileTexMaxAgeFailed = 300000;

/*static*/ const std::string &MapTiles::GetDebugText()
{
    const uint32_t now = TIME();
    _gDebugText.clear();
    _gDebugText += Ff::Sprintf("%d textures\n", (int)_gTiles.size());
    for (const auto &entry: _gTiles)
    {
        const auto &tile = entry.second;
        const uint32_t age = now - tile.lastUsed;
        const char *stateStr = "?";
        switch (tile.state)
        {
            case Tile::AVAILABLE: stateStr = "AVAI"; break;
            case Tile::LOADING:   stateStr = "LOAD"; break;
            case Tile::FAILED:    stateStr = "FAIL"; break;
            case Tile::OUTSIDE:   stateStr = "OUTS"; break;
        }
        _gDebugText += Ff::Sprintf("%-25s %s %6d\n", entry.first.c_str(), stateStr, age);
    }
    return _gDebugText;
}

/* ****************************************************************************************************************** */

MapTiles::MapTiles(const MapParams &mapParams) :
    _mapParams{mapParams}, _requestThreads{}, _requestThreadAbort{false}, _requestMutex{}, _requestSem{}, _requestQueue{}
{
    DEBUG("MapTiles()");

    // Load special tiles once
    if (!_gTileLoadTex)
    {
#include "tileload.c"
        _gTileLoadTex = std::make_unique<Texture>(cfggui_tileload_png, cfggui_tileload_png_len);
    }
    if (!_gTileFailTex)
    {
#include "tilefail.c"
        _gTileFailTex = std::make_unique<Texture>(cfggui_tilefail_png, cfggui_tilefail_png_len);
    }
    if (!_gTileNopeTex)
    {
#include "tilenope.c"
        _gTileNopeTex = std::make_unique<Texture>(cfggui_tilenope_png, cfggui_tilenope_png_len);
    }
    if (!_gTileTestTex)
    {
#include "tiletest.c"
        _gTileTestTex = std::make_unique<Texture>(cfggui_tiletest_png, cfggui_tiletest_png_len);
    }
    // Create threads
    if (_requestThreads.empty())
    {
        for (uint32_t n = 0; n < _mapParams.threads; n++)
        {
            std::string name = "tiles" + std::to_string(n);
            auto t = std::make_unique<std::thread>(&MapTiles::_ThreadWrapper, this, name);
            _requestThreads.push_back(std::move(t));
        }
    }
    _subDomainIx = 0;
}

MapTiles::~MapTiles()
{
    DEBUG("~MapTiles()");
    _requestThreadAbort = true;
    _requestSem.notify_all();
    for (auto &t: _requestThreads)
    {
        t->join();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void MapTiles::Loop(const double &now)
{
    // Housekeeping
    if ((now - _gLastHousekeeping) > _kHousekeepingInterval)
    {
        _gLastHousekeeping = now;
        if (_gTiles.size() > 0)
        {
            const uint32_t tilesNow = TIME();
            for (auto iter = _gTiles.begin(); iter != _gTiles.end(); )
            {
                auto &entry = *iter;
                auto &tile = entry.second;
                // Remove old tile from DB (and thereby free texture)
                switch (tile.state)
                {
                    case Tile::AVAILABLE:
                        if ((tilesNow - tile.lastUsed) > _kTileTexMaxAge)
                        {
                            iter = _gTiles.erase(iter);
                            continue;
                        }
                        break;
                    case Tile::OUTSIDE:
                    case Tile::FAILED:
                        if ((tilesNow - tile.lastUsed) > _kTileTexMaxAgeFailed)
                        {
                            iter = _gTiles.erase(iter);
                            continue;
                        }
                        //break;
                        break;
                    case Tile::LOADING:
                        break;
                }
                iter++;
            }
        }
    }

    // Load tile data into GPU textures
    while (!_dataQueue.empty())
    {
        std::unique_ptr<TileData> data = nullptr;

        {
            std::lock_guard<std::mutex> lock(_dataMutex);
            if (!_dataQueue.empty())
            {
                data = std::make_unique<TileData>( _dataQueue.front() );
                _dataQueue.pop();
            }
        }

        if (data)
        {
            auto entry = _gTiles.find(data->key);
            if (entry != _gTiles.end())
            {
                auto &tile = entry->second;
                if (data->data.empty())
                {
                    tile.state = Tile::FAILED;
                }
                else
                {
                    tile.tex = std::make_unique<Texture>(data->width, data->height, data->data);
                    tile.state = Tile::AVAILABLE;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

int MapTiles::NumTilesInQueue()
{
    return (int)_requestQueue.size();
}

// ---------------------------------------------------------------------------------------------------------------------

void *MapTiles::GetTileTex(const int tx, const int ty, const int tz)
{
    if ( (tz < _mapParams.zoomMin) || (tz > _mapParams.zoomMax) )
    {
        return _gTileNopeTex ? _gTileNopeTex->id : NULL;
    }

    // Built-in tiles
    if (_mapParams.downloadUrl.substr(0, 10) == "builtin://")
    {
        if (_mapParams.downloadUrl.substr(10) == "tiletest.png")
        {
            return _gTileTestTex->id;
        }
        else if (_mapParams.downloadUrl.substr(10) == "tilenope.png")
        {
            return _gTileNopeTex->id;
        }
        else if (_mapParams.downloadUrl.substr(10) == "tilefail.png")
        {
            return _gTileFailTex->id;
        }
        else if (_mapParams.downloadUrl.substr(10) == "tileload.png")
        {
            return _gTileLoadTex->id;
        }
        else
        {
            return _gTileFailTex->id;
        }
    }

    std::string key = _mapParams.name + "-" + std::to_string(tz) + "-" + std::to_string(ty) + "-" + std::to_string(tx);

    // See if we have this already
    auto entry = _gTiles.find(key);
    if (entry != _gTiles.end())
    {
        auto &tile = entry->second;
        switch (tile.state)
        {
            case Tile::AVAILABLE:
                tile.lastUsed = TIME();
                return tile.tex->id;
            case Tile::LOADING:
                return _gTileLoadTex ? _gTileLoadTex->id : NULL;
            case Tile::FAILED:
                return _gTileFailTex ? _gTileFailTex->id : NULL;
            case Tile::OUTSIDE:
                return _gTileNopeTex ? _gTileNopeTex->id : NULL;
        }
    }

    // Create new entry in the list of tiles
    auto ee = _gTiles.insert( { key, Tile(ty, ty, tz) } );

    // Check if it is within range
    const double minLat = MapTiles::TyToLat(ty + 1.0, tz);
    const double maxLat = MapTiles::TyToLat(ty, tz);
    const double minLon = MapTiles::TxToLon(tx, tz);
    const double maxLon = MapTiles::TxToLon(tx + 1.0, tz);

    // Request tile to be loaded (if tile rect overlaps with coverage rect)
    if ( (minLon < _mapParams.maxLon) && (maxLon > _mapParams.minLon) && (maxLat > _mapParams.minLat) && (minLat < _mapParams.maxLat) )
    {
        std::lock_guard<std::mutex> lock(_requestMutex);
        _requestQueue.push(TileRequest(key, tx, ty, tz));
        _requestSem.notify_one();
    }
    // Mark as outside of map coverage
    else
    {
        auto &tile = ee.first->second;
        tile.state = Tile::OUTSIDE;
        return _gTileNopeTex ? _gTileNopeTex->id : NULL;
    }

    // Use the "loading" tile until the thread has loaded the tile (or failed doing so)
    return _gTileLoadTex ? _gTileLoadTex->id : NULL;
}

// ---------------------------------------------------------------------------------------------------------------------

Tile::Tile(const int _tx, const int _ty, const int _tz) :
    tx{_tx}, ty{_ty}, tz{_tz}, state{LOADING}, lastUsed{TIME()}
{
    //DEBUG("MapTiles::Tile() %d %d %d", tx, ty, tz);
}

/* ****************************************************************************************************************** */

MapTiles::TileRequest::TileRequest(const std::string &_key, const int _tx, const int _ty, const int _tz) :
    key{_key}, tx{_tx}, ty{_ty}, tz{_tz}
{
}

// ---------------------------------------------------------------------------------------------------------------------

MapTiles::TileData::TileData(const std::string &_key, const int _width, const int _height, const uint8_t *_data, const int size) :
    key{_key}, width{_width}, height{_height}, data{_data, _data + size}
{
}

// ---------------------------------------------------------------------------------------------------------------------

void MapTiles::_ThreadWrapper(std::string name)
{
#ifndef _WIN32
    char currName[16];
    char threadName[32]; // max 16 cf. prctl(2)
    currName[0] = '\0';
    if (prctl(PR_GET_NAME, currName, 0, 0, 0) == 0)
    {
        std::snprintf(threadName, sizeof(threadName), "%s:%s", currName, name.c_str());
        prctl(PR_SET_NAME, threadName, 0, 0, 0);
    }
#endif
    DEBUG("thread %s (%p) started", name.c_str(), this);
    try
    {
        _Thread(name);
    }
    catch (const std::exception &e)
    {
        ERROR("thread %s (%p) crashed: %s", name.c_str(), this, e.what());
    }

    DEBUG("thread %s (%p) stopped", name.c_str(), this);
}

// ---------------------------------------------------------------------------------------------------------------------

void MapTiles::_Thread(std::string name)
{
    while (!_requestThreadAbort)
    {
        std::unique_ptr<MapTiles::TileRequest> req = nullptr;

        // Get next request
        {
            std::lock_guard<std::mutex> lock(_requestMutex);
            if (!_requestQueue.empty())
            {
                if ((int)_requestQueue.size() > kMaxtilesInQueue)
                {
                    WARNING("Too many tiles requested!");
                    while ((int)_requestQueue.size() > kMaxtilesInQueue)
                    {
                        req = std::make_unique<TileRequest>( _requestQueue.front() );
                        _requestQueue.pop();
                        _dataQueue.push(TileData(req->key, 0, 0, NULL, 0));
                    }
                }

                req = std::make_unique<TileRequest>( _requestQueue.front() );
                _requestQueue.pop();
            }
        }

        // Process request
        if (req)
        {
            std::string file = _mapParams.cachePath;
            Ff::StrReplace(file, "{x}", std::to_string(req->tx));
            Ff::StrReplace(file, "{y}", std::to_string(req->ty));
            Ff::StrReplace(file, "{z}", std::to_string(req->tz));

            // Full path to cache file
            std::filesystem::path path = Platform::CacheDir("tiles");
            path /= _mapParams.name;
            for (auto &part: Ff::StrSplit(file, "/"))
            {
                path /= part;
            }
            //DEBUG("thread %s request %s: %s", name.c_str(), req->key.c_str(), file.c_str());

            // Download
            bool tileOkay = true;
            if (!std::filesystem::exists(path))
            {
                // https://wiki.openstreetmap.org/wiki/Tile_servers
                std::string url = _mapParams.downloadUrl;
                Ff::StrReplace(url, "{x}", std::to_string(req->tx));
                Ff::StrReplace(url, "{y}", std::to_string(req->ty));
                Ff::StrReplace(url, "{z}", std::to_string(req->tz));
                if (_mapParams.subDomains.size() > 0)
                {
                    const int ix = _subDomainIx++ % _mapParams.subDomains.size();
                    Ff::StrReplace(url, "{s}", _mapParams.subDomains[ix]);
                }
                DEBUG("Download %s (%s)", url.c_str(), name.c_str());
                tileOkay = _DownloadTile(url, path, _mapParams.downloadTimeout, _mapParams.referer);
            }

            // Load and send data to main thread
            if (tileOkay && std::filesystem::exists(path))
            {
                //DEBUG("Load %s (%s)", path.c_str(), name.c_str());
                int width;
                int height;
                uint8_t *imageData = stbi_load(path.string().c_str(), &width, &height, NULL, 4);
                if (imageData != NULL)
                {
                    std::lock_guard<std::mutex> lock(_dataMutex);
                    _dataQueue.push(TileData(req->key, width, height, imageData, width * height * 4));
                    stbi_image_free(imageData);
                }
                else
                {
                    ERROR("Failed loading %s", path.string().c_str());
                    tileOkay = false;
                }
            }

            // Signal main thread that the tile is not available
            if (!tileOkay)
            {
                _dataQueue.push(TileData(req->key, 0, 0, NULL, 0));
            }

            // check if there are more requests
            continue;
        }

        // Wait for more requests
        {
            //DEBUG("thread %s waiting", name.c_str());
            std::unique_lock<std::mutex> lock(_requestMutex);
            _requestSem.wait(lock);
           // DEBUG("thread %s woke", name.c_str());
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

static size_t _downloadWriteCb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t totalSize = size * nmemb;
    std::vector<uint8_t> *vec = (std::vector<uint8_t> *)userdata;
    vec->insert(vec->end(), ptr, ptr + totalSize);
    return totalSize;
}

bool MapTiles::_DownloadTile(const std::string &url, const std::filesystem::path &path, uint32_t timeout, const std::string &referer)
{
    bool res = true;
    CURL *curl = NULL;
    FILE *file = NULL;
    try
    {
        // Create destination directory
        std::filesystem::path dir = path.parent_path();
        std::filesystem::create_directories(dir);

        // Buffer to download the image data into
        std::vector<uint8_t> data {};

        // Setup curl
        // https://curl.haxx.se/libcurl/c/curl_easy_setopt.html
        curl = curl_easy_init();
        if ( (curl == NULL) ||
            (curl_easy_setopt(curl, CURLOPT_URL,            url.c_str())                                      != CURLE_OK) ||
          //(curl_easy_setopt(curl, CURLOPT_VERBOSE,        true)                                             != CURLE_OK) ||
            (curl_easy_setopt(curl, CURLOPT_USERAGENT ,     "cfggui/" CONFIG_VERSION " (" CONFIG_GITHASH ")") != CURLE_OK) ||
            ( !referer.empty() &&
            (curl_easy_setopt(curl, CURLOPT_REFERER,        referer.c_str())                                  != CURLE_OK)) ||
            (curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true)                                             != CURLE_OK) ||
            (curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,     (long)(timeout > 1000 ? timeout : 5000))          != CURLE_OK) ||
            (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  _downloadWriteCb)                                 != CURLE_OK) ||
            (curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &data)                                            != CURLE_OK) )
        {
            throw std::runtime_error("Curl init failed");
        }

        data.reserve(CURL_MAX_WRITE_SIZE);

        // Initiate download
        const CURLcode curlRes = curl_easy_perform(curl);
        if (curlRes != CURLE_OK)
        {
            throw std::runtime_error(Ff::Sprintf("Failed downloading %s: %s", url.c_str(), curl_easy_strerror(curlRes)));
        }

        // Check response code if HTTP(S)
        if (url.substr(0, 4) == "http")
        {
            long respCode = -1;
            if ( (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &respCode) != CURLE_OK) || (respCode != 200) )
            {
                throw std::runtime_error(Ff::Sprintf("Failed downloading %s: code %ld", url.c_str(), respCode));
            }
        }

        // Check if data is valid image data
        int width = 0;
        int height = 0;
        uint8_t *imageData = stbi_load_from_memory(data.data(), (int)data.size(), &width, &height, NULL, 4);
        if (imageData == NULL)
        {
            throw std::runtime_error(Ff::Sprintf("Failed downloading %s: no (known) image data", url.c_str()));
        }
        stbi_image_free(imageData);

        // Store data
        if (!Platform::FileSpew(path.string(), data.data(), data.size(), false))
        {
            res = false;
        }
    }
    catch (const std::exception &e)
    {
        ERROR("Tile download fail: %s", e.what());
        res = false;
    }

    if (curl != NULL)
    {
        curl_easy_cleanup(curl);
    }
    if (file != NULL)
    {
        fclose(file);
    }

    return res;
}

/* ****************************************************************************************************************** */
