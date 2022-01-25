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

#ifndef __MATRIX_HPP__
#define __MATRIX_HPP__

#include "opengl.hpp"

/* ****************************************************************************************************************** */

class Matrix
{
    public:

        Matrix();
       ~Matrix();

       void *Render(const float width, const float height);

    private:
        OpenGL::FrameBuffer  _frameBuffer;
        OpenGL::Shader       _shader;
        OpenGL::ImageTexture _fontTex;

        unsigned int _glyphVertexArray;
        unsigned int _glyphVertexBuffer;

        static const char   *VERTEX_SHADER_SRC;
        static const char   *FRAGMENT_SHADER_SRC;
        static const uint8_t FONT_PNG[125539];
};

/* ****************************************************************************************************************** */
#endif // __MATRIX_HPP__
