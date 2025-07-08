#pragma once
#include "SyncWav.h" // For SyncWav definition
#include <memory>
#include <string>
#include <vector>

namespace AudioTester {
    class TrackManager {
    public:
        struct TrackInfo {
            std::unique_ptr<SyncWav> wav;
            double duration = 0.0;
            unsigned int handle = 0;
            bool isPlaying = false;
            bool isPaused = false;
            double lastSyncCheck = 0.0;
            double expectedPosition = 0.0;
            float volume = 1.0f;
        };

        // Track access methods
        size_t getTrackCount() const noexcept { return m_tracks.size(); }
        bool isEmpty() const noexcept { return m_tracks.empty(); }

        // Safe track access with bounds checking
        TrackInfo& getTrack(size_t idx);
        const TrackInfo& getTrack(size_t idx) const;

        // Track management
        void addTrack(TrackInfo&& info);
        bool removeTrack(size_t idx); // Returns true if track was removed
        void clear() noexcept { m_tracks.clear(); }

    private:
        std::vector<TrackInfo> m_tracks;
    };
} // namespace AudioTester