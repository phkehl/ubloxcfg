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

#include <cstring>

#include "ff_cpp.hpp"

#include "gui_inc.hpp"
#include "imgui_internal.h"

#include "gui_utils.hpp"

/* ****************************************************************************************************************** */

// https://github.com/ocornut/imgui/issues/1889#issuecomment-398681105
// There is now ImGui::BeginDisabled(bool) and ImGui::EndDisabled()

// void ImGui::BeginDisabled(const bool disabled)
// {
//     if (disabled)
//     {
//         ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
//         ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.65f);
//     }
// }

// void Gui::EndDisabled(const bool disabled)
// {
//     if (disabled)
//     {
//         ImGui::PopItemFlag();
//         ImGui::PopStyleVar();
//     }
// }

// ---------------------------------------------------------------------------------------------------------------------

ImU32 Gui::ColourHSV(const float h, const float s, const float v, const float a)
{
    ImColor c;
    ImGui::ColorConvertHSVtoRGB(h, s, v, c.Value.x, c.Value.y, c.Value.z);
    c.Value.w = a;
    return c;
}

// ---------------------------------------------------------------------------------------------------------------------

// void Gui::SetWindowScale(const float scale)
// {
//     if ( (scale > 0.24) && (scale < 4.01) )
//     {
//         auto ctx = ImGui::GetCurrentContext();
//         auto win = ctx->CurrentWindow;
//         win->FontWindowScale = scale;
//     }
// }

// ---------------------------------------------------------------------------------------------------------------------

void Gui::VerticalSeparator(const float offset_from_start_x)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
    {
        return;
    }
    ImGui::SameLine(offset_from_start_x);
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
}

// ---------------------------------------------------------------------------------------------------------------------

bool Gui::ItemTooltipBegin()
{
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::BeginTooltip();
        return true;
    }
    else
    {
        return false;
    }
}

void Gui::ItemTooltipEnd()
{
    ImGui::EndTooltip();
}

bool Gui::ItemTooltip(const char *text)
{
    if (ItemTooltipBegin())
    {
        ImGui::TextUnformatted(text);
        ItemTooltipEnd();
        return true;
    }
    return false;
}

bool Gui::ItemTooltip(const std::string &text)
{
    return ItemTooltip(text.c_str());
}

// ---------------------------------------------------------------------------------------------------------------------

void Gui::DebugTooltipThingy(const char *fmt, ...)
{
    ImGui::TextUnformatted("?");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        va_list args;
        va_start(args, fmt);
        ImGui::TextV(fmt, args);
        va_end(args);
        ImGui::EndTooltip();
    }
    ImGui::SameLine(0.0f, 1.0f);
}

// ---------------------------------------------------------------------------------------------------------------------

// Based on https://github.com/ocornut/imgui/issues/2644
bool Gui::CheckBoxTristate(const char *label, Tri_e *state, const bool cycleAllThree)
{
    bool res = false;
    switch (*state)
    {
        case TRI_IGNORE:
        {
            ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);
            bool b = false;
            res = ImGui::Checkbox(label, &b);
            if (res)
            {
                *state = TRI_ENABLED;
            }
            ImGui::PopItemFlag();
            break;
        }
        case TRI_ENABLED:
        {
            bool b = true;
            res = ImGui::Checkbox(label, &b);
            if (res)
            {
                *state = TRI_DISABLED;
            }
            break;
        }
        case TRI_DISABLED:
        {
            const float pos = ImGui::GetCursorPosX();
            bool b = false;
            res = ImGui::Checkbox(label, &b);
            if (res)
            {
                if (cycleAllThree)
                {
                    *state = TRI_IGNORE;
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(pos);
                    CheckBoxTristate(label, state);
                }
                else
                {
                    *state = TRI_ENABLED;
                }
            }
            break;
        }
    }
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

bool Gui::CheckboxFlagsX8(const char *label, uint64_t *flags, uint64_t flags_value)
{
    bool v = ((*flags & flags_value) == flags_value);
    bool pressed = ImGui::Checkbox(label, &v);
    if (pressed)
    {
        if (v)
        {
            *flags |= flags_value;
        }
        else
        {
            *flags &= ~flags_value;
        }
    }
    return pressed;
}

// ---------------------------------------------------------------------------------------------------------------------

void Gui::BeginMenuPersist()
{
    ImGui::PushItemFlag(ImGuiItemFlags_SelectableDontClosePopup, true);
}

void Gui::EndMenuPersist()
{
    ImGui::PopItemFlag();
}

// ---------------------------------------------------------------------------------------------------------------------

void Gui::TextLink(const char *url, const char *text)
{
    static uint32_t id;
    ImGui::PushID(id);
    const char *label = text ? text : url;
    ImVec2 size = ImGui::CalcTextSize(label);
    const ImVec2 cursor = ImGui::GetCursorPos();
    ImGui::InvisibleButton(label, size, ImGuiButtonFlags_MouseButtonLeft);
    if (ImGui::IsItemClicked())
    {
        std::string cmd = "xdg-open ";
        cmd += url;
        if (std::system(cmd.c_str()) != EXIT_SUCCESS)
        {
            WARNING("Command failed: %s", cmd.c_str());
        }
    }
    const bool isHovered = ImGui::IsItemHovered();
    if (isHovered)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    ImGui::SetCursorPos(cursor);
    ImGui::TextUnformatted(label);
    if (isHovered) { ImGui::PopStyleColor(); }
    ImGui::PopID();
    id++;
}

// ---------------------------------------------------------------------------------------------------------------------

bool Gui::ClickableText(const char *text)
{
    bool res = false;
    static uint32_t id;
    id++;
    ImGui::PushID(id);
    ImVec2 size = ImGui::CalcTextSize(text);
    const ImVec2 cursor = ImGui::GetCursorPos();
    ImGui::InvisibleButton("dummy", size, ImGuiButtonFlags_MouseButtonLeft);
    if (ImGui::IsItemClicked())
    {
        res = true;
    }
    const bool isHovered = ImGui::IsItemHovered();
    if (isHovered)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_HIGHLIGHT));
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
    ImGui::SetCursorPos(cursor);
    ImGui::TextUnformatted(text);
    if (isHovered) { ImGui::PopStyleColor(); }
    ImGui::PopID();
    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

bool Gui::ToggleButton(const char *labelOn, const char *labelOff, bool *toggle, const char *tooltipOn, const char *tooltipOff, const ImVec2 &size)
{
    bool res = false;
    const bool enabled = *toggle;
    if (labelOff == NULL)
    {
        if (!enabled) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM)); }
        if (ImGui::Button(labelOn, size))
        {
            *toggle = !*toggle;
            res = true;
        }
        if (!enabled) { ImGui::PopStyleColor(); }

    }
    else
    {
        if (enabled)
        {
            if (ImGui::Button(labelOn, size))
            {
                *toggle = false;
                res = true;
            }
        }
        else
        {
            if (ImGui::Button(labelOff, size))
            {
                *toggle = true;
                res = true;
            }
        }
    }

    if (enabled)
    {
        if (tooltipOn != NULL)
        {
            Gui::ItemTooltip(tooltipOn);
        }
    }
    else
    {
        if (tooltipOff != NULL)
        {
            Gui::ItemTooltip(tooltipOff);
        }
        else if (tooltipOn != NULL)
        {
            Gui::ItemTooltip(tooltipOn);
        }
    }

    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

void Gui::TextTitle(const char *text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

void Gui::TextTitle(const std::string &text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
    ImGui::TextUnformatted(text.c_str());
    ImGui::PopStyleColor();
}

void Gui::TextTitleF(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
    ImGui::TextV(fmt, args);
    ImGui::PopStyleColor();
    va_end(args);
}

// ---------------------------------------------------------------------------------------------------------------------

void Gui::TextDim(const char *text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

void Gui::TextDim(const std::string &text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
    ImGui::TextUnformatted(text.c_str());
    ImGui::PopStyleColor();
}

void Gui::TextDimF(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
    ImGui::TextV(fmt, args);
    ImGui::PopStyleColor();
    va_end(args);
}

/* ****************************************************************************************************************** */
// eof