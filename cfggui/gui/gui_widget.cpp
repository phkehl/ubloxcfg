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

#include "ff_cpp.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"
#include "IconsForkAwesome.h"

#include "gui_settings.hpp"
#include "gui_widget.hpp"

/* ****************************************************************************************************************** */

GuiWidgetFilter::GuiWidgetFilter(const std::string help) :
    _filterState{FILTER_NONE},
    _filterStr{},
    _filterUpdated{0.0},
    _filterErrorMsg{},
    _filterUserMsg{},
    _filterCaseSensitive{false},
    _filterHighlight{false},
    _filterInvert{false},
    _filterHelp(help)
{
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetFilter::DrawWidget(const float width)
{
    bool filterChanged = false;
    ImGui::PushID(this);

    // Filter menu button
    if (ImGui::Button(ICON_FK_FILTER "##FilterMenu"))
    {
        ImGui::OpenPopup("FilterMenu");
    }
    Gui::ItemTooltip("Filter options");
    if (ImGui::BeginPopup("FilterMenu"))
    {
        if (ImGui::MenuItem("Clear filter", NULL, false, _filterStr[0] != '\0'))
        {
            _filterStr[0] = '\0';
            filterChanged = true;
        }

        ImGui::Separator();

        if (ImGui::Checkbox("Filter case sensitive", &_filterCaseSensitive))
        {
            filterChanged = true;
        }
        if (ImGui::Checkbox("Filter highlights", &_filterHighlight))
        {
            filterChanged = true;
        }
        if (ImGui::Checkbox("Invert match", &_filterInvert))
        {
            filterChanged = true;
        }

        if (_filterHelp.size() > 0)
        {
            ImGui::Separator();
            ImGui::MenuItem("Help");
            Gui::ItemTooltip(_filterHelp.c_str());
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

    // Filter input
    if (_filterState == FILTER_BAD)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_BRIGHTRED));
    }
    ImGui::PushItemWidth(width);
    if (ImGui::InputTextWithHint("##Filter", "Filter (regex)", &_filterStr))
    {
        filterChanged = true;
    }
    ImGui::PopItemWidth();
    if (_filterState == FILTER_BAD)
    {
        ImGui::PopStyleColor();
    }
    if (!_filterErrorMsg.empty())
    {
        Gui::ItemTooltip(_filterErrorMsg.c_str());
    }
    else if ( (_filterState == FILTER_OK) && !_filterUserMsg.empty() )
    {
        Gui::ItemTooltip(_filterUserMsg.c_str());
    }

    ImGui::PopID();

    // Let caller know if the filter has changed
    const double now = ImGui::GetTime();
    if (filterChanged)
    {
        _filterUpdated = now;
    }
    else
    {
        if (_filterUpdated != 0.0)
        {
            // Debounce
            if ( (now - _filterUpdated) > 0.3 )
            {
                _UpdateFilter();
                _filterUpdated = 0.0;
                return true;
            }
        }
    }

    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetFilter::_UpdateFilter()
{
    // Filter cleared
    if (_filterStr.empty())
    {
        _filterState = FILTER_NONE;
        _filterErrorMsg.clear();
        _filterRegex = nullptr;
        return;
    }

    // Try regex
    try
    {
        std::regex_constants::syntax_option_type flags =
            std::regex_constants::nosubs | std::regex_constants::optimize;
        if (!_filterCaseSensitive)
        {
            flags |= std::regex_constants::icase;
        }
        _filterRegex = std::make_unique<std::regex>(_filterStr, flags);
        _filterState = FILTER_OK;
        _filterErrorMsg.clear();
    }
    //catch (const std::regex_error &e)
    //{
    //    snprintf(_filterMsg, sizeof(_filterMsg), "Bad filter: %s", e.what());
    //    _filterState = FILTER_BAD;
    //}
    catch (const std::exception &e)
    {
        _filterErrorMsg = Ff::Sprintf("Bad filter: %s", e.what());
        _filterState = FILTER_BAD;
        _filterRegex = nullptr;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetFilter::Match(const std::string &str)
{
    bool match = true;
    switch (_filterState)
    {
        case FILTER_OK:
            match = std::regex_search(str, *_filterRegex) == !_filterInvert;
            break;
        case FILTER_BAD:
        case FILTER_NONE:
            break;
    }
    return match;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetFilter::IsActive()
{
    return _filterState == FILTER_OK;
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetFilter::IsHighlight()
{
    return _filterHighlight;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetFilter::SetUserMsg(const std::string &msg)
{
    _filterUserMsg = msg;
}

// ---------------------------------------------------------------------------------------------------------------------

std::string GuiWidgetFilter::GetFilterStr()
{
    return _filterStr;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetFilter::SetFilterStr(const std::string &str)
{
    _filterStr = str;
    _UpdateFilter();
}

/* ****************************************************************************************************************** */

GuiWidgetLog::LogLine::LogLine(const char *_str, const ImU32 _colour) :
    str{_str},
    len{(int)std::strlen(_str)},
    time{ImGui::GetTime()},
    colour{_colour},
    match{false}
{
}

GuiWidgetLog::LogLine::LogLine(const std::string &_str, const ImU32 _colour) :
    str{_str},
    len{(int)_str.size()},
    time{ImGui::GetTime()},
    colour{_colour},
    match{false}
{
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWidgetLog::GuiWidgetLog(const int maxLines, const bool controls) :
    _lines{},
    _drawControls{controls},
    _logTimestamps{true},
    _logAutoscroll{true},
    _logCopy{false},
    _filterNumMatch{0}
{
    _maxLines = CLIP(maxLines, 10, 100000);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetLog::Clear()
{
    _lines.clear();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetLog::AddLine(const char *line, const ImU32 colour)
{
    if ( (line != NULL) && (line[0] != '\0') )
    {
        _AddLogLine(LogLine(line, colour));
    }
}

void GuiWidgetLog::AddLine(const std::string &line, const ImU32 colour)
{
    if (line.size() > 0)
    {
        _AddLogLine(LogLine(line, colour));
    }
}

void GuiWidgetLog::_AddLogLine(LogLine logLine)
{
    logLine.match = _filterWidget.Match(logLine.str);
    if (logLine.match)
    {
        _filterNumMatch++;
    }
    _lines.push_back(logLine);

    while ((int)_lines.size() > _maxLines)
    {
        if (_lines[0].match)
        {
            _filterNumMatch--;
        }
        _lines.pop_front();
    }

    if (_filterWidget.IsActive())
    {
        _filterWidget.SetUserMsg(Ff::Sprintf("Filter matches %d of %d lines", _filterNumMatch, (int)_lines.size()));
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetLog::DrawWidget()
{
    if (_drawControls)
    {
        _DrawLogControls();
        ImGui::Separator();
    }

    //ImGui::Text("Lines: %d/%d, %d match", (int)_lines.size(), _maxLines, _filterNumMatch);

    _DrawLogLines();
}

// ---------------------------------------------------------------------------------------------------------------------

std::string GuiWidgetLog::GetFilterStr()
{
    return _filterWidget.GetFilterStr();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetLog::SetFilterStr(const std::string &str)
{
    _filterWidget.SetFilterStr(str);
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetLog::_DrawLogControls()
{
    ImGui::PushID(this);

    // Options
    if (ImGui::Button(ICON_FK_COG "##Options"))
    {
        ImGui::OpenPopup("Options");
    }
    Gui::ItemTooltip("Options");
    if (ImGui::BeginPopup("Options"))
    {
        ImGui::Checkbox("Auto-scroll", &_logAutoscroll);
        ImGui::Checkbox("Timestamps", &_logTimestamps);
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    // Clear
    if (ImGui::Button(ICON_FK_ERASER "##Clear"))
    {
        Clear();
    }
    Gui::ItemTooltip("Clear");

    ImGui::SameLine();

    // Copy
    _logCopy = ImGui::Button(ICON_FK_FILES_O "##Copy");
    Gui::ItemTooltip("Copy log to clipboard");

    Gui::VerticalSeparator();

    // Filter
    if (_filterWidget.DrawWidget())
    {
        // Update filter match flags
        const bool active = _filterWidget.IsActive();
        _filterNumMatch = 0;
        for (auto &line: _lines)
        {
            line.match = active ? _filterWidget.Match(line.str) : true;
            if (line.match)
            {
                _filterNumMatch++;
            }
        }
    }

    ImGui::PopID();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetLog::_DrawLogLines()
{
    // Work out longest line and consider that for the horizontal scrolling
    if (_lines.size() > 0)
    {
        int maxLen = 0;
        const std::string *maxStr = &_lines[0].str;
        for (auto &line: _lines)
        {
            if (line.len > maxLen)
            {
                maxLen = line.len;
                maxStr = &line.str;
            }
        }
        if (maxLen > 0)
        {
            const float maxWidth = ImGui::CalcTextSize(maxStr->c_str()).x;
            // https://github.com/ocornut/imgui/issues/3285
            ImGui::SetNextWindowContentSize(ImVec2(maxWidth + 100, 0.0)); // Add a bit for the timestamp...
        }
    }

    ImGui::BeginChild("Lines", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (_logCopy)
    {
        ImGui::LogToClipboard(); // copy all text output from now on
    }

    ImVec2 spacing = ImGui::GetStyle().ItemInnerSpacing;
    spacing.y *= 0.5;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, spacing);
    //ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_WHITE));

    const bool haveFilter = _filterWidget.IsActive();
    const bool highlight = haveFilter && _filterWidget.IsHighlight();

    // Displaying all lines -> can use _lines[ix]
    if (!haveFilter || highlight)
    {
        const float dimVal = ImGui::GetStyle().Alpha * 0.4f;
        ImGuiListClipper clipper;
        clipper.Begin(_lines.size());
        while (clipper.Step())
        {
            for (int ix = clipper.DisplayStart; ix < clipper.DisplayEnd; ix++)
            {
                auto &line = _lines[ix];
                if (_logTimestamps)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_GREY));
                    ImGui::Text("%09.3f", line.time);
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                }
                const bool dim = highlight && !line.match;
                if (dim) { ImGui::PushStyleVar(ImGuiStyleVar_Alpha, dimVal); }
                if (line.colour != 0) { ImGui::PushStyleColor(ImGuiCol_Text, line.colour); }
                ImGui::TextUnformatted(line.str.c_str());
                if (line.colour != 0) { ImGui::PopStyleColor(); }
                if (dim) { ImGui::PopStyleVar(); }
            }
        }
        clipper.End();
    }
    // Displaying only some lines
    else
    {
        ImGuiListClipper clipper;
        clipper.Begin(_filterNumMatch);
        while (clipper.Step())
        {
            int linesIx = -1;
            int matchIx = -1;
            for (int ix = clipper.DisplayStart; ix < clipper.DisplayEnd; ix++)
            {
                // Find (ix+1)-th matching line
                while (matchIx < ix)
                {
                    while (true)
                    {
                        if (linesIx >= (int)_lines.size())
                        {
                            matchIx = ix;
                            break;
                        }
                        else if (_lines[++linesIx].match)
                        {
                            matchIx++;
                            break;
                        }
                    }
                }
                if (linesIx < (int)_lines.size())
                {
                    auto &line = _lines[linesIx];
                    if (_logTimestamps)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(C_GREY));
                        ImGui::Text("%09.3f", line.time);
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                    }
                    if (line.colour != 0) { ImGui::PushStyleColor(ImGuiCol_Text, line.colour); }
                    ImGui::TextUnformatted(line.str.c_str());
                    if (line.colour != 0) { ImGui::PopStyleColor(); }
                }
                //else
                //{
                //    ImGui::TextUnformatted("wtf?!");
                //}
            }
        }
    }

    //ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    if (_logCopy)
    {
        ImGui::LogFinish();
    }

    // Autoscroll, but only when at end
    if (_logAutoscroll && (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}

/* ****************************************************************************************************************** */

// https://github.com/ocornut/imgui/issues/1889#issuecomment-398681105

void Gui::BeginDisabled()
{
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.65f);
}

void Gui::EndDisabled()
{
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
}

// ---------------------------------------------------------------------------------------------------------------------

ImU32 Gui::ColourHSV(const float h, const float s, const float v, const float a)
{
    ImColor c;
    ImGui::ColorConvertHSVtoRGB(h, s, v, c.Value.x, c.Value.y, c.Value.z);
    c.Value.w = a;
    return c;
}

// ---------------------------------------------------------------------------------------------------------------------

void Gui::SetWindowScale(const float scale)
{
    if ( (scale > 0.24) && (scale < 4.01) )
    {
        auto ctx = ImGui::GetCurrentContext();
        auto win = ctx->CurrentWindow;
        win->FontWindowScale = scale;
    }
}

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

bool Gui::ItemTooltipBegin(const double delay)
{
    if (ImGui::IsItemHovered() && (GImGui->HoveredIdTimer > delay))
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

bool Gui::ItemTooltip(const char *text, const double delay)
{
    if (ItemTooltipBegin(delay))
    {
        ImGui::TextUnformatted(text);
        ItemTooltipEnd();
        return true;
    }
    return false;
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

/* ****************************************************************************************************************** */

GuiWidgetTable::GuiWidgetTable() :
    _totalWidth{0.0f}
{
}

void GuiWidgetTable::AddColumn(const char *title, const float width)
{
    _columns.push_back(Column(title, width));
    _totalWidth += width;
}

GuiWidgetTable::Column::Column(const char *_title, const float _width)
    : title{}, width{_width}
{
    std::strncpy(title, _title, sizeof(title) - 1);
}

void GuiWidgetTable::BeginDraw()
{
    ImGui::PushID(this);

    const float currWidth = ImGui::GetWindowContentRegionWidth();

    // Child window of fixed width with horizontal scrolling
    ImGui::SetNextWindowContentSize(ImVec2(currWidth > _totalWidth ? currWidth : _totalWidth, 0.0f));
    ImGui::BeginChild("##Table", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

    // Header
    ImGui::BeginChild("##Head", ImVec2(0.0f, ImGui::GetTextLineHeight() * 1.5));

    // Setup columns
    ImGui::Columns(_columns.size(), "##HeadCols");
    int ix = 0;
    for (auto &col: _columns)
    {
        ImGui::SetColumnWidth(ix, col.width);
        ix++;
    }

    // Title
    ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_TITLE));
    for (auto &col: _columns)
    {
        ImGui::TextUnformatted(col.title);
        ImGui::NextColumn();
    }
    ImGui::PopStyleColor();

    ImGui::Separator();

    ImGui::EndChild(); // Head

    // Body
    ImGui::BeginChild("##Body", ImVec2(0.0f, 0.0f));

    // Setup columns
    ImGui::Columns(_columns.size(), "##BodyCols");
    ix = 0;
    for (auto &col: _columns)
    {
        ImGui::SetColumnWidth(ix, col.width);
        ix++;
    }

    // Col*()...
    // ...
    // EndDraw()
}

void GuiWidgetTable::ColText(const char *text)
{
    ImGui::TextUnformatted(text);
    ImGui::NextColumn();
}

void GuiWidgetTable::ColTextF(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
    ImGui::NextColumn();
}

bool GuiWidgetTable::ColSelectable(const char *label, bool selected, ImGuiSelectableFlags flags)
{
    const bool res = ImGui::Selectable(label, selected, flags);
    ImGui::NextColumn();
    return res;
}

void GuiWidgetTable::ColSkip()
{
    ImGui::NextColumn();
}

void GuiWidgetTable::EndDraw()
{
    ImGui::EndChild(); // Body
    ImGui::EndChild(); // Table
    ImGui::PopID();
}

/* ****************************************************************************************************************** */
// eof