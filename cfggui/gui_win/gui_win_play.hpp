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

#ifndef __GUI_WIN_PLAY_H__
#define __GUI_WIN_PLAY_H__

#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

#include "gui_win.hpp"

extern "C" {
#include "tetris.h"
}

/* ***** Play games ************************************************************************************************* */

class GuiWinPlay : public GuiWin
{
    public:
        GuiWinPlay();
       ~GuiWinPlay();

        void DrawWindow() final;

    protected:

        std::atomic<bool>             _playWinFocused;

        int                           _tetrisRows;
        int                           _tetrisCols;
        tetris_game                  *_tetrisGame;
        std::mutex                    _tetrisGameMutex;
        std::atomic<tetris_move>      _tetrisMove;
        std::unique_ptr<std::thread>  _tetrisThread;
        std::atomic<bool>             _tetrisThreadAbort;
        std::atomic<bool>             _tetrisThreadRunning;
        std::atomic<bool>             _tetrisThreadPaused;

        void _TetrisThread();

        void _DrawTetris();
        void _DrawTetromino(ImDrawList *draw, const ImVec2 &p0, const ImVec2 &square, const tetris_block &block);
        ImU32 _TetrominoColour(const tetris_type type, const bool dim = false);
};

/* ****************************************************************************************************************** */
#endif // __GUI_WIN_PLAY_H__
