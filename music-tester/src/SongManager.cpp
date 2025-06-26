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
            effect.enabled = json.value("enabled", false);

            if (json.contains("parameters") && json["parameters"].is_object()) {
                effect.parameters.clear();
                for (const auto& [paramName, paramValue] : json["parameters"].items()) {
                    if (paramValue.is_number()) {
                        effect.parameters[paramName] = paramValue.get<float>();
                    }
                }
            }

            return true;
        } catch (const std::exception& e) {
            logError("Error parsing effect state: " + std::string(e.what()));
            return false;
        }
    }

    bool SongManager::parseTrackState(const nlohmann::json& json, TrackStateExtended& track) {
        try {
            track.file = json.value("file", "");
            track.volume = json.value("volume", 1.0f);
            track.active = json.value("active", true);

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

} // namespace AudioTester