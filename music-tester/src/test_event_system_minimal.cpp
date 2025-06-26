#include "EventSystem.h"
#include "SongManager.h"
#include <chrono>
#include <iostream>
#include <thread>

using namespace AudioTester;

// Mock AudioController for testing
class MockAudioController {
public:
    MockAudioController() = default;

    // Mock methods that EventSystem needs
    const SongManager* getSongManager() const { return &m_songManager; }
    void setMasterTempo(float tempo) { m_masterTempo = tempo; }
    void setGranularTempo(float tempo) { m_granularTempo = tempo; }
    void setBusVolume(float volume) { m_busVolume = volume; }
    void setTrackVolume(int index, float volume) {
        if (index >= 0 && index < static_cast<int>(m_trackVolumes.size())) {
            m_trackVolumes[index] = volume;
        }
    }
    void setTrackActive(int index, bool active) {
        if (index >= 0 && index < static_cast<int>(m_trackActive.size())) {
            m_trackActive[index] = active;
        }
    }
    void setTrackEffectEnabled(int index, const std::string& effect, bool enabled) {
        std::cout << "  [Mock] Track " << index << " effect " << effect << " enabled: " << enabled
                  << std::endl;
    }
    void setTrackEffectParameter(int index, const std::string& effect, const std::string& param,
                                 float value) {
        std::cout << "  [Mock] Track " << index << " effect " << effect << " param " << param
                  << " = " << value << std::endl;
    }
    void setBusEffectEnabled(const std::string& effect, bool enabled) {
        std::cout << "  [Mock] Bus effect " << effect << " enabled: " << enabled << std::endl;
    }
    void setBusEffectParameter(const std::string& effect, const std::string& param, float value) {
        std::cout << "  [Mock] Bus effect " << effect << " param " << param << " = " << value
                  << std::endl;
    }

    // Getters for state capture
    float getMasterTempo() const { return m_masterTempo; }
    float getGranularTempo() const { return m_granularTempo; }
    float getBusVolume() const { return m_busVolume; }

    // Song manager access
    SongManager& getSongManagerRef() { return m_songManager; }

private:
    SongManager m_songManager;
    float m_masterTempo = 1.0f;
    float m_granularTempo = 1.0f;
    float m_busVolume = 1.0f;
    std::vector<float> m_trackVolumes = {1.0f, 1.0f, 1.0f, 1.0f};
    std::vector<bool> m_trackActive = {true, true, true, true};
};

// Mock EventSystem that works with our mock controller
class MockEventSystem {
public:
    MockEventSystem(MockAudioController* controller) : m_controller(controller) {}

    void triggerSongEvent(const std::string& songName, const std::string& eventName) {
        if (!m_controller) {
            std::cerr << "[MockEventSystem] ERROR: No controller available" << std::endl;
            return;
        }

        const SongManager* songManager = m_controller->getSongManager();
        if (!songManager) {
            std::cerr << "[MockEventSystem] ERROR: No song manager available" << std::endl;
            return;
        }

        // Find the song
        const Song* song = songManager->findSong(songName);
        if (!song) {
            std::cerr << "[MockEventSystem] ERROR: Song not found: " << songName << std::endl;
            return;
        }

        // Find the event
        auto eventIt =
            std::find_if(song->events.begin(), song->events.end(),
                         [&eventName](const SongEvent& event) { return event.name == eventName; });

        if (eventIt == song->events.end()) {
            std::cerr << "[MockEventSystem] ERROR: Event not found: " << eventName
                      << " in song: " << songName << std::endl;
            return;
        }

        std::cout << "[MockEventSystem] INFO: Triggering song event: " << songName << " -> "
                  << eventName << std::endl;

        // Apply the event state directly (no lerping for this test)
        applySongEvent(eventIt->state);
    }

    void triggerMasterEvent(const std::string& eventName) {
        if (!m_controller) {
            std::cerr << "[MockEventSystem] ERROR: No controller available" << std::endl;
            return;
        }

        const SongManager* songManager = m_controller->getSongManager();
        if (!songManager) {
            std::cerr << "[MockEventSystem] ERROR: No song manager available" << std::endl;
            return;
        }

        // Find the master event
        const MasterBus& masterBus = songManager->getMasterBus();
        auto eventIt = std::find_if(
            masterBus.events.begin(), masterBus.events.end(),
            [&eventName](const MasterEvent& event) { return event.name == eventName; });

        if (eventIt == masterBus.events.end()) {
            std::cerr << "[MockEventSystem] ERROR: Master event not found: " << eventName
                      << std::endl;
            return;
        }

        std::cout << "[MockEventSystem] INFO: Triggering master event: " << eventName << std::endl;

        // Apply the event state directly
        applyMasterEvent(eventIt->state);
    }

private:
    MockAudioController* m_controller;

    void applySongEvent(const StateSnapshot& state) {
        std::cout << "  [Mock] Applying song state:" << std::endl;
        std::cout << "    Master tempo: " << state.masterTempo << std::endl;
        std::cout << "    Granular tempo: " << state.granularTempo << std::endl;

        m_controller->setMasterTempo(state.masterTempo);
        m_controller->setGranularTempo(state.granularTempo);

        for (size_t i = 0; i < state.tracks.size(); ++i) {
            const auto& track = state.tracks[i];
            std::cout << "    Track " << i << " (" << track.file << "):" << std::endl;
            std::cout << "      Volume: " << track.volume << std::endl;
            std::cout << "      Volume: " << track.volume << std::endl;

            m_controller->setTrackVolume(i, track.volume);

            for (const auto& [effectName, effectState] : track.effects) {
                for (const auto& [paramName, paramValue] : effectState.parameters) {
                    m_controller->setTrackEffectParameter(i, effectName, paramName, paramValue);
                }
            }
        }
    }

    void applyMasterEvent(const MasterBusState& state) {
        std::cout << "  [Mock] Applying master state:" << std::endl;
        std::cout << "    Master tempo: " << state.masterTempo << std::endl;
        std::cout << "    Granular tempo: " << state.granularTempo << std::endl;
        std::cout << "    Volume: " << state.volume << std::endl;

        m_controller->setMasterTempo(state.masterTempo);
        m_controller->setGranularTempo(state.granularTempo);
        m_controller->setBusVolume(state.volume);

        for (const auto& [effectName, effectState] : state.effects) {
            // Apply effect parameters instead of enabled state
            for (const auto& [paramName, paramValue] : effectState.parameters) {
                m_controller->setBusEffectParameter(effectName, paramName, paramValue);
            }
        }
    }
};

int main() {
    std::cout << "=== Testing Event System (Minimal) ===" << std::endl;

    MockAudioController controller;

    // Load songs from sound_staging directory
    controller.getSongManagerRef().loadSongsFromDirectory("sound_staging");

    // Display loaded songs
    const auto* songManager = controller.getSongManager();
    if (songManager) {
        const auto& songs = songManager->getSongs();
        std::cout << "\nLoaded " << songs.size() << " songs:" << std::endl;

        for (const auto& song : songs) {
            std::cout << "  Song: " << song.name << " (" << song.events.size() << " events)"
                      << std::endl;
            for (const auto& event : song.events) {
                std::cout << "    - " << event.name << std::endl;
            }
        }

        const auto& masterBus = songManager->getMasterBus();
        std::cout << "\nMaster Bus: " << masterBus.name << " (" << masterBus.events.size()
                  << " events)" << std::endl;
        for (const auto& event : masterBus.events) {
            std::cout << "  - " << event.name << std::endl;
        }
    }

    // Test event triggering
    std::cout << "\n=== Testing Event Triggering ===" << std::endl;

    MockEventSystem eventSystem(&controller);

    // Trigger a song event
    std::cout << "\nTriggering song event: Test Song -> Intro" << std::endl;
    eventSystem.triggerSongEvent("Test Song", "Intro");

    // Trigger another event
    std::cout << "\nTriggering song event: Test Song -> Verse" << std::endl;
    eventSystem.triggerSongEvent("Test Song", "Verse");

    // Test master event
    std::cout << "\nTriggering master event: Underwater" << std::endl;
    eventSystem.triggerMasterEvent("Underwater");

    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}