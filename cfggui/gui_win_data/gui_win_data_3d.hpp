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

#ifndef __GUI_WIN_DATA_THREED_HPP__
#define __GUI_WIN_DATA_THREED_HPP__

#include <vector>
#include <glm/glm.hpp>

#include "opengl.hpp"
#include "gui_widget_opengl.hpp"
#include "gui_win_data.hpp"

/* ***** 3d view **************************************************************************************************** */

class GuiWinData3d : public GuiWinData
{
    public:
        GuiWinData3d(const std::string &name, std::shared_ptr<Database> database);
       ~GuiWinData3d();

    private:

        void _ProcessData(const InputData &data) final;
        void _DrawContent() final;
        void _DrawToolbar() final;

        static const char *VERTEX_SHADER_SRC;
        static const char *FRAGMENT_SHADER_SRC;

        GuiWidgetOpenGl _glWidget;
        OpenGL::Shader  _shader;
        bool            _forceRender;

        // North-East grid
        std::vector<OpenGL::Vertex> _gridVertices;
        unsigned int _gridVertexArray;
        unsigned int _gridVertexBuffer;
        int _gridSize;
        void _UpdateGrid();

        // Markers
        struct MarkerInstance
        {
            MarkerInstance(const glm::vec3 &_translate, const glm::vec4 &_colour) :
                translate{_translate}, colour{_colour} { }
            glm::vec3 translate;
            glm::vec4 colour;
            static constexpr uint32_t TRANS_OFFS  = 0;
            static constexpr int32_t  TRANS_NUM   = 3;
            static constexpr uint32_t COLOUR_OFFS = sizeof(translate);
            static constexpr int32_t  COLOUR_NUM  = 4;
            static constexpr uint32_t SIZE        = sizeof(translate) + sizeof(colour);
        };
        std::vector<OpenGL::Vertex> _markerVertices;
        std::vector<MarkerInstance> _markerInstances;
        unsigned int _markerVertexArray;
        unsigned int _markerVertexBuffer;
        unsigned int _markerInstancesBuffer;

        // Trajectory
        std::vector<OpenGL::Vertex> _trajVertices;
        unsigned int _trajVertexArray;
        unsigned int _trajVertexBuffer;

        void _UpdatePoints();

        glm::mat4 _modelIdentity;
        float     _markerSize;
        glm::mat4 _modelMarker;

        bool _drawGrid;
        bool _drawMarker;
        bool _drawTraj;

        static const std::vector<OpenGL::Vertex> CUBE_VERTICES;
        static const float TEST_POINTS[142][3+4];
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_DATA_THREED_HPP__
