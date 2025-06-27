#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace AudioTester {

    // Effect state for complete automation
    struct EffectState {
        std::map<std::string, float> parameters;
    };

    // Enhanced track state with complete effect automation
    struct TrackStateExtended {
        std::string file; // Track filename
        float volume = 1.0f;
        std::map<std::string, EffectState> effects;
    };

    // Complete state snapshot for events
    struct StateSnapshot {
        float masterTempo = 1.0f;   // Per-song tempo control
        float granularTempo = 1.0f; // Per-song granular tempo control
        std::vector<TrackStateExtended> tracks;
    };

    // Song event definition
    struct SongEvent {
        std::string name;
        float fadeTime;      // Fade-in duration in seconds
        StateSnapshot state; // Complete song state (tracks + tempo)
    };

    // Song container
    struct Song {
        std::vector<SongEvent> events;
        std::filesystem::path folderPath;
    };

    // Master bus state with effects and tempo (only exists at master level)
    struct MasterBusState {
        float masterTempo = 1.0f;   // Master-level tempo control
        float granularTempo = 1.0f; // Master-level granular tempo control
        float volume = 1.0f;
        std::map<std::string, EffectState> effects;
    };

    // Master bus event definition
    struct MasterEvent {
        std::string name;
        float fadeTime;       // Fade-in duration in seconds
        MasterBusState state; // Complete master bus state (effects + tempo)
    };

    // Master bus container (separate from songs)
    struct MasterBus {
        std::string name;
        std::vector<MasterEvent> events; // Events containing MasterBusState and tempo
    };

    class AudioState {
    public:
        struct TrackState {
            std::string name;
            std::string filepath;
            bool looping = true;
            bool isPlaying = false;
            // Note: volume is now managed by AudioSystem::TrackInfo as single source of truth
        };

        // State data
        std::vector<TrackState> tracks;
        float busVolume = 1.0f;
        bool globalPlaying = false;

        // Simple notification system for UI updates
        std::function<void()> onStateChanged;

        // State modification methods
        void addTrack(const std::string& name, const std::string& filepath) {
            TrackState track;
            track.name = name;
            track.filepath = filepath;
            tracks.push_back(track);
            notifyChanged();
        }

        void clearTracks() {
            tracks.clear();
            globalPlaying = false;
            notifyChanged();
        }

        void removeTrack(size_t index) {
            if (index < tracks.size()) {
                tracks.erase(tracks.begin() + index);
                notifyChanged();
            }
        }

        void setTrackLooping(size_t index, bool looping) {
            if (index < tracks.size()) {
                tracks[index].looping = looping;
                notifyChanged();
            }
        }

        void setGlobalPlaying(bool playing) {
            globalPlaying = playing;
            // Update individual track playing states
            for (auto& track : tracks) {
                track.isPlaying = playing;
            }
            notifyChanged();
        }

        void setBusVolume(float volume) {
            busVolume = volume;
            notifyChanged();
        }

        // Getters
        size_t getTrackCount() const { return tracks.size(); }
        const TrackState& getTrack(size_t index) const { return tracks[index]; }
        TrackState& getTrack(size_t index) { return tracks[index]; }

    private:
        void notifyChanged() {
            if (onStateChanged) {
                onStateChanged();
            }
        }
    };

} // namespace AudioTester