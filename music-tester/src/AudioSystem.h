#pragma once

#include "soloud.h"
#include "soloud_bassboostfilter.h"
#include "soloud_biquadresonantfilter.h"
#include "soloud_bus.h"
#include "soloud_dcremovalfilter.h"
#include "soloud_echofilter.h"
#include "soloud_flangerfilter.h"
#include "soloud_freeverbfilter.h"
#include "soloud_lofifilter.h"
#include "soloud_robotizefilter.h"
#include "soloud_wav.h"
#include "soloud_waveshaperfilter.h"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// RAII wrapper for SoLoud engine
class SoloudEngine {
public:
    SoloudEngine() {
        if (m_soloud.init() != SoLoud::SO_NO_ERROR) {
            throw std::runtime_error("Failed to initialize SoLoud engine");
        }
    }
    ~SoloudEngine() { m_soloud.deinit(); }

    // Delete copy
    SoloudEngine(const SoloudEngine&) = delete;
    SoloudEngine& operator=(const SoloudEngine&) = delete;

    // Allow move
    SoloudEngine(SoloudEngine&&) = default;
    SoloudEngine& operator=(SoloudEngine&&) = default;

    SoLoud::Soloud& get() { return m_soloud; }
    const SoLoud::Soloud& get() const { return m_soloud; }

private:
    SoLoud::Soloud m_soloud;
};

// RAII wrapper for audio bus
class AudioBus {
public:
    AudioBus(SoLoud::Soloud& soloud, float initialVolume = 1.0f)
        : m_soloud(soloud), m_volume(initialVolume) {
        m_handle = m_soloud.play(m_bus);
        m_bus.setVolume(m_volume);
        m_soloud.setVolume(m_handle, m_volume);
    }

    ~AudioBus() {
        if (m_handle) {
            m_soloud.stop(m_handle);
        }
    }

    // Delete copy
    AudioBus(const AudioBus&) = delete;
    AudioBus& operator=(const AudioBus&) = delete;

    // Allow move
    AudioBus(AudioBus&&) = default;
    AudioBus& operator=(AudioBus&&) = default;

    void setVolume(float volume) {
        m_volume = volume;
        m_bus.setVolume(volume);
        m_soloud.setVolume(m_handle, volume);
    }

    float getVolume() const { return m_volume; }
    SoLoud::Bus& get() { return m_bus; }
    const SoLoud::Bus& get() const { return m_bus; }
    unsigned int getHandle() const { return m_handle; }

    // Additional methods needed for AudioSystem
    unsigned int playClocked(double startTime, SoLoud::AudioSource& source, float volume) {
        return m_bus.playClocked(startTime, source, volume);
    }

    void setFilter(int slot, SoLoud::Filter* filter) { m_bus.setFilter(slot, filter); }

    int getActiveVoiceCount() const { return m_bus.getActiveVoiceCount(); }

private:
    SoLoud::Soloud& m_soloud;
    mutable SoLoud::Bus m_bus; // Made mutable to handle const-correctness issues with SoLoud's API
    unsigned int m_handle = 0;
    float m_volume;
};

struct FilterParameter {
    float value;
    float min;
    float max;
    std::string name;
    bool changed = false;
};

// RAII wrapper for SoLoud filter
class FilterWrapper {
public:
    FilterWrapper() = default;

    // Factory method to create appropriate filter
    static std::unique_ptr<FilterWrapper> create(const std::string& filterName) {
        auto wrapper = std::make_unique<FilterWrapper>();
        wrapper->initialize(filterName);
        return wrapper;
    }

    // Initialize the filter with the given name
    void initialize(const std::string& filterName) {
        if (filterName == "BassBoost") {
            m_filter = std::make_unique<SoLoud::BassboostFilter>();
        } else if (filterName == "BiquadResonant") {
            m_filter = std::make_unique<SoLoud::BiquadResonantFilter>();
        } else if (filterName == "DCRemoval") {
            m_filter = std::make_unique<SoLoud::DCRemovalFilter>();
        } else if (filterName == "Echo") {
            m_filter = std::make_unique<SoLoud::EchoFilter>();
        } else if (filterName == "Flanger") {
            m_filter = std::make_unique<SoLoud::FlangerFilter>();
        } else if (filterName == "Freeverb") {
            m_filter = std::make_unique<SoLoud::FreeverbFilter>();
        } else if (filterName == "Lofi") {
            m_filter = std::make_unique<SoLoud::LofiFilter>();
        } else if (filterName == "Robotize") {
            m_filter = std::make_unique<SoLoud::RobotizeFilter>();
        } else if (filterName == "WaveShaper") {
            m_filter = std::make_unique<SoLoud::WaveShaperFilter>();
        }
    }

    // Get the underlying filter
    SoLoud::Filter* get() { return m_filter.get(); }
    const SoLoud::Filter* get() const { return m_filter.get(); }

    // Check if filter is valid
    bool isValid() const { return m_filter != nullptr; }

private:
    std::unique_ptr<SoLoud::Filter> m_filter;
};

struct FilterInstance {
    FilterInstance() = default;

    // Initialize with a specific filter type
    void initialize(const std::string& filterName) {
        filter = FilterWrapper::create(filterName);
        enabled = false;
        needsUpdate = false;
        slot = -1;
    }

    // Update filter parameters
    void update() {
        if (!filter || !filter->isValid() || !enabled)
            return;

        for (auto& [paramId, param] : parameters) {
            if (param.changed) {
                // Each filter type has its own parameter setting method
                if (auto* bassboost = dynamic_cast<SoLoud::BassboostFilter*>(filter->get())) {
                    bassboost->setParams(param.value);
                } else if (auto* biquad =
                               dynamic_cast<SoLoud::BiquadResonantFilter*>(filter->get())) {
                    biquad->setParams(param.value, param.value, param.value);
                } else if (auto* echo = dynamic_cast<SoLoud::EchoFilter*>(filter->get())) {
                    echo->setParams(param.value, 0.7f, 0.0f); // Default decay and filter values
                } else if (auto* flanger = dynamic_cast<SoLoud::FlangerFilter*>(filter->get())) {
                    flanger->setParams(param.value, param.value);
                } else if (auto* freeverb = dynamic_cast<SoLoud::FreeverbFilter*>(filter->get())) {
                    freeverb->setParams(param.value, 0.5f, 0.5f,
                                        0.5f); // Default roomSize, damp, width values
                } else if (auto* lofi = dynamic_cast<SoLoud::LofiFilter*>(filter->get())) {
                    lofi->setParams(param.value, param.value);
                } else if (auto* robotize = dynamic_cast<SoLoud::RobotizeFilter*>(filter->get())) {
                    robotize->setParams(param.value, param.value);
                } else if (auto* waveshaper =
                               dynamic_cast<SoLoud::WaveShaperFilter*>(filter->get())) {
                    waveshaper->setParams(param.value);
                }
                param.changed = false;
            }
        }
        needsUpdate = false;
    }

    std::unique_ptr<FilterWrapper> filter;
    std::unordered_map<int, FilterParameter> parameters;
    bool enabled = false;
    bool needsUpdate = false;
    int slot = -1; // Track which slot this filter is assigned to
};

struct TrackFilters {
    TrackFilters() = default;

    // Initialize filters for a track
    void initialize(const std::vector<std::string>& filterNames) {
        for (const auto& name : filterNames) {
            FilterInstance instance;
            instance.initialize(name);
            filters[name] = std::move(instance);
        }
    }

    // Update all filters
    void update() {
        for (auto& [name, instance] : filters) {
            instance.update();
        }
    }

    std::unordered_map<std::string, FilterInstance> filters;
};

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem() = default; // All cleanup handled by RAII

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
    void setFilterParameter(size_t trackIndex, const std::string& filterName, int paramId,
                            float value);
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
    static bool isSupportedFileExtension(const std::string& extension);

private:
    std::unique_ptr<SoloudEngine> m_engine;
    std::unique_ptr<AudioBus> m_masterBus;
    std::vector<std::unique_ptr<SoLoud::Wav>> m_tracks;
    std::vector<float> m_trackVolumes;
    std::vector<TrackFilters> m_trackFilters;
    std::unordered_map<size_t, unsigned int> m_voiceHandles;
    bool m_loopAll = true;
    float m_busVolume = 1.0f;

    // Bus FX
    std::unordered_map<std::string, FilterInstance> m_busFilters;
    void updateBusFilterParams();

    void initializeFilter(FilterInstance& instance, const std::string& filterName);
    void updateFilterInstance(FilterInstance& instance, const std::string& filterName);
};