#pragma once

#include "ErrorHandling.h"
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
        AudioBus(SoLoud::Soloud& soloud, float volume = 1.0f)
            : m_soloud(soloud), m_bus(), m_volume(volume) {
            m_bus.setVolume(volume);
        }

        ~AudioBus() {
            // Bus is automatically cleaned up by SoLoud
        }

        // Delete copy operations
        AudioBus(const AudioBus&) = delete;
        AudioBus& operator=(const AudioBus&) = delete;

        // Delete move operations since we have a reference member
        AudioBus(AudioBus&&) = delete;
        AudioBus& operator=(AudioBus&&) = delete;

        void setVolume(float volume) {
            m_volume = volume;
            m_bus.setVolume(volume);
        }

        float getVolume() const { return m_volume; }
        SoLoud::Bus& get() { return m_bus; }
        const SoLoud::Bus& get() const { return m_bus; }
        unsigned int getHandle() const { return m_bus.mChannelHandle; }

        // Additional methods needed for AudioSystem
        unsigned int playClocked(double startTime, SoLoud::AudioSource& source, float volume) {
            return m_bus.playClocked(startTime, source, volume);
        }

        void setFilter(int slot, std::shared_ptr<SoLoud::Filter> filter) {
            m_bus.setFilter(slot, filter.get());
        }

        int getActiveVoiceCount() { return m_bus.getActiveVoiceCount(); }

    private:
        SoLoud::Soloud& m_soloud;
        SoLoud::Bus m_bus;
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
                m_filter = std::make_shared<SoLoud::BassboostFilter>();
            } else if (filterName == "BiquadResonant") {
                m_filter = std::make_shared<SoLoud::BiquadResonantFilter>();
            } else if (filterName == "DCRemoval") {
                m_filter = std::make_shared<SoLoud::DCRemovalFilter>();
            } else if (filterName == "Echo") {
                m_filter = std::make_shared<SoLoud::EchoFilter>();
            } else if (filterName == "Flanger") {
                m_filter = std::make_shared<SoLoud::FlangerFilter>();
            } else if (filterName == "Freeverb") {
                m_filter = std::make_shared<SoLoud::FreeverbFilter>();
            } else if (filterName == "Lofi") {
                m_filter = std::make_shared<SoLoud::LofiFilter>();
            } else if (filterName == "Robotize") {
                m_filter = std::make_shared<SoLoud::RobotizeFilter>();
            } else if (filterName == "WaveShaper") {
                m_filter = std::make_shared<SoLoud::WaveShaperFilter>();
            }
        }

        // Get the underlying filter
        std::shared_ptr<SoLoud::Filter> get() { return m_filter; }
        const std::shared_ptr<SoLoud::Filter>& get() const { return m_filter; }

        // Check if filter is valid
        bool isValid() const { return m_filter != nullptr; }

    private:
        std::shared_ptr<SoLoud::Filter> m_filter;
    };

    struct FilterInstance {
        FilterInstance() = default;

        // Initialize with a specific filter type
        void initialize(const std::string& filterName) {
            if (filterName == "BassBoost") {
                filter = std::make_unique<SoLoud::BassboostFilter>();
            } else if (filterName == "BiquadResonant") {
                filter = std::make_unique<SoLoud::BiquadResonantFilter>();
            } else if (filterName == "DCRemoval") {
                filter = std::make_unique<SoLoud::DCRemovalFilter>();
            } else if (filterName == "Echo") {
                filter = std::make_unique<SoLoud::EchoFilter>();
            } else if (filterName == "Flanger") {
                filter = std::make_unique<SoLoud::FlangerFilter>();
            } else if (filterName == "Freeverb") {
                filter = std::make_unique<SoLoud::FreeverbFilter>();
            } else if (filterName == "Lofi") {
                filter = std::make_unique<SoLoud::LofiFilter>();
            } else if (filterName == "Robotize") {
                filter = std::make_unique<SoLoud::RobotizeFilter>();
            } else if (filterName == "WaveShaper") {
                filter = std::make_unique<SoLoud::WaveShaperFilter>();
            }
            enabled = false;
            needsUpdate = false;
            slot = -1;
        }

        // Update filter parameters
        void update() {
            if (!filter || !enabled)
                return;

            for (auto& [paramId, param] : parameters) {
                if (param.changed) {
                    if (auto* bassboost = dynamic_cast<SoLoud::BassboostFilter*>(filter.get())) {
                        bassboost->setParams(param.value);
                    } else if (auto* biquad =
                                   dynamic_cast<SoLoud::BiquadResonantFilter*>(filter.get())) {
                        biquad->setParams(param.value, param.value, param.value);
                    } else if (auto* echo = dynamic_cast<SoLoud::EchoFilter*>(filter.get())) {
                        echo->setParams(param.value, 0.7f, 0.0f);
                    } else if (auto* flanger = dynamic_cast<SoLoud::FlangerFilter*>(filter.get())) {
                        flanger->setParams(param.value, param.value);
                    } else if (auto* freeverb =
                                   dynamic_cast<SoLoud::FreeverbFilter*>(filter.get())) {
                        freeverb->setParams(param.value, 0.5f, 0.5f, 0.5f);
                    } else if (auto* lofi = dynamic_cast<SoLoud::LofiFilter*>(filter.get())) {
                        lofi->setParams(param.value, param.value);
                    } else if (auto* robotize =
                                   dynamic_cast<SoLoud::RobotizeFilter*>(filter.get())) {
                        robotize->setParams(param.value, param.value);
                    } else if (auto* waveshaper =
                                   dynamic_cast<SoLoud::WaveShaperFilter*>(filter.get())) {
                        waveshaper->setParams(param.value);
                    }
                    param.changed = false;
                }
            }
            needsUpdate = false;
        }

        std::unique_ptr<SoLoud::Filter> filter;
        std::unordered_map<int, FilterParameter> parameters;
        bool enabled = false;
        bool needsUpdate = false;
        int slot = -1;
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

        // Static members
        static const std::vector<std::string> AVAILABLE_FILTERS;
        static bool isSupportedFileExtension(const std::string& ext);

    private:
        std::unique_ptr<SoloudEngine> m_engine;
        std::unique_ptr<AudioBus> m_masterBus;
        std::vector<std::unique_ptr<SoLoud::Wav>> m_tracks;
        std::vector<FilterInstance> m_trackFilters;
        std::vector<FilterInstance> m_busFilters;
        std::unordered_map<size_t, unsigned int>
            m_trackHandles; // Track index to voice handle mapping
        float m_busVolume = 1.0f;
        bool m_isInitialized = false;
        unsigned int m_busHandle = 0;
    };
} // namespace AudioTester