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

#include <chrono>
#include <ctime>
#include <algorithm>
#include <filesystem>

#include "ff_debug.h"

#include "platform.hpp"

#include "gui_inc.hpp"
#include "gui_app.hpp"

#include "gui_win_filedialog.hpp"

/* ****************************************************************************************************************** */

GuiWinFileDialog::GuiWinFileDialog(const std::string &name) :
    GuiWin(name),
    _dialogMode          { FILE_OPEN },
    _dialogState         { UNINIT },
    _showHidden          { false },
    _dirsFirst           { true },
    _editCurrentDir      { false },
    _selectedFileValid   { false },
    _sortInfo            { { NAME, ASC }, /* and because _dirsFirst=true: */{ TYPE, ASC } }
{
    _winSize    = { 100, 30 };
    _winSizeMin = {  80, 20 };
    _winFlags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking; //ImGuiWindowFlags_NoTitleBar;
    _confirmWinName = std::string("Confirm##") + _winName;

    _ChangeDir(GuiSettings::GetValue(_winName + ".lastDir"));
}

GuiWinFileDialog::~GuiWinFileDialog()
{
    GuiSettings::SetValue(_winName + ".lastDir", _currentDir);
}


// ---------------------------------------------------------------------------------------------------------------------

void GuiWinFileDialog::InitDialog(const Mode_e mode)
{
    _dialogMode  = mode;
    _dialogState = SELECT;
    _selectedPath.clear();
    _fileFilter.SetFilterStr("");
    if (_currentDir.empty())
    {
        _ChangeDir(std::filesystem::current_path());
    }
    switch (_dialogMode)
    {
        case FILE_OPEN:
            _confirmSelect    = false;
            _confirmOverwrite = false;
            WinSetTitle("Open file...");
            break;
        case FILE_SAVE:
            _confirmSelect    = false;
            _confirmOverwrite = true;
            WinSetTitle("Save file...");
            break;
    }
    WinOpen();
    WinFocus();
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWinFileDialog::IsInit()
{
    return _dialogState != UNINIT;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinFileDialog::SetFilename(const std::string &fileName)
{
    if ( (_dialogState == SELECT) && (_dialogMode == FILE_SAVE) )
    {
        _selectedFileName = fileName;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinFileDialog::SetDirectory(const std::string &directory)
{
    if (_dialogState == SELECT)
    {
        _ChangeDir(directory);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinFileDialog::SetConfirmSelect(const bool confirm)
{
    _confirmSelect = confirm;
}
// ---------------------------------------------------------------------------------------------------------------------

void GuiWinFileDialog::SetConfirmOverwrite(const bool confirm)
{
    _confirmOverwrite = confirm;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinFileDialog::SetFileFilter(const std::string regex, const bool highlight)
{
    _fileFilter.SetFilterStr(regex);
    _fileFilter.SetHightlight(highlight);
    _ChangeDir(_currentDirInput);
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWinFileDialog::DrawDialog()
{
    if (_dialogState == UNINIT)
    {
        return false;
    }

    if (!WinIsOpen())
    {
        _selectedFileValid = false;
        _Done();
        return true;
    }

    _RefreshDir();

    if (_dialogState == SELECT)
    {
        if (_DrawWindowBegin())
        {
            if (_DrawDialog())
            {
                if (!_selectedFileValid)
                {
                    _Done();
                    return true;
                }

                _selectedPath = _currentDir + "/" + _selectedFileName;

                switch (_dialogMode)
                {
                    case FILE_OPEN:
                        break;
                    case FILE_SAVE:
                        if (!Platform::FileExists(_selectedPath) && _confirmOverwrite)
                        {
                            _confirmOverwrite = false;
                        }
                        break;
                }
                if (_confirmSelect || _confirmOverwrite)
                {
                    _dialogState = CONFIRM;
                }
                else
                {
                    _dialogState = DONE;
                }
            }
            _DrawWindowEnd();
        }
    }

    else if (_dialogState == CONFIRM)
    {

        if (_DrawWindowBegin())
        {
            const FfVec2f winPos  = ImGui::GetCursorPos();
            const FfVec2f winSize = ImGui::GetContentRegionAvail();

            // Draw dialog with all stuff disabled (unclickable)
            ImGui::BeginDisabled();
            _DrawDialog();
            ImGui::EndDisabled();

            // Confirmation dialog (inspired by ImGui::BeginChildFrame())
            const FfVec2f dialogSize = GuiSettings::charSize * FfVec2f(43, 11);
            ImGui::SetCursorPos(winPos + (winSize * 0.5f) - (dialogSize * 0.5f));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, GuiSettings::style->Colors[ImGuiCol_WindowBg]);
            if (ImGui::BeginChild("Confirm", dialogSize, true, ImGuiWindowFlags_NoMove))
            {
                switch (_dialogMode)
                {
                    case FILE_OPEN:
                        ImGui::TextUnformatted("Really open this file?");
                        break;
                    case FILE_SAVE:
                        ImGui::TextUnformatted(_confirmOverwrite ? "Really overwrite this file?" : "Really save this file?");
                        break;
                }

                ImGui::NewLine();
                ImGui::TextWrapped("%s", _selectedPath.c_str());
                ImGui::NewLine();

                if (ImGui::Button(ICON_FK_CHECK " Yes, sure thing!"))
                {
                    ImGui::CloseCurrentPopup();
                    _dialogState = DONE;
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button(ICON_FK_TIMES " No, better not!"))
                {
                    _dialogState = SELECT;
                }
            }
            ImGui::PopStyleColor();
            ImGui::EndChild();

            _DrawWindowEnd();
        }
    }
    else if (_dialogState == DONE)
    {
        _Done();
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

std::string &GuiWinFileDialog::GetPath()
{
    return _resultPath;
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinFileDialog::_Done()
{
    if (_selectedFileValid)
    {
        _resultPath = _selectedPath;
    }
    _dialogState       = UNINIT;
    _selectedFileValid = false;
    _selectedFileName.clear();
    _selectedPath.clear();
    WinClose();
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinFileDialog::_ChangeDir(const std::string &path)
{
    std::string newPath;

    // Use previously given directory, default to home
    if (path.empty())
    {
        newPath = Platform::HomeDir();
    }
    // One up
    else if (path == "..")
    {
        newPath = std::filesystem::path(_currentDir).parent_path().string();
    }
    // Use given directory
    else
    {
        newPath = path;
    }

    // Check if that's a valid directory, that we can read
    try
    {
        // This will throw in case the directory doesn't exist or is not readable by us
        std::filesystem::directory_iterator dummy(newPath);
        UNUSED(dummy);

        // Set new directory
        _currentDir = newPath;

        // Keep auxillary data up-to-date
        _currentDirInput = _currentDir;
        _currentDirParts.clear();
        for (const auto &part: std::filesystem::path(_currentDir))
        {
            _currentDirParts.push_back(part);
        }

        switch (_dialogMode)
        {
            case FILE_OPEN:
                _selectedFileValid = false;
                _selectedFileName.clear();
                break;
            case FILE_SAVE:
                break;
        }

        // Load list of files
        _currentDirEntriesDirty = true;
    }
    // Otherwise carp, and don't change current directory
    catch (std::exception &e)
    {
        WARNING("%s", e.what());
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinFileDialog::_Check()
{
    switch (_dialogMode)
    {
       // In open file mode, only selected/entered file names that exist in the current directory are valid options
        case FILE_OPEN:
            _selectedFileValid = false;
            for (const auto &entry: _currentDirEntries)
            {
                if (entry.fileName == _selectedFileName)
                {
                    _selectedFileValid = true;
                    break;
                }
            }
            break;
        // In save file mode, any valid filename is allowed
        case FILE_SAVE:
            _selectedFileValid = !_selectedFileName.empty() && (_selectedFileName.find('/') == std::string::npos);
            break;
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool GuiWinFileDialog::_DrawDialog()
{
    bool res = false;

    // Controls
    bool forceSort = false;
    {
        // Options
        if (ImGui::Button(ICON_FK_BARS "##Options", GuiSettings::iconSize))
        {
            ImGui::OpenPopup("Options");
        }
        Gui::ItemTooltip("Options");
        if (ImGui::BeginPopup("Options"))
        {
            if (ImGui::Checkbox("Show hidden",  &_showHidden))
            {
                forceSort = true;
            }
            if (ImGui::Checkbox("Directories first", &_dirsFirst))
            {
                forceSort = true;
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        // Home
        if (ImGui::Button(ICON_FK_HOME "##Home", GuiSettings::iconSize))
        {
            _ChangeDir("");
            _Check();
        }
        Gui::ItemTooltip("Go to home directory");

        ImGui::SameLine();

        // One up
        if (ImGui::Button(ICON_FK_ARROW_UP "##Up", GuiSettings::iconSize))
        {
            _ChangeDir("..");
            _Check();
        }
        Gui::ItemTooltip("Go one directory level up");

    }

    Gui::VerticalSeparator();

    const float remWidth = ImGui::GetContentRegionAvail().x;

    // Current directory path
    if (ImGui::BeginChild("currentDir", ImVec2(0.75f * remWidth, GuiSettings::iconSize.y), false, ImGuiWindowFlags_NoScrollbar))
    {
        if (ImGui::Button(ICON_FK_PENCIL_SQUARE_O "##EditPath", GuiSettings::iconSize))
        {
            _editCurrentDir = !_editCurrentDir;
        }
        Gui::ItemTooltip("Edit path");

        ImGui::SameLine();

        if (_editCurrentDir)
        {
            ImGui::PushItemWidth(-1.0f);
            if (ImGui::InputTextWithHint("##Path", "Path...", &_currentDirInput, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                _ChangeDir(_currentDirInput);
                if (_currentDirInput != _currentDir)
                {
                    _currentDirInput = _currentDir;
                }
                _Check();
            }
            ImGui::PopItemWidth();
        }
        else
        {
            for (int ix = 0; ix < (int)_currentDirParts.size(); ix++)
            {
                const auto &part = _currentDirParts[ix];
                if (ix > 0)
                {
                    ImGui::SameLine(0.0f, GuiSettings::style->ItemInnerSpacing.x);
                }
                ImGui::PushID(ix + 1);
                if (ImGui::Button(part.c_str()))
                {
                    std::filesystem::path newPath { _currentDirParts[0] };
                    for (int iix = 1; iix <= ix; iix++)
                    {
                        newPath /= _currentDirParts[iix];
                    }
                    _ChangeDir(newPath);
                    _Check();
                }
                ImGui::PopID();
            }
            if (_currentDirParts.empty())
            {
                ImGui::NewLine();
            }

            ImGui::SetScrollHereX(1.0);
        }
    }
    ImGui::EndChild();

    Gui::VerticalSeparator();

    // Filter
    if (_fileFilter.DrawWidget())
    {
        _ChangeDir(_currentDir);
    }

    ImGui::Separator();

    // Selected file/dir
    {
        // Size of Cancel and Open/Save buttons
        const ImVec2 buttonSize { 10 * GuiSettings::charSize.x, 0 };

        // Input for showing (and editing) selected file name
        ImGui::PushItemWidth(-2 * (buttonSize.x + GuiSettings::style->ItemSpacing.x));
        if (!_selectedFileValid) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_ERROR)); }

        if (ImGui::InputTextWithHint("##Selected", "Name...", &_selectedFileName, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            // Accept [Enter] key as clicking the Open/Save button in case of a valid file
            if (_selectedFileValid)
            {
                res = true;
            }
        }
        if (!_selectedFileValid) { ImGui::PopStyleColor(); }

        ImGui::SameLine();

        if (ImGui::Button(ICON_FK_TIMES " Cancel", buttonSize))
        {
            _selectedFileName.clear();
            _selectedFileValid = false; // It may be valid, but user cancelled
            res = true;
        }

        ImGui::SameLine();

        // Open/save button, only available if a valid filename is selected
        ImGui::BeginDisabled(!_selectedFileValid);
        switch (_dialogMode)
        {
            case FILE_OPEN:
                if (ImGui::Button(ICON_FK_CHECK " Open", buttonSize))
                {
                    res = true;
                }
                break;
            case FILE_SAVE:
                if (ImGui::Button(ICON_FK_FLOPPY_O " Save", buttonSize))
                {
                    res = true;
                }
                break;
        }
        ImGui::EndDisabled();
    }

    // Update selected file validity. This is maybe a bit expensive, but the InpuText() callbacks don't let us do this
    _Check();

    // File list
    {
        constexpr ImGuiTableFlags tableFlags =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Hideable |
            ImGuiTableFlags_Sortable | /* ImGuiTableFlags_SortMulti | ImGuiTableFlags_SortTristate | */
            ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
            /* ImGuiTableFlags_NoBordersInBody | */ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_SizingStretchProp;

        const float typeWidth = 1.5f * GuiSettings::charSize.x;
        const struct { const char *label; float width; ImGuiTableColumnFlags flags; } columns[] =
        {
            { .label = "##Type", .width = typeWidth, .flags = ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoResize },
            { .label = "Name",   .width = 10.0f,     .flags = ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch},
            { .label = "Size",   .width =  2.0f,     .flags = ImGuiTableColumnFlags_WidthStretch },
            { .label = "Date",   .width =  4.0f,     .flags = ImGuiTableColumnFlags_WidthStretch },
        };

        if (ImGui::BeginTable(_winName.c_str(), NUMOF(columns), tableFlags))
        {
            // Header row
            ImGui::TableSetupScrollFreeze(0, 1);
            for (int ix = 0; ix < NUMOF(columns); ix++)
            {
                ImGui::TableSetupColumn(columns[ix].label, columns[ix].flags, columns[ix].width);
            }
            ImGui::TableHeadersRow();

            // Sort if needed
            // FIXME: when usinging SortMulti there doesn't seem to be any way of defaulting the order of sorted columns
            // (sort by name first, by type second). We use _dirsFirst instead, which gets us close to what we actually want.
            ImGuiTableSortSpecs *sortSpecs = ImGui::TableGetSortSpecs();
            if (sortSpecs && (forceSort || sortSpecs->SpecsDirty))
            {
                _sortInfo.clear();
                //bool sortType = false;
                for (int ix = 0; ix < sortSpecs->SpecsCount; ix++) // always 1, unless SortMulti
                {
                    const ImGuiTableColumnSortSpecs *spec = &sortSpecs->Specs[ix];
                    const ImGuiSortDirection dir = spec->SortDirection;
                    switch (spec->ColumnIndex)
                    {
                        //case 0: _sortInfo.emplace_back(Col_e::TYPE, dir == ImGuiSortDirection_Ascending ? Order_e::ASC : Order_e::DESC); sortType = true; break;
                        case 1: _sortInfo.emplace_back(Col_e::NAME, dir == ImGuiSortDirection_Ascending ? Order_e::ASC : Order_e::DESC); break;
                        case 2: _sortInfo.emplace_back(Col_e::SIZE, dir == ImGuiSortDirection_Ascending ? Order_e::ASC : Order_e::DESC); break;
                        case 3: _sortInfo.emplace_back(Col_e::DATE, dir == ImGuiSortDirection_Ascending ? Order_e::ASC : Order_e::DESC); break;
                    }
                }
                if (_dirsFirst /*&& !sortType*/)
                {
                    _sortInfo.emplace_back(Col_e::TYPE, Order_e::ASC);
                }
                sortSpecs->SpecsDirty = false;

                _currentDirEntriesDirty = true;
            }

            const DirEntry *changeDirEntry = nullptr;

            // Table body (entries)
            ImGuiListClipper clipper;
            clipper.Begin(_currentDirEntries.size());
            while (clipper.Step())
            {
                for (int ix = clipper.DisplayStart; ix < clipper.DisplayEnd; ix++)
                {
                    const DirEntry &entry = _currentDirEntries[ix];

                    ImGui::TableNextRow();
                    int colIx = 0;

                    if (entry.highlight) { ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_HIGHLIGHT)); }

                    ImGui::TableSetColumnIndex(colIx++);
                    ImGui::PushID(entry.fileName.c_str());
                    bool selected = false;
                    if (!_selectedFileName.empty() && (_selectedFileName == entry.fileName))
                    {
                        selected = true;
                    }
                    if (ImGui::Selectable(entry.fileType.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
                    {
                        // Note: For double-clicks we get in here twice!
                        const bool doubleClick = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

                        // (Double-) click on file
                        if (entry.isFile)
                        {
                            switch (_dialogMode)
                            {
                                case FILE_OPEN:
                                case FILE_SAVE:
                                    _selectedFileName = entry.fileName;
                                    _Check();
                                    if (doubleClick)
                                    {
                                        res = true;
                                    }
                                    break;
                            }
                        }
                        // (Double-) click on directory
                        else if (entry.isDir)
                        {
                            switch (_dialogMode)
                            {
                                case FILE_OPEN:
                                case FILE_SAVE:
                                    if (doubleClick)
                                    {
                                        changeDirEntry = &entry;
                                    }
                            }
                        }
                    }
                    ImGui::PopID();

                    ImGui::TableSetColumnIndex(colIx++);
                    ImGui::TextUnformatted(entry.fileName.c_str());
                    if (!entry.linkTarget.empty())
                    {
                        ImGui::SameLine();
                        // ImGui::PushStyleColor(ImGuiCol_Text, GUI_COLOUR(TEXT_DIM));
                        ImGui::TextUnformatted(entry.linkTarget.c_str());
                        // ImGui::PopStyleColor();
                    }

                    if (!entry.fileSize.empty())
                    {
                        ImGui::TableSetColumnIndex(colIx++);
                        // Right align
                        // if (ix == clipper.DisplayStart)
                        // {
                        //     ImGui::PushItemWidth(-FLT_MIN); // FIXME: doesn't work it seems, but it does in imgui_demo.cpp
                        // }
                        const float columnWidth = ImGui::GetColumnWidth();
                        if (entry.fileSizeWidth < columnWidth)
                        {
                            ImGui::Dummy(ImVec2(entry.fileSizeLeftPad + columnWidth - entry.fileSizeWidth, 1.0f));
                            ImGui::SameLine(0, 0);
                        }
                        ImGui::TextUnformatted(entry.fileSize.c_str());
                    }
                    else
                    {
                        colIx++;
                    }
                    ImGui::TableSetColumnIndex(colIx++);
                    ImGui::TextUnformatted(entry.fileDate.c_str());

                    if (entry.highlight) { ImGui::PopStyleColor(); }
                }
            }
            ImGui::EndTable();

            // Change dir after iterating _currentDirEntries as _ChangeDir() modifies that!
            if (changeDirEntry)
            {
                _ChangeDir(changeDirEntry->fullPath);
                _Check();
            }
        }
    }

    return res;
}

// ---------------------------------------------------------------------------------------------------------------------

// https://stackoverflow.com/questions/56788745/how-to-convert-stdfilesystemfile-time-type-to-a-string-using-gcc-9/58237530#58237530
template <typename TP>
std::time_t to_time_t(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
              + system_clock::now());
    return system_clock::to_time_t(sctp);
}

namespace fs = std::filesystem;

GuiWinFileDialog::DirEntry::DirEntry(const std::filesystem::directory_entry &entry) :
    fullPath{entry.path().string()}, fileSizeLeftPad{0.0f}, isFile{false}, isDir{false}, isHidden{false}, highlight{false}
{
    if (entry.is_regular_file())
    {
        fileType = ICON_FK_FILE_O;
        sortType = 1;
        isFile   = true;
    }
    else if (entry.is_directory())
    {
        fileType = ICON_FK_FOLDER_O;
        sortType = 0;
        isDir    = true;
    }
    else
    {
        throw std::runtime_error("wtf?!");
    }

    std::filesystem::path fsPath { fullPath };
    fileName = fsPath.filename().string();
    if (entry.is_symlink())
    {
        //name += " " ICON_FK_LONG_ARROW_RIGHT;
        //name += " " ICON_FK_LINK;
        linkTarget = "-> " + std::filesystem::read_symlink(fullPath).string();
    }

    isHidden = fileName.substr(0, 1) == ".";

    sortDate = to_time_t(entry.last_write_time());
    fileDate = std::asctime(std::localtime(&sortDate));

    bool haveSize = false;
    if (!entry.is_directory())
    {
        sortSize = entry.file_size();
        haveSize = true;
    }
    else
    {
        // sortSize = std::filesystem::directory_entry( entry.path() / "." ).file_size(); // TODO
        sortSize = 0; // std::filesystem::directory_entry( entry.path() / "." ).file_size();
    }
    if (haveSize)
    {
        if (sortSize > (1024 * 1024 * 1024))
        {
            fileSize = Ff::Sprintf("%.1f GiB", (double)sortSize / (1024.0 * 1024.0 * 1024.0));
        }
        else if (sortSize > (1024 * 1024))
        {
            fileSize = Ff::Sprintf("%.1f MiB", (double)sortSize / (1024.0 * 1024.0));
        }
        else if (sortSize > 1024)
        {
            fileSize = Ff::Sprintf("%.1f KiB", (double)sortSize / 1024.0);
        }
        else
        {
            fileSize = Ff::Sprintf("%lu B", sortSize);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void GuiWinFileDialog::_RefreshDir()
{
    if (!_currentDirEntriesDirty)
    {
        return;
    }
    _currentDirEntriesDirty = false;

    _currentDirEntries.clear();
    if (_currentDir.empty())
    {
        return;
    }

    const bool filterActive    = _fileFilter.IsActive();
    const bool filterHighlight = _fileFilter.IsHighlight();

    // Find files in directory
    for (const auto &entry: std::filesystem::directory_iterator{_currentDir})
    {
        try
        {
            DirEntry dirEntry { entry };
            const bool filterMatch = _fileFilter.Match(dirEntry.fileName);

            if (_showHidden || !dirEntry.isHidden)
            {
                // Always add directories to the list
                if (dirEntry.isDir)
                {
                    dirEntry.highlight = filterActive && filterMatch;
                    _currentDirEntries.push_back(dirEntry);
                }
                // Add file entries only depending on filter settings
                else if (dirEntry.isFile)
                {
                    if (!filterActive || filterHighlight || filterMatch)
                    {
                        dirEntry.highlight = filterActive && filterMatch;
                        _currentDirEntries.push_back(dirEntry);
                    }
                }
            }
        }
        // Not a dir or regular file (fifo, device, etc.)
        catch (std::exception &e)
        {
            //WARNING("%s", e.what());
        }
    }

    // Calculate padding for size string (to right-align it)
    float maxWidth = 0.0f;
    for (auto &entry: _currentDirEntries)
    {
        entry.fileSizeWidth = ImGui::CalcTextSize(entry.fileSize.c_str()).x;
        if (entry.fileSizeWidth > maxWidth)
        {
            maxWidth = entry.fileSizeWidth;
        }
    }
    for (auto &entry: _currentDirEntries)
    {
        entry.fileSizeLeftPad = maxWidth - entry.fileSizeWidth;
        entry.fileSizeWidth += entry.fileSizeLeftPad;
    }

    // Sort
    std::sort(_currentDirEntries.begin(), _currentDirEntries.end(),
        [&](const DirEntry &a, const DirEntry &b)
        {
            //for (const auto &sort: _sortInfo)
            for (auto sortIt = _sortInfo.rbegin(); sortIt != _sortInfo.rend(); sortIt++)
            {
                const auto &sort = *sortIt;
                switch (sort.col)
                {
                    case Col_e::TYPE:
                        if (a.sortType != b.sortType)
                        {
                            return sort.order == Order_e::ASC ? a.sortType < b.sortType : b.sortType < a.sortType;
                        }
                        break;
                    case Col_e::NAME:
                        if (a.fileName != b.fileName)
                        {
                            return sort.order == Order_e::ASC ? a.fileName < b.fileName : b.fileName < a.fileName;
                        }
                        break;
                    case Col_e::SIZE:
                        if (a.sortSize != b.sortSize)
                        {
                            return sort.order == Order_e::ASC ? a.sortSize < b.sortSize : b.sortSize < a.sortSize;
                        }
                        break;
                    case Col_e::DATE:
                        if (a.sortDate != b.sortDate)
                        {
                            return sort.order == Order_e::ASC ? a.sortDate < b.sortDate : b.sortDate < a.sortDate;
                        }
                        break;
                }
            }
            return false;
        });

    DEBUG("_RefreshDir() %s, %d entries", _currentDir.c_str(), (int)_currentDirEntries.size());
}

/* ****************************************************************************************************************** */
