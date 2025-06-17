#include "ResourceManager.h"
#include <iostream>
#include <set>

// intentionally, we only support ogg files for now bc we want a lot of their features
namespace {
    const std::set<std::string> VALID_EXTENSIONS = {".ogg"};
}

bool ResourceManager::isValidAudioFile(const std::filesystem::path& filePath) const noexcept {
    return VALID_EXTENSIONS.find(filePath.extension().string()) != VALID_EXTENSIONS.end();
}

std::shared_ptr<SoLoud::Wav> ResourceManager::loadAudioFile(const std::filesystem::path& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Check if file is already loaded
    if (isLoaded(filePath)) {
        return getAudioFile(filePath);
    }

    // Validate file exists
    if (!std::filesystem::exists(filePath)) {
        throw ResourceError("File does not exist: " + filePath.string());
    }

    // Validate file extension
    if (!isValidAudioFile(filePath)) {
        throw ResourceError("Invalid audio file extension: " + filePath.extension().string());
    }

    // Check file size
    auto fileSize = std::filesystem::file_size(filePath);
    if (fileSize > MAX_FILE_SIZE) {
        throw ResourceError("File too large: " + filePath.string());
    }

    // Check resource limit
    if (m_audioFiles.size() >= MAX_RESOURCES) {
        throw ResourceError("Maximum number of resources reached");
    }

    // Create new audio file
    auto audioFile = std::make_shared<SoLoud::Wav>();
    if (audioFile->load(filePath.string().c_str()) != SoLoud::SO_NO_ERROR) {
        throw ResourceError("Failed to load audio file: " + filePath.string());
    }

    try {
        // Store the loaded file
        m_audioFiles[filePath.string()] = audioFile;
        return audioFile;
    } catch (const std::exception& e) {
        // Cleanup on failure
        throw ResourceError("Failed to store audio file: " + std::string(e.what()));
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
}