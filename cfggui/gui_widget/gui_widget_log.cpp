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

#include "gui_inc.hpp"

#include "gui_widget_log.hpp"

/* ****************************************************************************************************************** */

GuiWidgetLog::LogLine::LogLine(const char *_str, const ImU32 _colour) :
    str    { _str },
    len    { (int)std::strlen(_str) },
    time   { ImGui::GetTime() },
    colour { _colour },
    match  { false }
{
    // FIXME: we should do this in ff_debug
    Ff::StrReplace(str, "\r", "\\r");
    Ff::StrReplace(str, "\n", "\\n");
}

GuiWidgetLog::LogLine::LogLine(const std::string &_str, const ImU32 _colour) :
    str    { _str },
    len    { (int)_str.size() },
    time   { ImGui::GetTime() },
    colour { _colour },
    match  { false }
{
    // FIXME: we should do this in ff_debug
    Ff::StrReplace(str, "\r", "\\r");
    Ff::StrReplace(str, "\n", "\\n");
}

// ---------------------------------------------------------------------------------------------------------------------

GuiWidgetLog::GuiWidgetLog(const int maxLines) :
    _lines          { },
    _logTimestamps  { true },
    _logAutoscroll  { true },
    _scrollToBottom { true },
    _logCopy        { false },
    _filterNumMatch { 0 }
{
    _maxLines = CLIP(maxLines, 10, 100000);
}

// ---------------------------------------------------------------------------------------------------------------------

std::string GuiWidgetLog::GetSettings()
{
    return Ff::Sprintf("%d:%d:", _logTimestamps ? 1 : 0, _logAutoscroll ? 1 : 0) + _filterWidget.GetFilterSettings();
}

void GuiWidgetLog::SetSettings(const std::string &settings)
{
    const auto set = Ff::StrSplit(settings, ":", 3);
    if (set.size() == 3)
    {
        _logTimestamps = (set[0] == "1");
        _logAutoscroll = (set[1] == "1");
        _filterWidget.SetFilterSettings(set[2]);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWidgetLog::Clear()
{
    _lines.clear();
    _filterNumMatch = 0;
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
    DrawControls();
    ImGui::Separator();
    //ImGui::Text("Lines: %d/%d, %d match", (int)_lines.size(), _maxLines, _filterNumMatch);
    DrawLog();
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

void GuiWidgetLog::DrawControls()
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
        if (ImGui::Checkbox("Auto-scroll", &_logAutoscroll))
        {
            _scrollToBottom = _logAutoscroll;
        }
        ImGui::Checkbox("Timestamps", &_logTimestamps);
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    // Clear
    if (ImGui::Button(ICON_FK_ERASER "##Clear"))
    {
        Clear();
    }
    Gui::ItemTooltip("Clear log");

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

void GuiWidgetLog::DrawLog()
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

    ImGui::PopStyleVar();

    if (_scrollToBottom)
    {
        ImGui::SetScrollHereY();
        _scrollToBottom = false;
    }

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

    if (ImGui::BeginPopupContextItem("LogSettings"))
    {
        if (ImGui::Checkbox("Auto-scroll", &_logAutoscroll))
        {
            _scrollToBottom = _logAutoscroll;
        }
        ImGui::Checkbox("Timestamps", &_logTimestamps);
        ImGui::Separator();
        if (ImGui::MenuItem("Clear log"))
        {
            Clear();
        }
        ImGui::EndPopup();
    }

}

/* ****************************************************************************************************************** */
// eof