#include "ResourceManager.h"
#include <iostream>
#include <set>

namespace AudioTester {

    // intentionally, we only support ogg files for now bc we want a lot of their features
    namespace {
        const std::set<std::string> VALID_EXTENSIONS = {".ogg"};
    }

    bool ResourceManager::isValidAudioFile(const std::filesystem::path& filePath) const noexcept {
        return VALID_EXTENSIONS.find(filePath.extension().string()) != VALID_EXTENSIONS.end();
    }

    std::shared_ptr<SoLoud::Wav>
    ResourceManager::loadAudioFile(const std::filesystem::path& filePath) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Check if file is already loaded
        if (isLoaded(filePath)) {
            Logger::debug("File already loaded: " + filePath.string(), "ResourceManager");
            return getAudioFile(filePath);
        }

        // Validate file exists
        if (!std::filesystem::exists(filePath)) {
            THROW_ERROR(ResourceError, "File does not exist: " + filePath.string());
        }

        // Validate file extension
        if (!isValidAudioFile(filePath)) {
            THROW_ERROR(ResourceError,
                        "Invalid audio file extension: " + filePath.extension().string());
        }

        // Check file size
        auto fileSize = std::filesystem::file_size(filePath);
        if (fileSize > MAX_FILE_SIZE) {
            THROW_ERROR(ResourceError, "File too large: " + filePath.string());
        }

        // Check resource limit
        if (m_audioFiles.size() >= MAX_RESOURCES) {
            THROW_ERROR(ResourceError, "Maximum number of resources reached");
        }

        // Create new audio file
        auto audioFile = std::make_shared<SoLoud::Wav>();
        if (audioFile->load(filePath.string().c_str()) != SoLoud::SO_NO_ERROR) {
            THROW_ERROR(ResourceError, "Failed to load audio file: " + filePath.string());
        }

        try {
            // Store the loaded file
            m_audioFiles[filePath.string()] = audioFile;
            Logger::info("Successfully loaded audio file: " + filePath.string(), "ResourceManager");
            return audioFile;
        } catch (const std::exception& e) {
            // Cleanup on failure
            THROW_ERROR(ResourceError, "Failed to store audio file: " + std::string(e.what()));
        }
    }

    std::shared_ptr<SoLoud::Wav>
    ResourceManager::getAudioFile(const std::filesystem::path& filePath) const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_audioFiles.find(filePath.string());
        if (it != m_audioFiles.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool ResourceManager::isLoaded(const std::filesystem::path& filePath) const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_audioFiles.find(filePath.string()) != m_audioFiles.end();
    }

    void ResourceManager::clear() noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_audioFiles.clear();
        Logger::info("Cleared all audio resources", "ResourceManager");
    }

} // namespace AudioTester