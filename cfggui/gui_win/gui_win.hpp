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

#ifndef __GUI_WIN_HPP__
#define __GUI_WIN_HPP__

#include <memory>
#include <cinttypes>
#include <string>

#include "imgui.h"

#include "gui_settings.hpp"

/* ***** Window base class ********************************************************************** */

class GuiWin
{
    public:
        GuiWin(const std::string &name);
        virtual ~GuiWin() {};

        void                 Open();
        void                 Close();
        virtual bool         IsOpen();
        bool                 IsDrawn();
        bool                *GetOpenFlag();
        const std::string   &GetName();
        const std::string   &GetTitle(); // Title with ID ("title###id")
        void                 SetTitle(const std::string &title);
        void                 Focus();
        const std::string   &GetUidStr(); // Run-time (!) UID

        virtual void         Loop(const uint32_t &frame, const double &now);

        virtual void         DrawWindow();

    protected:

        std::string          _winTitle;
        std::string          _winName;
        std::string          _winImguiName;
        bool                 _winOpen;
        bool                 _winDrawn;
        ImGuiWindowFlags     _winFlags;
        ImVec2               _winSize;
        ImVec2               _winSizeMin;
        uint64_t             _winUid;
        std::string          _winUidStr;
        std::unique_ptr<ImGuiWindowClass> _winClass;

        bool                 _DrawWindowBegin();
        void                 _DrawWindowEnd();
        ImVec2               _WinSizeToVec(ImVec2 size);

        static const std::vector<FfVec2f> NEW_WIN_POS;
        static int _newWinPosIx;
        FfVec2f     _newWinInitPos;

    private:
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_HPP__
