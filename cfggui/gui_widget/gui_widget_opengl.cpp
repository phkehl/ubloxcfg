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
    _width      { 0 },
    _height     { 0 },
    _nvgContext { nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES /*| NVG_DEBUG*/) }
{
    if (!_nvgContext)
    {
        WARNING("GuiWidgetOpenGl: NanoVG init fail!");
    }
}

GuiWidgetOpenGl::~GuiWidgetOpenGl()
{
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
    _width = w;
    _height = h;

    if (_framebuffer.Begin(w, h))
    {
        _framebuffer.Clear();
        return true;
    }
    else
    {
        return false;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetOpenGl::EndDraw()
{
    _framebuffer.End();

    // Place rendered texture into ImGui draw list
    const FfVec2 pos0 = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddImage(_framebuffer.GetTexture(),
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
