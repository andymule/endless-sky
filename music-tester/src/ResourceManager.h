#pragma once

#include "ErrorHandling.h"
#include "soloud_wav.h"
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace AudioTester {

    /**
     * Manages audio file resources with thread-safe operations
     * and proper resource lifecycle management.
     */
    class ResourceManager {
    public:
        ResourceManager() = default;
        ~ResourceManager() = default;

        // Delete copy operations
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        // Allow move operations
        ResourceManager(ResourceManager&&) noexcept = default;
        ResourceManager& operator=(ResourceManager&&) noexcept = default;

        /**
         * Loads an audio file and returns a handle to it.
         * @throws ResourceError if:
         *   - File doesn't exist
         *   - File extension is invalid
         *   - Resource limit reached
         *   - File loading fails
         */
        std::shared_ptr<SoLoud::Wav> loadAudioFile(const std::filesystem::path& filePath);

        /**
         * Gets an already loaded audio file.
         * @return nullptr if file is not loaded
         */
        std::shared_ptr<SoLoud::Wav>
        getAudioFile(const std::filesystem::path& filePath) const noexcept;

        /**
         * Checks if a file is already loaded.
         */
        bool isLoaded(const std::filesystem::path& filePath) const noexcept;

        /**
         * Clears all loaded resources.
         */
        void clear() noexcept;

        // Maximum number of resources to prevent memory issues
        static constexpr size_t MAX_RESOURCES = 1000;

        // Maximum file size in bytes (100MB)
        static constexpr size_t MAX_FILE_SIZE = 100 * 1024 * 1024;

    private:
        std::unordered_map<std::string, std::shared_ptr<SoLoud::Wav>> m_audioFiles;
        mutable std::mutex m_mutex;

        /**
         * Validates if the file extension is supported.
         */
        bool isValidAudioFile(const std::filesystem::path& filePath) const noexcept;
    };

} // namespace AudioTester