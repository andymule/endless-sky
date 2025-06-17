#include "AudioController.h"
#include <algorithm>
#include <iostream>

namespace AudioTester {

    AudioController::AudioController() { m_currentDirectory = "sound_staging"; }

    bool AudioController::initialize() {
        if (m_isInitialized) {
            return true;
        }

        if (!m_audioSystem.initialize()) {
            std::cerr << "Failed to initialize audio system" << std::endl;
            return false;
        }

        // Load initial music directory
        loadMusicFromDirectory();

        m_isInitialized = true;
        return true;
    }

    void AudioController::cleanup() {
        if (m_isInitialized) {
            m_audioSystem.cleanup();
            m_isInitialized = false;
        }
    }

    void AudioController::setMusicDirectory(const std::string& directory) {
        m_currentDirectory = directory;
        loadMusicFromDirectory();
    }

    void AudioController::loadMusicFromDirectory() {
        if (m_currentDirectory.empty()) {
            return;
        }

        m_state.clearTracks();

        try {
            for (const auto& entry : std::filesystem::directory_iterator(m_currentDirectory)) {
                if (entry.is_regular_file()) {
                    const auto& path = entry.path();
                    if (isSupportedFile(path.string())) {
                        // Add to state
                        m_state.addTrack(path.filename().string(), path.string());
                        // Load into audio system
                        m_audioSystem.loadAudioFile(path.string());
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error loading music directory: " << e.what() << std::endl;
        }

        // Sync all tracks with audio system
        syncAllTracksToAudioSystem();
    }

    bool AudioController::isSupportedFile(const std::string& filepath) const {
        std::filesystem::path path(filepath);
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return AudioSystem::isSupportedFileExtension(ext);
    }

    void AudioController::toggleGlobalPlayback() {
        if (m_state.globalPlaying) {
            stopPlayback();
        } else {
            startPlayback();
        }
    }

    void AudioController::startPlayback() {
        m_state.setGlobalPlaying(true);
        for (size_t i = 0; i < m_state.getTrackCount(); ++i) {
            m_audioSystem.playTrack(i);
        }
    }

    void AudioController::stopPlayback() {
        m_state.setGlobalPlaying(false);
        for (size_t i = 0; i < m_state.getTrackCount(); ++i) {
            m_audioSystem.stopTrack(i);
        }
    }

    void AudioController::setTrackActive(size_t index, bool active) {
        m_state.setTrackActive(index, active);
        syncTrackToAudioSystem(index);
    }

    void AudioController::setTrackVolume(size_t index, float volume) {
        m_state.setTrackVolume(index, volume);
        syncTrackToAudioSystem(index);
    }

    void AudioController::setTrackLooping(size_t index, bool looping) {
        m_state.setTrackLooping(index, looping);
        m_audioSystem.setTrackLooping(index, looping);
    }

    void AudioController::setBusVolume(float volume) {
        m_state.setBusVolume(volume);
        m_audioSystem.setBusVolume(volume);
    }

    void AudioController::setTrackFilterEnabled(size_t trackIndex, const std::string& filterName,
                                                bool enabled) {
        m_audioSystem.setFilterEnabled(trackIndex, filterName, enabled);
    }

    void AudioController::setTrackFilterParameter(size_t trackIndex, const std::string& filterName,
                                                  int paramId, float value) {
        m_audioSystem.setFilterParameter(trackIndex, filterName, paramId, value);
    }

    void AudioController::setBusFilterEnabled(const std::string& filterName, bool enabled) {
        m_audioSystem.setBusFilterEnabled(filterName, enabled);
    }

    void AudioController::setBusFilterParameter(const std::string& filterName, int paramId,
                                                float value) {
        m_audioSystem.setBusFilterParameter(filterName, paramId, value);
    }

    void AudioController::syncTrackToAudioSystem(size_t index) {
        if (index >= m_state.getTrackCount()) {
            return;
        }

        const auto& track = m_state.getTrack(index);
        // Apply volume based on active state
        float effectiveVolume = track.active ? track.volume : 0.0f;
        m_audioSystem.setTrackVolume(index, effectiveVolume);
    }

    void AudioController::syncAllTracksToAudioSystem() {
        for (size_t i = 0; i < m_state.getTrackCount(); ++i) {
            syncTrackToAudioSystem(i);
        }
    }

} // namespace AudioTester