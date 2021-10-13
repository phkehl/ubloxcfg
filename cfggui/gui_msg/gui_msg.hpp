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

#ifndef __GUI_MSG_H__
#define __GUI_MSG_H__

#include <memory>
#include <string>
#include <vector>

#include "imgui.h"

#include "receiver.hpp"
#include "logfile.hpp"
#include "gui_settings.hpp"

#include "ff_cpp.hpp"

/* ***** Receiver message renderer base class *********************************************************************** */

class GuiMsg
{
    public:
        GuiMsg(std::shared_ptr<Receiver> receiver = nullptr, std::shared_ptr<Logfile> logfile = nullptr);

        virtual void Update(const std::shared_ptr<Ff::ParserMsg> &msg);
        virtual void Buttons();
        virtual bool Render(const std::shared_ptr<Ff::ParserMsg> &msg, const ImVec2 &sizeAvail);
        virtual void Clear();

        static std::unique_ptr<GuiMsg> GetRenderer(const std::string &msgName,
            std::shared_ptr<Receiver> receiver = nullptr, std::shared_ptr<Logfile> logfile = nullptr);

    protected:
        std::shared_ptr<GuiSettings> _winSettings;
        std::shared_ptr<Receiver>    _receiver;
        std::shared_ptr<Logfile>     _logfile;

        static constexpr ImGuiTableFlags TABLE_FLAGS = ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody |
            ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX |
             ImGuiTableFlags_ScrollY;

        struct StatusFlags
        {
            int         value;
            const char *text;
            ImU32       colour;
        };

        void _RenderStatusFlag(const std::vector<StatusFlags> &flags, const int value, const char *label, const float offs);


    private:
};

/* ****************************************************************************************************************** */
#endif // __GUI_MSG_H__
