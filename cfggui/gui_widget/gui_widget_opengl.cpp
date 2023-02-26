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

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GL/gl3w.h"

#include "gui_inc.hpp"
#include "gui_widget_opengl.hpp"

/* ****************************************************************************************************************** */

GuiWidgetOpenGl::GuiWidgetOpenGl() :
    _stuffChanged      { true },
    _controlsDebug     { false },
    _cameraFieldOfView { 45.0f },
    _cameraLookAt      { 0.0f, 0.0f, 0.0f },
    _cameraAzim        { 25.0f },
    _cameraElev        { 45.0f },
    _cameraDist        { 20.0f },
    _cameraRoll        {  0.0f },
    _projType          { PERSPECTIVE },
    _cullNear          { 0.01 },
    _cullFar           { 1000.0 },
    _forceRender       { false },
    _isDragging        { ImGuiMouseButton_COUNT },
    _ambientLight      { 0.5f, 0.5f, 0.5f },
    _diffuseLight      { 0.5f, 0.5f, 0.5f },
    _diffuseDirection  { -0.5f, 0.8f, 0.0f }
{
    _UpdateViewAndProjection();
}

GuiWidgetOpenGl::~GuiWidgetOpenGl()
{
}

// ---------------------------------------------------------------------------------------------------------------------

ImVec2 GuiWidgetOpenGl::GetSize()
{
    return _size;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetOpenGl::BeginDraw(const ImVec2 &size, const bool forceRender)
{
    // Use user-supplied size or get available space
    ImVec2 newSize = size;
    if ( (newSize.x <= 0) || (newSize.y <= 0) )
    {
        const ImVec2 sizeAvail = ImGui::GetContentRegionAvail();
        if (newSize.x <= 0)
        {
            newSize.x = sizeAvail.x;
        }
        if (newSize.y <= 0)
        {
            newSize.y = sizeAvail.y;
        }
    }
    _pos0 = ImGui::GetCursorScreenPos();
    if ( (newSize.x != _size.x) || (newSize.y != size.y) )
    {
        _size = newSize;
        _UpdateViewAndProjection();
        _stuffChanged = true;
    }
    if (_forceRender || forceRender)
    {
        _stuffChanged = true;
    }

    // Activate OpenGL framebuffer to draw into
    if (_framebuffer.Begin(_size.x, _size.y))
    {
        // ImGui child window where we'll place the rendered framebuffer into
        ImGui::BeginChild(ImGui::GetID(this), _size, false, ImGuiWindowFlags_NoScrollbar);

        // Draw debug controls
        if (_controlsDebug && _DrawDebugControls())
        {
            _stuffChanged = true;
        }

        // Handle mouse interactions
        if (_HandleMouse())
        {
            _stuffChanged = true;
        }

        // Update stuff if stuff changed :)
        if (_stuffChanged)
        {
            _UpdateViewAndProjection();
            Clear();
        }

        // Setup OpenGL drawing
        _glState.Apply();

        ImGui::SetCursorScreenPos(_pos0);

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
    _stuffChanged = false;

    // Place rendered texture into ImGui draw list
    ImGui::GetWindowDrawList()->AddImage(_framebuffer.GetTexture(), _pos0, _pos0 + _size);

    ImGui::EndChild();
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetOpenGl::StuffChanged()
{
    return _stuffChanged;
}

// ---------------------------------------------------------------------------------------------------------------------

const glm::mat4 &GuiWidgetOpenGl::GetViewMatrix()
{
    return _view;
}

// ---------------------------------------------------------------------------------------------------------------------

const glm::mat4 &GuiWidgetOpenGl::GetProjectionMatrix()
{
    return _projection;
}

// ---------------------------------------------------------------------------------------------------------------------

const glm::vec3 &GuiWidgetOpenGl::GetAmbientLight()
{
    return _ambientLight;
}

// ---------------------------------------------------------------------------------------------------------------------

const glm::vec3 &GuiWidgetOpenGl::GetDiffuseLight()
{
    return _diffuseLight;
}

// ---------------------------------------------------------------------------------------------------------------------

const glm::vec3 &GuiWidgetOpenGl::GetDiffuseDirection()
{
    return _diffuseDirection;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetOpenGl::Clear(const float r, const float g, const float b, const float a)
{
    _framebuffer.Clear(r, g, b, a);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetOpenGl::_UpdateViewAndProjection()
{
    // Sanitise values
    while (_cameraAzim > CAMERA_AZIM_MAX)
    {
        _cameraAzim -= 360.0f;
    }
    while (_cameraAzim < CAMERA_AZIM_MIN)
    {
        _cameraAzim += 360.0f;
    }
    _cameraAzim = std::clamp(_cameraAzim, CAMERA_AZIM_MIN, CAMERA_AZIM_MAX);
    _cameraElev = std::clamp(_cameraElev, CAMERA_ELEV_MIN, CAMERA_ELEV_MAX);
    _cameraRoll = std::clamp(_cameraRoll, CAMERA_ROLL_MIN, CAMERA_ROLL_MAX);
    _cameraDist = std::clamp(_cameraDist, CAMERA_DIST_MIN, CAMERA_DIST_MAX);

    // Calculate view matrix, https://stannum.io/blog/0UaG8R
    glm::mat4 view {1.0f};
    view = glm::translate(view, _cameraLookAt);
    view = glm::rotate(view, glm::radians(_cameraRoll), glm::vec3(0.0f, 0.0f, 1.0f));
    view = glm::rotate(view, glm::radians(_cameraAzim), glm::vec3(0.0f, 1.0f, 0.0f));
    view = glm::rotate(view, glm::radians(_cameraElev), glm::vec3(1.0f, 0.0f, 0.0f));

    _diffuseDirection = glm::normalize(view * glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)); // TODO... hmmm...

    view = glm::translate(view, glm::vec3(0.0f, 0.0f, _cameraDist));
    _view = glm::inverse(view);

    // Projection matrix
    const float aspect = _size.y > 0.0f ? _size.x / _size.y : 1.0f;
    switch (_projType)
    {
        case PERSPECTIVE:
            _projection = glm::perspective(glm::radians(_cameraFieldOfView), aspect, _cullNear, _cullFar);
            break;
        case ORTHOGRAPHIC:
            _projection = glm::ortho(-_cameraDist * aspect, _cameraDist * aspect, -_cameraDist, _cameraDist, _cullNear, _cullFar);
            break;
    }

    // Light
    _diffuseDirection = glm::normalize(_diffuseDirection);
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetOpenGl::_HandleMouse()
{
    bool changed = false;

    // Make canvas area interactive
    ImGui::SetCursorScreenPos(_pos0);
    ImGui::InvisibleButton("canvas", _size, ImGuiButtonFlags_MouseButtonLeft);
    ImGui::SetItemAllowOverlap();
    const bool isHovered = ImGui::IsItemHovered();

    // Debug popup
    if (ImGui::BeginPopupContextItem("GuiWidgetOpenGl"))
    {
        ImGui::Checkbox("Debug", &_controlsDebug);
        ImGui::EndPopup();
    }

    auto &io = ImGui::GetIO();
    if (isHovered)
    {
        // Not dragging
        if (_isDragging == ImGuiMouseButton_COUNT)
        {
            // mouse wheel: forward/backwards
            if (io.MouseWheel != 0.0)
            {
                const float step = io.KeyShift ? 0.1 : (io.KeyCtrl ? 1.0 : 0.5);
                DEBUG("wheel %.1f %.1f", io.MouseWheel, step);
                _cameraDist -= (step * io.MouseWheel);
                changed = true;
            }

            // start of dragging
            else if (io.MouseClicked[ImGuiMouseButton_Left])
            {
                _isDragging = ImGuiMouseButton_Left;
                _dragLastDelta = FfVec2f(0,0);
            }
            else if (io.MouseClicked[ImGuiMouseButton_Middle])
            {
                _isDragging = ImGuiMouseButton_Right;
                _dragLastDelta = FfVec2f(0,0);
            }
            else if (io.MouseClicked[ImGuiMouseButton_Right])
            {
                _isDragging = ImGuiMouseButton_Right;
                _dragLastDelta = FfVec2f(0,0);
            }
        }
        // Dragging
        else
        {
            if (_isDragging == ImGuiMouseButton_Left)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                const FfVec2f totDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
                const FfVec2f delta = totDelta - _dragLastDelta;
                _dragLastDelta = totDelta;

                if (delta.x != 0.0f)
                {
                    float step = io.KeyShift ? 0.1 : (io.KeyCtrl ? 2.0 : 1.0);
                    _cameraAzim -= 180 * delta.x / _size.x * step;
                    changed = true;
                }
                if (delta.y != 0.0f)
                {
                    float step = io.KeyShift ? 0.1 : (io.KeyCtrl ? 2.0 : 1.0);
                    _cameraElev += 90 * delta.y / _size.y * step;
                    changed = true;
                }
            }

            else if (_isDragging == ImGuiMouseButton_Right)
            {
#if 0
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                const FfVec2f totDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
                const FfVec2f delta = totDelta - _dragLastDelta;
                _dragLastDelta = totDelta;
                if ( (delta.x != 0.0f) || (delta.y != 0.0f) )
                {
                    //DEBUG("drag right %.1f %.1f", delta.x, delta.y);
                    changed = true;
                    float step = io.KeyShift ? 0.05f : (io.KeyCtrl ? 0.5f : 0.1f);
                    const float azim = glm::radians(360 - _cameraAzim);
                    const float cosAzim = std::cos(azim);
                    const float sinAzim = std::sin(azim);
                    const float dx = cosAzim * delta.x + sinAzim * delta.y;
                    const float dy = sinAzim * delta.x + cosAzim * delta.y;
                    _cameraLookAt.x -= step * dx;
                    _cameraLookAt.z -= step * dy;
                }
#endif
            }
            // End dragging
            if (io.MouseReleased[ImGuiMouseButton_Left] || io.MouseReleased[ImGuiMouseButton_Right])
            {
                _isDragging = ImGuiMouseButton_COUNT;
            }
        }
    }

    return changed;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetOpenGl::_DrawDebugControls()
{
    bool changed = false;

    ImGui::SetCursorScreenPos(_pos0);

    const float controlWidth = GuiSettings::charSize.x * 15;

    ImGui::TextUnformatted("Camera:");

    ImGui::PushItemWidth(3 * controlWidth);
    if (ImGui::SliderFloat3("Look at", glm::value_ptr(_cameraLookAt), -25.0f, 25.0f, "%.1f"))
    {
        changed = true;
    }
    ImGui::PopItemWidth();

    ImGui::PushItemWidth(controlWidth);
    if (ImGui::SliderFloat("FoV", &_cameraFieldOfView, FIELD_OF_VIEW_MIN, FIELD_OF_VIEW_MAX, "%.1f"))
    {
        changed = true;
    }
    if (ImGui::SliderFloat("Azim", &_cameraAzim, CAMERA_AZIM_MIN, CAMERA_AZIM_MAX, "%.1f"))
    {
        changed = true;
    }
    if (ImGui::SliderFloat("Elev", &_cameraElev, CAMERA_ELEV_MIN, CAMERA_ELEV_MAX, "%.1f"))
    {
        changed = true;
    }
    if (ImGui::SliderFloat("Roll", &_cameraRoll, CAMERA_ROLL_MIN, CAMERA_ROLL_MAX, "%.1f"))
    {
        changed = true;
    }
    if (ImGui::SliderFloat("Dist", &_cameraDist, CAMERA_DIST_MIN, CAMERA_DIST_MAX, "%.1f"))
    {
        changed = true;
    }
    ImGui::PopItemWidth();

    ImGui::TextUnformatted("Projection:");
    ImGui::PushItemWidth(controlWidth);
    if (ImGui::Combo("##ProjType", &_projType, "Perspective\0Orhometric\0"))
    {
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::SliderFloat("Near", &_cullNear, 0.01, 1.0, "%.2f"))
    {
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::SliderFloat("Far", &_cullFar, 100.0, 2000.0, "%.0f"))
    {
        changed = true;
    }
    ImGui::PopItemWidth();

    ImGui::TextUnformatted("Light:");

    if (ImGui::ColorEdit3("Ambient", glm::value_ptr(_ambientLight), ImGuiColorEditFlags_NoInputs))
    {
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::ColorEdit3("Diffuse", glm::value_ptr(_diffuseLight), ImGuiColorEditFlags_NoInputs))
    {
        changed = true;
    }
    // ImGui::PushItemWidth(3 * controlWidth);
    // if (ImGui::SliderFloat3("Direction", glm::value_ptr(_diffuseDirection), -1.0f, 1.0f, "%.3f"))
    // {
    //     changed = true;
    // }
    // ImGui::PopItemWidth();
    ImGui::Text("%.3f %.3f %.3f", _diffuseDirection.x, _diffuseDirection.y, _diffuseDirection.z);

    ImGui::TextUnformatted("Flags:");

    if (ImGui::Checkbox("Depth test", &_glState.depthTestEnable))
    {
        changed = true;
    }
    ImGui::BeginDisabled(!_glState.depthTestEnable);
    ImGui::SameLine();
    if (_StateEnumCombo("##depthFunc", _glState.depthFunc, OpenGL::State::DEPTH_FUNC))
    {
        changed = true;
    }
    ImGui::EndDisabled();

    if (ImGui::Checkbox("Cull face", &_glState.cullFaceEnable))
    {
        changed = true;
    }
    ImGui::BeginDisabled(!_glState.cullFaceEnable);
    ImGui::SameLine();
    if (_StateEnumCombo("##cullFace", _glState.cullFace, OpenGL::State::CULL_FACE))
    {
        changed = true;
    }
    ImGui::SameLine();
    if (_StateEnumCombo("##cullFrontFace", _glState.cullFrontFace, OpenGL::State::CULL_FRONTFACE))
    {
        changed = true;
    }
    ImGui::EndDisabled();

    if (_StateEnumCombo("Polygon mode", _glState.polygonMode, OpenGL::State::POLYGONMODE_MODE))
    {
        changed = true;
    }

    if (ImGui::Checkbox("Blend", &_glState.blendEnable))
    {
        changed = true;
    }
    ImGui::BeginDisabled(!_glState.blendEnable);
    ImGui::SameLine();
    if (_StateEnumCombo("##blendSrcRgb", _glState.blendSrcRgb, OpenGL::State::BLENDFUNC_FACTOR))
    {
        changed = true;
    }
    ImGui::SameLine();
    if (_StateEnumCombo("##blendDstRgb", _glState.blendDstRgb, OpenGL::State::BLENDFUNC_FACTOR))
    {
        changed = true;
    }
    ImGui::SameLine();
    if (_StateEnumCombo("##blendSrcAlpha", _glState.blendSrcAlpha, OpenGL::State::BLENDFUNC_FACTOR))
    {
        changed = true;
    }
    ImGui::SameLine();
    if (_StateEnumCombo("##blendDstAlpha", _glState.blendDstAlpha, OpenGL::State::BLENDFUNC_FACTOR))
    {
        changed = true;
    }
    ImGui::SameLine();
    if (_StateEnumCombo("##blendEqModeRgb", _glState.blendEqModeRgb, OpenGL::State::BLENDEQ_MODE))
    {
        changed = true;
    }
    ImGui::SameLine();
    if (_StateEnumCombo("##blendEqModeAlpha", _glState.blendEqModeAlpha, OpenGL::State::BLENDEQ_MODE))
    {
        changed = true;
    }
    ImGui::EndDisabled();

    ImGui::Checkbox("Force render", &_forceRender);

    return changed;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetOpenGl::_StateEnumCombo(const char *label, uint32_t &value, const std::vector<OpenGL::Enum> enums)
{
    const char *preview = "?";
    for (auto &e: enums)
    {
        if (e.value == value)
        {
            preview = e.label;
            break;
        }
    }

    bool res = false;
    ImGui::PushItemWidth(GuiSettings::charSize.x * 12);
    if (ImGui::BeginCombo(label, preview))
    {
        for (auto &e: enums)
        {
            if (ImGui::Selectable(e.label, e.value == value))
            {
                value = e.value;
                res = true;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    return res;
}


/* ****************************************************************************************************************** */
