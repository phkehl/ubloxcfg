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

#include "gui_inc.hpp"

#include "gui_widget_filter.hpp"

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
    _filterChanged{true},
    _filterHelp(help)
{
}

// ---------------------------------------------------------------------------------------------------------------------

std::string GuiWidgetFilter::GetFilterSettings()
{
    return Ff::Sprintf("%d:%d:%d:%s",
        _filterCaseSensitive ? 1 : 0, _filterHighlight ? 1 : 0, _filterInvert ? 1 : 0, _filterStr.c_str());
}

void GuiWidgetFilter::SetFilterSettings(const std::string &settings)
{
    const auto set = Ff::StrSplit(settings, ":", 4);
    if (set.size() == 4)
    {
        _filterCaseSensitive = (set[0] == "1");
        _filterHighlight     = (set[1] == "1");
        _filterInvert        = (set[2] == "1");
        _filterStr = set[3];
        _filterChanged = true;
    }
    _UpdateFilter();
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWidgetFilter::DrawWidget(const float width)
{
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
            _filterChanged = true;
        }

        ImGui::Separator();

        if (ImGui::Checkbox("Filter case sensitive", &_filterCaseSensitive))
        {
            _filterChanged = true;
        }
        if (ImGui::Checkbox("Filter highlights", &_filterHighlight))
        {
            _filterChanged = true;
        }
        if (ImGui::Checkbox("Invert match", &_filterInvert))
        {
            _filterChanged = true;
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
        _filterChanged = true;
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
    if (_filterChanged)
    {
        _filterUpdated = now;
        _filterChanged = false;
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

void GuiWidgetFilter::SetHightlight(const bool highlight)
{
    _filterHighlight = highlight;
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
