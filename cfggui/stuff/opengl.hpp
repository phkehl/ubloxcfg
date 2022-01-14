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
#include <glm/glm.hpp>

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

        // No copy, no move
        FrameBuffer &operator=(const FrameBuffer&) = delete;
        FrameBuffer(const FrameBuffer&) = delete;
        FrameBuffer(FrameBuffer &&) = delete;
        FrameBuffer &operator=(FrameBuffer &&) = delete;

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

class Shader
{
    public:
        Shader(const char *vertexCode, const char *fragmentCode, const char *geometryCode = nullptr);
       ~Shader();

        bool Ready();
        void Use();

        void SetUniform(const char *name, const bool b);
        void SetUniform(const char *name, const float f);
        void SetUniform(const char *name, const int32_t i);
        void SetUniform(const char *name, const uint32_t u);
        void SetUniform(const char *name, const glm::vec2 &v);
        void SetUniform(const char *name, const glm::vec3 &v);
        void SetUniform(const char *name, const glm::vec4 &v);
        void SetUniform(const char *name, const float x, const float y);
        void SetUniform(const char *name, const float x, const float y, const float z);
        void SetUniform(const char *name, const float x, const float y, const float z, const float w);
        void SetUniform(const char *name, const glm::mat2 &m);
        void SetUniform(const char *name, const glm::mat3 &m);
        void SetUniform(const char *name, const glm::mat4 &m);

        // No copy, no move
        Shader &operator=(const Shader &) = delete;
        Shader(const Shader &) = delete;
        Shader(Shader &&) = delete;
        Shader &operator=(Shader &&) = delete;

    private:

        unsigned int _shaderProgram;

        bool _CheckShader(const unsigned int shader, const char *info);
        bool _CheckProgram(const unsigned int program, const char *info);

};

// ---------------------------------------------------------------------------------------------------------------------

struct Vertex
{
    Vertex(const glm::vec3 &_pos    = { 0.0f, 0.0f, 0.0f },
           const glm::vec3 &_normal = { 0.0f, 0.0f, 0.0f },
           const glm::vec4 &_colour = { 0.0f, 0.0f, 0.0f, 1.0f });
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec4 colour;
    static constexpr uint32_t POS_OFFS    = 0;
    static constexpr int32_t  POS_NUM     = 3;
    static constexpr uint32_t NORMAL_OFFS = sizeof(pos);
    static constexpr int32_t  NORMAL_NUM  = 3;
    static constexpr uint32_t COLOUR_OFFS = sizeof(pos) + sizeof(normal);
    static constexpr int32_t  COLOUR_NUM  = 4;
    static constexpr uint32_t SIZE        = sizeof(pos) + sizeof(normal) + sizeof(colour);
};

// ---------------------------------------------------------------------------------------------------------------------

const char *GetGlErrorStr();

/* ****************************************************************************************************************** */
} // namespace OpenGL
#endif // __OPENGL_HPP__
