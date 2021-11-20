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
