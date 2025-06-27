#pragma once

#include "AudioState.h"
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace AudioTester {

    class SongManager {
    public:
        SongManager() = default;
        ~SongManager() = default;

        // Core loading functions
        bool loadSong(const std::filesystem::path& songFolder);
        bool loadMasterBus(const std::filesystem::path& masterJsonPath);
        void loadSongsFromDirectory(const std::string& directory);

        // Auto-discovery
        std::vector<std::string> discoverTracks(const std::filesystem::path& folder);

        // Access
        const std::vector<Song>& getSongs() const { return m_songs; }
        const MasterBus& getMasterBus() const { return m_masterBus; }

        // Song lookup
        const Song* findSong(const std::string& songName) const;

        // Event management
        bool addSongEvent(const std::string& songName, const SongEvent& event);
        bool addMasterEvent(const MasterEvent& event);
        bool overwriteSongEvent(const std::string& songName, const SongEvent& event);
        bool overwriteMasterEvent(const MasterEvent& event);
        bool hasSongEvent(const std::string& songName, const std::string& eventName) const;
        bool hasMasterEvent(const std::string& eventName) const;
        bool saveSongJson(const std::string& songName);
        bool saveMasterJson();

        // Event deletion
        bool deleteSongEvent(const std::string& songName, const std::string& eventName);
        bool deleteMasterEvent(const std::string& eventName);

        // Clear all loaded data
        void clear();

    private:
        std::vector<Song> m_songs;
        MasterBus m_masterBus;

        // JSON parsing helpers
        bool parseStateSnapshot(const nlohmann::json& json, StateSnapshot& state);
        bool parseMasterBusState(const nlohmann::json& json, MasterBusState& state);
        bool parseEffectState(const nlohmann::json& json, EffectState& effect);
        bool parseTrackState(const nlohmann::json& json, TrackStateExtended& track);

        // Validation
        bool validateSongJson(const nlohmann::json& json);
        bool validateMasterJson(const nlohmann::json& json);

        // File utilities
        bool isSupportedAudioFile(const std::string& filepath) const;
        bool loadJsonFile(const std::filesystem::path& path, nlohmann::json& json);

        // Logging
        void logError(const std::string& message) const;
        void logInfo(const std::string& message) const;

        // Helper for master JSON path
        std::string getCurrentMasterDirectory() const;
    };

} // namespace AudioTester