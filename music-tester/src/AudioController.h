#pragma once

#include "AudioState.h"
#include "AudioSystem.h"
#include <filesystem>
#include <functional>
#include <string>

namespace AudioTester {

    // Controller class that handles all business logic
    class AudioController {
    public:
        AudioController();
        ~AudioController() = default;

        // Initialization
        bool initialize();
        void cleanup();

        // Directory and file management
        void setMusicDirectory(const std::string& directory);
        void loadMusicFromDirectory();
        bool isSupportedFile(const std::string& filepath) const;

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
        void setTrackActive(size_t index, bool active);
        void setTrackVolume(size_t index, float volume);
        void setTrackLooping(size_t index, bool looping);

        // Bus management
        void setBusVolume(float volume);

        // Filter management
        void setTrackFilterEnabled(size_t trackIndex, const std::string& filterName, bool enabled);
        void setTrackFilterParameter(size_t trackIndex, const std::string& filterName, int paramId,
                                     float value);
        void setBusFilterEnabled(const std::string& filterName, bool enabled);
        void setBusFilterParameter(const std::string& filterName, int paramId, float value);

        // Master tempo controls (playback speed)
        void setMasterTempo(float tempo);
        float getMasterTempo() const;

        // State access (read-only for the view)
        const AudioState& getState() const { return m_state; }
        const AudioSystem& getAudioSystem() const { return m_audioSystem; }

        // View notifications
        void setStateChangeCallback(std::function<void()> callback) {
            m_state.onStateChanged = callback;
        }

        // Current directory access
        const std::string& getCurrentDirectory() const { return m_currentDirectory; }

    private:
        void syncTrackToAudioSystem(size_t index);
        void syncAllTracksToAudioSystem();

        AudioState m_state;
        AudioSystem m_audioSystem;
        std::string m_currentDirectory;
        bool m_isInitialized = false;

        // Store current tempo value for UI synchronization
        float m_currentTempo = 1.0f;
    };

} // namespace AudioTester