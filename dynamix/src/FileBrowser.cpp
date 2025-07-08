#include "FileBrowser.h"
#include <algorithm>
#include <iostream>

namespace Dynamix {

    FileBrowser::FileBrowser() : m_selectedIndex(-1), m_filterDirty(false) {}

    void FileBrowser::initialize(const Config& config) {
        m_config = config;
        m_currentPath = config.defaultPath;
        m_filter = config.filterText;
        refresh();
    }

    void FileBrowser::setSelectionCallback(SelectionCallback callback) {
        m_selectionCallback = callback;
    }

    void FileBrowser::setCurrentPath(const std::string& path) {
        m_currentPath = path;
        refresh();
    }

    void FileBrowser::setFilter(const std::string& filter) {
        m_filter = filter;
        m_filterDirty = true;
    }

    void FileBrowser::refresh() {
        scanDirectory();
        applyFilter();
    }

    bool FileBrowser::render(bool& isOpen) {
        if (!isOpen) {
            return false;
        }

        ImGui::SetNextWindowSize(m_config.windowSize, ImGuiCond_FirstUseEver);
        if (ImGui::Begin(m_config.title.c_str(), &isOpen)) {
            // Display current path for user orientation
            ImGui::Text("Current Path: %s", m_currentPath.c_str());

            // Navigation controls section
            if (ImGui::Button("Go Up")) {
                navigateUp();
            }
            ImGui::SameLine();
            if (ImGui::Button("Home")) {
                navigateHome();
            }
            ImGui::SameLine();
            if (ImGui::Button("Refresh")) {
                refresh();
            }

            ImGui::Separator();

            // Real-time filtering section
            ImGui::Text("Filter:");
            ImGui::SameLine();
            char filterBuffer[256];
            strncpy(filterBuffer, m_filter.c_str(), sizeof(filterBuffer) - 1);
            filterBuffer[sizeof(filterBuffer) - 1] = '\0';

            if (ImGui::InputText("##filter", filterBuffer, sizeof(filterBuffer))) {
                setFilter(filterBuffer);
            }

            ImGui::Separator();

            // File/directory list with scrollable area
            ImGui::BeginChild("##browser_list", ImVec2(0, m_config.listHeight), true);

            for (int i = 0; i < static_cast<int>(m_filteredEntries.size()); ++i) {
                const auto& entry = m_filteredEntries[i];

                // Create selectable item with current selection state
                bool isSelected = (m_selectedIndex == i);
                if (ImGui::Selectable(entry.displayName.c_str(), isSelected)) {
                    m_selectedIndex = i;
                }

                // Handle double-click navigation into directories
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    if (entry.isDirectory) {
                        navigateTo(entry.path.string());
                    }
                }

                // Visual indicators for file types
                ImGui::SameLine();
                if (entry.isDirectory) {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "[DIR]");
                } else {
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), entry.fileType.c_str());
                }
            }

            ImGui::EndChild();

            ImGui::Separator();

            // Action buttons for final selection
            if (ImGui::Button(m_config.actionButtonText.c_str())) {
                if (m_selectedIndex >= 0 &&
                    m_selectedIndex < static_cast<int>(m_filteredEntries.size())) {
                    const auto& selectedEntry = m_filteredEntries[m_selectedIndex];
                    if (m_selectionCallback) {
                        m_selectionCallback(selectedEntry.path);
                    }
                    isOpen = false;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(m_config.cancelButtonText.c_str())) {
                isOpen = false;
            }

            ImGui::End();
        }

        return isOpen;
    }

    std::filesystem::path FileBrowser::getSelectedPath() const {
        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_filteredEntries.size())) {
            return m_filteredEntries[m_selectedIndex].path;
        }
        return std::filesystem::path();
    }

    void FileBrowser::scanDirectory() {
        m_entries.clear();

        try {
            std::filesystem::path currentPath(m_currentPath);
            if (std::filesystem::exists(currentPath) &&
                std::filesystem::is_directory(currentPath)) {
                // Iterate through all entries in the current directory
                for (const auto& entry : std::filesystem::directory_iterator(currentPath)) {
                    // Skip hidden files on Unix-like systems (files starting with '.')
                    std::string filename = entry.path().filename().string();
                    if (filename.empty() || filename[0] == '.') {
                        continue;
                    }

                    bool isDirectory = std::filesystem::is_directory(entry);

                    // Apply file type filtering
                    if (isDirectory) {
                        if (m_config.showDirectories) {
                            FileEntry fileEntry;
                            fileEntry.path = entry.path();
                            fileEntry.displayName = filename;
                            fileEntry.isDirectory = true;
                            fileEntry.fileType = "[DIR]";
                            m_entries.push_back(fileEntry);
                        }
                    } else if (m_config.showFiles) {
                        if (matchesFileTypeFilter(entry.path())) {
                            FileEntry fileEntry;
                            fileEntry.path = entry.path();
                            fileEntry.displayName = filename;
                            fileEntry.isDirectory = false;
                            fileEntry.fileType = m_config.fileTypeLabel;
                            m_entries.push_back(fileEntry);
                        }
                    }
                }

                // Sort entries with intelligent ordering: directories first, then files
                // alphabetically
                std::sort(m_entries.begin(), m_entries.end(),
                          [](const FileEntry& a, const FileEntry& b) {
                              if (a.isDirectory != b.isDirectory) {
                                  return a.isDirectory > b.isDirectory; // Directories first
                              }
                              return a.displayName < b.displayName; // Alphabetical within each type
                          });
            }
        } catch (const std::exception& e) {
            // Handle file system errors gracefully
            m_entries.clear();
        }

        m_filterDirty = true;
    }

    void FileBrowser::applyFilter() {
        if (!m_filterDirty) {
            return;
        }

        m_filteredEntries.clear();

        if (m_filter.empty()) {
            // No filter, show all entries
            m_filteredEntries = m_entries;
        } else {
            // Apply case-insensitive filter
            std::string lowerFilter = m_filter;
            std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

            for (const auto& entry : m_entries) {
                std::string lowerName = entry.displayName;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

                if (lowerName.find(lowerFilter) != std::string::npos) {
                    m_filteredEntries.push_back(entry);
                }
            }
        }

        m_filterDirty = false;
        m_selectedIndex = -1; // Clear selection when filter changes
    }

    void FileBrowser::navigateTo(const std::string& newPath) {
        m_currentPath = newPath;
        m_selectedIndex = -1;
        refresh();
    }

    void FileBrowser::navigateUp() {
        std::filesystem::path currentPath(m_currentPath);
        if (currentPath.has_parent_path()) {
            navigateTo(currentPath.parent_path().string());
        }
    }

    void FileBrowser::navigateHome() { navigateTo(m_config.defaultPath); }

    bool FileBrowser::matchesFileTypeFilter(const std::filesystem::path& filePath) const {
        if (m_config.fileTypeFilter.empty()) {
            return true; // No filter specified, accept all files
        }

        std::string ext = filePath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        return ext == m_config.fileTypeFilter;
    }

} // namespace Dynamix