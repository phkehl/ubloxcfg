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

#include "gui_inc.hpp"

#include "gui_notify.hpp"

/* ****************************************************************************************************************** */

GuiNotify::GuiNotify()
{
}

// ---------------------------------------------------------------------------------------------------------------------

GuiNotify::Notification::Notification(
    const enum Type _type, const std::string &_title, const std::string &_content, const double _duration) :
    type      { _type },
    name      { "Notification" + std::to_string(++notiNr) },
    title     { _title },
    content   { _content },
    created   { ImGui::GetTime() },
    duration  { CLIP(_duration, 1.0, 30.0) },
    destroy   { created + duration },
    remaining { destroy - created },
    hovered   { false }
{
}

/*static*/ uint32_t GuiNotify::Notification::notiNr = 0;

// ---------------------------------------------------------------------------------------------------------------------

/*static*/ std::vector<GuiNotify::Notification> GuiNotify::_notifications = {};

/*static*/ std::mutex GuiNotify::_mutex;

/*static*/ void GuiNotify::Message(const std::string &_title, const std::string &_content, const double _duration)
{
    std::lock_guard<std::mutex> guard(_mutex);
    _notifications.push_back(Notification(Notification::MESSAGE, _title, _content, _duration));
}

/*static*/ void GuiNotify::Notice(const std::string &_title, const std::string &_content, const double _duration)
{
    std::lock_guard<std::mutex> guard(_mutex);
    _notifications.push_back(Notification(Notification::NOTICE, _title, _content, _duration));
}

/*static*/ void GuiNotify::Warning(const std::string &_title, const std::string &_content, const double _duration)
{
    std::lock_guard<std::mutex> guard(_mutex);
    _notifications.push_back(Notification(Notification::WARNING, _title, _content, _duration));
}

/*static*/ void GuiNotify::Error(const std::string &_title, const std::string &_content, const double _duration)
{
    std::lock_guard<std::mutex> guard(_mutex);
    _notifications.push_back(Notification(Notification::ERROR, _title, _content, _duration));
}

/*static*/ void GuiNotify::Success(const std::string &_title, const std::string &_content, const double _duration)
{
    std::lock_guard<std::mutex> guard(_mutex);
    _notifications.push_back(Notification(Notification::SUCCESS, _title, _content, _duration));
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiNotify::Loop(const double now)
{
    std::lock_guard<std::mutex> guard(_mutex);
    // Remove expired notifications
    for (auto iter = _notifications.begin(); iter != _notifications.end(); )
    {
        if ( (iter->destroy >= 0.0) && (iter->destroy < now) )
        {
            iter = _notifications.erase(iter);
        }
        else
        {
            iter->remaining = iter->destroy - now;
            iter++;
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiNotify::Draw()
{
    if (_notifications.empty())
    {
        return;
    }

    auto &io = ImGui::GetIO();
    auto &style = ImGui::GetStyle();

    const auto charSize =  ImGui::CalcTextSize("X");
    const FfVec2f notiMinSize { charSize.x * 50, charSize.y };
    const float maxContentHeight = charSize.y * 5;
    const FfVec2f notiMaxSize { charSize.x * 50, maxContentHeight + (4 * charSize.y) };
    FfVec2f winPos { io.DisplaySize.x - charSize.x, io.DisplaySize.y - charSize.y };

    std::lock_guard<std::mutex> guard(_mutex);

    for (auto iter = _notifications.rbegin(); iter != _notifications.rend(); iter++)
    {
        auto &noti = *iter;

        // Open notification window
        ImGui::SetNextWindowPos(winPos, ImGuiCond_Always, ImVec2(1, 1));
        ImGui::SetNextWindowSizeConstraints(notiMinSize, notiMaxSize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, charSize.x);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, (noti.hovered ? 1.0f : 0.65f) * style.Alpha);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, style.Colors[ImGuiCol_MenuBarBg]);
        const auto winFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoDocking;
        const bool winOpen = ImGui::Begin(noti.name.c_str(), nullptr, winFlags);
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();

        if (!winOpen)
        {
            continue;
        }

        // Remaining time bar
        if (noti.remaining > 0)
        {
            const FfVec2f bar0 = FfVec2f(ImGui::GetCursorScreenPos()) + FfVec2f(style.FramePadding.x, -style.FramePadding.y);
            const float width = ImGui::GetContentRegionAvail().x - ( 2 * style.FramePadding.x);
            const float frac = noti.remaining / noti.duration;
            const FfVec2f bar1 = FfVec2f(bar0.x + (frac * width), bar0.y);
            auto *draw = ImGui::GetWindowDrawList();
            draw->AddLine(bar0, bar1, GUI_COLOUR(TEXT_DIM), 2.0f);
        }

        // Render notification
        const char *title = nullptr;
        bool haveIcon = true;
        switch (noti.type)
        {
            case Notification::MESSAGE:
                haveIcon = false;
                break;
            case Notification::NOTICE:
                ImGui::TextColored(ImVec4(0, 157, 255, 255), ICON_FK_INFO_CIRCLE);
                title = "Notice";
                break;
            case Notification::WARNING:
                ImGui::TextColored(ImVec4(255, 255, 0, 255), ICON_FK_EXCLAMATION_TRIANGLE);
                title = "Warning";
                break;
            case Notification::ERROR:
                ImGui::TextColored(ImVec4(255, 0, 0, 255), ICON_FK_TIMES_CIRCLE);
                title = "Error";
                break;
            case Notification::SUCCESS:
                ImGui::TextColored(ImVec4(0, 255, 0, 255), ICON_FK_CHECK_CIRCLE);
                title = "Success";
                break;
        }

        if (!noti.title.empty())
        {
            title = noti.title.c_str();
        }

        // Title
        if (title)
        {
            if (haveIcon)
            {
                ImGui::SameLine();
            }
            ImGui::TextUnformatted(title);
        }

        // Show controls when hovered
        noti.hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        if (noti.hovered)
        {
            const float maxX = ImGui::GetContentRegionAvail().x;

            // Destroy now
            ImGui::SameLine(maxX - (2 * charSize.x));
            if (ImGui::SmallButton("X"))
            {
                noti.destroy = 0.0;
            }

            // Never destroy (showing remaining time)
            if (noti.destroy > 0.0)
            {
                char str[20];
                std::snprintf(str, sizeof(str), "%.1f###destroy", noti.remaining);
                ImGui::SameLine(maxX - (8 * charSize.x));
                if (ImGui::SmallButton(str))
                {
                    noti.destroy = -1.0;
                    noti.remaining = -1.0;
                }
            }
        }

        // Content
        if (!noti.content.empty())
        {
            if (title)
            {
                ImGui::Separator();
            }
            ImVec2 textSize = ImGui::CalcTextSize(noti.content.c_str(), nullptr, false,
                ImGui::GetContentRegionAvail().x - (3 * charSize.x));
            if (ImGui::BeginChild("Content", ImVec2(0, MIN(textSize.y, maxContentHeight))))
            {
                ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
                ImGui::TextUnformatted(noti.content.c_str());
                ImGui::PopTextWrapPos();
            }
            ImGui::EndChild();
        }

        // TODO: close on click (show (x) on hover?), allow to keep it forever,
        //       handle too many notifications, ...

        // Bottom right of next notification
        float newWinPosY = ImGui::GetWindowPos().y - charSize.y;
        if (newWinPosY > notiMaxSize.y)
        {
            winPos.y = newWinPosY;
        }
        ImGui::End();
    }
}

/* ****************************************************************************************************************** */
