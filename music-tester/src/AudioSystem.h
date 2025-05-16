#pragma once

#include "soloud.h"
#include "soloud_bus.h"
#include "soloud_wav.h"
#include "soloud_biquadresonantfilter.h"
#include "soloud_echofilter.h"
#include "soloud_lofifilter.h"
#include "soloud_flangerfilter.h"
#include "soloud_dcremovalfilter.h"
#include "soloud_bassboostfilter.h"
#include "soloud_waveshaperfilter.h"
#include "soloud_robotizefilter.h"
#include "soloud_freeverbfilter.h"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct FilterParameter {
    float value;
    float min;
    float max;
    std::string name;
    bool changed = false;
};

struct FilterInstance {
    std::unique_ptr<SoLoud::Filter> filter;
    std::unordered_map<int, FilterParameter> parameters;
    bool enabled = false;
    bool needsUpdate = false;
};

struct TrackFilters {
    std::unordered_map<std::string, FilterInstance> filters;
};

class AudioSystem
{
  public:
    AudioSystem();
    ~AudioSystem();

    // Initialize the audio system
    bool initialize();

    // Load all audio files from a directory
    bool loadDirectory(const std::filesystem::path& directory);

    // Play all loaded tracks
    void playAll();

    // Stop all tracks
    void stopAll();

    // Pause all tracks
    void pauseAll();

    // Resume all tracks
    void resumeAll();

    // Get the current playback position
    float getPlaybackPosition();

    // Set the playback position
    void setPlaybackPosition(float position);

    // Set volume for a specific track
    void setTrackVolume(size_t trackIndex, float volume);

    // Get volume for a specific track
    float getTrackVolume(size_t trackIndex) const;

    // Filter management
    void updateFilterParams(size_t trackIndex);
    void setFilterParameter(size_t trackIndex, const std::string& filterName, int paramId, float value);
    float getFilterParameter(size_t trackIndex, const std::string& filterName, int paramId) const;
    void setFilterEnabled(size_t trackIndex, const std::string& filterName, bool enabled);
    bool isFilterEnabled(size_t trackIndex, const std::string& filterName) const;
    const std::unordered_map<std::string, FilterInstance>& getFilters(size_t trackIndex) const;
    size_t getTrackCount() const { return m_tracks.size(); }

    float getLongestTrackLength() const;
    void setLoopAll(bool loop);
    bool isLoopAll() const { return m_loopAll; }

    float getBusVolume() const { return m_busVolume; }
    void setBusVolume(float volume);

    // Track looping control
    void setTrackLooping(size_t trackIndex, bool loop);
    bool isTrackLooping(size_t trackIndex) const;

    // Bus FX API
    void setBusFilterEnabled(const std::string& filterName, bool enabled);
    bool isBusFilterEnabled(const std::string& filterName) const;
    void setBusFilterParameter(const std::string& filterName, int paramId, float value);
    float getBusFilterParameter(const std::string& filterName, int paramId) const;
    const std::unordered_map<std::string, FilterInstance>& getBusFilters() const;

    static const std::vector<std::string> AVAILABLE_FILTERS;

  private:
    SoLoud::Soloud m_soloud;
    SoLoud::Bus m_masterBus;
    std::vector<std::unique_ptr<SoLoud::Wav>> m_tracks;
    std::vector<float> m_trackVolumes;
    std::vector<TrackFilters> m_trackFilters;
    std::unordered_map<size_t, unsigned int> m_voiceHandles;
    bool m_isInitialized;
    bool m_loopAll = true;
    float m_busVolume = 1.0f;
    unsigned int m_busHandle = 0;

    // Bus FX
    std::unordered_map<std::string, FilterInstance> m_busFilters;
    void updateBusFilterParams();

    void initializeFilter(FilterInstance& instance, const std::string& filterName);
    void updateFilterInstance(FilterInstance& instance, const std::string& filterName);
};