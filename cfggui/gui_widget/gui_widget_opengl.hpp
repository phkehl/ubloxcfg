/* ************************************************************************************************/ // clang-format off
// flipflip's cfggui
//
// Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org),
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


#include <glm/glm.hpp>

#include "imgui.h"

#include "opengl.hpp"

/* ****************************************************************************************************************** */

class GuiWidgetOpenGl
{
    public:
        GuiWidgetOpenGl();
       ~GuiWidgetOpenGl();

        // 1. Setup and bind OpenGL frambuffer that you can draw into, returns true if ready for drawing
        bool BeginDraw(const ImVec2 &size = ImVec2(0,0), const bool forceRender = false);

        // 2. if BeginDraw() returned true, optionally do:

        // Framebuffer changed and must be drawn again. We can skip redrawing if this returns false.
        bool StuffChanged();

        // Get framebuffer size
        ImVec2 GetSize();

        const glm::mat4 &GetViewMatrix();
        const glm::mat4 &GetProjectionMatrix();
        const glm::vec3 &GetAmbientLight();
        const glm::vec3 &GetDiffuseLight();
        const glm::vec3 &GetDiffuseDirection();

        // Clear framebuffer (automatically called when framebuffer changed)
        void Clear(const float r = 0.0f, const float g = 0.0f, const float b = 0.0f, const float a = 0.0f);

        // 3. if BeginDraw() returned true: put it on screen, unbind framebuffer, switch back to default one
        void EndDraw();

    protected:

        FfVec2f             _size;
        FfVec2f             _pos0;
        OpenGL::FrameBuffer _framebuffer;

        bool      _stuffChanged;

        bool      _controlsDebug;

        float     _cameraFieldOfView; // [deg]

        glm::vec3 _cameraLookAt;
        float     _cameraAzim; // 0 = -z direction
        float     _cameraElev; // 0 = horizon, >0 up
        float     _cameraDist;
        float     _cameraRoll; // 180 = horizontal

        glm::mat4 _view;       // Camera (view, eye) matrix: world -> view
        glm::mat4 _projection; // Projection matrix: view -> clip
        enum Projection_e { PERSPECTIVE = 0, ORTHOGRAPHIC = 1};
        int       _projType;
        float     _cullNear;
        float     _cullFar;

        OpenGL::State _glState;
        bool _forceRender;
        bool _StateEnumCombo(const char *label, uint32_t &value, const std::vector<OpenGL::Enum> enums);

        static constexpr float FIELD_OF_VIEW_MIN =  10.0f;
        static constexpr float FIELD_OF_VIEW_MAX = 100.0f;
        static constexpr float CAMERA_AZIM_MIN  =    0.0f;
        static constexpr float CAMERA_AZIM_MAX  =  360.0f;
        static constexpr float CAMERA_ELEV_MIN  =  -90.0f;
        static constexpr float CAMERA_ELEV_MAX  =   90.0f;
        static constexpr float CAMERA_ROLL_MIN  = -180.0f;
        static constexpr float CAMERA_ROLL_MAX  =  180.0f;
        static constexpr float CAMERA_DIST_MIN  =    1.0f;
        static constexpr float CAMERA_DIST_MAX  =  100.0f;

        int     _isDragging;
        FfVec2f _dragLastDelta;

        glm::vec3 _ambientLight;
        glm::vec3 _diffuseLight;
        glm::vec3 _diffuseDirection;

        bool _HandleMouse();
        bool _DrawDebugControls();
        void _UpdateViewAndProjection();
};

/* ****************************************************************************************************************** */

#endif // __GUI_WIDGET_OPENGL_HPP__
