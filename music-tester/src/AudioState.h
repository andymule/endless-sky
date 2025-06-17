#pragma once

#include <functional>
#include <string>
#include <vector>

namespace AudioTester {

    class AudioState {
    public:
        struct TrackState {
            std::string name;
            std::string filepath;
            float volume = 1.0f;
            bool active = false;
            bool looping = true;
            bool isPlaying = false;
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
            track.active = true; // Default to enabled
            tracks.push_back(track);
            notifyChanged();
        }

        void clearTracks() {
            tracks.clear();
            globalPlaying = false;
            notifyChanged();
        }

        void setTrackVolume(size_t index, float volume) {
            if (index < tracks.size()) {
                tracks[index].volume = volume;
                notifyChanged();
            }
        }

        void setTrackActive(size_t index, bool active) {
            if (index < tracks.size()) {
                tracks[index].active = active;
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