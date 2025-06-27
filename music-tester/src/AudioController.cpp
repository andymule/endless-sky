#include "AudioController.h"
#include "EventSystem.h"
#include "Logger.h"
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <set>

namespace AudioTester {

    AudioController::AudioController() : m_eventSystem(std::make_unique<EventSystem>(this)) {}

    bool AudioController::initialize(const std::string& executableDirectory) {
        if (m_isInitialized) {
            return true;
        }

        // Store executable directory for path resolution
        m_executableDirectory = executableDirectory;

        // Set default music directory to ~/Music/Dynamix
        if (m_currentDirectory.empty()) {
#ifdef __APPLE__
            const char* homeDir = getenv("HOME");
            if (homeDir) {
                m_currentDirectory = std::string(homeDir) + "/Music/Dynamix";
            } else {
                m_currentDirectory = "./Music/Dynamix";
            }
#elif defined(_WIN32)
            const char* userProfile = getenv("USERPROFILE");
            if (userProfile) {
                m_currentDirectory = std::string(userProfile) + "\\Music\\Dynamix";
            } else {
                m_currentDirectory = ".\\Music\\Dynamix";
            }
#else
            const char* homeDir = getenv("HOME");
            if (homeDir) {
                m_currentDirectory = std::string(homeDir) + "/Music/Dynamix";
            } else {
                m_currentDirectory = "./Music/Dynamix";
            }
#endif
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

            // Set the first song as current if no song is currently set
            if (m_currentSongName.empty()) {
                const auto& songs = m_songManager.getSongs();
                if (!songs.empty()) {
                    setCurrentSong(songs[0].folderPath.filename().string());
                }
            } else {
                // Load tracks for the current song only
                const SongManager* mgr = getSongManager();
                if (mgr) {
                    const Song* song = mgr->findSongByFolder(m_currentSongName);
                    if (song) {
                        std::filesystem::path songFolder = song->folderPath;
                        if (std::filesystem::exists(songFolder)) {
                            // Load all .ogg files from the current song's folder
                            for (const auto& entry :
                                 std::filesystem::directory_iterator(songFolder)) {
                                if (entry.is_regular_file()) {
                                    auto filepath = entry.path().string();
                                    if (isSupportedFile(filepath)) {
                                        std::string trackFile = entry.path().filename().string();
                                        // Add to state with the track filename as the name
                                        m_state.addTrack(trackFile, filepath);
                                        // Load into audio system
                                        m_audioSystem.loadTrack(filepath);
                                        // Ensure track is set to loop (tracks should always loop in
                                        // a song)
                                        size_t trackIndex = m_state.getTrackCount() - 1;
                                        m_audioSystem.setTrackLooping(trackIndex, true);
                                    }
                                }
                            }
                        }
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
        if (m_currentSongName == songName) {
            return; // No change
        }

        // Stop playback and clear all tracks from audio system and state
        pausePlayback();
        m_audioSystem.stopAllTracks();
        m_audioSystem.clearAllTracks();
        m_state.clearTracks();

        m_currentSongName = songName;

        // Load all tracks for the new song (from the folder)
        const SongManager* mgr = getSongManager();
        if (!mgr)
            return;
        const Song* song = mgr->findSongByFolder(songName);
        if (!song)
            return;
        std::filesystem::path songFolder = song->folderPath;
        if (!std::filesystem::exists(songFolder))
            return;

        std::vector<std::string> trackFiles;
        for (const auto& entry : std::filesystem::directory_iterator(songFolder)) {
            if (entry.is_regular_file()) {
                auto filepath = entry.path().string();
                if (isSupportedFile(filepath)) {
                    trackFiles.push_back(entry.path().filename().string());
                }
            }
        }
        std::sort(trackFiles.begin(), trackFiles.end());

        for (const auto& trackFile : trackFiles) {
            std::filesystem::path trackPath = songFolder / trackFile;
            m_state.addTrack(trackFile, trackPath.string());
            m_audioSystem.loadTrack(trackPath.string());
            size_t trackIndex = m_state.getTrackCount() - 1;
            m_audioSystem.setTrackLooping(trackIndex, true);
        }

        // If global play state was active, resume playback
        if (m_state.globalPlaying) {
            resumePlayback();
        }
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
            pausePlayback();
        } else {
            resumePlayback();
        }
    }

    void AudioController::pausePlayback() {
        if (!m_isInitialized) {
            return;
        }

        m_audioSystem.pauseAllTracks(); // Use synchronized pause
        m_state.setGlobalPlaying(false);
    }

    void AudioController::resumePlayback() {
        if (!m_isInitialized) {
            return;
        }

        // Check if we have any tracks loaded
        if (m_state.getTrackCount() == 0) {
            return; // No tracks to play
        }

        // Only resume from pause - if no tracks are paused, we need to start fresh
        if (m_audioSystem.hasPausedTracks()) {
            m_audioSystem.resumeAllTracks(); // Resume from paused position
        } else {
            // No paused tracks - this means we're starting fresh
            // This should only happen on the very first play or after track reloading
            m_audioSystem.playAllTracks(); // Start fresh
        }
        m_state.setGlobalPlaying(true);
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
                    triggerSongEvent(song.folderPath.filename().string(), eventName);
                    return;
                }
            }
        }

        // Event not found anywhere
        LOG_ERROR_COMP("AudioController", "Event not found: " + eventName);
    }

    bool AudioController::createSongEvent(const std::string& songName, const std::string& eventName,
                                          float fadeTime) {
        // Check if event already exists
        if (m_songManager.hasSongEvent(songName, eventName)) {
            LOG_ERROR_COMP("AudioController", "Event '" + eventName + "' already exists in song '" +
                                                  songName + "'. Use overwrite instead.");
            return false;
        }

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

    bool AudioController::overwriteSongEvent(const std::string& songName,
                                             const std::string& eventName, float fadeTime) {
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

        // Overwrite existing event and save
        if (m_songManager.overwriteSongEvent(songName, event)) {
            return m_songManager.saveSongJson(songName);
        }
        return false;
    }

    bool AudioController::createMasterEvent(const std::string& eventName, float fadeTime) {
        // Check if event already exists
        if (m_songManager.hasMasterEvent(eventName)) {
            LOG_ERROR_COMP("AudioController", "Master event '" + eventName +
                                                  "' already exists. Use overwrite instead.");
            return false;
        }

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

    bool AudioController::overwriteMasterEvent(const std::string& eventName, float fadeTime) {
        LOG_INFO_COMP("AudioController", "Overwriting master event: " + eventName);

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

        // Overwrite existing event and save
        if (m_songManager.overwriteMasterEvent(event)) {
            LOG_INFO_COMP("AudioController", "Event overwritten in memory, now saving...");
            bool saveResult = m_songManager.saveMasterJson();
            if (saveResult) {
                LOG_INFO_COMP("AudioController", "Event saved successfully!");
            } else {
                LOG_ERROR_COMP("AudioController", "Failed to save event to JSON");
            }
            return saveResult;
        } else {
            LOG_ERROR_COMP("AudioController", "Failed to overwrite event in memory");
            return false;
        }
    }

    bool AudioController::hasSongEvent(const std::string& songName,
                                       const std::string& eventName) const {
        return m_songManager.hasSongEvent(songName, eventName);
    }

    bool AudioController::hasMasterEvent(const std::string& eventName) const {
        return m_songManager.hasMasterEvent(eventName);
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

    bool AudioController::createNewMasterDirectory(const std::string& directoryName) {
        if (m_currentDirectory.empty()) {
            LOG_ERROR_COMP("AudioController", "No current directory set");
            return false;
        }

        std::filesystem::path newDirPath =
            std::filesystem::path(m_currentDirectory) / directoryName;

        try {
            if (std::filesystem::exists(newDirPath)) {
                LOG_ERROR_COMP("AudioController",
                               "Directory already exists: " + newDirPath.string());
                return false;
            }

            if (!std::filesystem::create_directories(newDirPath)) {
                LOG_ERROR_COMP("AudioController",
                               "Failed to create directory: " + newDirPath.string());
                return false;
            }

            // Create empty _master.json file
            std::filesystem::path masterJsonPath = newDirPath / "_master.json";
            std::ofstream masterFile(masterJsonPath);
            if (!masterFile.is_open()) {
                LOG_ERROR_COMP("AudioController",
                               "Failed to create _master.json: " + masterJsonPath.string());
                return false;
            }
            masterFile << "{\n  \"events\": []\n}\n";
            masterFile.close();

            LOG_INFO_COMP("AudioController",
                          "Created new master directory: " + newDirPath.string());
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR_COMP("AudioController",
                           "Exception creating directory: " + std::string(e.what()));
            return false;
        }
    }

    bool AudioController::createNewSongFolder(const std::string& songName) {
        if (m_currentDirectory.empty()) {
            LOG_ERROR_COMP("AudioController", "No current directory set");
            return false;
        }

        std::filesystem::path songDirPath = std::filesystem::path(m_currentDirectory) / songName;

        try {
            if (std::filesystem::exists(songDirPath)) {
                LOG_ERROR_COMP("AudioController",
                               "Song folder already exists: " + songDirPath.string());
                return false;
            }

            if (!std::filesystem::create_directories(songDirPath)) {
                LOG_ERROR_COMP("AudioController",
                               "Failed to create song folder: " + songDirPath.string());
                return false;
            }

            // Create empty song JSON file
            std::filesystem::path songJsonPath = songDirPath / (songName + ".json");
            std::ofstream songFile(songJsonPath);
            if (!songFile.is_open()) {
                LOG_ERROR_COMP("AudioController",
                               "Failed to create song JSON: " + songJsonPath.string());
                return false;
            }
            songFile << "{\n  \"events\": []\n}\n";
            songFile.close();

            LOG_INFO_COMP("AudioController", "Created new song folder: " + songDirPath.string());
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR_COMP("AudioController",
                           "Exception creating song folder: " + std::string(e.what()));
            return false;
        }
    }

    void AudioController::createNewMaster() {
        // Create a new master directory with a default name
        std::string defaultName = "master_" + std::to_string(std::time(nullptr));
        if (createNewMasterDirectory(defaultName)) {
            LOG_INFO_COMP("AudioController", "Created new master directory: " + defaultName);
        }
    }

    void AudioController::createNewSong() {
        // Create a new song folder with a default name
        std::string defaultName = "song_" + std::to_string(std::time(nullptr));
        if (createNewSongFolder(defaultName)) {
            LOG_INFO_COMP("AudioController", "Created new song folder: " + defaultName);
        }
    }

    std::filesystem::path AudioController::getCurrentSongFolderPath() const {
        const SongManager* mgr = getSongManager();
        if (!mgr)
            return {};
        const Song* song = mgr->findSongByFolder(m_currentSongName);
        if (!song)
            return {};
        return song->folderPath;
    }

    bool AudioController::addTrackToCurrentEvent(const std::string& filename) {
        // Get the current song and event
        SongManager* mgr = getSongManagerMutable();
        if (!mgr)
            return false;
        Song* song = const_cast<Song*>(mgr->findSongByFolder(m_currentSongName));
        if (!song)
            return false;
        if (song->events.empty())
            return false;

        // Add to the first event (or you could add to a selected event if UI supports it)
        SongEvent& event = song->events[0];
        // Check if already present
        for (const auto& track : event.state.tracks) {
            if (track.file == filename)
                return false; // Already present
        }

        // Add new track with default volume and no effects
        TrackStateExtended newTrack;
        newTrack.file = filename;
        newTrack.volume = 1.0f;
        event.state.tracks.push_back(newTrack);

        // Save song JSON
        if (!mgr->saveSongJson(song->folderPath.filename().string()))
            return false;

        // Smart track addition: only add the new track to the system
        // Find the track file in the song folder
        std::filesystem::path trackPath = song->folderPath / filename;
        if (std::filesystem::exists(trackPath)) {
            // Add to state with the track filename as the name
            m_state.addTrack(filename, trackPath.string());
            // Load into audio system
            m_audioSystem.loadTrack(trackPath.string());
            // Get the actual track index from AudioSystem (should be the last one)
            size_t trackIndex = m_audioSystem.getTrackCount() - 1;
            // Ensure track is set to loop (tracks should always loop in a song)
            m_audioSystem.setTrackLooping(trackIndex, true);

            // Sync the new track to audio system
            syncTrackToAudioSystem(trackIndex);

            // If the song is currently playing, start the new track at the current position
            if (m_audioSystem.isPlaying()) {
                // Get current playback position from the master track
                double currentTime = m_audioSystem.getGlobalTime();
                // Start the new track at the current position
                m_audioSystem.playTrack(trackIndex);
                // Seek to the current position to sync with other tracks
                if (currentTime > 0.0) {
                    // Note: SoLoud will handle the seeking automatically when the track starts
                    // The track will be in sync with the others
                }
            }

            LOG_INFO_COMP("AudioController", "Smart-added track: " + filename + " to system");

            return true;
        } else {
            LOG_ERROR_COMP("AudioController", "Track file not found: " + trackPath.string());
            return false;
        }
    }

    bool AudioController::removeTrackFromCurrentSong(const std::string& filename) {
        // Get the current song
        SongManager* mgr = getSongManagerMutable();
        if (!mgr)
            return false;
        Song* song = const_cast<Song*>(mgr->findSongByFolder(m_currentSongName));
        if (!song)
            return false;

        // Debug: print all track filenames in all events
        for (const auto& event : song->events) {
            std::cout << "[DEBUG] Event: " << event.name << " tracks: ";
            for (const auto& track : event.state.tracks) {
                std::cout << '"' << track.file << '"' << " ";
            }
            std::cout << std::endl;
        }

        // Delete the actual file from the song folder
        std::filesystem::path filePath = song->folderPath / filename;
        if (std::filesystem::exists(filePath)) {
            try {
                std::filesystem::remove(filePath);
                LOG_INFO_COMP("AudioController", "Deleted file: " + filePath.string());
            } catch (const std::exception& e) {
                LOG_ERROR_COMP("AudioController",
                               "Failed to delete file: " + std::string(e.what()));
                return false;
            }
        } else {
            LOG_WARN_COMP("AudioController", "File not found for deletion: " + filePath.string());
        }

        bool trackRemoved = false;

        // Remove the track from all events in the song
        for (auto& event : song->events) {
            auto it = std::find_if(
                event.state.tracks.begin(), event.state.tracks.end(),
                [&filename](const TrackStateExtended& track) { return track.file == filename; });
            if (it != event.state.tracks.end()) {
                event.state.tracks.erase(it);
                trackRemoved = true;
            }
        }

        if (!trackRemoved) {
            LOG_WARN_COMP("AudioController", "Track not found in song events: " + filename);
            // Still return true since we deleted the file
            return true;
        }

        // Save song JSON
        if (!mgr->saveSongJson(song->folderPath.filename().string())) {
            LOG_ERROR_COMP("AudioController", "Failed to save song after removing track");
            return false;
        }

        // Remove from AudioState and AudioSystem
        int trackIndex = findTrackByFilename(filename);
        if (trackIndex >= 0) {
            // Remove from AudioSystem completely
            m_audioSystem.removeTrack(trackIndex);

            // Remove from AudioState
            m_state.removeTrack(trackIndex);
        }

        LOG_INFO_COMP("AudioController", "Removed track: " + filename + " from song");
        return true;
    }

} // namespace AudioTester