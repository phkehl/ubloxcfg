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

#include "GL/gl3w.h"

#include "nanovg.h"
#include "nanovg_gl.h"

#include "gui_inc.hpp"
#include "gui_widget_opengl.hpp"

/* ****************************************************************************************************************** */

GuiWidgetOpenGl::GuiWidgetOpenGl() :
    _width           { 0 },
    _height          { 0 },
    _canDraw         { false },
    _framebuffer     { 0 },
    _renderbuffer    { 0 },
    _texture         { 0 },
    //_clearColour     { 0.2f, 0.2f, 0.2f, 1.0f }, // RGBA
    _clearColour     { 0.0f, 0.0f, 0.0f, 0.0f }, // RGBA
    _nvgContext      { nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES /*| NVG_DEBUG*/) }
{
    if (!_nvgContext)
    {
        WARNING("GuiWidgetOpenGl: NanoVG init fail!");
    }
}

GuiWidgetOpenGl::~GuiWidgetOpenGl()
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
    if (_nvgContext)
    {
        nvgDeleteGL3((NVGcontext *)_nvgContext);
        _nvgContext = nullptr;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

int GuiWidgetOpenGl::GetWidth()
{
    return _width;
}

// ---------------------------------------------------------------------------------------------------------------------

int GuiWidgetOpenGl::GetHeight()
{

    return _height;
}

// ---------------------------------------------------------------------------------------------------------------------

FfVec2 GuiWidgetOpenGl::GetSize()
{
    return FfVec2(_width, _height);
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetOpenGl::BeginDraw(const int width, const int height)
{
    // Use user-supplied size or get available space
    int w = width;
    int h = height;
    if ( (w <= 0) && (h <= 0) )
    {
        const auto sizeAvail = ImGui::GetContentRegionAvail();
        w = sizeAvail.x;
        h = sizeAvail.y;
    }

    // Limit drawing to some reasonably (?) small rect
    if ( (w < 10) || (h < 10) )
    {
        _width   = 0;
        _height  = 0;
        _canDraw = false;
        return false;
    }

    // https://learnopengl.com/Advanced-OpenGL/Framebuffers

    // Size changed? (Or first run.)
    if ( (w != _width) || (h != _height) )
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        // FIXME: hmmm...
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Create renderbuffer
        if (_renderbuffer != 0)
        {
            glDeleteRenderbuffers(1, &_renderbuffer);
        }
        glGenRenderbuffers(1, &_renderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _renderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        //DEBUG("GuiWidgetOpenGl: %dx%d, fb %u rb %u tex %u", w, h, _framebuffer, _renderbuffer, _texture);
    }

    // Activate framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _renderbuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        WARNING("GuiWidgetOpenGl: framebuffer not complete");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // FIXME: detach renderbuffer, too?
        return false;
    }

    // Set viewport, clear framebuffer
    glViewport(0, 0, w, h);
    glClearColor(_clearColour[0], _clearColour[1], _clearColour[2], _clearColour[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // We should be good to go
    _width   = w;
    _height  = h;
    _canDraw = true;

    // We could use nvgluCreateFramebuffer(), nvgluDeleteFramebuffer(), nvgluBindFramebuffer() ...

    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetOpenGl::EndDraw()
{
    if (!_canDraw)
    {
        return;
    }

    // Switch back to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Place rendered texture into ImGui draw list
    const FfVec2 pos0 = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddImage((void *)(uintptr_t)_texture,
        pos0, pos0 + FfVec2(_width, _height), ImVec2(0, 1), ImVec2(1, 0));
}

// ---------------------------------------------------------------------------------------------------------------------

void *GuiWidgetOpenGl::NanoVgBeginFrame()
{
    if (_nvgContext)
    {
        nvgBeginFrame((NVGcontext *)_nvgContext, _width, _height, 1.0);
    }
    return _nvgContext;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetOpenGl::NanoVgDebug()
{
    if (_nvgContext)
    {
        NVGcontext *vg = (NVGcontext *)_nvgContext;
        nvgBeginPath(vg);
        nvgMoveTo(vg, 1, 1);
        nvgLineTo(vg, _width - 2, 1);
        nvgLineTo(vg, _width - 2, _height - 2);
        nvgLineTo(vg, 1, _height - 2);
        nvgLineTo(vg, 1, 1);
        nvgLineTo(vg, _width - 2, _height - 2);
        nvgMoveTo(vg, 1, _height - 2);
        nvgLineTo(vg, _width - 2, 1);
        nvgStrokeColor(vg, nvgRGBAf(1.0, 0.0, 0.0, 0.6));
        //nvgStrokeColor(vg, nvgRGBA(255, 0, 0, 255));
        nvgStrokeWidth(vg, 3);
        nvgStroke(vg);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetOpenGl::NanoVgEndFrame()
{
    if (_nvgContext)
    {
        nvgEndFrame((NVGcontext *)_nvgContext);
    }
}

/* ****************************************************************************************************************** */
