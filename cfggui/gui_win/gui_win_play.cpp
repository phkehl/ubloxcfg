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

#include <cstring>
#include <cmath>
#include <chrono>

#include "gui_inc.hpp"

#include "platform.hpp"

#include "gui_win_play.hpp"

/* ****************************************************************************************************************** */

GuiWinPlay::GuiWinPlay() :
    GuiWin("Play"),
    _tetrisRows { 22 },
    _tetrisCols { 10 },
    _tetrisGame { tg_create(_tetrisRows, _tetrisCols) },
    _tetrisMove { TM_NONE }
{
    _winSize = { 100, 50 };
    _winFlags |= ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking;
}

GuiWinPlay::~GuiWinPlay()
{
    if (_tetrisThread)
    {
        _tetrisThreadAbort = true;
        _tetrisThread->join();
    }

    tg_delete(_tetrisGame);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinPlay::DrawWindow()
{
    if (!_DrawWindowBegin())
    {
        return;
    }

    if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_FittingPolicyScroll))
    {
        if (ImGui::BeginTabItem("Tetris"))
        {
            _DrawTetris();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    _playWinFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

    _DrawWindowEnd();
}

// ---------------------------------------------------------------------------------------------------------------------

#define USE_TM_HOLD 0 // doesn't quiet work (it can get stuck in some loop sometimes), https://github.com/brenns10/tetris/issues/9

void GuiWinPlay::_DrawTetris()
{
    std::lock_guard<std::mutex> lock(_tetrisGameMutex);

    const float lineHeight = _winSettings->charSize.y + _winSettings->style.ItemSpacing.y;
    const float cellSize = 2 * _winSettings->charSize.x;
    const FfVec2 cell { cellSize, cellSize };
    const float padding = cellSize;
    const FfVec2 boardSize { cellSize * _tetrisCols, cellSize * _tetrisRows };
    const FfVec2 nextSize  { cellSize * 4, cellSize * 4 };
    const FfVec2 canvasSize { padding + boardSize.x + padding + nextSize.x + padding, padding + boardSize.y + padding };

    if (!ImGui::BeginChild("Board", canvasSize))
    {
        ImGui::EndChild();
        return;
    }

    ImDrawList *draw = ImGui::GetWindowDrawList();

    // Canvas
    const FfVec2 canvas0 = ImGui::GetCursorScreenPos();
    const FfVec2 canvasS = ImGui::GetContentRegionAvail();
    const FfVec2 canvas1 = canvas0 + canvasS;
    draw->AddRect(canvas0, canvas1, GUI_COLOUR(C_GREY));

    // Board
    const FfVec2 board0 = canvas0 + FfVec2(padding, padding);
    const FfVec2 board1 = board0 + boardSize;
    draw->AddRect(board0 - FfVec2(2, 2), board1 + FfVec2(2, 2), GUI_COLOUR(C_WHITE));

    // Next block
    const FfVec2 next0 { board1.x + padding, board0.y + lineHeight };
    const FfVec2 next1 = next0 + nextSize;
    draw->AddRect(next0 - FfVec2(2, 2), next1 + FfVec2(2, 2), GUI_COLOUR(C_WHITE));
    ImGui::SetCursorScreenPos(ImVec2(next0.x, next0.y - lineHeight));
    ImGui::TextUnformatted("Next:");

    // Stored block
    const FfVec2 stored0 { board1.x + padding, next1.y + padding + lineHeight };
    const FfVec2 stored1 = stored0 + nextSize;
#if USE_TM_HOLD
    draw->AddRect(stored0 - FfVec2(2, 2), stored1 + FfVec2(2, 2), GUI_COLOUR(C_WHITE));
    ImGui::SetCursorScreenPos(FfVec2(stored0.x, stored0.y - lineHeight));
    ImGui::TextUnformatted("Stored:");
#endif

    // Score
    float y0 = stored1.y + padding;
    const FfVec2 score0 { board1.x + padding, y0 };
    const FfVec2 score1 { score0.x, score0.y + lineHeight };
    ImGui::SetCursorScreenPos(score0);
    ImGui::TextUnformatted("Score:");
    ImGui::SetCursorScreenPos(score1);
    ImGui::Text("%d", _tetrisGame->points);
    y0 += padding + lineHeight + lineHeight;

    // Level
    const FfVec2 level0 { board1.x + padding, y0 };
    const FfVec2 level1 { level0.x, level0.y + lineHeight };
    ImGui::SetCursorScreenPos(level0);
    ImGui::TextUnformatted("Level:");
    ImGui::SetCursorScreenPos(level1);
    ImGui::Text("%d", _tetrisGame->level);
    y0 += padding + lineHeight + lineHeight;

    // Lines
    const FfVec2 lines0 { board1.x + padding, y0 };
    const FfVec2 lines1 { lines0.x, lines0.y + lineHeight };
    ImGui::SetCursorScreenPos(lines0);
    ImGui::TextUnformatted("Lines:");
    ImGui::SetCursorScreenPos(lines1);
    ImGui::Text("%d", _tetrisGame->lines_remaining);
    y0 += padding + lineHeight + lineHeight;

    // Draw game
    if (_tetrisGame->falling.loc.row > 0)
    {
        _DrawTetromino(draw, next0, cell, _tetrisGame->next);
        _DrawTetromino(draw, stored0, cell, _tetrisGame->stored);
    }
    for (int row = 0; row < _tetrisGame->rows; row++)
    {
        for (int col = 0; col < _tetrisGame->cols; col++)
        {
            const tetris_cell c = (tetris_cell)tg_get(_tetrisGame, row, col);
            if (c != TC_EMPTY)
            {
                const FfVec2 block0 { board0.x + (col * cell.x), board0.y + (row * cell.y) };
                const tetris_type type = (tetris_type)(c - 1);
                draw->AddRectFilled(block0, block0 + cell, _TetrominoColour(type));
                draw->AddRect(block0, block0 + cell, _TetrominoColour(type, true));
            }
        }
    }

    // Handle keys
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
    {
        _tetrisMove = TM_CLOCK;
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
    {
        _tetrisMove = TM_LEFT;
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
    {
        _tetrisMove = TM_RIGHT;
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
    {
        _tetrisMove = (tetris_move)(100 + 20);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
    {
        _tetrisMove = TM_DROP;
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Space)))
    {
        _tetrisThreadPaused = !_tetrisThreadPaused;
    }
#if USE_TM_HOLD
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Space)))
    {
        _tetrisMove = TM_HOLD;
    }
#endif

    // Game finished
    if (!_tetrisThreadRunning && _tetrisThread)
    {
        _tetrisThread->join();
        _tetrisThread = nullptr;
    }

    // Draw start button
    if (!_tetrisThread)
    {
        draw->AddRectFilled(board0, board1, GUI_COLOUR(C_GREY) & IM_COL32(0xff, 0xff, 0xff, 0x7f));
        ImGui::SetCursorScreenPos(board0 + ((boardSize - _winSettings->iconButtonSize) * 0.5));
        if (ImGui::Button(ICON_FK_PLAY "##Play", _winSettings->iconButtonSize))
        {
            tg_init(_tetrisGame, _tetrisRows, _tetrisCols);
            _tetrisThreadAbort = false;
            _tetrisThreadPaused = false;
            _tetrisThread = std::make_unique<std::thread>(&GuiWinPlay::_TetrisThread, this);
        }
        Gui::ItemTooltip("Play");
    }

    ImGui::EndChild();
}

// ---------------------------------------------------------------------------------------------------------------------

ImU32 GuiWinPlay::_TetrominoColour(const tetris_type type, const bool dim)
{
    // https://en.wikipedia.org/wiki/Tetromino
    ImU32 colour = GUI_COLOUR(C_GREY);
    switch (type)
    {
        case TET_I: colour = GUI_COLOUR(C_CYAN);       break;
        case TET_J: colour = GUI_COLOUR(C_BRIGHTBLUE); break;
        case TET_L: colour = GUI_COLOUR(C_ORANGE);     break;
        case TET_O: colour = GUI_COLOUR(C_YELLOW);     break;
        case TET_S: colour = GUI_COLOUR(C_GREEN);      break;
        case TET_T: colour = GUI_COLOUR(C_MAGENTA);    break;
        case TET_Z: colour = GUI_COLOUR(C_RED);        break;
    }
    if (dim)
    {
        return colour & IM_COL32(0x7f, 0x7f, 0x7f, 0x7f);
    }
    else
    {
        return colour;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinPlay::_DrawTetromino(ImDrawList *draw, const FfVec2 &p0, const FfVec2 &cell, const tetris_block &block)
{
    if (block.typ >= 0)
    {
        FfVec2 offs { 0, 0 };
        switch (block.typ)
        {
            case TET_I: offs.y = cell.y / 2; break;
            case TET_J:
            case TET_L: offs.x = cell.x / 2; offs.y = cell.y; break;
            case TET_O: offs.y = cell.y; break;
            case TET_S:
            case TET_T:
            case TET_Z: offs.x = cell.x / 2; offs.y = cell.y; break;
        }
        for (int b = 0; b < TETRIS; b++)
        {
            const tetris_location c = TETROMINOS[block.typ][block.ori][b];
            const FfVec2 block0 = p0 + offs + FfVec2(cell.x * c.col, cell.y * c.row);
            draw->AddRectFilled(block0, block0 + cell, _TetrominoColour((tetris_type)block.typ));
            draw->AddRect(block0, block0 + cell, _TetrominoColour((tetris_type)block.typ, true));
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinPlay::_TetrisThread()
{
    Platform::SetThreadName("tetris");
    _tetrisThreadRunning = true;
    while (!_tetrisThreadAbort && _tetrisThreadRunning)
    {
        if (_winOpen && _playWinFocused && !_tetrisThreadPaused)
        {
            std::lock_guard<std::mutex> lock(_tetrisGameMutex);

            if (_tetrisMove >= 100)
            {
                int n = _tetrisMove - 100;
                while ( _tetrisThreadRunning && (n > 0) )
                {
                    _tetrisThreadRunning = tg_tick(_tetrisGame, TM_NONE);
                    n--;
                }
            }
            else
            {
                _tetrisThreadRunning = tg_tick(_tetrisGame, _tetrisMove);
            }
        }

        _tetrisMove = TM_NONE;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    _tetrisThreadRunning = false;
}


/* ****************************************************************************************************************** */
