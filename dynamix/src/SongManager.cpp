#include "SongManager.h"
#include "ConsoleLog.h"
#include "Logger.h"
#include <algorithm>
#include <fstream>

namespace Dynamix {

    void SongManager::loadSongsFromDirectory(const std::string& directory) {
        clear();

        logInfo("Loading songs from directory: " + directory);

        try {
            std::filesystem::path dirPath(directory);
            m_loadedDirectory = dirPath;

            if (!std::filesystem::exists(dirPath)) {
                logError("Directory does not exist: " + directory);
                return;
            }

            // First, try to load master bus
            auto masterPath = dirPath / "_master.json";
            if (std::filesystem::exists(masterPath)) {
                loadMasterBus(masterPath);
            }

            // Then scan for song folders
            for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
                if (entry.is_directory()) {
                    auto songFolder = entry.path();
                    auto songJsonPath = songFolder / "_song.json";

                    if (std::filesystem::exists(songJsonPath) &&
                        !std::filesystem::exists(songFolder / "_master.json")) {
                        logInfo("Found song folder: " + songFolder.filename().string());
                        loadSong(songFolder);
                    }
                }
            }

            logInfo("Loaded " + std::to_string(m_songs.size()) + " songs");

        } catch (const std::filesystem::filesystem_error& e) {
            logError("Filesystem error: " + std::string(e.what()));
        }
    }

    bool SongManager::loadSong(const std::filesystem::path& songFolder) {
        try {
            auto jsonPath = songFolder / "_song.json";

            if (!std::filesystem::exists(jsonPath)) {
                logError("Song JSON not found: " + jsonPath.string());
                return false;
            }

            nlohmann::json json;
            if (!loadJsonFile(jsonPath, json)) {
                return false;
            }

            // Use comprehensive JSON validation
            auto validationResult = m_jsonValidator.validateSongJson(json, songFolder);
            if (!validationResult.errors.empty() || !validationResult.warnings.empty()) {
                logError(validationResult.getErrorSummary());
                if (m_consoleLog) {
                    m_consoleLog->LogValidationResult(validationResult, "Song JSON (" + songFolder.filename().string() + ")");
                }
            }
            if (!json.is_object()) {
                logError("Invalid song JSON: " + jsonPath.string());
                return false;
            }

            Song song;
            song.folderPath = songFolder;

            // Discover tracks in the folder (this is the source of truth)
            auto trackFiles = discoverTracks(songFolder);
            if (trackFiles.empty()) {
                logInfo("No audio tracks yet in: " + songFolder.string());
            }

            // Parse events and ensure all folder tracks are included and all event tracks exist in
            // folder
            if (json.contains("events") && json["events"].is_array() && !json["events"].empty()) {
                for (const auto& eventJson : json["events"]) {
                    SongEvent event;
                    event.name = eventJson.value("name", "Unnamed Event");
                    event.fadeTime = eventJson.value("fadeTime", 1.0f);

                    if (eventJson.contains("state")) {
                        if (!parseStateSnapshot(eventJson["state"], event.state)) {
                            logError("Failed to parse state for event: " + event.name);
                            continue;
                        }
                    }

                    event.state.tracks.erase(
                        std::remove_if(event.state.tracks.begin(), event.state.tracks.end(),
                                       [&trackFiles](const TrackStateExtended& track) {
                                           return std::find(trackFiles.begin(), trackFiles.end(),
                                                            track.file) == trackFiles.end();
                                       }),
                        event.state.tracks.end());

                    for (const auto& trackFile : trackFiles) {
                        bool trackExists = false;
                        for (const auto& existingTrack : event.state.tracks) {
                            if (existingTrack.file == trackFile) {
                                trackExists = true;
                                break;
                            }
                        }
                        if (!trackExists) {
                            TrackStateExtended newTrack;
                            newTrack.file = trackFile;
                            newTrack.volume = 0.0f;
                            event.state.tracks.push_back(newTrack);
                            logInfo("Added missing track to event: " + trackFile);
                        }
                    }

                    song.events.push_back(event);
                }
            }

            if (song.events.empty()) {
                SongEvent defaultEvent;
                defaultEvent.name = "Default";
                defaultEvent.fadeTime = 1.0f;
                for (const auto& trackFile : trackFiles) {
                    TrackStateExtended track;
                    track.file = trackFile;
                    track.volume = 1.0f;
                    defaultEvent.state.tracks.push_back(track);
                }
                song.events.push_back(defaultEvent);
                logInfo("Created default event with " + std::to_string(trackFiles.size()) +
                        " tracks");
            }

            m_songs.push_back(song);
            logInfo("Loaded song: " + song.folderPath.filename().string() + " (" +
                    std::to_string(song.events.size()) + " events, " +
                    std::to_string(trackFiles.size()) + " tracks)");
            return true;

        } catch (const std::exception& e) {
            logError("Exception loading song: " + std::string(e.what()));
            return false;
        }
    }

    bool SongManager::loadMasterBus(const std::filesystem::path& masterJsonPath) {
        try {
            nlohmann::json json;
            if (!loadJsonFile(masterJsonPath, json)) {
                return false;
            }

            // Use comprehensive JSON validation  
            auto validationResult = m_jsonValidator.validateMasterJson(json);
            if (!validationResult.errors.empty() || !validationResult.warnings.empty()) {
                if (!validationResult.isValid) {
                    logError("Master JSON issues: " + masterJsonPath.string());
                }
                logError(validationResult.getErrorSummary());
                if (m_consoleLog) {
                    m_consoleLog->LogValidationResult(validationResult, "Master JSON");
                }
            }
            if (!json.is_object()) {
                return false;
            }

            m_masterBus.name = json.value("name", "Master Bus");

            // Parse events
            if (json.contains("events") && json["events"].is_array()) {
                for (const auto& eventJson : json["events"]) {
                    MasterEvent event;
                    event.name = eventJson.value("name", "Unnamed Master Event");
                    event.fadeTime = eventJson.value("fadeTime", 1.0f);

                    if (eventJson.contains("state")) {
                        if (!parseMasterBusState(eventJson["state"], event.state)) {
                            logError("Failed to parse master state for event: " + event.name);
                            continue;
                        }
                    }

                    m_masterBus.events.push_back(event);
                }
            }

            if (m_masterBus.events.empty()) {
                MasterEvent defaultEvent;
                defaultEvent.name = "Normal";
                defaultEvent.fadeTime = 0.0f;
                defaultEvent.state.volume = 1.0f;
                defaultEvent.state.masterTempo = 1.0f;
                defaultEvent.state.granularTempo = 1.0f;
                m_masterBus.events.push_back(defaultEvent);
                logInfo("Created default master event");
            }

            logInfo("Loaded master bus: " + m_masterBus.name + " (" +
                    std::to_string(m_masterBus.events.size()) + " events)");
            return true;

        } catch (const std::exception& e) {
            logError("Exception loading master bus: " + std::string(e.what()));
            return false;
        }
    }

    std::vector<std::string> SongManager::discoverTracks(const std::filesystem::path& folder) {
        /**
         * Discover Tracks - Audio File Scanner
         *
         * This method scans a directory for supported audio files and returns a sorted list
         * of track filenames. It's the source of truth for track discovery in the system.
         *
         * Features:
         * - Scans directory for supported audio file formats
         * - Filters out non-audio files and unsupported formats
         * - Provides consistent alphabetical ordering
         * - Graceful error handling for inaccessible directories
         * - Thread-safe file system operations
         *
         * @param folder Directory path to scan for audio files
         * @return Sorted vector of audio filenames (without path)
         */

        std::vector<std::string> tracks;

        try {
            // Iterate through all files in the specified directory
            for (const auto& entry : std::filesystem::directory_iterator(folder)) {
                if (entry.is_regular_file()) {
                    auto filepath = entry.path().string();
                    // Validate file format using extension checking
                    if (isSupportedAudioFile(filepath)) {
                        // Store only the filename, not the full path
                        tracks.push_back(entry.path().filename().string());
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            // Handle file system errors gracefully (permissions, non-existent dir, etc.)
            logError("Error discovering tracks: " + std::string(e.what()));
        }

        // Sort for consistent ordering across different file systems and OS
        std::sort(tracks.begin(), tracks.end());
        return tracks;
    }

    const Song* SongManager::findSongByFolder(const std::string& folderName) const {
        auto it = std::find_if(m_songs.begin(), m_songs.end(), [&folderName](const Song& song) {
            return song.folderPath.filename().string() == folderName;
        });

        return (it != m_songs.end()) ? &(*it) : nullptr;
    }

    void SongManager::clear() {
        m_songs.clear();
        m_masterBus.events.clear();
        m_masterBus.name.clear();
        m_loadedDirectory.clear();
    }

    bool SongManager::parseStateSnapshot(const nlohmann::json& json, StateSnapshot& state) {
        /**
         * Parse State Snapshot - JSON State Reconstruction
         *
         * This method parses a JSON object representing a complete audio state snapshot,
         * including master tempo settings and individual track states with effects.
         *
         * JSON Structure Expected:
         * {
         *   "masterTempo": 1.0,
         *   "granularTempo": 1.0,
         *   "tracks": [
         *     {
         *       "file": "track1.ogg",
         *       "volume": 0.8,
         *       "effects": {
         *         "echo": { "parameters": { "0": 0.5, "1": 0.3 } },
         *         "freeverb": { "parameters": { "0": 0.7, "2": 0.6 } }
         *       }
         *     }
         *   ]
         * }
         *
         * @param json JSON object containing the state snapshot
         * @param state Output parameter to populate with parsed state
         * @return true on successful parsing, false on error
         */

        try {
            // Parse master tempo settings with defaults
            state.masterTempo = std::clamp(json.value("masterTempo", 1.0f), 0.1f, 4.0f);
            state.granularTempo = std::clamp(json.value("granularTempo", 1.0f), 0.5f, 2.0f);

            // Parse track states if present
            if (json.contains("tracks") && json["tracks"].is_array()) {
                state.tracks.clear();
                for (const auto& trackJson : json["tracks"]) {
                    TrackStateExtended track;
                    if (parseTrackState(trackJson, track)) {
                        state.tracks.push_back(track);
                    }
                    // Continue parsing other tracks even if one fails
                }
            }

            return true;
        } catch (const std::exception& e) {
            logError("Error parsing state snapshot: " + std::string(e.what()));
            return false;
        }
    }

    bool SongManager::parseMasterBusState(const nlohmann::json& json, MasterBusState& state) {
        try {
            state.masterTempo = std::clamp(json.value("masterTempo", 1.0f), 0.1f, 4.0f);
            state.granularTempo = std::clamp(json.value("granularTempo", 1.0f), 0.5f, 2.0f);

            if (json.contains("bus")) {
                const auto& busJson = json["bus"];
                state.volume = std::clamp(busJson.value("volume", 1.0f),
                                          JsonValidator::ValidationConstants::MIN_VOLUME,
                                          JsonValidator::ValidationConstants::MAX_VOLUME);

                if (busJson.contains("effects") && busJson["effects"].is_object()) {
                    state.effects.clear();
                    for (const auto& [effectName, effectJson] : busJson["effects"].items()) {
                        if (!m_filterManager.isValidFilterName(effectName)) {
                            logInfo("Ignoring unknown master effect: " + effectName);
                            continue;
                        }
                        EffectState effect;
                        if (parseEffectState(effectJson, effect, effectName)) {
                            state.effects[effectName] = effect;
                        }
                    }
                }
            }

            return true;
        } catch (const std::exception& e) {
            logError("Error parsing master bus state: " + std::string(e.what()));
            return false;
        }
    }

    bool SongManager::parseEffectState(const nlohmann::json& json, EffectState& effect,
                                       const std::string& effectName) {
        try {
            if (json.contains("parameters") && json["parameters"].is_object()) {
                effect.parameters.clear();
                for (const auto& [paramKey, paramValue] : json["parameters"].items()) {
                    if (!paramValue.is_number()) {
                        continue;
                    }
                    const int paramId = m_filterManager.resolveParameterId(effectName, paramKey);
                    if (paramId < 0) {
                        continue;
                    }
                    float value = paramValue.get<float>();
                    const std::string paramName = m_filterManager.getParameterName(effectName, paramId);
                    const float minVal = m_filterManager.getParameterMin(effectName, paramName);
                    const float maxVal = m_filterManager.getParameterMax(effectName, paramName);
                    if (value < minVal || value > maxVal) {
                        logInfo("Clamping " + effectName + "." + paramName + " from " +
                                std::to_string(value) + " to [" + std::to_string(minVal) + ", " +
                                std::to_string(maxVal) + "]");
                        value = std::clamp(value, minVal, maxVal);
                    }
                    effect.parameters[std::to_string(paramId)] = value;
                }
            }
            return true;
        } catch (const std::exception& e) {
            logError("Failed to parse effect state: " + std::string(e.what()));
            return false;
        }
    }

    bool SongManager::parseTrackState(const nlohmann::json& json, TrackStateExtended& track) {
        try {
            track.file = json.value("file", "");
            track.volume = std::clamp(json.value("volume", 1.0f),
                                      JsonValidator::ValidationConstants::MIN_VOLUME,
                                      JsonValidator::ValidationConstants::MAX_VOLUME);
            // Note: active field removed - tracks are always active, use volume for enable/disable

            if (json.contains("effects") && json["effects"].is_object()) {
                track.effects.clear();
                for (const auto& [effectName, effectJson] : json["effects"].items()) {
                    if (!m_filterManager.isValidFilterName(effectName)) {
                        logInfo("Ignoring unknown effect: " + effectName);
                        continue;
                    }
                    EffectState effect;
                    if (parseEffectState(effectJson, effect, effectName)) {
                        track.effects[effectName] = effect;
                    }
                }
            }

            return true;
        } catch (const std::exception& e) {
            logError("Error parsing track state: " + std::string(e.what()));
            return false;
        }
    }

    bool SongManager::validateSongJson(const nlohmann::json& json) {
        /**
         * Validate Song JSON - Structure Validation
         *
         * This method validates that a JSON object has the correct structure for a song file.
         * It checks for required fields and correct data types without parsing the full content.
         *
         * Required Structure:
         * {
         *   "events": [ ... ]  // Array of song events
         * }
         *
         * @param json JSON object to validate
         * @return true if valid song JSON structure, false otherwise
         */

        // Basic validation - should have events array
        if (!json.is_object()) {
            logError("Song JSON must be an object");
            return false;
        }

        if (!json.contains("events") || !json["events"].is_array()) {
            logError("Song JSON must contain 'events' array");
            return false;
        }

        return true;
    }

    bool SongManager::validateMasterJson(const nlohmann::json& json) {
        /**
         * Validate Master JSON - Structure Validation
         *
         * This method validates that a JSON object has the correct structure for a master bus file.
         * It checks for required fields and correct data types without parsing the full content.
         *
         * Required Structure:
         * {
         *   "events": [ ... ]  // Array of master events
         * }
         *
         * @param json JSON object to validate
         * @return true if valid master JSON structure, false otherwise
         */

        // Basic validation - should have events array
        if (!json.is_object()) {
            logError("Master JSON must be an object");
            return false;
        }

        if (!json.contains("events") || !json["events"].is_array()) {
            logError("Master JSON must contain 'events' array");
            return false;
        }

        return true;
    }

    bool SongManager::isSupportedAudioFile(const std::string& filepath) const {
        std::filesystem::path path(filepath);
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // Only support OGG as specified in the format
        return ext == ".ogg";
    }

    bool SongManager::loadJsonFile(const std::filesystem::path& path, nlohmann::json& json) {
        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                logError("Could not open JSON file: " + path.string());
                return false;
            }

            file >> json;
            return true;

        } catch (const std::exception& e) {
            logError("Error parsing JSON file " + path.string() + ": " + e.what());
            return false;
        }
    }

    void SongManager::logError(const std::string& message) const {
        LOG_ERROR_COMP("SongManager", message);
    }

    void SongManager::logInfo(const std::string& message) const {
        LOG_INFO_COMP("SongManager", message);
    }

    bool SongManager::addSongEvent(const std::string& folderName, const SongEvent& event) {
        for (auto& song : m_songs) {
            if (song.folderPath.filename().string() == folderName) {
                // Check for duplicate event name
                auto existingIt = std::find_if(song.events.begin(), song.events.end(),
                                               [&event](const SongEvent& existingEvent) {
                                                   return existingEvent.name == event.name;
                                               });

                if (existingIt != song.events.end()) {
                    logError("Event '" + event.name + "' already exists in song '" + folderName +
                             "'. Use overwrite instead.");
                    return false;
                }

                song.events.push_back(event);
                logInfo("Added event '" + event.name + "' to song '" + folderName + "'");
                return true;
            }
        }
        logError("Song not found: " + folderName);
        return false;
    }

    bool SongManager::addMasterEvent(const MasterEvent& event) {
        // Check for duplicate event name
        auto existingIt = std::find_if(m_masterBus.events.begin(), m_masterBus.events.end(),
                                       [&event](const MasterEvent& existingEvent) {
                                           return existingEvent.name == event.name;
                                       });

        if (existingIt != m_masterBus.events.end()) {
            logError("Master event '" + event.name + "' already exists. Use overwrite instead.");
            return false;
        }

        m_masterBus.events.push_back(event);
        logInfo("Added master event: " + event.name);
        return true;
    }

    bool SongManager::overwriteSongEvent(const std::string& folderName, const SongEvent& event) {
        for (auto& song : m_songs) {
            if (song.folderPath.filename().string() == folderName) {
                // Find and replace the event
                auto existingIt = std::find_if(song.events.begin(), song.events.end(),
                                               [&event](const SongEvent& existingEvent) {
                                                   return existingEvent.name == event.name;
                                               });

                if (existingIt != song.events.end()) {
                    *existingIt = event;
                    logInfo("Overwrote event '" + event.name + "' in song '" + folderName + "'");
                    return true;
                } else {
                    logError("Event '" + event.name + "' not found in song '" + folderName +
                             "'. Use add instead.");
                    return false;
                }
            }
        }
        logError("Song not found: " + folderName);
        return false;
    }

    bool SongManager::overwriteMasterEvent(const MasterEvent& event) {
        // Find existing event to overwrite
        auto existingIt = std::find_if(m_masterBus.events.begin(), m_masterBus.events.end(),
                                       [&event](const MasterEvent& existingEvent) {
                                           return existingEvent.name == event.name;
                                       });

        if (existingIt != m_masterBus.events.end()) {
            // Overwrite the existing event while preserving its position
            *existingIt = event;
            logInfo("Overwrote master event: " + event.name);
            return true;
        } else {
            logError("Master event '" + event.name + "' not found for overwrite");
            return false;
        }
    }

    bool SongManager::hasSongEvent(const std::string& folderName,
                                   const std::string& eventName) const {
        for (const auto& song : m_songs) {
            if (song.folderPath.filename().string() == folderName) {
                auto it = std::find_if(
                    song.events.begin(), song.events.end(),
                    [&eventName](const SongEvent& event) { return event.name == eventName; });
                return it != song.events.end();
            }
        }
        return false;
    }

    bool SongManager::hasMasterEvent(const std::string& eventName) const {
        auto it = std::find_if(
            m_masterBus.events.begin(), m_masterBus.events.end(),
            [&eventName](const MasterEvent& event) { return event.name == eventName; });
        return it != m_masterBus.events.end();
    }

    bool SongManager::saveSongJson(const std::string& folderName) {
        for (const auto& song : m_songs) {
            if (song.folderPath.filename().string() == folderName) {
                try {
                    nlohmann::json json;
                    json["events"] = nlohmann::json::array();

                    for (const auto& event : song.events) {
                        nlohmann::json eventJson;
                        eventJson["name"] = event.name;
                        eventJson["fadeTime"] = event.fadeTime;

                        // Serialize state
                        nlohmann::json stateJson;
                        stateJson["masterTempo"] = event.state.masterTempo;
                        stateJson["granularTempo"] = event.state.granularTempo;
                        stateJson["tracks"] = nlohmann::json::array();

                        for (const auto& track : event.state.tracks) {
                            nlohmann::json trackJson;
                            trackJson["file"] = track.file;
                            trackJson["volume"] = track.volume;
                            trackJson["effects"] = nlohmann::json::object();

                            for (const auto& [effectName, effectState] : track.effects) {
                                nlohmann::json effectJson;
                                effectJson["parameters"] = nlohmann::json::object();
                                for (const auto& [paramName, paramValue] : effectState.parameters) {
                                    effectJson["parameters"][paramName] = paramValue;
                                }
                                trackJson["effects"][effectName] = effectJson;
                            }

                            stateJson["tracks"].push_back(trackJson);
                        }

                        eventJson["state"] = stateJson;
                        json["events"].push_back(eventJson);
                    }

                    // Write to file
                    auto jsonPath = song.folderPath / "_song.json";
                    std::ofstream file(jsonPath);
                    if (!file.is_open()) {
                        logError("Could not open file for writing: " + jsonPath.string());
                        return false;
                    }

                    file << json.dump(2); // Pretty print with 2-space indentation
                    logInfo("Saved song JSON: " + jsonPath.string());
                    return true;

                } catch (const std::exception& e) {
                    logError("Error saving song JSON: " + std::string(e.what()));
                    return false;
                }
            }
        }
        logError("Song not found for saving: " + folderName);
        return false;
    }

    bool SongManager::saveMasterJson() {
        try {
            nlohmann::json json;
            json["name"] = m_masterBus.name;
            json["events"] = nlohmann::json::array();

            for (const auto& event : m_masterBus.events) {
                nlohmann::json eventJson;
                eventJson["name"] = event.name;
                eventJson["fadeTime"] = event.fadeTime;

                // Serialize master state
                nlohmann::json stateJson;
                stateJson["masterTempo"] = event.state.masterTempo;
                stateJson["granularTempo"] = event.state.granularTempo;

                nlohmann::json busJson;
                busJson["volume"] = event.state.volume;
                busJson["effects"] = nlohmann::json::object();

                for (const auto& [effectName, effectState] : event.state.effects) {
                    nlohmann::json effectJson;
                    effectJson["parameters"] = nlohmann::json::object();
                    for (const auto& [paramName, paramValue] : effectState.parameters) {
                        effectJson["parameters"][paramName] = paramValue;
                    }
                    busJson["effects"][effectName] = effectJson;
                }

                stateJson["bus"] = busJson;
                eventJson["state"] = stateJson;
                json["events"].push_back(eventJson);
            }

            // Write to master.json in current directory
            // For now, assume _master.json is in the root of the loaded directory
            auto masterPath = std::filesystem::path(getCurrentMasterDirectory()) / "_master.json";
            std::ofstream file(masterPath);
            if (!file.is_open()) {
                logError("Could not open master file for writing: " + masterPath.string());
                return false;
            }

            file << json.dump(2);
            logInfo("Saved master JSON: " + masterPath.string());
            return true;

        } catch (const std::exception& e) {
            logError("Error saving master JSON: " + std::string(e.what()));
            return false;
        }
    }

    std::string SongManager::getCurrentMasterDirectory() const {
        if (!m_loadedDirectory.empty()) {
            return m_loadedDirectory.string();
        }
        if (!m_songs.empty()) {
            return m_songs[0].folderPath.parent_path().string();
        }
#ifdef __APPLE__
        const char* homeDir = getenv("HOME");
        if (homeDir) {
            return std::string(homeDir) + "/Music/Dynamix";
        } else {
            return "./Music/Dynamix";
        }
#elif defined(_WIN32)
        const char* userProfile = getenv("USERPROFILE");
        if (userProfile) {
            return std::string(userProfile) + "\\Music\\Dynamix";
        } else {
            return ".\\Music\\Dynamix";
        }
#else
        const char* homeDir = getenv("HOME");
        if (homeDir) {
            return std::string(homeDir) + "/Music/Dynamix";
        } else {
            return "./Music/Dynamix";
        }
#endif
    }

    bool SongManager::deleteSongEvent(const std::string& folderName, const std::string& eventName) {
        for (auto& song : m_songs) {
            if (song.folderPath.filename().string() == folderName) {
                auto it = std::find_if(
                    song.events.begin(), song.events.end(),
                    [&eventName](const SongEvent& event) { return event.name == eventName; });

                if (it != song.events.end()) {
                    song.events.erase(it);
                    logInfo("Deleted event '" + eventName + "' from song '" + folderName + "'");
                    return saveSongJson(folderName); // Save after deletion
                } else {
                    logError("Event not found in song '" + folderName + "': " + eventName);
                    return false;
                }
            }
        }
        logError("Song not found: " + folderName);
        return false;
    }

    bool SongManager::deleteMasterEvent(const std::string& eventName) {
        auto it = std::find_if(
            m_masterBus.events.begin(), m_masterBus.events.end(),
            [&eventName](const MasterEvent& event) { return event.name == eventName; });

        if (it != m_masterBus.events.end()) {
            m_masterBus.events.erase(it);
            logInfo("Deleted master event: " + eventName);
            return saveMasterJson(); // Save after deletion
        } else {
            logError("Master event not found: " + eventName);
            return false;
        }
    }

} // namespace Dynamix