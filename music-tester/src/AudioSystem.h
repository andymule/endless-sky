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

namespace AudioTester {

    // RAII wrapper for SoLoud engine
    class SoloudEngine {
    public:
        SoloudEngine() = default;
        ~SoloudEngine() = default;

        bool initialize() { return m_soloud.init() == 0; }
        void deinitialize() { m_soloud.deinit(); }
        SoLoud::Soloud& get() { return m_soloud; }
        const SoLoud::Soloud& get() const { return m_soloud; }

    private:
        SoLoud::Soloud m_soloud;
    };

    // RAII wrapper for audio bus
    class AudioBus {
    public:
        AudioBus(SoLoud::Soloud& /*soloud*/, float volume = 1.0f) : m_bus(), m_volume(volume) {
            m_bus.setVolume(volume);
        }

        ~AudioBus() = default;

        // Delete copy and move operations
        AudioBus(const AudioBus&) = delete;
        AudioBus& operator=(const AudioBus&) = delete;
        AudioBus(AudioBus&&) = delete;
        AudioBus& operator=(AudioBus&&) = delete;

        void setVolume(float volume) {
            m_volume = volume;
            m_bus.setVolume(volume);
        }

        float getVolume() const { return m_volume; }
        SoLoud::Bus& get() { return m_bus; }
        const SoLoud::Bus& get() const { return m_bus; }

    private:
        SoLoud::Bus m_bus;
        float m_volume;
    };

    struct FilterParameter {
        float value;
        float min;
        float max;
        std::string name;
    };

    struct FilterInstance {
        std::unique_ptr<SoLoud::Filter> filter;
        std::unordered_map<int, FilterParameter> parameters;
        bool enabled = false;
        int slot = -1;
    };

    struct TrackFilters {
        std::unordered_map<std::string, FilterInstance> filters;
    };

    class AudioSystem {
    public:
        AudioSystem();
        ~AudioSystem();

        bool initialize();
        void cleanup();

        // Track management
        void loadAudioFile(const std::string& path);
        void playTrack(size_t index);
        void stopTrack(size_t index);
        void setTrackVolume(size_t index, float volume);
        void setTrackLooping(size_t index, bool looping);

        // Bus management
        void setBusVolume(float volume);
        float getBusVolume() const;

        // Filter management
        void addFilterToTrack(size_t trackIndex, const std::string& filterName);
        void removeFilterFromTrack(size_t trackIndex, const std::string& filterName);
        void setFilterParameter(size_t trackIndex, const std::string& filterName, int paramId,
                                float value);
        void setFilterEnabled(size_t trackIndex, const std::string& filterName, bool enabled);
        bool isFilterEnabled(size_t trackIndex, const std::string& filterName) const;
        float getFilterParameter(size_t trackIndex, const std::string& filterName,
                                 int paramId) const;
        const std::unordered_map<std::string, FilterInstance>& getFilters(size_t trackIndex) const;

        // Bus filter management
        void setBusFilterEnabled(const std::string& filterName, bool enabled);
        bool isBusFilterEnabled(const std::string& filterName) const;
        void setBusFilterParameter(const std::string& filterName, int paramId, float value);
        float getBusFilterParameter(const std::string& filterName, int paramId) const;
        const std::unordered_map<std::string, FilterInstance>& getBusFilters() const;

        // Static members
        static const std::vector<std::string> AVAILABLE_FILTERS;
        static bool isSupportedFileExtension(const std::string& ext);

    private:
        void applyFiltersToTrack(size_t trackIndex);
        void updateBusFilterParams();
        void initializeFilter(FilterInstance& instance, const std::string& filterName);
        void updateFilterInstance(FilterInstance& instance, const std::string& filterName);

        std::unique_ptr<SoloudEngine> m_engine;
        std::unique_ptr<AudioBus> m_masterBus;
        std::vector<std::unique_ptr<SoLoud::Wav>> m_tracks;
        std::vector<TrackFilters> m_trackFilters;
        std::unordered_map<std::string, FilterInstance> m_busFilters;
        std::unordered_map<size_t, unsigned int> m_trackHandles;
        float m_busVolume = 1.0f;
        bool m_isInitialized = false;
        unsigned int m_busHandle = 0;
    };

} // namespace AudioTester