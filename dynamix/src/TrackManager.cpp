#include "TrackManager.h"
#include <stdexcept>

namespace AudioTester {

    TrackManager::TrackInfo& TrackManager::getTrack(size_t idx) {
        if (idx >= m_tracks.size()) {
            throw std::out_of_range("Track index out of range");
        }
        return m_tracks[idx];
    }

    const TrackManager::TrackInfo& TrackManager::getTrack(size_t idx) const {
        if (idx >= m_tracks.size()) {
            throw std::out_of_range("Track index out of range");
        }
        return m_tracks[idx];
    }

    void TrackManager::addTrack(TrackManager::TrackInfo&& info) {
        m_tracks.push_back(std::move(info));
    }

    bool TrackManager::removeTrack(size_t idx) {
        if (idx >= m_tracks.size()) {
            return false; // Track not found
        }
        m_tracks.erase(m_tracks.begin() + idx);
        return true; // Track successfully removed
    }

} // namespace AudioTester