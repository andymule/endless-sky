#include "AudioController.h"
#include "Logger.h"
#include <algorithm>
#include <iostream>

namespace AudioTester {

    AudioController::AudioController() : m_eventSystem(this) {}

    bool AudioController::initialize(const std::string& executableDirectory) {
        if (m_isInitialized) {
            return true;
        }

        // Store executable directory for path resolution
        m_executableDirectory = executableDirectory;

        // Set default music directory relative to executable
        if (m_currentDirectory.empty()) {
            m_currentDirectory = resolvePath("sound_staging");
        }

        if (!m_audioSystem.initialize()) {
            LOG_ERROR_COMP("AudioController", "Failed to initialize audio system");
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
        m_currentDirectory = resolvePath(directory);
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
            LOG_ERROR_COMP("AudioController",
                           "Error loading music directory: " + std::string(e.what()));
        }

        // Sync all tracks with audio system
        syncAllTracksToAudioSystem();
    }

    void AudioController::loadSongsFromDirectory(const std::string& directory) {
        std::string resolvedPath = resolvePath(directory);
        m_songManager.loadSongsFromDirectory(resolvedPath);
    }

    std::string AudioController::resolvePath(const std::string& relativePath) const {
        // If it's already an absolute path, return as-is
        if (std::filesystem::path(relativePath).is_absolute()) {
            return relativePath;
        }

        // If we have an executable directory, resolve relative to it
        if (!m_executableDirectory.empty()) {
            return (std::filesystem::path(m_executableDirectory) / relativePath).string();
        }

        // Otherwise, resolve relative to current working directory
        return (std::filesystem::current_path() / relativePath).string();
    }

    void AudioController::setCurrentSong(const std::string& songName) {
        m_currentSongName = songName;
        // TODO: Implement song switching logic
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
        if (!m_isInitialized) {
            return;
        }

        m_audioSystem.playAllTracks(); // Use synchronized playback
        m_state.setGlobalPlaying(true);
    }

    void AudioController::stopPlayback() {
        if (!m_isInitialized) {
            return;
        }

        m_audioSystem.stopAllTracks(); // Use synchronized stop
        m_state.setGlobalPlaying(false);
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

    int AudioController::findTrackByFilename(const std::string& filename) const {
        for (size_t i = 0; i < m_state.getTrackCount(); ++i) {
            const auto& track = m_state.getTrack(i);
            std::filesystem::path trackPath(track.filepath);
            if (trackPath.filename().string() == filename) {
                return static_cast<int>(i);
            }
        }
        return -1; // Not found
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

    // New effect automation methods
    void AudioController::setTrackEffectEnabled(size_t trackIndex, const std::string& effectName,
                                                bool enabled) {
        // Map effect names to filter names (they're the same in our system)
        setTrackFilterEnabled(trackIndex, effectName, enabled);
    }

    namespace {
        // Helper to map effect parameter names to parameter IDs
        int mapEffectParamNameToId(const std::string& /*effectName*/,
                                   const std::string& paramName) {
            if (paramName == "boost" || paramName == "delay" || paramName == "samplerate" ||
                paramName == "wet" || paramName == "freq" || paramName == "amount")
                return 0;
            if (paramName == "decay" || paramName == "roomsize" || paramName == "bitdepth" ||
                paramName == "wave")
                return 1;
            if (paramName == "filter" || paramName == "damp")
                return 2;
            if (paramName == "width")
                return 3;
            // Default fallback
            return 0;
        }
    } // namespace

    void AudioController::setTrackEffectParameter(size_t trackIndex, const std::string& effectName,
                                                  const std::string& paramName, float value) {
        int paramId = mapEffectParamNameToId(effectName, paramName);
        setTrackFilterParameter(trackIndex, effectName, paramId, value);
    }

    void AudioController::setBusEffectEnabled(const std::string& effectName, bool enabled) {
        setBusFilterEnabled(effectName, enabled);
    }

    void AudioController::setBusEffectParameter(const std::string& effectName,
                                                const std::string& paramName, float value) {
        int paramId = mapEffectParamNameToId(effectName, paramName);
        setBusFilterParameter(effectName, paramId, value);
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

    void AudioController::updateSync() {
        if (m_isInitialized) {
            m_audioSystem.updateSync();
        }
    }

    void AudioController::updateEvents(float deltaTime) { m_eventSystem.update(deltaTime); }

    double AudioController::getMasterDuration() const { return m_audioSystem.getMasterDuration(); }

    double AudioController::getGlobalTime() const { return m_audioSystem.getGlobalTime(); }

    bool AudioController::isPlaying() const { return m_audioSystem.isPlaying(); }

    // Master tempo controls (tape-style playback speed)
    void AudioController::setMasterTempo(float tempo) {
        // Store the tempo value for UI synchronization
        m_currentTempo = tempo;
        // Use the new dual tape speed architecture - this sets user tape speed
        m_audioSystem.setGlobalPlaybackRate(tempo);
    }

    float AudioController::getMasterTempo() const { return m_currentTempo; }

    // Granular tempo controls (pitch-preserving)
    void AudioController::setGranularTempo(float tempo) {
        // Use the new dual tape speed architecture - this coordinates with user tape speed
        m_audioSystem.setGranularTempo(tempo);
    }

    float AudioController::getGranularTempo() const { return m_audioSystem.getGranularTempo(); }

    float AudioController::getGranularLatencyMs() const {
        return m_audioSystem.getGranularLatencyMs();
    }

    bool AudioController::isGranularEnabled() const { return m_audioSystem.isGranularEnabled(); }

    // Event triggering (external API)
    void AudioController::triggerSongEvent(const std::string& songName,
                                           const std::string& eventName) {
        m_eventSystem.triggerSongEvent(songName, eventName);
    }

    void AudioController::triggerMasterEvent(const std::string& eventName) {
        m_eventSystem.triggerMasterEvent(eventName);
    }

} // namespace AudioTester