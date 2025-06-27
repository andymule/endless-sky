#pragma once

#include "imgui.h"
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace AudioTester {

    /**
     * FileBrowser - Reusable File System Browser Component
     *
     * This class provides a unified file browsing interface that can be configured
     * for different use cases (directory selection, file selection, filtered browsing).
     * It eliminates code duplication between different file browser implementations.
     *
     * Features:
     * - Hierarchical navigation with breadcrumb-style path display
     * - Real-time filtering of entries by filename
     * - Visual distinction between directories and files
     * - Double-click navigation into directories
     * - Configurable file type filtering
     * - Customizable action buttons and callbacks
     */
    class FileBrowser {
    public:
        /**
         * File entry information for display and selection
         */
        struct FileEntry {
            std::filesystem::path path;
            std::string displayName;
            bool isDirectory;
            std::string fileType; // For custom file type indicators
        };

        /**
         * Configuration for the file browser behavior
         */
        struct Config {
            std::string title = "File Browser";
            std::string defaultPath = "";
            std::string filterText = "";
            std::string fileTypeFilter = "";      // e.g., ".ogg" for OGG files only
            std::string fileTypeLabel = "[FILE]"; // Custom label for files
            bool showDirectories = true;
            bool showFiles = true;
            std::string actionButtonText = "Select";
            std::string cancelButtonText = "Cancel";
            ImVec2 windowSize = ImVec2(600, 400);
            float listHeight = 250.0f;
        };

        /**
         * Callback function type for when a file/directory is selected
         */
        using SelectionCallback = std::function<void(const std::filesystem::path&)>;

        FileBrowser();
        ~FileBrowser() = default;

        /**
         * Initialize the file browser with configuration
         * @param config Browser configuration
         */
        void initialize(const Config& config);

        /**
         * Set the callback function for when a selection is made
         * @param callback Function to call when user selects a file/directory
         */
        void setSelectionCallback(SelectionCallback callback);

        /**
         * Set the current path to browse
         * @param path Directory path to browse
         */
        void setCurrentPath(const std::string& path);

        /**
         * Get the current browsing path
         * @return Current directory path
         */
        std::string getCurrentPath() const { return m_currentPath; }

        /**
         * Set the filter text for filtering entries
         * @param filter Filter text (case-insensitive substring matching)
         */
        void setFilter(const std::string& filter);

        /**
         * Get the current filter text
         * @return Current filter text
         */
        std::string getFilter() const { return m_filter; }

        /**
         * Refresh the browser entries (re-scan current directory)
         */
        void refresh();

        /**
         * Render the file browser dialog
         * @param isOpen Reference to boolean controlling dialog visibility
         * @return true if dialog should remain open, false if closed
         */
        bool render(bool& isOpen);

        /**
         * Get the currently selected entry index
         * @return Index of selected entry, -1 if none selected
         */
        int getSelectedIndex() const { return m_selectedIndex; }

        /**
         * Get the currently selected entry path
         * @return Path of selected entry, empty if none selected
         */
        std::filesystem::path getSelectedPath() const;

    private:
        /**
         * Scan the current directory and populate the entries list
         */
        void scanDirectory();

        /**
         * Apply the current filter to the entries list
         */
        void applyFilter();

        /**
         * Navigate to a new directory path
         * @param newPath Directory path to navigate to
         */
        void navigateTo(const std::string& newPath);

        /**
         * Navigate to parent directory if available
         */
        void navigateUp();

        /**
         * Navigate to home/default directory
         */
        void navigateHome();

        /**
         * Check if a file matches the configured file type filter
         * @param filePath Path to check
         * @return true if file matches filter, false otherwise
         */
        bool matchesFileTypeFilter(const std::filesystem::path& filePath) const;

        // Configuration
        Config m_config;
        SelectionCallback m_selectionCallback;

        // State
        std::string m_currentPath;
        std::string m_filter;
        std::vector<FileEntry> m_entries;
        std::vector<FileEntry> m_filteredEntries;
        int m_selectedIndex = -1;
        bool m_filterDirty = false;
    };

} // namespace AudioTester