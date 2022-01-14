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

#ifndef __GUI_MSG_HPP__
#define __GUI_MSG_HPP__

#include <memory>
#include <string>
#include <vector>

#include "imgui.h"

#include "input_receiver.hpp"
#include "input_logfile.hpp"
#include "gui_settings.hpp"

#include "ff_cpp.hpp"

/* ***** Receiver message renderer base class *********************************************************************** */

class GuiMsg
{
    public:
        GuiMsg(std::shared_ptr<InputReceiver> receiver = nullptr, std::shared_ptr<InputLogfile> logfile = nullptr);

        virtual void Update(const std::shared_ptr<Ff::ParserMsg> &msg);
        virtual void Buttons();
        virtual bool Render(const std::shared_ptr<Ff::ParserMsg> &msg, const FfVec2f &sizeAvail);
        virtual void Clear();

        static std::unique_ptr<GuiMsg> GetRenderer(const std::string &msgName,
            std::shared_ptr<InputReceiver> receiver = nullptr, std::shared_ptr<InputLogfile> logfile = nullptr);

    protected:
        std::shared_ptr<InputReceiver>    _receiver;
        std::shared_ptr<InputLogfile>     _logfile;

        static constexpr ImGuiTableFlags TABLE_FLAGS = ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody |
            ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX |
             ImGuiTableFlags_ScrollY;

        struct StatusFlags
        {
            int         value;
            const char *text;
            ImU32       colour;
        };

        ImVec2 _CalcTopSize(const int nLinesOfTopText);
        void _RenderStatusText(const std::string &label, const std::string &text, const float dataOffs);
        void _RenderStatusText(const std::string &label, const char        *text, const float dataOffs);
        void _RenderStatusText(const char        *label, const std::string &text, const float dataOffs);
        void _RenderStatusText(const char        *label, const char        *text, const float dataOffs);
        void _RenderStatusFlag(const std::vector<StatusFlags> &flags, const int value, const char *label, const float offs);

    private:
};

/* ****************************************************************************************************************** */
#endif // __GUI_MSG_HPP__
