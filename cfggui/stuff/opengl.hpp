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

#ifndef __OPENGL_HPP__
#define __OPENGL_HPP__

#include <vector>
#include <string>
#include <cstdint>

namespace OpenGL {
/* ****************************************************************************************************************** */

class ImageTexture
{
    public:
        ImageTexture();

        // Create OpenGL image texture from PNG data
        ImageTexture(const std::string &pngPath);
        ImageTexture(const uint8_t *pngData, const int pngSize);

        // Create OpenGL image texture from raw RGBA data
        ImageTexture(const int width, const int height, const std::vector<uint8_t> &rgbaData);

        int GetWidth();
        int GetHeight();
        void *GetImageTexture();

        // Destroy OpenGL image texture, actively or on destruct
        void Destroy();
       ~ImageTexture();

        // Must not copy texture (as that would call the destructor, which would remote the texture from the GPU)
        ImageTexture(ImageTexture const &) = delete;
        ImageTexture &operator=(ImageTexture const &) = delete;

        // But can move
        ImageTexture(ImageTexture &&rhs);
        ImageTexture &operator=(ImageTexture &&rhs);

    private:

        int          _width;
        int          _height;
        unsigned int _texture;

        static unsigned int _MakeTex(const int width, const int height, const uint8_t *data);
};


// ---------------------------------------------------------------------------------------------------------------------

class FrameBuffer
{
    public:
        FrameBuffer();
       ~FrameBuffer();

        bool Begin(const int width, const int height);
        void Clear(const float r = 0.0f, const float g = 0.0f, const float b = 0.0f, const float a = 0.0f);
        void End();
        void *GetTexture();

        static constexpr int MIN_WIDTH  = 10;
        static constexpr int MIN_HEIGHT = 10;

    private:

        int          _width;
        int          _height;
        bool         _fbok;
        unsigned int _framebuffer;
        unsigned int _renderbuffer;
        unsigned int _texture;
};

// ---------------------------------------------------------------------------------------------------------------------


const char *GetGlErrorStr();

/* ****************************************************************************************************************** */
} // namespace OpenGL
#endif // __OPENGL_HPP__
