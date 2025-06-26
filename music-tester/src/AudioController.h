#pragma once

#include "AudioState.h"
#include "AudioSystem.h"
#include "SongManager.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace AudioTester {

    // Forward declaration
    class EventSystem;

    // Controller class that handles all business logic
    class AudioController {
    public:
        AudioController();
        ~AudioController() = default;

        // Initialization
        bool initialize(const std::string& executableDirectory = "");
        void cleanup();

        // Directory and file management
        void setMusicDirectory(const std::string& directory);
        void loadMusicFromDirectory();
        bool isSupportedFile(const std::string& filepath) const;

        // Song management (read-only access)
        void setCurrentSong(const std::string& songName);
        const SongManager* getSongManager() const { return &m_songManager; }
        SongManager* getSongManagerMutable() { return &m_songManager; }

        // Playback control
        void toggleGlobalPlayback();
        void startPlayback();
        void stopPlayback();

        // Synchronization
        void updateSync(); // Call this regularly to maintain sync
        double getMasterDuration() const;
        double getGlobalTime() const;
        bool isPlaying() const;

        // Track management
        void setTrackVolume(size_t index, float volume);
        void setTrackLooping(size_t index, bool looping);
        int findTrackByFilename(const std::string& filename) const;

        // Bus management
        void setBusVolume(float volume);

        // Filter management
        void setTrackFilterEnabled(size_t trackIndex, const std::string& filterName, bool enabled);
        void setTrackFilterParameter(size_t trackIndex, const std::string& filterName, int paramId,
                                     float value);
        void setBusFilterEnabled(const std::string& filterName, bool enabled);
        void setBusFilterParameter(const std::string& filterName, int paramId, float value);

        // Effect automation (new methods for EventSystem)
        void setTrackEffectEnabled(size_t trackIndex, const std::string& effectName, bool enabled);
        void setTrackEffectParameter(size_t trackIndex, const std::string& effectName,
                                     const std::string& paramName, float value);
        void setBusEffectEnabled(const std::string& effectName, bool enabled);
        void setBusEffectParameter(const std::string& effectName, const std::string& paramName,
                                   float value);

        // Master tempo controls (playback speed)
        void setMasterTempo(float tempo);
        float getMasterTempo() const;

        // Granular tempo controls (pitch-preserving)
        void setGranularTempo(float tempo);
        float getGranularTempo() const;
        float getGranularLatencyMs() const;
        bool isGranularEnabled() const;

        // Event triggering (external API)
        void triggerSongEvent(const std::string& songName, const std::string& eventName);
        void triggerMasterEvent(const std::string& eventName);
        void triggerEvent(const std::string& eventName); // Search both songs and master
        void updateEvents(float deltaTime);

        // State access (read-only for the view)
        const AudioState& getState() const { return m_state; }
        const AudioSystem& getAudioSystem() const { return m_audioSystem; }

        // View notifications
        void setStateChangeCallback(std::function<void()> callback) {
            m_state.onStateChanged = callback;
        }

        // Current directory access
        const std::string& getCurrentDirectory() const { return m_currentDirectory; }

        // Event creation methods
        bool createSongEvent(const std::string& songName, const std::string& eventName,
                             float fadeTime);
        bool createMasterEvent(const std::string& eventName, float fadeTime);

        // Event deletion methods
        bool deleteSongEvent(const std::string& songName, const std::string& eventName);
        bool deleteMasterEvent(const std::string& eventName);

    private:
        void syncTrackToAudioSystem(size_t index);
        void syncAllTracksToAudioSystem();
        std::string resolvePath(const std::string& relativePath) const;

        AudioState m_state;
        AudioSystem m_audioSystem;
        SongManager m_songManager;
        std::unique_ptr<EventSystem> m_eventSystem;
        std::string m_executableDirectory;
        std::string m_currentDirectory;
        std::string m_currentSongName;
        bool m_isInitialized = false;

        // Store current tempo value for UI synchronization
        float m_currentTempo = 1.0f;
    };

} // namespace AudioTester