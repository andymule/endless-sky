#include "SongManager.h"
#include "Logger.h"
#include <algorithm>
#include <fstream>

namespace AudioTester {

    void SongManager::loadSongsFromDirectory(const std::string& directory) {
        clear();

        logInfo("Loading songs from directory: " + directory);

        try {
            std::filesystem::path dirPath(directory);

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

                    if (std::filesystem::exists(songJsonPath)) {
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

            if (!validateSongJson(json)) {
                logError("Invalid song JSON: " + jsonPath.string());
                return false;
            }

            Song song;
            song.name = json.value("name", songFolder.filename().string());
            song.folderPath = songFolder;

            // Discover tracks in the folder
            auto trackFiles = discoverTracks(songFolder);
            if (trackFiles.empty()) {
                logError("No audio tracks found in: " + songFolder.string());
                return false;
            }

            // Parse events
            if (json.contains("events") && json["events"].is_array()) {
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

                    song.events.push_back(event);
                }
            }

            m_songs.push_back(song);
            logInfo("Loaded song: " + song.name + " (" + std::to_string(song.events.size()) +
                    " events)");
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

            if (!validateMasterJson(json)) {
                logError("Invalid master JSON: " + masterJsonPath.string());
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

            logInfo("Loaded master bus: " + m_masterBus.name + " (" +
                    std::to_string(m_masterBus.events.size()) + " events)");
            return true;

        } catch (const std::exception& e) {
            logError("Exception loading master bus: " + std::string(e.what()));
            return false;
        }
    }

    std::vector<std::string> SongManager::discoverTracks(const std::filesystem::path& folder) {
        std::vector<std::string> tracks;

        try {
            for (const auto& entry : std::filesystem::directory_iterator(folder)) {
                if (entry.is_regular_file()) {
                    auto filepath = entry.path().string();
                    if (isSupportedAudioFile(filepath)) {
                        tracks.push_back(entry.path().filename().string());
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            logError("Error discovering tracks: " + std::string(e.what()));
        }

        // Sort for consistent ordering
        std::sort(tracks.begin(), tracks.end());
        return tracks;
    }

    const Song* SongManager::findSong(const std::string& songName) const {
        auto it = std::find_if(m_songs.begin(), m_songs.end(),
                               [&songName](const Song& song) { return song.name == songName; });

        return (it != m_songs.end()) ? &(*it) : nullptr;
    }

    void SongManager::clear() {
        m_songs.clear();
        m_masterBus.events.clear();
        m_masterBus.name.clear();
    }

    bool SongManager::parseStateSnapshot(const nlohmann::json& json, StateSnapshot& state) {
        try {
            state.masterTempo = json.value("masterTempo", 1.0f);
            state.granularTempo = json.value("granularTempo", 1.0f);

            if (json.contains("tracks") && json["tracks"].is_array()) {
                state.tracks.clear();
                for (const auto& trackJson : json["tracks"]) {
                    TrackStateExtended track;
                    if (parseTrackState(trackJson, track)) {
                        state.tracks.push_back(track);
                    }
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
            state.masterTempo = json.value("masterTempo", 1.0f);
            state.granularTempo = json.value("granularTempo", 1.0f);

            if (json.contains("bus")) {
                const auto& busJson = json["bus"];
                state.volume = busJson.value("volume", 1.0f);

                if (busJson.contains("effects") && busJson["effects"].is_object()) {
                    state.effects.clear();
                    for (const auto& [effectName, effectJson] : busJson["effects"].items()) {
                        EffectState effect;
                        if (parseEffectState(effectJson, effect)) {
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

    bool SongManager::parseEffectState(const nlohmann::json& json, EffectState& effect) {
        try {
            if (json.contains("parameters") && json["parameters"].is_object()) {
                effect.parameters.clear();
                for (const auto& [paramName, paramValue] : json["parameters"].items()) {
                    if (paramValue.is_number()) {
                        float value = paramValue.get<float>();
                        effect.parameters[paramName] = value;
                    }
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
            track.volume = json.value("volume", 1.0f);
            // Note: active field removed - tracks are always active, use volume for enable/disable

            if (json.contains("effects") && json["effects"].is_object()) {
                track.effects.clear();
                for (const auto& [effectName, effectJson] : json["effects"].items()) {
                    EffectState effect;
                    if (parseEffectState(effectJson, effect)) {
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

    bool SongManager::addSongEvent(const std::string& songName, const SongEvent& event) {
        for (auto& song : m_songs) {
            if (song.name == songName) {
                // Check for duplicate event name
                auto existingIt = std::find_if(song.events.begin(), song.events.end(),
                                               [&event](const SongEvent& existingEvent) {
                                                   return existingEvent.name == event.name;
                                               });

                if (existingIt != song.events.end()) {
                    logError("Event '" + event.name + "' already exists in song '" + songName +
                             "'. Use overwrite instead.");
                    return false;
                }

                song.events.push_back(event);
                logInfo("Added event '" + event.name + "' to song '" + songName + "'");
                return true;
            }
        }
        logError("Song not found: " + songName);
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

    bool SongManager::overwriteSongEvent(const std::string& songName, const SongEvent& event) {
        for (auto& song : m_songs) {
            if (song.name == songName) {
                // Find existing event to overwrite
                auto existingIt = std::find_if(song.events.begin(), song.events.end(),
                                               [&event](const SongEvent& existingEvent) {
                                                   return existingEvent.name == event.name;
                                               });

                if (existingIt != song.events.end()) {
                    // Overwrite the existing event while preserving its position
                    *existingIt = event;
                    logInfo("Overwrote event '" + event.name + "' in song '" + songName + "'");
                    return true;
                } else {
                    logError("Event '" + event.name + "' not found in song '" + songName +
                             "' for overwrite");
                    return false;
                }
            }
        }
        logError("Song not found: " + songName);
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

    bool SongManager::hasSongEvent(const std::string& songName,
                                   const std::string& eventName) const {
        for (const auto& song : m_songs) {
            if (song.name == songName) {
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

    bool SongManager::saveSongJson(const std::string& songName) {
        for (const auto& song : m_songs) {
            if (song.name == songName) {
                try {
                    nlohmann::json json;
                    json["name"] = song.name;
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
        logError("Song not found for saving: " + songName);
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
        // For now, use the parent directory of the first song
        if (!m_songs.empty()) {
            return m_songs[0].folderPath.parent_path().string();
        }
        return "sound_staging"; // Fallback
    }

    bool SongManager::deleteSongEvent(const std::string& songName, const std::string& eventName) {
        for (auto& song : m_songs) {
            if (song.name == songName) {
                auto it = std::find_if(
                    song.events.begin(), song.events.end(),
                    [&eventName](const SongEvent& event) { return event.name == eventName; });

                if (it != song.events.end()) {
                    song.events.erase(it);
                    logInfo("Deleted event '" + eventName + "' from song '" + songName + "'");
                    return saveSongJson(songName); // Save after deletion
                } else {
                    logError("Event not found in song '" + songName + "': " + eventName);
                    return false;
                }
            }
        }
        logError("Song not found: " + songName);
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

} // namespace AudioTester