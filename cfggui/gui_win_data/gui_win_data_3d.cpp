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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GL/gl3w.h"

#include "ff_trafo.h"

#include "gui_inc.hpp"

#include "gui_win_data_3d.hpp"

/* ****************************************************************************************************************** */

GuiWinData3d::GuiWinData3d(const std::string &name, std::shared_ptr<Database> database) :
    GuiWinData(name, database),
    _shader         { VERTEX_SHADER_SRC, FRAGMENT_SHADER_SRC },
    _forceRender    { false },
    _gridSize       { 50 },
    _modelIdentity  { 1.0f },
    _markerSize     { 0.05f },
    _modelMarker    { glm::scale(glm::mat4(1.0f), glm::vec3(_markerSize)) },
    _drawGrid       { true },
    _drawMarker     { true },
    _drawTraj       { true }
{
    _winSize = { 80, 25 };
    ClearData();

    // Markers
    glGenVertexArrays(1, &_markerVertexArray);
    glGenBuffers(1, &_markerVertexBuffer);
    glGenBuffers(1, &_markerInstancesBuffer);

    // Trajectory
    glGenVertexArrays(1, &_trajVertexArray);
    glGenBuffers(1, &_trajVertexBuffer);

    // Grid
    glGenVertexArrays(1, &_gridVertexArray);
    glGenBuffers(1, &_gridVertexBuffer);

    _UpdatePoints();
    _UpdateGrid();
}

GuiWinData3d::~GuiWinData3d()
{
    glDeleteVertexArrays(1, &_markerVertexArray);
    glDeleteBuffers(1, &_markerVertexBuffer);
    glDeleteBuffers(1, &_markerInstancesBuffer);

    glDeleteVertexArrays(1, &_trajVertexArray);
    glDeleteBuffers(1, &_trajVertexBuffer);

    glDeleteVertexArrays(1, &_gridVertexArray);
    glDeleteBuffers(1, &_gridVertexBuffer);
}

// ---------------------------------------------------------------------------------------------------------------------

#define SHADER_MODE_GRID     (int32_t)0
#define SHADER_MODE_CUBES    (int32_t)1
#define SHADER_MODE_TRAJ     (int32_t)2

#define SHADER_ATTR_VERTEX_POS    0
#define SHADER_ATTR_VERTEX_NORMAL 1
#define SHADER_ATTR_VERTEX_COLOUR 2
#define SHADER_ATTR_INST_OFFS     3
#define SHADER_ATTR_INST_COLOUR   4

/*static*/ const char *GuiWinData3d::VERTEX_SHADER_SRC = R"glsl(
    #version 330 core

    // Vertex attributes
    layout (location = 0) in vec3 vertexPos;
    layout (location = 1) in vec3 vertexNormal;
    layout (location = 2) in vec4 vertexColor;

    // Instance attributes
    layout (location = 3) in vec3 instanceOffset; // vertex offset (world space)
    layout (location = 4) in vec4 instanceColour;

    uniform int shaderMode;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    out vec3 fragmentNormal;
    out vec4 fragmentColor;

    const int SHADER_MODE_GRID  = 0;
    const int SHADER_MODE_CUBES = 1;
    const int SHADER_MODE_TRAJ  = 2;

    void main()
    {
        if (shaderMode == SHADER_MODE_CUBES)
        {
            gl_Position = projection * view * ((model * vec4(vertexPos, 1.0)) + vec4(instanceOffset, 0.0));
            fragmentColor = vertexColor * instanceColour;
        }
        else if (shaderMode == SHADER_MODE_TRAJ)
        {
            gl_Position = projection * view * model * vec4(vertexPos, 1.0);
            fragmentColor = vertexColor;
        }
        else if (shaderMode == SHADER_MODE_GRID)
        {
            gl_Position = projection * view * model * vec4(vertexPos, 1.0); // x, y, z, w = 1
            fragmentColor = vertexColor;   // pass-through
        }
        fragmentNormal = vertexNormal; // pass-through FIXME: should be * model perhaps?
    }
)glsl";

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ const char *GuiWinData3d::FRAGMENT_SHADER_SRC = R"glsl(
    #version 330 core

    in vec3 fragmentNormal;
    in vec4 fragmentColor;

    uniform int shaderMode;        // shared with vertex shader

    uniform vec3 ambientLight;     // ambient light (colour/level)
    uniform vec3 diffuseLight;     // diffuse light (colour/level)
    uniform vec3 diffuseDirection; // diffuse light (direction)

    out vec4 OutColor;

    const int SHADER_MODE_GRID  = 0;
    const int SHADER_MODE_CUBES = 1;
    const int SHADER_MODE_TRAJ  = 2;

    void main()
    {
        if (shaderMode == SHADER_MODE_CUBES)
        {
            // ambient light only
            // OutColor = vec4( ambientLight * fragmentColor.xyz, fragmentColor.w );
            // add diffuse light
            float diffuseFact = max( dot( normalize(fragmentNormal), normalize(diffuseDirection) ), 0.0 );
            vec3 diffuse = diffuseFact * diffuseLight;
            OutColor = vec4( (ambientLight + diffuse) * fragmentColor.xyz, fragmentColor.w );
        }
        else if (shaderMode == SHADER_MODE_TRAJ)
        {
            OutColor = fragmentColor;
        }
        else if (shaderMode == SHADER_MODE_GRID)
        {
            OutColor = fragmentColor;
        }
    }
)glsl";

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinData3d::_ProcessData(const InputData &data)
{
    if (data.type == InputData::DATA_EPOCH)
    {
        _UpdatePoints();
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinData3d::_UpdatePoints()
{
    // One maker
    _markerVertices.clear();
    _markerVertices = CUBE_VERTICES;
    glBindBuffer(GL_ARRAY_BUFFER, _markerVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, _markerVertices.size() * OpenGL::Vertex::SIZE, _markerVertices.data(), GL_STATIC_DRAW);

    // Instances (= markers) and trajectory
    _markerInstances.clear();
    _trajVertices.clear();
#if 0
    _markerInstances.emplace_back( glm::vec3(0.0f, 0.0f, 0.0f ), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) );
    _markerInstances.emplace_back( glm::vec3(1.0f, 0.0f, -1.0f ), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) );
    _markerInstances.emplace_back( glm::vec3(2.0f, 0.0f, -2.0f ), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) );
    for (int ix = 0; ix < NUMOF(TEST_POINTS); ix++)
    {
        const float *t = TEST_POINTS[ix]; // E/N/U: X/Y/Z = E/U/-N
        _markerInstances.emplace_back( glm::vec3(t[0], t[2], -t[1]), glm::vec4(t[3], t[4], t[5], t[6]) );
        _trajVertices.push_back(OpenGL::Vertex({ t[0], t[2], -t[1] }, { 0.0f, 0.0f, 0.0f }, { t[3], t[4], t[5], t[6] }));
    }
#else
    _database->ProcRows([&](const Database::Row &row)
    {
        if (!std::isnan(row.pos_enu_ref_east))
        {
            const float east  = row.pos_enu_ref_east;
            const float north = row.pos_enu_ref_north;
            const float up    = row.pos_enu_ref_up;
            const ImVec4 &col = GuiSettings::FixColour4(row.fix_type, row.fix_ok);
            // E/N/U: X/Y/Z = E/U/-N
            _markerInstances.emplace_back(glm::vec3(east, up, -north), glm::vec4(col.x, col.y, col.z, col.w));
            _trajVertices.push_back(OpenGL::Vertex({ east, up, -north }, { 0.0f, 0.0f, 0.0f }, { col.x, col.y, col.z, col.w }));
        }
        return true;
    });
#endif
    glBindBuffer(GL_ARRAY_BUFFER, _markerInstancesBuffer);
    glBufferData(GL_ARRAY_BUFFER, _markerInstances.size() * sizeof(_markerInstances[0]), _markerInstances.data(), GL_STATIC_DRAW);

    // Trajectory

    glBindBuffer(GL_ARRAY_BUFFER, _trajVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, _trajVertices.size() * sizeof(_trajVertices[0]), _trajVertices.data(), GL_STATIC_DRAW);

    _forceRender = true;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinData3d::_UpdateGrid()
{
    _gridVertices.clear();

    // Calculate grid lines
    const int gridSize = std::clamp(_gridSize, 2, 200);
    const int xz1 = gridSize / 2;
    const int xz0 = -xz1;
    const glm::vec4 gridMinor { GUI_COLOUR4(PLOT_GRID_MINOR).x, GUI_COLOUR4(PLOT_GRID_MINOR).y, GUI_COLOUR4(PLOT_GRID_MINOR).z, GUI_COLOUR4(PLOT_GRID_MINOR).w };
    const glm::vec4 gridMajor { GUI_COLOUR4(PLOT_GRID_MAJOR).x, GUI_COLOUR4(PLOT_GRID_MAJOR).y, GUI_COLOUR4(PLOT_GRID_MAJOR).z, GUI_COLOUR4(PLOT_GRID_MAJOR).w };

    for (int xz = xz0; xz <= xz1; xz++)
    {
        _gridVertices.push_back(OpenGL::Vertex({ (float)xz0, 0.0f, (float)xz }, { 0.0f, 1.0f, 0.0f }, xz == 0 ? gridMajor : gridMinor));
        _gridVertices.push_back(OpenGL::Vertex({ (float)xz1, 0.0f, (float)xz }, { 0.0f, 1.0f, 0.0f }, xz == 0 ? gridMajor : gridMinor));

        _gridVertices.push_back(OpenGL::Vertex({ (float)xz, 0.0f, (float)xz0 }, { 0.0f, 1.0f, 0.0f }, xz == 0 ? gridMajor : gridMinor));
        _gridVertices.push_back(OpenGL::Vertex({ (float)xz, 0.0f, (float)xz1 }, { 0.0f, 1.0f, 0.0f }, xz == 0 ? gridMajor : gridMinor));
    }

    _gridVertices.push_back(OpenGL::Vertex({ 0.0f, (float)xz0, 0.0f }, { 0.0f, 0.0f, 1.0f }, gridMajor));
    _gridVertices.push_back(OpenGL::Vertex({ 0.0f, (float)xz1, 0.0f }, { 0.0f, 0.0f, 1.0f }, gridMajor));

    const float N[][2][2] =
    {
        { { 0.2f, 0.2f }, { 0.2f, 0.6f } },
        { { 0.2f, 0.6f }, { 0.4f, 0.2f } },
        { { 0.4f, 0.2f }, { 0.4f, 0.6f } },
    };
    for (int ix = 0; ix < NUMOF(N); ix++)
    {
        _gridVertices.push_back(OpenGL::Vertex({ N[ix][0][0], 0.0f, -1.0f - N[ix][0][1] }, { 0.0f, 0.0f, 1.0f }, gridMajor));
        _gridVertices.push_back(OpenGL::Vertex({ N[ix][1][0], 0.0f, -1.0f - N[ix][1][1] }, { 0.0f, 0.0f, 1.0f }, gridMajor));
    }
    const float E[][2][2] =
    {
        { { 0.2f, 0.2f }, { 0.4f, 0.2f } },
        { { 0.2f, 0.2f }, { 0.2f, 0.6f } },
        { { 0.2f, 0.6f }, { 0.4f, 0.6f } },
        { { 0.2f, 0.4f }, { 0.4f, 0.4f } },
    };
    for (int ix = 0; ix < NUMOF(E); ix++)
    {
        _gridVertices.push_back(OpenGL::Vertex({ 1.0f + E[ix][0][0], 0.0f, -E[ix][0][1] }, { 0.0f, 0.0f, 1.0f }, gridMajor));
        _gridVertices.push_back(OpenGL::Vertex({ 1.0f + E[ix][1][0], 0.0f, -E[ix][1][1] }, { 0.0f, 0.0f, 1.0f }, gridMajor));
    }

    // Copy data to GPU
    glBindVertexArray(_gridVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, _gridVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, _gridVertices.size() * sizeof(_gridVertices[0]), _gridVertices.data(), GL_STATIC_DRAW);

    _forceRender = true;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinData3d::_DrawToolbar()
{
    Gui::VerticalSeparator();

    if (Gui::ToggleButton(ICON_FK_CIRCLE "##DrawMarker", nullptr, &_drawMarker,
        "Drawing markers", "Not drawing markers", GuiSettings::iconSize))
    {
        _forceRender = true;
    }
    ImGui::SameLine();
    if (Gui::ToggleButton(ICON_FK_LINE_CHART "##DrawTraj", nullptr, &_drawTraj,
        "Drawing trajectory", "Not drawing trajectory", GuiSettings::iconSize))
    {
        _forceRender = true;
    }
    ImGui::SameLine();
    if (Gui::ToggleButton(ICON_FK_PLUS_SQUARE "##DrawGrid", nullptr, &_drawGrid,
        "Drawing grid", "Not drawing grid", GuiSettings::iconSize))
    {
        _forceRender = true;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinData3d::_DrawContent()
{
    //const ImVec2 canvasSize = FfVec2f(ImGui::GetContentRegionAvail()) - FfVec2f(0, 30);
    if (!_glWidget.BeginDraw(ImVec2(), _forceRender))
    {
        return;
    }

    // Debugging
    if (ImGui::BeginPopupContextItem("GuiWidgetOpenGl"))
    {
        ImGui::Separator();

        ImGui::BeginDisabled(!_drawMarker);
        ImGui::PushItemWidth(GuiSettings::charSize.x * 20);
        if (ImGui::SliderFloat("Marker size", &_markerSize, 0.001f, 1.0f))
        {
            _modelMarker = glm::scale(glm::mat4(1.0f), glm::vec3(_markerSize));
            _forceRender = true;
        }
        ImGui::PopItemWidth();
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!_drawGrid);
        ImGui::PushItemWidth(GuiSettings::charSize.x * 20);
        if (ImGui::SliderInt("Grid size", &_gridSize, 2.0f, 200.0f))
        {
            DEBUG("grid size %d", _gridSize);
            _UpdateGrid();
        }
        ImGui::PopItemWidth();
        ImGui::EndDisabled();

        ImGui::EndPopup();
    }

    // Render
    if ( _glWidget.StuffChanged() && _shader.Ready() )
    {
        _forceRender = false;

        // Enable shader
        _shader.Use();

        // Configure transformations
        _shader.SetUniform("projection", _glWidget.GetProjectionMatrix());
        _shader.SetUniform("view", _glWidget.GetViewMatrix());

        // Render grid
        if (_drawGrid)
        {
            _shader.SetUniform("shaderMode", SHADER_MODE_GRID);
            _shader.SetUniform("model", _modelIdentity);
            glBindVertexArray(_gridVertexArray);
            glBindBuffer(GL_ARRAY_BUFFER, _gridVertexBuffer);
            glEnableVertexAttribArray(SHADER_ATTR_VERTEX_POS);
            glVertexAttribPointer(    SHADER_ATTR_VERTEX_POS,    OpenGL::Vertex::POS_NUM,    GL_FLOAT, GL_FALSE, OpenGL::Vertex::SIZE, (void *)OpenGL::Vertex::POS_OFFS);
            glEnableVertexAttribArray(SHADER_ATTR_VERTEX_NORMAL);
            glVertexAttribPointer(    SHADER_ATTR_VERTEX_NORMAL, OpenGL::Vertex::NORMAL_NUM, GL_FLOAT, GL_FALSE, OpenGL::Vertex::SIZE, (void *)OpenGL::Vertex::NORMAL_OFFS);
            glEnableVertexAttribArray(SHADER_ATTR_VERTEX_COLOUR);
            glVertexAttribPointer(    SHADER_ATTR_VERTEX_COLOUR, OpenGL::Vertex::COLOUR_NUM, GL_FLOAT, GL_FALSE, OpenGL::Vertex::SIZE, (void *)OpenGL::Vertex::COLOUR_OFFS);
            glDrawArrays(GL_LINES, 0, _gridVertices.size());
        }

        // Render trajectory
        if (_drawTraj)
        {
            _shader.SetUniform("shaderMode", SHADER_MODE_TRAJ);
            _shader.SetUniform("model", _modelIdentity);
            glBindVertexArray(_trajVertexArray);
            glBindBuffer(GL_ARRAY_BUFFER, _trajVertexBuffer);
            glEnableVertexAttribArray(SHADER_ATTR_VERTEX_POS);
            glVertexAttribPointer(    SHADER_ATTR_VERTEX_POS,    OpenGL::Vertex::POS_NUM,    GL_FLOAT, GL_FALSE, OpenGL::Vertex::SIZE, (void *)OpenGL::Vertex::POS_OFFS);
            glEnableVertexAttribArray(SHADER_ATTR_VERTEX_NORMAL);
            glVertexAttribPointer(    SHADER_ATTR_VERTEX_NORMAL, OpenGL::Vertex::NORMAL_NUM, GL_FLOAT, GL_FALSE, OpenGL::Vertex::SIZE, (void *)OpenGL::Vertex::NORMAL_OFFS);
            glEnableVertexAttribArray(SHADER_ATTR_VERTEX_COLOUR);
            glVertexAttribPointer(    SHADER_ATTR_VERTEX_COLOUR, OpenGL::Vertex::COLOUR_NUM, GL_FLOAT, GL_FALSE, OpenGL::Vertex::SIZE, (void *)OpenGL::Vertex::COLOUR_OFFS);
            glDrawArrays(GL_LINE_STRIP, 0, _trajVertices.size());
        }

        // Render markers
        if (_drawMarker)
        {
            _shader.SetUniform("shaderMode", SHADER_MODE_CUBES);
            _shader.SetUniform("model", _modelMarker);
            _shader.SetUniform("ambientLight", _glWidget.GetAmbientLight());
            _shader.SetUniform("diffuseLight", _glWidget.GetDiffuseLight());
            _shader.SetUniform("diffuseDirection", _glWidget.GetDiffuseDirection());
            glBindVertexArray(_markerVertexArray);
            glBindBuffer(GL_ARRAY_BUFFER, _markerVertexBuffer);
            glEnableVertexAttribArray(SHADER_ATTR_VERTEX_POS);
            glVertexAttribPointer(    SHADER_ATTR_VERTEX_POS,    OpenGL::Vertex::POS_NUM,    GL_FLOAT, GL_FALSE, OpenGL::Vertex::SIZE, (void *)OpenGL::Vertex::POS_OFFS);
            glEnableVertexAttribArray(SHADER_ATTR_VERTEX_NORMAL);
            glVertexAttribPointer(    SHADER_ATTR_VERTEX_NORMAL, OpenGL::Vertex::NORMAL_NUM, GL_FLOAT, GL_FALSE, OpenGL::Vertex::SIZE, (void *)OpenGL::Vertex::NORMAL_OFFS);
            glEnableVertexAttribArray(SHADER_ATTR_VERTEX_COLOUR);
            glVertexAttribPointer(    SHADER_ATTR_VERTEX_COLOUR, OpenGL::Vertex::COLOUR_NUM, GL_FLOAT, GL_FALSE, OpenGL::Vertex::SIZE, (void *)OpenGL::Vertex::COLOUR_OFFS);
            glBindBuffer(GL_ARRAY_BUFFER, _markerInstancesBuffer);
            glEnableVertexAttribArray(SHADER_ATTR_INST_OFFS);
            glVertexAttribPointer(    SHADER_ATTR_INST_OFFS,     MarkerInstance::TRANS_NUM,  GL_FLOAT, GL_FALSE, MarkerInstance::SIZE, (void*)MarkerInstance::TRANS_OFFS);
            glVertexAttribDivisor(    SHADER_ATTR_INST_OFFS, 1);
            glEnableVertexAttribArray(SHADER_ATTR_INST_COLOUR);
            glVertexAttribPointer(    SHADER_ATTR_INST_COLOUR,   MarkerInstance::COLOUR_NUM, GL_FLOAT, GL_FALSE, MarkerInstance::SIZE, (void*)MarkerInstance::COLOUR_OFFS);
            glVertexAttribDivisor(    SHADER_ATTR_INST_COLOUR, 1);
            glDrawArraysInstanced(GL_TRIANGLES, 0, CUBE_VERTICES.size(), _markerInstances.size());
        }
    }

    _glWidget.EndDraw();
}

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ const std::vector<OpenGL::Vertex> GuiWinData3d::CUBE_VERTICES =
{
    // Fronts are CCW

    // Back face
    { {  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },

    // Front face
    { { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },

    // Left face
    { { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },

    // Right face
    { {  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },

    // Bottom face
    { { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },

    // Top face
    { {  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
    { {  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
};

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ const float GuiWinData3d::TEST_POINTS[142][3+4]=
{
    // East, North, Up, red, green, blue, alpha
    /*000*/ { -1.667, -0.964,  0.444,   0.000, 0.851, 0.000, 0.667 },
    /*001*/ { -1.686, -1.002,  0.452,   0.000, 0.851, 0.000, 0.667 },
    /*002*/ { -1.707, -1.039,  0.458,   0.000, 0.851, 0.000, 0.667 },
    /*003*/ { -1.726, -1.069,  0.461,   0.000, 0.851, 0.000, 0.667 },
    /*004*/ { -1.755, -1.110,  0.458,   0.000, 0.851, 0.000, 0.667 },
    /*005*/ { -1.770, -1.143,  0.438,   0.000, 0.851, 0.000, 0.667 },
    /*006*/ { -1.786, -1.173,  0.419,   0.000, 0.851, 0.000, 0.667 },
    /*007*/ { -1.808, -1.208,  0.420,   0.000, 0.851, 0.000, 0.667 },
    /*008*/ { -1.830, -1.244,  0.414,   0.000, 0.851, 0.000, 0.667 },
    /*009*/ { -1.853, -1.278,  0.407,   0.000, 0.851, 0.000, 0.667 },
    /*010*/ { -1.872, -1.305,  0.394,   0.000, 0.851, 0.000, 0.667 },
    /*011*/ { -1.888, -1.333,  0.388,   0.000, 0.851, 0.000, 0.667 },
    /*012*/ { -1.903, -1.362,  0.383,   0.000, 0.851, 0.000, 0.667 },
    /*013*/ { -1.921, -1.392,  0.377,   0.000, 0.851, 0.000, 0.667 },
    /*014*/ { -1.946, -1.420,  0.354,   0.000, 0.851, 0.000, 0.667 },
    /*015*/ { -1.961, -1.439,  0.321,   0.000, 0.851, 0.000, 0.667 },
    /*016*/ { -1.979, -1.457,  0.296,   0.000, 0.851, 0.000, 0.667 },
    /*017*/ { -1.978, -1.456,  0.247,   0.000, 0.851, 0.000, 0.667 },
    /*018*/ { -1.978, -1.463,  0.218,   0.000, 0.851, 0.000, 0.667 },
    /*019*/ { -1.981, -1.481,  0.211,   0.000, 0.851, 0.000, 0.667 },
    /*020*/ { -1.991, -1.504,  0.214,   0.000, 0.851, 0.000, 0.667 },
    /*021*/ { -1.999, -1.520,  0.218,   0.000, 0.851, 0.000, 0.667 },
    /*022*/ { -2.014, -1.538,  0.203,   0.000, 0.851, 0.000, 0.667 },
    /*023*/ { -2.018, -1.551,  0.184,   0.000, 0.851, 0.000, 0.667 },
    /*024*/ { -2.011, -1.549,  0.181,   0.000, 0.851, 0.000, 0.667 },
    /*025*/ { -2.020, -1.562,  0.173,   0.000, 0.851, 0.000, 0.667 },
    /*026*/ { -2.030, -1.573,  0.154,   0.000, 0.851, 0.000, 0.667 },
    /*027*/ { -2.040, -1.581,  0.156,   0.000, 0.851, 0.000, 0.667 },
    /*028*/ { -2.052, -1.595,  0.148,   0.000, 0.851, 0.000, 0.667 },
    /*029*/ { -2.060, -1.604,  0.168,   0.000, 0.851, 0.000, 0.667 },
    /*030*/ { -2.049, -1.596,  0.162,   0.000, 0.851, 0.000, 0.667 },
    /*031*/ { -2.039, -1.594,  0.161,   0.000, 0.851, 0.000, 0.667 },
    /*032*/ { -2.042, -1.604,  0.170,   0.000, 0.851, 0.000, 0.667 },
    /*033*/ { -2.042, -1.610,  0.180,   0.000, 0.851, 0.000, 0.667 },
    /*034*/ { -2.048, -1.621,  0.202,   0.000, 0.851, 0.000, 0.667 },
    /*035*/ { -2.054, -1.631,  0.217,   0.000, 0.851, 0.000, 0.667 },
    /*036*/ { -2.053, -1.638,  0.229,   0.000, 0.851, 0.000, 0.667 },
    /*037*/ { -2.044, -1.636,  0.227,   0.000, 0.851, 0.000, 0.667 },
    /*038*/ { -2.034, -1.633,  0.221,   0.000, 0.851, 0.000, 0.667 },
    /*039*/ { -2.035, -1.643,  0.234,   0.000, 0.851, 0.000, 0.667 },
    /*040*/ { -2.041, -1.660,  0.232,   0.000, 0.851, 0.000, 0.667 },
    /*041*/ { -2.040, -1.662,  0.233,   0.000, 0.851, 0.000, 0.667 },
    /*042*/ { -2.040, -1.660,  0.246,   0.000, 0.851, 0.000, 0.667 },
    /*043*/ { -2.045, -1.660,  0.265,   0.000, 0.851, 0.000, 0.667 },
    /*044*/ { -2.043, -1.655,  0.271,   0.000, 0.851, 0.000, 0.667 },
    /*045*/ { -2.034, -1.645,  0.267,   0.000, 0.851, 0.000, 0.667 },
    /*046*/ { -2.035, -1.644,  0.254,   0.000, 0.851, 0.000, 0.667 },
    /*047*/ { -2.034, -1.649,  0.259,   0.000, 0.851, 0.000, 0.667 },
    /*048*/ { -2.042, -1.651,  0.262,   0.000, 0.851, 0.000, 0.667 },
    /*049*/ { -2.056, -1.658,  0.272,   0.000, 0.851, 0.000, 0.667 },
    /*050*/ { -2.074, -1.680,  0.258,   0.000, 0.851, 0.000, 0.667 },
    /*051*/ { -2.081, -1.692,  0.253,   0.000, 0.851, 0.000, 0.667 },
    /*052*/ { -2.075, -1.697,  0.233,   0.000, 0.851, 0.000, 0.667 },
    /*053*/ { -2.076, -1.705,  0.206,   0.000, 0.851, 0.000, 0.667 },
    /*054*/ { -2.072, -1.708,  0.168,   0.000, 0.851, 0.000, 0.667 },
    /*055*/ { -2.080, -1.714,  0.144,   0.000, 0.851, 0.000, 0.667 },
    /*056*/ { -2.092, -1.720,  0.126,   0.000, 0.851, 0.000, 0.667 },
    /*057*/ { -2.100, -1.726,  0.105,   0.000, 0.851, 0.000, 0.667 },
    /*058*/ { -2.100, -1.728,  0.071,   0.000, 0.851, 0.000, 0.667 },
    /*059*/ { -2.098, -1.729,  0.058,   0.000, 0.851, 0.000, 0.667 },
    /*060*/ { -2.102, -1.733,  0.048,   0.000, 0.851, 0.000, 0.667 },
    /*061*/ { -2.111, -1.738,  0.031,   0.000, 0.851, 0.000, 0.667 },
    /*062*/ { -2.128, -1.749,  0.023,   0.000, 0.851, 0.000, 0.667 },
    /*063*/ { -2.139, -1.759,  0.018,   0.000, 0.851, 0.000, 0.667 },
    /*064*/ { -2.146, -1.769, -0.002,   0.000, 0.851, 0.000, 0.667 },
    /*065*/ { -2.151, -1.768, -0.004,   0.000, 0.851, 0.000, 0.667 },
    /*066*/ { -2.160, -1.767, -0.017,   0.000, 0.851, 0.000, 0.667 },
    /*067*/ { -2.177, -1.767, -0.022,   0.000, 0.851, 0.000, 0.667 },
    /*068*/ { -2.186, -1.762, -0.018,   0.000, 0.851, 0.000, 0.667 },
    /*069*/ { -2.191, -1.752, -0.017,   0.000, 0.851, 0.000, 0.667 },
    /*070*/ { -2.202, -1.753, -0.010,   0.000, 0.851, 0.000, 0.667 },
    /*071*/ { -2.220, -1.755,  0.006,   0.000, 0.851, 0.000, 0.667 },
    /*072*/ { -2.224, -1.749,  0.013,   0.000, 0.851, 0.000, 0.667 },
    /*073*/ { -2.228, -1.744,  0.001,   0.000, 0.851, 0.000, 0.667 },
    /*074*/ { -2.236, -1.743,  0.013,   0.000, 0.851, 0.000, 0.667 },
    /*075*/ { -2.241, -1.742,  0.008,   0.000, 0.851, 0.000, 0.667 },
    /*076*/ { -2.265, -1.765,  0.015,   0.000, 0.851, 0.000, 0.667 },
    /*077*/ { -2.275, -1.769,  0.018,   0.000, 0.851, 0.000, 0.667 },
    /*078*/ { -2.285, -1.771,  0.011,   0.000, 0.851, 0.000, 0.667 },
    /*079*/ { -2.285, -1.765, -0.007,   0.000, 0.851, 0.000, 0.667 },
    /*080*/ { -2.284, -1.758, -0.035,   0.000, 0.851, 0.000, 0.667 },
    /*081*/ { -2.289, -1.757, -0.060,   0.000, 0.851, 0.000, 0.667 },
    /*082*/ { -2.301, -1.762, -0.063,   0.000, 0.851, 0.000, 0.667 },
    /*083*/ { -2.301, -1.752, -0.077,   0.000, 0.851, 0.000, 0.667 },
    /*084*/ { -2.312, -1.752, -0.083,   0.000, 0.851, 0.000, 0.667 },
    /*085*/ { -2.315, -1.746, -0.088,   0.000, 0.851, 0.000, 0.667 },
    /*086*/ { -2.303, -1.732, -0.101,   0.000, 0.851, 0.000, 0.667 },
    /*087*/ { -2.287, -1.716, -0.127,   0.000, 0.851, 0.000, 0.667 },
    /*088*/ { -2.262, -1.708, -0.199,   0.000, 0.851, 0.000, 0.667 },
    /*089*/ { -2.235, -1.696, -0.274,   0.000, 0.851, 0.000, 0.667 },
    /*090*/ { -2.206, -1.679, -0.356,   0.000, 0.851, 0.000, 0.667 },
    /*091*/ { -2.177, -1.666, -0.426,   0.000, 0.851, 0.000, 0.667 },
    /*092*/ { -2.151, -1.661, -0.485,   0.000, 0.851, 0.000, 0.667 },
    /*093*/ { -2.110, -1.640, -0.561,   0.000, 0.851, 0.000, 0.667 },
    /*094*/ { -2.075, -1.625, -0.622,   0.000, 0.851, 0.000, 0.667 },
    /*095*/ { -2.048, -1.615, -0.678,   0.000, 0.851, 0.000, 0.667 },
    /*096*/ { -2.008, -1.605, -0.747,   0.000, 0.851, 0.000, 0.667 },
    /*097*/ { -1.967, -1.595, -0.794,   0.000, 0.851, 0.000, 0.667 },
    /*098*/ { -1.935, -1.554, -0.813,   0.000, 0.851, 0.000, 0.667 },
    /*099*/ { -1.896, -1.507, -0.831,   0.000, 0.851, 0.000, 0.667 },
    /*100*/ { -1.869, -1.470, -0.830,   0.000, 0.851, 0.000, 0.667 },
    /*101*/ { -1.847, -1.433, -0.829,   0.000, 0.851, 0.000, 0.667 },
    /*102*/ { -1.809, -1.393, -0.835,   0.000, 0.851, 0.000, 0.667 },
    /*103*/ { -1.770, -1.345, -0.849,   0.000, 0.851, 0.000, 0.667 },
    /*104*/ { -1.730, -1.296, -0.860,   0.000, 0.851, 0.000, 0.667 },
    /*105*/ { -1.699, -1.258, -0.868,   0.000, 0.851, 0.000, 0.667 },
    /*106*/ { -1.664, -1.214, -0.854,   0.000, 0.851, 0.000, 0.667 },
    /*107*/ { -1.634, -1.173, -0.826,   0.000, 0.851, 0.000, 0.667 },
    /*108*/ { -1.575, -1.103, -0.654,   0.000, 0.851, 0.000, 0.667 },
    /*109*/ { -1.499, -1.023, -0.501,   0.000, 0.851, 0.000, 0.667 },
    /*110*/ { -1.421, -0.941, -0.354,   0.000, 0.851, 0.000, 0.667 },
    /*111*/ { -1.354, -0.874, -0.211,   0.000, 0.851, 0.000, 0.667 },
    /*112*/ { -1.446, -1.463,  0.844,   0.000, 0.851, 0.000, 0.667 },
    /*113*/ { -0.938, -1.348,  1.516,   1.000, 1.000, 0.000, 0.667 },
    /*114*/ { -0.555, -1.143,  1.765,   1.000, 1.000, 0.000, 0.667 },
    /*115*/ { -0.018, -0.576,  2.124,   1.000, 1.000, 0.000, 0.667 },
    /*116*/ {  0.382, -0.213,  2.287,   1.000, 1.000, 0.000, 0.667 },
    /*117*/ {  0.991,  0.353,  2.370,   1.000, 1.000, 0.000, 0.667 },
    /*118*/ {  1.486,  0.763,  2.348,   1.000, 1.000, 0.000, 0.667 },
    /*119*/ {  2.281,  1.417,  2.207,   1.000, 1.000, 0.000, 0.667 },
    /*120*/ {  2.868,  1.892,  2.028,   1.000, 1.000, 0.000, 0.667 },
    /*121*/ {  3.843,  2.721,  1.818,   1.000, 1.000, 0.000, 0.667 },
    /*122*/ {  4.557,  3.326,  1.587,   1.000, 1.000, 0.000, 0.667 },
    /*123*/ {  5.469,  4.127,  1.279,   1.000, 1.000, 0.000, 0.667 },
    /*124*/ {  6.139,  4.719,  0.985,   1.000, 1.000, 0.000, 0.667 },
    /*125*/ {  6.970,  5.420,  0.664,   1.000, 1.000, 0.000, 0.667 },
    /*126*/ {  7.594,  5.947,  0.387,   1.000, 1.000, 0.000, 0.667 },
    /*127*/ {  8.408,  6.629,  0.061,   1.000, 1.000, 0.000, 0.667 },
    /*128*/ {  9.022,  7.144, -0.207,   1.000, 1.000, 0.000, 0.667 },
    /*129*/ {  9.763,  7.760, -0.494,   1.000, 1.000, 0.000, 0.667 },
    /*130*/ { 10.323,  8.226, -0.737,   1.000, 1.000, 0.000, 0.667 },
    /*131*/ { 11.021,  8.803, -0.943,   1.000, 1.000, 0.000, 0.667 },
    /*132*/ { 11.552,  9.242, -1.116,   1.000, 1.000, 0.000, 0.667 },
    /*133*/ { 12.065,  9.656, -1.206,   1.000, 1.000, 0.000, 0.667 },
    /*134*/ { 12.476,  9.982, -1.300,   1.000, 1.000, 0.000, 0.667 },
    /*135*/ { 13.097, 10.427, -1.654,   1.000, 1.000, 0.000, 0.667 },
    /*136*/ { 13.567, 10.762, -1.931,   1.000, 1.000, 0.000, 0.667 },
    /*137*/ { 14.167, 11.210, -2.251,   1.000, 1.000, 0.000, 0.667 },
    /*138*/ { 14.620, 11.541, -2.504,   1.000, 1.000, 0.000, 0.667 },
    /*139*/ { 15.103, 11.905, -2.765,   1.000, 1.000, 0.000, 0.667 },
    /*140*/ { 15.474, 12.185, -2.980,   1.000, 1.000, 0.000, 0.667 },
    /*141*/ { 15.934, 12.517, -3.203,   1.000, 1.000, 0.000, 0.667 },
};

/* ****************************************************************************************************************** */
