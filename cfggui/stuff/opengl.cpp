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

#include <glm/gtc/type_ptr.hpp>

#include "GL/gl3w.h"

#include "ff_debug.h"
#include "ff_utils.hpp"
#include "stb_image.h"

#include "opengl.hpp"

/*

Notes

- https://learnopengl.com/Getting-Started/OpenGL
 https://docs.gl
- https://www.opengl-tutorial.org

-  https://www.khronos.org/registry/OpenGL/specs/gl/glspec33.core.pdf

- https://www.khronos.org/opengl/wiki/Rendering_Pipeline_Overview
- https://www.khronos.org/opengl/wiki/Shader
- https://www.khronos.org/opengl/wiki/Vertex_Shader
- https://www.khronos.org/opengl/wiki/Vertex_Specification
- https://www.khronos.org/opengl/wiki/Primitive
- https://www.khronos.org/opengl/wiki/Buffer_Object
- http://www.g-truc.net/
- https://github.com/rswinkle/opengl_reference.git
- http://www.songho.ca/opengl/gl_projectionmatrix.html


- transformation
    local --[model]-> world --[view]-> view/camera space --[project]-> clip space --viewport--> screen space

- rotate and translate
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, cubePositions[i]);
    model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

*/

/* ****************************************************************************************************************** */

OpenGL::ImageTexture::ImageTexture() :
    _width   { 0 },
    _height  { 0 },
    _texture { 0 }
{
}

OpenGL::ImageTexture::~ImageTexture()
{
    Destroy();
}

// ---------------------------------------------------------------------------------------------------------------------

void  OpenGL::ImageTexture::Destroy()
{
    if (_texture != 0)
    {
        //DEBUG("OpenGL::Destroy() %u", _texture);
        glDeleteTextures(1, &_texture);
        _texture = 0;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

OpenGL::ImageTexture::ImageTexture(OpenGL::ImageTexture &&rhs) :
    ImageTexture()
{
    std::swap(_texture, rhs._texture);
}

OpenGL::ImageTexture &OpenGL::ImageTexture::operator=(OpenGL::ImageTexture &&rhs)
{
    std::swap(_texture, rhs._texture);
    return *this;
}

// ---------------------------------------------------------------------------------------------------------------------

OpenGL::ImageTexture::ImageTexture(const std::string &pngPath, const bool genMipmap) :
    ImageTexture()
{
    int width = 0;
    int height = 0;
    uint8_t *data = stbi_load(pngPath.c_str(), &width, &height, NULL, 4);
    if (data == NULL)
    {
        WARNING("Failed loading texture (%s)", pngPath.c_str());
        return;
    }

    _texture = _MakeTex(width, height, data, genMipmap);
    _width   = width;
    _height  = height;

    stbi_image_free(data);

    //DEBUG("OpenGL::ImageTexture() %s %dx%d %u", pngPath.c_str(), _width, _height, _texture);
}

// ---------------------------------------------------------------------------------------------------------------------

OpenGL::ImageTexture::ImageTexture(const uint8_t *pngData, const int pngSize, const bool genMipmap) :
    ImageTexture()
{
    int width = 0;
    int height = 0;
    uint8_t *data = stbi_load_from_memory(pngData, pngSize, &width, &height, NULL, 4);
    if (data == NULL)
    {
        WARNING("Failed loading texture");
        return;
    }

    _texture = _MakeTex(width, height, data, genMipmap);
    _width   = width;
    _height  = height;

    stbi_image_free(data);

    //DEBUG("OpenGL::ImageTexture() png %dx%d %u", _width, _height, _texture);
}

// ---------------------------------------------------------------------------------------------------------------------

OpenGL::ImageTexture::ImageTexture(const int width, const int height, const std::vector<uint8_t> &rgbaData, const bool genMipmap) :
    _width   { width },
    _height  { height },
    _texture { _MakeTex(width, height, rgbaData.data(), genMipmap) }
{
    //DEBUG("OpenGL::ImageTexture() rgba %dx%d %u", _width, _height, _texture);
}

// ---------------------------------------------------------------------------------------------------------------------

int OpenGL::ImageTexture::GetWidth()
{
    return _width;
}

// ---------------------------------------------------------------------------------------------------------------------

int OpenGL::ImageTexture::GetHeight()
{
    return _height;
}

// ---------------------------------------------------------------------------------------------------------------------

void *OpenGL::ImageTexture::GetImageTexture()
{
    return (void *)(uintptr_t)_texture;
}

unsigned int OpenGL::ImageTexture::GetTexture()
{
    return _texture;
}

// ---------------------------------------------------------------------------------------------------------------------

/* static */ unsigned int OpenGL::ImageTexture::_MakeTex(const int width, const int height, const uint8_t *data, const bool genMipmap)
{
    GLint lastTex;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTex);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    // Minifying/magnifying parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    if (genMipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, lastTex);

    return tex;
}

/* ****************************************************************************************************************** */

OpenGL::FrameBuffer::FrameBuffer() :
    _width        { 0 },
    _height       { 0 },
    _fbok         { false },
    _framebuffer  { 0 },
    _renderbuffer { 0 },
    _texture      { 0 }
{
}

OpenGL::FrameBuffer::~FrameBuffer()
{
    if (_framebuffer != 0)
    {
        glDeleteFramebuffers(1, &_framebuffer);
        _framebuffer = 0;
    }
    if (_renderbuffer != 0)
    {
        glDeleteRenderbuffers(1, &_renderbuffer);
    }
    if (_texture != 0)
    {
        glDeleteTextures(1, &_texture);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool OpenGL::FrameBuffer::Begin(const int width, const int height)
{
    _fbok = false;

    if ( (width < MIN_WIDTH) || (height < MIN_HEIGHT) )
    {
        // _width  = 0;
        // _height = 0;
        return false;
    }

    // Size changed? (Or first run.)
    if ( (width != _width) || (height != _height) )
    {
        // TODO error handling?

        // Create framebuffer
        if (_framebuffer == 0)
        {
            glGenFramebuffers(1, &_framebuffer);
        }

        // Create texture
        if (_texture != 0)
        {
            glDeleteTextures(1, &_texture);
        }
        glGenTextures(1, &_texture);
        glBindTexture(GL_TEXTURE_2D, _texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Create renderbuffer
        if (_renderbuffer != 0)
        {
            glDeleteRenderbuffers(1, &_renderbuffer);
        }
        glGenRenderbuffers(1, &_renderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _renderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        //DEBUG("OpenGL::FrameBuffer: %dx%d, fb %u rb %u tex %u", width, height, _framebuffer, _renderbuffer, _texture);
    }

    // Activate framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _renderbuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        WARNING("OpenGL::FrameBuffer: incomplete");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // FIXME: detach renderbuffer, too?
        return false;
    }

    // Set viewport
    glViewport(0, 0, width, height);

    //glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LESS);
    //glEnable(GL_CULL_FACE);

    // We should be good to go
    _fbok    = true;
    _width   = width;
    _height  = height;
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

void OpenGL::FrameBuffer::Clear(const float r, const float g, const float b, const float a)
{
    if (_fbok)
    {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void OpenGL::FrameBuffer::End()
{
    if (_fbok)
    {
        // Switch back to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void *OpenGL::FrameBuffer::GetTexture()
{
    return _fbok ? (void *)(uintptr_t)_texture : nullptr;
}

/* ****************************************************************************************************************** */

OpenGL::Shader::Shader(const char *vertexCode, const char *fragmentCode, const char *geometryCode) :
    _shaderProgram  { 0 }
{
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar **)&vertexCode, NULL);
    glCompileShader(vertexShader);
    if (!_CheckShader(vertexShader, "OpenGL::Shader() vertex"))
    {
        glDeleteShader(vertexShader);
        vertexShader = 0;
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar **)&fragmentCode, NULL);
    glCompileShader(fragmentShader);
    if (!_CheckShader(fragmentShader, "OpenGL::Shader() fragment"))
    {
        glDeleteShader(fragmentShader);
        fragmentShader = 0;
    }

    // Compile geometry shader
    GLuint geometryShader = 0;
    if (geometryCode)
    {
        geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometryShader, 1, (const GLchar **)&geometryCode, NULL);
        glCompileShader(geometryShader);
        if (!_CheckShader(geometryShader, "OpenGL::Shader() geometry"))
        {
            glDeleteShader(geometryShader);
            fragmentShader = 0;
        }
    }

    // Link shader program
    if ( (vertexShader != 0) && (fragmentShader != 0) && (!geometryCode || (geometryShader !=0)) )
    {
        _shaderProgram = glCreateProgram();
        DEBUG("OpenGL::Shader() vertex %u, fragment %u, geometry %u -> program %u",
            vertexShader, fragmentShader, geometryShader, _shaderProgram);
        glAttachShader(_shaderProgram, vertexShader);
        glAttachShader(_shaderProgram, fragmentShader);
        if (geometryShader != 0)
        {
            glAttachShader(_shaderProgram, geometryShader);
        }
        glLinkProgram(_shaderProgram);
        if (!_CheckProgram(_shaderProgram, "OpenGL::Shader() program"))
        {
            glDeleteProgram(_shaderProgram);
            _shaderProgram = 0;
        }
    }

    DEBUG("OpenGL::Shader() program %u", _shaderProgram);
}

// ---------------------------------------------------------------------------------------------------------------------

OpenGL::Shader::~Shader()
{
    DEBUG("OpenGL::~Shader() %u", _shaderProgram);
    if (_shaderProgram != 0)
    {
        glDeleteProgram(_shaderProgram);
        glUseProgram(0);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool OpenGL::Shader::Ready()
{
    return _shaderProgram != 0;
}
// ---------------------------------------------------------------------------------------------------------------------

void OpenGL::Shader::Use()
{
    if (_shaderProgram != 0)
    {
        glUseProgram(_shaderProgram);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void OpenGL::Shader::SetUniform(const char *name, const bool b)
{
    glUniform1i( glGetUniformLocation(_shaderProgram, name), b ? GL_TRUE : GL_FALSE );
}

void OpenGL::Shader::SetUniform(const char *name, const float f)
{
    glUniform1f( glGetUniformLocation(_shaderProgram, name), f );
}

void OpenGL::Shader::SetUniform(const char *name, const int32_t i)
{
    glUniform1i( glGetUniformLocation(_shaderProgram, name), i );
}

void OpenGL::Shader::SetUniform(const char *name, const uint32_t u)
{
    glUniform1ui( glGetUniformLocation(_shaderProgram, name), u );
}

void OpenGL::Shader::SetUniform(const char *name, const glm::vec2 &v)
{
    glUniform2fv( glGetUniformLocation(_shaderProgram, name), 1, glm::value_ptr(v) );
}

void OpenGL::Shader::SetUniform(const char *name, const glm::vec3 &v)
{
    glUniform3fv( glGetUniformLocation(_shaderProgram, name), 1, glm::value_ptr(v) );
}

void OpenGL::Shader::SetUniform(const char *name, const glm::vec4 &v)
{
    glUniform4fv( glGetUniformLocation(_shaderProgram, name), 1, glm::value_ptr(v) );
}

void OpenGL::Shader::SetUniform(const char *name, const float x, const float y)
{
    glUniform2f( glGetUniformLocation(_shaderProgram, name), x, y );
}

void OpenGL::Shader::SetUniform(const char *name, const float x, const float y, const float z)
{
    glUniform3f( glGetUniformLocation(_shaderProgram, name), x, y, z );
}

void OpenGL::Shader::SetUniform(const char *name, const float x, const float y, const float z, const float w)
{
    glUniform4f( glGetUniformLocation(_shaderProgram, name), x, y, z, w );
}

void OpenGL::Shader::SetUniform(const char *name, const glm::mat2 &m)
{
    glUniformMatrix2fv( glGetUniformLocation(_shaderProgram, name), 1, GL_FALSE, glm::value_ptr(m) );
}

void OpenGL::Shader::SetUniform(const char *name, const glm::mat3 &m)
{
    glUniformMatrix3fv( glGetUniformLocation(_shaderProgram, name), 1, GL_FALSE, glm::value_ptr(m) );
}

void OpenGL::Shader::SetUniform(const char *name, const glm::mat4 &m)
{
    glUniformMatrix4fv( glGetUniformLocation(_shaderProgram, name), 1, GL_FALSE, glm::value_ptr(m) );
}

// ---------------------------------------------------------------------------------------------------------------------

bool OpenGL::Shader::_CheckShader(const unsigned int shader, const char *info)
{
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE)
    {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        if (length <= 0)
        {
            ERROR("%s", info);
        }
        else
        {
            GLchar log[length];
            log[0] = '\0';
            glGetShaderInfoLog(shader, length, NULL, log);

            const auto lines = Ff::StrSplit(log, "\n");
            for (auto &line: lines)
            {
                if (!line.empty())
                {
                    ERROR("%s:%s", info, line.c_str());
                }
            }
        }
    }

    return success == GL_TRUE;
}

bool OpenGL::Shader::_CheckProgram(const unsigned int program, const char *info)
{
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success != GL_TRUE)
    {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        if (length <= 0)
        {
            ERROR("%s", info);
        }
        else
        {
            GLchar log[length];
            log[0] = '\0';
            glGetProgramInfoLog(program, length, NULL, log);

            const auto lines = Ff::StrSplit(log, "\n");
            for (auto &line: lines)
            {
                if (!line.empty())
                {
                    ERROR("%s:%s", info, line.c_str());
                }
            }
        }
    }

    return success == GL_TRUE;
}

// ---------------------------------------------------------------------------------------------------------------------

OpenGL::Vertex::Vertex(const glm::vec3 &_pos, const glm::vec3 &_normal, const glm::vec4 &_colour) :
    pos    { _pos },
    normal { _normal },
    colour { _colour }
{
}

/* ****************************************************************************************************************** */

const char *OpenGL::GetGlErrorStr()
{
    const GLenum err = glGetError();
    switch (err)
    {
        case GL_NO_ERROR:                      return "GL_NO_ERROR";
        case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
        case GL_STACK_OVERFLOW:                return "GL_STACK_OVERFLOW";
        case GL_STACK_UNDERFLOW:               return "GL_STACK_UNDERFLOW";
        case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_CONTEXT_LOST:                  return "GL_CONTEXT_LOST";
        //case GL_TABLE_TOO_LARGE:               return "GL_TABLE_TOO_LARGE";
    }
    return "???";
}

/* ****************************************************************************************************************** */
