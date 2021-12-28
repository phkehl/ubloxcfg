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

#ifndef __GUI_WIDGET_OPENGL_HPP__
#define __GUI_WIDGET_OPENGL_HPP__

#include "imgui.h"

#include "opengl.hpp"

/* ****************************************************************************************************************** */

class GuiWidgetOpenGl
{
    public:
        GuiWidgetOpenGl();
       ~GuiWidgetOpenGl();

        // Setup and bind OpenGL frambuffer that you can draw into
        bool BeginDraw(const int width = 0, const int height = 0);

        // Get framebuffer size (after BeginDraw()!)
        FfVec2 GetSize();
        int    GetWidth();
        int    GetHeight();

        // Draw debugging (after BeginDraw()!)
        void *NanoVgBeginFrame(); // returns a NVGcontext *
        void NanoVgDebug();
        void NanoVgEndFrame();

        // Unbind framebuffer, switch back to default one
        void EndDraw();

    protected:

        int                 _width;
        int                 _height;
        OpenGL::FrameBuffer _framebuffer;
        void               *_nvgContext;
};

/* ****************************************************************************************************************** */

#endif // __GUI_WIDGET_OPENGL_HPP__
