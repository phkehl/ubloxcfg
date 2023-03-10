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

#ifndef __GUI_UTILS_HPP__
#define __GUI_UTILS_HPP__

#include <cstdint>
#include <cstdarg>

#include "imgui.h"

/* ****************************************************************************************************************** */

namespace Gui
{
    ImU32 ColourHSV(const float h, const float s, const float v, const float a = 1.0f);
    // void SetWindowScale(const float scale = 1.0f);
    void VerticalSeparator(const float offset_from_start_x = 0.0f);
    bool ItemTooltip(const char *text);
    bool ItemTooltip(const std::string &text);
    bool ItemTooltipBegin();
    void ItemTooltipEnd();
    void DebugTooltipThingy(const char *fmt, ...) PRINTF_ATTR(1);

    enum Tri_e { TRI_IGNORE = -1, TRI_DISABLED = 0, TRI_ENABLED = 1 };
    bool CheckBoxTristate(const char *label, Tri_e *state, const bool cycleAllThree = false);
    bool CheckboxFlagsX8(const char *label, uint64_t *flags, uint64_t flags_value);

    void BeginMenuPersist();
    void EndMenuPersist();

    void TextLink(const char *url, const char *text = nullptr);
    bool ClickableText(const char *text);

    bool ToggleButton(const char *label, const char *labelOff, bool *toggle, const char *tooltipOn, const char *tooltipOff, const ImVec2 &size = ImVec2(0, 0));

    void TextTitle(const char *text);
    void TextTitle(const std::string &text);
    void TextTitleF(const char *fmt, ...) PRINTF_ATTR(1);

    void TextDim(const char *text);
    void TextDim(const std::string &text);
    void TextDimF(const char *fmt, ...) PRINTF_ATTR(1);
};

/* ****************************************************************************************************************** */

#endif // __GUI_UTILS_HPP__
