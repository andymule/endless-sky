#include "AudioController.h"
#include "EventSystem.h"
#include "Logger.h"
#include <algorithm>
#include <iostream>

namespace AudioTester {

    AudioController::AudioController() : m_eventSystem(std::make_unique<EventSystem>(this)) {}

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
            // First load songs/events from the directory (unified loading)
            m_songManager.loadSongsFromDirectory(m_currentDirectory);

            // Then load individual track files for playback
            for (const auto& entry : std::filesystem::directory_iterator(m_currentDirectory)) {
                if (entry.is_regular_file()) {
                    const auto& path = entry.path();
                    if (isSupportedFile(path.string())) {
                        // Add to state
                        m_state.addTrack(path.filename().string(), path.string());
                        // Load into audio system
                        m_audioSystem.loadTrack(path.string());

                        // Ensure track is set to loop (tracks should always loop in a song)
                        size_t trackIndex = m_state.getTrackCount() - 1;
                        m_audioSystem.setTrackLooping(trackIndex, true);
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

        // More efficient case-insensitive comparison for .ogg extension
        if (ext.length() == 4 && (ext[0] == '.' || ext[0] == 'O' || ext[0] == 'o') &&
            (ext[1] == 'o' || ext[1] == 'O') && (ext[2] == 'g' || ext[2] == 'G') &&
            (ext[3] == 'g' || ext[3] == 'G')) {
            return true;
        }

        return false;
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

    void AudioController::setTrackEffectParameter(size_t trackIndex, const std::string& effectName,
                                                  const std::string& paramName, float value) {
        // Use the new FilterManager-based method instead of local parameter mapping
        m_audioSystem.setFilterParameterByName(trackIndex, effectName, paramName, value);
    }

    void AudioController::setBusEffectEnabled(const std::string& effectName, bool enabled) {
        setBusFilterEnabled(effectName, enabled);
    }

    void AudioController::setBusEffectParameter(const std::string& effectName,
                                                const std::string& paramName, float value) {
        // Use the new FilterManager-based method instead of local parameter mapping
        m_audioSystem.setBusFilterParameterByName(effectName, paramName, value);
    }

    void AudioController::syncTrackToAudioSystem(size_t index) {
        if (index >= m_state.getTrackCount()) {
            return;
        }

        const auto& track = m_state.getTrack(index);
        // Apply volume directly - no active state check needed
        m_audioSystem.setTrackVolume(index, track.volume);
    }

    void AudioController::syncAllTracksToAudioSystem() {
        // Optimized: sync all tracks directly without method call overhead
        for (size_t i = 0; i < m_state.getTrackCount(); ++i) {
            const auto& track = m_state.getTrack(i);
            // Apply volume directly - no active state check needed
            m_audioSystem.setTrackVolume(i, track.volume);
        }
    }

    void AudioController::updateSync() {
        if (m_isInitialized) {
            m_audioSystem.updateSync();
        }
    }

    void AudioController::updateEvents(float deltaTime) {
        if (m_eventSystem) {
            m_eventSystem->update(deltaTime);
        }
    }

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
        m_eventSystem->triggerSongEvent(songName, eventName);
    }

    void AudioController::triggerMasterEvent(const std::string& eventName) {
        m_eventSystem->triggerMasterEvent(eventName);
    }

    void AudioController::triggerEvent(const std::string& eventName) {
        // First, try to find the event in master events
        const auto& masterBus = m_songManager.getMasterBus();
        for (const auto& event : masterBus.events) {
            if (event.name == eventName) {
                triggerMasterEvent(eventName);
                return;
            }
        }

        // If not found in master, search all loaded songs
        const auto& songs = m_songManager.getSongs();
        for (const auto& song : songs) {
            for (const auto& event : song.events) {
                if (event.name == eventName) {
                    triggerSongEvent(song.name, eventName);
                    return;
                }
            }
        }

        // Event not found anywhere
        LOG_ERROR_COMP("AudioController", "Event not found: " + eventName);
    }

    bool AudioController::createSongEvent(const std::string& songName, const std::string& eventName,
                                          float fadeTime) {
        // Capture current state
        StateSnapshot currentState;
        currentState.masterTempo = getMasterTempo();
        currentState.granularTempo = getGranularTempo();

        // Capture track states with effects
        for (size_t i = 0; i < m_state.getTrackCount(); ++i) {
            const auto& track = m_state.getTrack(i);

            TrackStateExtended trackState;
            trackState.file = std::filesystem::path(track.filepath).filename().string();
            trackState.volume = track.volume;

            // Capture only enabled effect states from AudioSystem
            const auto& trackFilters = m_audioSystem.getFilters(i);
            for (const auto& [filterName, filterInstance] : trackFilters) {
                // Only capture effects that are enabled (wet > 0)
                if (filterInstance.enabled) {
                    EffectState effectState;

                    // Capture all parameters using their IDs, not names (to avoid validation
                    // issues)
                    for (const auto& [paramId, param] : filterInstance.parameters) {
                        // Store parameter by ID as string key for JSON compatibility
                        effectState.parameters[std::to_string(paramId)] = param.value;
                    }

                    trackState.effects[filterName] = effectState;
                }
            }

            currentState.tracks.push_back(trackState);
        }

        // Create the event
        SongEvent event;
        event.name = eventName;
        event.fadeTime = fadeTime;
        event.state = currentState;

        // Add to song and save
        if (m_songManager.addSongEvent(songName, event)) {
            return m_songManager.saveSongJson(songName);
        }
        return false;
    }

    bool AudioController::createMasterEvent(const std::string& eventName, float fadeTime) {
        LOG_INFO_COMP("AudioController", "Creating master event: " + eventName);

        // Capture current master state
        MasterBusState masterState;
        masterState.masterTempo = getMasterTempo();
        masterState.granularTempo = getGranularTempo();
        masterState.volume = m_state.busVolume;

        // Capture only enabled master effects from AudioSystem
        const auto& busFilters = m_audioSystem.getBusFilters();
        for (const auto& [filterName, filterInstance] : busFilters) {
            // Only capture effects that are enabled (wet > 0)
            if (filterInstance.enabled) {
                EffectState effectState;

                // Capture all parameters using their IDs, not names (for consistency with song
                // events)
                for (const auto& [paramId, param] : filterInstance.parameters) {
                    // Store parameter by ID as string key for JSON compatibility
                    effectState.parameters[std::to_string(paramId)] = param.value;
                }

                masterState.effects[filterName] = effectState;
            }
        }

        // Create the event
        MasterEvent event;
        event.name = eventName;
        event.fadeTime = fadeTime;
        event.state = masterState;

        LOG_INFO_COMP("AudioController",
                      "Event state captured - Tempo: " + std::to_string(masterState.masterTempo) +
                          ", Volume: " + std::to_string(masterState.volume) +
                          ", Effects: " + std::to_string(masterState.effects.size()));

        // Add to master bus and save
        if (m_songManager.addMasterEvent(event)) {
            LOG_INFO_COMP("AudioController", "Event added to memory, now saving...");
            bool saveResult = m_songManager.saveMasterJson();
            if (saveResult) {
                LOG_INFO_COMP("AudioController", "Event saved successfully!");
            } else {
                LOG_ERROR_COMP("AudioController", "Failed to save event to JSON");
            }
            return saveResult;
        } else {
            LOG_ERROR_COMP("AudioController", "Failed to add event to memory");
            return false;
        }
    }

    bool AudioController::deleteSongEvent(const std::string& songName,
                                          const std::string& eventName) {
        LOG_INFO_COMP("AudioController",
                      "Deleting song event: " + eventName + " from song: " + songName);
        return m_songManager.deleteSongEvent(songName, eventName);
    }

    bool AudioController::deleteMasterEvent(const std::string& eventName) {
        LOG_INFO_COMP("AudioController", "Deleting master event: " + eventName);
        return m_songManager.deleteMasterEvent(eventName);
    }

} // namespace AudioTester