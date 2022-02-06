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

namespace OpenGL {
/* ****************************************************************************************************************** */

ImageTexture::ImageTexture() :
    _width   { 0 },
    _height  { 0 },
    _texture { 0 }
{
}

ImageTexture::~ImageTexture()
{
    Destroy();
}

// ---------------------------------------------------------------------------------------------------------------------

void ImageTexture::Destroy()
{
    if (_texture != 0)
    {
        //DEBUG("Destroy() %u", _texture);
        glDeleteTextures(1, &_texture);
        _texture = 0;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

ImageTexture::ImageTexture(ImageTexture &&rhs) :
    ImageTexture()
{
    std::swap(_texture, rhs._texture);
}

ImageTexture &ImageTexture::operator=(ImageTexture &&rhs)
{
    std::swap(_texture, rhs._texture);
    return *this;
}

// ---------------------------------------------------------------------------------------------------------------------

ImageTexture::ImageTexture(const std::string &pngPath, const bool genMipmap) :
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

    //DEBUG("ImageTexture() %s %dx%d %u", pngPath.c_str(), _width, _height, _texture);
}

// ---------------------------------------------------------------------------------------------------------------------

ImageTexture::ImageTexture(const uint8_t *pngData, const int pngSize, const bool genMipmap) :
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

    //DEBUG("ImageTexture() png %dx%d %u", _width, _height, _texture);
}

// ---------------------------------------------------------------------------------------------------------------------

ImageTexture::ImageTexture(const int width, const int height, const std::vector<uint8_t> &rgbaData, const bool genMipmap) :
    _width   { width },
    _height  { height },
    _texture { _MakeTex(width, height, rgbaData.data(), genMipmap) }
{
    //DEBUG("ImageTexture() rgba %dx%d %u", _width, _height, _texture);
}

// ---------------------------------------------------------------------------------------------------------------------

ImageTexture::ImageTexture(const int width, const int height, const uint8_t *rgbaData, const bool genMipmap) :
    _width   { width },
    _height  { height },
    _texture { _MakeTex(width, height, rgbaData, genMipmap) }
{
}

// ---------------------------------------------------------------------------------------------------------------------

int ImageTexture::GetWidth()
{
    return _width;
}

// ---------------------------------------------------------------------------------------------------------------------

int ImageTexture::GetHeight()
{
    return _height;
}

// ---------------------------------------------------------------------------------------------------------------------

void *ImageTexture::GetImageTexture()
{
    return (void *)(uintptr_t)_texture;
}

uint32_t ImageTexture::GetTexture()
{
    return _texture;
}

// ---------------------------------------------------------------------------------------------------------------------

/* static */ uint32_t ImageTexture::_MakeTex(const int width, const int height, const uint8_t *data, const bool genMipmap)
{
    GLint lastTex;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTex);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    // Minifying/magnifying parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, genMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
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

FrameBuffer::FrameBuffer() :
    _width        { 0 },
    _height       { 0 },
    _fbok         { false },
    _framebuffer  { 0 },
    _renderbuffer { 0 },
    _texture      { 0 }
{
}

FrameBuffer::~FrameBuffer()
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

bool FrameBuffer::Begin(const int width, const int height)
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

        //DEBUG("FrameBuffer: %dx%d, fb %u rb %u tex %u", width, height, _framebuffer, _renderbuffer, _texture);
    }

    // Activate framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _renderbuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        WARNING("FrameBuffer: incomplete");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // FIXME: detach renderbuffer, too?
        return false;
    }

    // Set viewport
    glViewport(0, 0, width, height);

    // We should be good to go
    _fbok    = true;
    _width   = width;
    _height  = height;
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

void FrameBuffer::Clear(const float r, const float g, const float b, const float a)
{
    if (_fbok)
    {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void FrameBuffer::End()
{
    if (_fbok)
    {
        // Switch back to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void *FrameBuffer::GetTexture()
{
    return _fbok ? (void *)(uintptr_t)_texture : nullptr;
}

/* ****************************************************************************************************************** */

Shader::Shader(const char *vertexCode, const char *fragmentCode, const char *geometryCode) :
    _shaderProgram  { 0 }
{
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar **)&vertexCode, NULL);
    glCompileShader(vertexShader);
    if (!_CheckShader(vertexShader, "Shader() vertex"))
    {
        glDeleteShader(vertexShader);
        vertexShader = 0;
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar **)&fragmentCode, NULL);
    glCompileShader(fragmentShader);
    if (!_CheckShader(fragmentShader, "Shader() fragment"))
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
        if (!_CheckShader(geometryShader, "Shader() geometry"))
        {
            glDeleteShader(geometryShader);
            fragmentShader = 0;
        }
    }

    // Link shader program
    if ( (vertexShader != 0) && (fragmentShader != 0) && (!geometryCode || (geometryShader !=0)) )
    {
        _shaderProgram = glCreateProgram();
        DEBUG("Shader() vertex %u, fragment %u, geometry %u -> program %u",
            vertexShader, fragmentShader, geometryShader, _shaderProgram);
        glAttachShader(_shaderProgram, vertexShader);
        glAttachShader(_shaderProgram, fragmentShader);
        if (geometryShader != 0)
        {
            glAttachShader(_shaderProgram, geometryShader);
        }
        glLinkProgram(_shaderProgram);
        if (!_CheckProgram(_shaderProgram, "Shader() program"))
        {
            glDeleteProgram(_shaderProgram);
            _shaderProgram = 0;
        }
    }

    DEBUG("Shader() program %u", _shaderProgram);
}

// ---------------------------------------------------------------------------------------------------------------------

Shader::~Shader()
{
    DEBUG("~Shader() %u", _shaderProgram);
    if (_shaderProgram != 0)
    {
        glDeleteProgram(_shaderProgram);
        glUseProgram(0);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool Shader::Ready()
{
    return _shaderProgram != 0;
}
// ---------------------------------------------------------------------------------------------------------------------

void Shader::Use()
{
    if (_shaderProgram != 0)
    {
        glUseProgram(_shaderProgram);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void Shader::SetUniform(const char *name, const bool b)
{
    glUniform1i( glGetUniformLocation(_shaderProgram, name), b ? GL_TRUE : GL_FALSE );
}

void Shader::SetUniform(const char *name, const float f)
{
    glUniform1f( glGetUniformLocation(_shaderProgram, name), f );
}

void Shader::SetUniform(const char *name, const int32_t i)
{
    glUniform1i( glGetUniformLocation(_shaderProgram, name), i );
}

void Shader::SetUniform(const char *name, const uint32_t u)
{
    glUniform1ui( glGetUniformLocation(_shaderProgram, name), u );
}

void Shader::SetUniform(const char *name, const glm::vec2 &v)
{
    glUniform2fv( glGetUniformLocation(_shaderProgram, name), 1, glm::value_ptr(v) );
}

void Shader::SetUniform(const char *name, const glm::vec3 &v)
{
    glUniform3fv( glGetUniformLocation(_shaderProgram, name), 1, glm::value_ptr(v) );
}

void Shader::SetUniform(const char *name, const glm::vec4 &v)
{
    glUniform4fv( glGetUniformLocation(_shaderProgram, name), 1, glm::value_ptr(v) );
}

void Shader::SetUniform(const char *name, const float x, const float y)
{
    glUniform2f( glGetUniformLocation(_shaderProgram, name), x, y );
}

void Shader::SetUniform(const char *name, const float x, const float y, const float z)
{
    glUniform3f( glGetUniformLocation(_shaderProgram, name), x, y, z );
}

void Shader::SetUniform(const char *name, const float x, const float y, const float z, const float w)
{
    glUniform4f( glGetUniformLocation(_shaderProgram, name), x, y, z, w );
}

void Shader::SetUniform(const char *name, const glm::mat2 &m)
{
    glUniformMatrix2fv( glGetUniformLocation(_shaderProgram, name), 1, GL_FALSE, glm::value_ptr(m) );
}

void Shader::SetUniform(const char *name, const glm::mat3 &m)
{
    glUniformMatrix3fv( glGetUniformLocation(_shaderProgram, name), 1, GL_FALSE, glm::value_ptr(m) );
}

void Shader::SetUniform(const char *name, const glm::mat4 &m)
{
    glUniformMatrix4fv( glGetUniformLocation(_shaderProgram, name), 1, GL_FALSE, glm::value_ptr(m) );
}

// ---------------------------------------------------------------------------------------------------------------------

bool Shader::_CheckShader(const uint32_t shader, const char *info)
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

bool Shader::_CheckProgram(const uint32_t program, const char *info)
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

Vertex::Vertex(const glm::vec3 &_pos, const glm::vec3 &_normal, const glm::vec4 &_colour, const glm::vec2 &_tex) :
    pos    { _pos },
    normal { _normal },
    colour { _colour },
    tex    { _tex }
{
}

Vertex::Vertex(const glm::vec3 &_pos, const glm::vec3 &_normal, const glm::vec2 &_tex) :
    pos    { _pos },
    normal { _normal },
    colour { 0.0f, 0.0f, 0.0f, 0.0f },
    tex    { _tex }
{
}

Vertex::Vertex(const glm::vec3 &_pos, const glm::vec2 &_tex) :
    pos    { _pos },
    normal { 0.0f, 0.0f, 0.0f },
    colour { 0.0f, 0.0f, 0.0f, 0.0f },
    tex    { _tex }
{
}

/* ****************************************************************************************************************** */

State::State() :
    depthTestEnable   { true },
    depthFunc         { GL_LESS },

    cullFaceEnable    { true },
    cullFace          { GL_BACK },
    cullFrontFace     { GL_CCW },

    polygonMode       { GL_FILL },

    blendEnable       { false },
    blendSrcRgb       { GL_ONE },
    blendDstRgb       { GL_ZERO },
    blendSrcAlpha     { GL_ONE },
    blendDstAlpha     { GL_ZERO },
    blendEqModeRgb    { GL_FUNC_ADD },
    blendEqModeAlpha  { GL_FUNC_ADD }
{
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ const std::vector<Enum> State::DEPTH_FUNC
{
    { GL_NEVER,    "GL_NEVER"    },
    { GL_LESS,     "GL_LESS"     },
    { GL_EQUAL,    "GL_EQUAL"    },
    { GL_LEQUAL,   "GL_LEQUAL"   },
    { GL_GREATER,  "GL_GREATER"  },
    { GL_NOTEQUAL, "GL_NOTEQUAL" },
    { GL_GEQUAL,   "GL_GEQUAL"   },
    { GL_ALWAYS,   "GL_ALWAYS"   },
};

/*static*/ const std::vector<Enum> State::CULL_FACE
{
    { GL_FRONT,          "GL_FRONT"          },
    { GL_BACK,           "GL_BACK"           },
    { GL_FRONT_AND_BACK, "GL_FRONT_AND_BACK" },
};

/*static*/ const std::vector<Enum> State::CULL_FRONTFACE
{
    { GL_CW,  "GL_CW"  },
    { GL_CCW, "GL_CCW" },
};

/*static*/ const std::vector<Enum> State::POLYGONMODE_MODE
{
    { GL_POINT,  "GL_POINT" },
    { GL_LINE,   "GL_LINE"  },
    { GL_FILL,   "GL_FILL"  },
};

/*static*/ const std::vector<Enum> State::BLENDFUNC_FACTOR
{
    { GL_ZERO,                     "GL_ZERO"                     },
    { GL_ONE,                      "GL_ONE"                      },
    { GL_SRC_COLOR,                "GL_SRC_COLOR"                },
    { GL_SRC_ALPHA,                "GL_SRC_ALPHA"                },
    { GL_SRC_ALPHA_SATURATE,       "GL_SRC_ALPHA_SATURATE"       },
    { GL_SRC1_COLOR,               "GL_SRC1_COLOR"               },
    { GL_SRC1_ALPHA,               "GL_SRC1_ALPHA"               },
    { GL_DST_COLOR,                "GL_DST_COLOR"                },
    { GL_DST_ALPHA,                "GL_DST_ALPHA"                },
    { GL_CONSTANT_ALPHA,           "GL_CONSTANT_ALPHA"           },
    { GL_CONSTANT_COLOR,           "GL_CONSTANT_COLOR"           },
    { GL_ONE_MINUS_SRC_COLOR,      "GL_ONE_MINUS_SRC_COLOR"      },
    { GL_ONE_MINUS_DST_COLOR,      "GL_ONE_MINUS_DST_COLOR"      },
    { GL_ONE_MINUS_SRC_ALPHA,      "GL_ONE_MINUS_SRC_ALPHA"      },
    { GL_ONE_MINUS_DST_ALPHA,      "GL_ONE_MINUS_DST_ALPHA"      },
    { GL_ONE_MINUS_CONSTANT_COLOR, "GL_ONE_MINUS_CONSTANT_COLOR" },
    { GL_ONE_MINUS_CONSTANT_ALPHA, "GL_ONE_MINUS_CONSTANT_ALPHA" },
    { GL_ONE_MINUS_SRC1_COLOR,     "GL_ONE_MINUS_SRC1_COLOR"     },
    { GL_ONE_MINUS_SRC1_ALPHA,     "GL_ONE_MINUS_SRC1_ALPHA"     },
};

/*static*/ const std::vector<Enum> State::BLENDEQ_MODE
{
    { GL_FUNC_ADD,              "GL_FUNC_ADD"              },
    { GL_FUNC_SUBTRACT,         "GL_FUNC_SUBTRACT"         },
    { GL_FUNC_REVERSE_SUBTRACT, "GL_FUNC_REVERSE_SUBTRACT" },
    { GL_MIN,                   "GL_MIN"                   },
    { GL_MAX,                   "GL_MAX"                   },
};

// ---------------------------------------------------------------------------------------------------------------------

void State::Apply()
{
    if (depthTestEnable)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(depthFunc);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    if (cullFaceEnable)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(cullFace);
        glFrontFace(cullFrontFace);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }

    glPolygonMode(GL_FRONT_AND_BACK, polygonMode);

    if (blendEnable)
    {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(blendSrcRgb, blendDstRgb, blendSrcAlpha, blendDstAlpha);
        glBlendEquationSeparate(blendEqModeRgb, blendEqModeAlpha);
    }
    else
    {
        glDisable(GL_BLEND);
    }

}

/* ****************************************************************************************************************** */

const char *GetGlErrorStr()
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
} // namespace OpenGL
