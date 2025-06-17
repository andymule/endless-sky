#include "AudioSystem.h"
#include "ErrorHandling.h"
#include <filesystem>
#include <iostream>

namespace AudioTester {

    const std::vector<std::string> AudioSystem::AVAILABLE_FILTERS = {
        "BassBoost", "BiquadResonant", "DCRemoval", "Echo",      "Flanger",
        "Freeverb",  "Lofi",           "Robotize",  "WaveShaper"};

    AudioSystem::AudioSystem() = default;

    AudioSystem::~AudioSystem() { cleanup(); }

    bool AudioSystem::initialize() {
        try {
            m_engine = std::make_unique<SoloudEngine>();
            if (!m_engine->initialize()) {
                return false;
            }

            m_masterBus = std::make_unique<AudioBus>(m_engine->get());
            m_isInitialized = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize audio system: " << e.what() << std::endl;
            return false;
        }
    }

    void AudioSystem::cleanup() {
        if (!m_isInitialized)
            return;

        // Stop all tracks
        for (const auto& [index, handle] : m_trackHandles) {
            m_engine->get().stop(handle);
        }
        m_trackHandles.clear();

        if (m_busHandle != 0) {
            m_engine->get().stop(m_busHandle);
            m_busHandle = 0;
        }

        m_masterBus.reset();
        m_engine.reset();
        m_isInitialized = false;
    }

    void AudioSystem::loadAudioFile(const std::string& path) {
        if (!m_isInitialized)
            return;

        auto track = std::make_unique<SoLoud::Wav>();
        if (track->load(path.c_str()) != SoLoud::SO_NO_ERROR) {
            throw std::runtime_error("Failed to load audio file: " + path);
        }
        m_tracks.push_back(std::move(track));

        // Initialize filter instances for this track
        m_trackFilters.push_back(std::vector<FilterInstance>());
    }

    void AudioSystem::playTrack(size_t index) {
        if (!m_isInitialized || index >= m_tracks.size())
            return;

        if (m_busHandle == 0) {
            m_masterBus->setVolume(m_busVolume);
            m_busHandle = m_engine->get().play(m_masterBus->get());
            m_engine->get().setVolume(m_busHandle, m_busVolume);
        }

        // Apply any active filters to the track before playing
        applyFiltersToTrack(index);

        // Play the track through the bus
        unsigned int handle = m_masterBus->get().play(*m_tracks[index]);
        m_trackHandles[index] = handle;
    }

    void AudioSystem::stopTrack(size_t index) {
        if (!m_isInitialized || index >= m_tracks.size())
            return;

        auto it = m_trackHandles.find(index);
        if (it != m_trackHandles.end()) {
            m_engine->get().stop(it->second);
            m_trackHandles.erase(it);
        }
    }

    void AudioSystem::setTrackVolume(size_t index, float volume) {
        if (!m_isInitialized || index >= m_tracks.size())
            return;

        // Set the volume directly on the track
        m_tracks[index]->setVolume(volume);

        // Update the track's volume through the bus
        auto it = m_trackHandles.find(index);
        if (it != m_trackHandles.end()) {
            m_engine->get().setVolume(it->second, volume);
        }
    }

    void AudioSystem::setTrackLooping(size_t index, bool looping) {
        if (!m_isInitialized || index >= m_tracks.size())
            return;

        m_tracks[index]->setLooping(looping);
    }

    void AudioSystem::setBusVolume(float volume) {
        if (!m_isInitialized)
            return;

        m_busVolume = volume;

        // Update the bus's internal volume
        m_masterBus->setVolume(volume);

        // Update the bus handle volume
        if (m_busHandle != 0) {
            m_engine->get().setVolume(m_busHandle, volume);
        }
    }

    float AudioSystem::getBusVolume() const { return m_busVolume; }

    void AudioSystem::addFilterToTrack(size_t trackIndex, const std::string& filterName) {
        if (!m_isInitialized || trackIndex >= m_tracks.size())
            return;

        // Check if filter already exists
        for (auto& instance : m_trackFilters[trackIndex]) {
            if (instance.filterName == filterName) {
                instance.enabled = true;
                applyFiltersToTrack(trackIndex);
                return;
            }
        }

        // Create new filter instance
        FilterInstance instance;
        instance.initialize(filterName);
        instance.filterName = filterName;
        instance.enabled = true;
        m_trackFilters[trackIndex].push_back(std::move(instance));

        applyFiltersToTrack(trackIndex);
    }

    void AudioSystem::removeFilterFromTrack(size_t trackIndex, const std::string& filterName) {
        if (!m_isInitialized || trackIndex >= m_tracks.size())
            return;

        // Find and disable the filter
        for (auto& instance : m_trackFilters[trackIndex]) {
            if (instance.filterName == filterName) {
                instance.enabled = false;
                applyFiltersToTrack(trackIndex);
                return;
            }
        }
    }

    void AudioSystem::setFilterParameter(size_t trackIndex, const std::string& filterName,
                                         int paramId, float value) {
        if (!m_isInitialized || trackIndex >= m_tracks.size())
            return;

        // Find the filter and update its parameter
        for (auto& instance : m_trackFilters[trackIndex]) {
            if (instance.filterName == filterName && instance.enabled) {
                auto paramIt = instance.parameters.find(paramId);
                if (paramIt != instance.parameters.end()) {
                    paramIt->second.value = value;
                    paramIt->second.changed = true;
                    instance.needsUpdate = true;

                    // Apply the parameter change immediately for realtime effect
                    updateFilterParameters(instance);

                    // If track is playing, apply the parameter change to the voice
                    auto voiceIt = m_trackHandles.find(trackIndex);
                    if (voiceIt != m_trackHandles.end() && instance.slot >= 0) {
                        // Use SoLoud's fadeFilterParameter for smooth realtime updates
                        m_engine->get().fadeFilterParameter(voiceIt->second, instance.slot, paramId,
                                                            value, 0.05f);
                    }
                }
                break;
            }
        }
    }

    void AudioSystem::applyFiltersToTrack(size_t trackIndex) {
        if (!m_isInitialized || trackIndex >= m_tracks.size())
            return;

        // Check if track is currently playing
        auto voiceIt = m_trackHandles.find(trackIndex);
        if (voiceIt == m_trackHandles.end()) {
            // Track not playing, just apply filters to the source for next play
            for (int slot = 0; slot < 8; ++slot) {
                m_tracks[trackIndex]->setFilter(slot, nullptr);
            }

            int filterSlot = 0;
            for (auto& instance : m_trackFilters[trackIndex]) {
                if (instance.enabled && instance.filter && filterSlot < 8) {
                    m_tracks[trackIndex]->setFilter(filterSlot, instance.filter.get());
                    updateFilterParameters(instance);
                    instance.slot = filterSlot;
                    filterSlot++;
                }
            }
            return;
        }

        // Track is playing - need to restart it with new filters
        unsigned int handle = voiceIt->second;
        float position = m_engine->get().getStreamPosition(handle);
        float volume = m_engine->get().getVolume(handle);

        // Stop current voice
        m_engine->get().stop(handle);
        m_trackHandles.erase(voiceIt);

        // Clear all filters from the track source first
        for (int slot = 0; slot < 8; ++slot) {
            m_tracks[trackIndex]->setFilter(slot, nullptr);
        }

        // Apply enabled filters to the track source
        int filterSlot = 0;
        for (auto& instance : m_trackFilters[trackIndex]) {
            if (instance.enabled && instance.filter && filterSlot < 8) {
                m_tracks[trackIndex]->setFilter(filterSlot, instance.filter.get());
                updateFilterParameters(instance);
                instance.slot = filterSlot;
                filterSlot++;
            }
        }

        // Restart the track with new filters
        unsigned int newHandle = m_masterBus->get().play(*m_tracks[trackIndex]);
        m_trackHandles[trackIndex] = newHandle;
        m_engine->get().setVolume(newHandle, volume);
        m_engine->get().seek(newHandle, position);

        // Apply all filter parameters to the new voice immediately
        filterSlot = 0;
        for (auto& instance : m_trackFilters[trackIndex]) {
            if (instance.enabled && instance.filter && filterSlot < 8) {
                for (const auto& [paramId, param] : instance.parameters) {
                    m_engine->get().setFilterParameter(newHandle, filterSlot, paramId, param.value);
                }
                filterSlot++;
            }
        }
    }

    void AudioSystem::updateFilterParameters(FilterInstance& instance) {
        if (!instance.filter || !instance.enabled)
            return;

        // Update filter parameters based on filter type
        if (auto* bassboost = dynamic_cast<SoLoud::BassboostFilter*>(instance.filter.get())) {
            auto it = instance.parameters.find(0);
            if (it != instance.parameters.end()) {
                bassboost->setParams(it->second.value);
            }
        } else if (auto* biquad =
                       dynamic_cast<SoLoud::BiquadResonantFilter*>(instance.filter.get())) {
            auto freq = instance.parameters.find(0);
            auto res = instance.parameters.find(1);
            if (freq != instance.parameters.end() && res != instance.parameters.end()) {
                biquad->setParams(0, freq->second.value, res->second.value); // Type 0 = lowpass
            }
        } else if (auto* echo = dynamic_cast<SoLoud::EchoFilter*>(instance.filter.get())) {
            auto delay = instance.parameters.find(0);
            auto decay = instance.parameters.find(1);
            auto filter = instance.parameters.find(2);
            if (delay != instance.parameters.end()) {
                float delayVal = delay != instance.parameters.end() ? delay->second.value : 0.3f;
                float decayVal = decay != instance.parameters.end() ? decay->second.value : 0.7f;
                float filterVal = filter != instance.parameters.end() ? filter->second.value : 0.0f;
                echo->setParams(delayVal, decayVal, filterVal);
            }
        } else if (auto* flanger = dynamic_cast<SoLoud::FlangerFilter*>(instance.filter.get())) {
            auto delay = instance.parameters.find(0);
            auto freq = instance.parameters.find(1);
            if (delay != instance.parameters.end()) {
                float delayVal = delay->second.value;
                float freqVal = freq != instance.parameters.end() ? freq->second.value : delayVal;
                flanger->setParams(delayVal, freqVal);
            }
        } else if (auto* freeverb = dynamic_cast<SoLoud::FreeverbFilter*>(instance.filter.get())) {
            auto wet = instance.parameters.find(0);
            if (wet != instance.parameters.end()) {
                freeverb->setParams(wet->second.value, 0.5f, 0.5f, 0.5f);
            }
        } else if (auto* lofi = dynamic_cast<SoLoud::LofiFilter*>(instance.filter.get())) {
            auto samplerate = instance.parameters.find(0);
            auto bitdepth = instance.parameters.find(1);
            if (samplerate != instance.parameters.end()) {
                float srVal = samplerate->second.value;
                float bdVal =
                    bitdepth != instance.parameters.end() ? bitdepth->second.value : srVal;
                lofi->setParams(srVal, bdVal);
            }
        } else if (auto* robotize = dynamic_cast<SoLoud::RobotizeFilter*>(instance.filter.get())) {
            auto freq = instance.parameters.find(0);
            auto wave = instance.parameters.find(1);
            if (freq != instance.parameters.end()) {
                float freqVal = freq->second.value;
                float waveVal = wave != instance.parameters.end() ? wave->second.value : 0.0f;
                robotize->setParams(freqVal, static_cast<int>(waveVal));
            }
        } else if (auto* waveshaper =
                       dynamic_cast<SoLoud::WaveShaperFilter*>(instance.filter.get())) {
            auto amount = instance.parameters.find(0);
            if (amount != instance.parameters.end()) {
                waveshaper->setParams(amount->second.value);
            }
        }

        // Mark parameters as applied
        for (auto& [id, param] : instance.parameters) {
            param.changed = false;
        }
        instance.needsUpdate = false;
    }

    bool AudioSystem::isFilterEnabled(size_t trackIndex, const std::string& filterName) const {
        if (!m_isInitialized || trackIndex >= m_trackFilters.size())
            return false;

        for (const auto& instance : m_trackFilters[trackIndex]) {
            if (instance.filterName == filterName) {
                return instance.enabled;
            }
        }
        return false;
    }

    float AudioSystem::getFilterParameter(size_t trackIndex, const std::string& filterName,
                                          int paramId) const {
        if (!m_isInitialized || trackIndex >= m_trackFilters.size())
            return 0.0f;

        for (const auto& instance : m_trackFilters[trackIndex]) {
            if (instance.filterName == filterName && instance.enabled) {
                auto it = instance.parameters.find(paramId);
                if (it != instance.parameters.end()) {
                    return it->second.value;
                }
            }
        }
        return 0.0f;
    }

    const std::vector<FilterInstance>& AudioSystem::getTrackFilters(size_t trackIndex) const {
        static const std::vector<FilterInstance> empty;
        if (!m_isInitialized || trackIndex >= m_trackFilters.size())
            return empty;
        return m_trackFilters[trackIndex];
    }

    void AudioSystem::setFilterEnabled(size_t trackIndex, const std::string& filterName,
                                       bool enabled) {
        if (!m_isInitialized || trackIndex >= m_trackFilters.size())
            return;

        // Find existing filter or create new one
        bool found = false;
        for (auto& instance : m_trackFilters[trackIndex]) {
            if (instance.filterName == filterName) {
                instance.enabled = enabled;
                found = true;
                break;
            }
        }

        if (!found && enabled) {
            // Create new filter instance
            FilterInstance instance;
            instance.initialize(filterName);
            instance.filterName = filterName;
            instance.enabled = true;
            m_trackFilters[trackIndex].push_back(std::move(instance));
        }

        applyFiltersToTrack(trackIndex);
    }

    const std::unordered_map<std::string, FilterInstance>&
    AudioSystem::getFilters(size_t trackIndex) const {
        static std::unordered_map<std::string, FilterInstance> filterMap;
        filterMap.clear();

        if (!m_isInitialized || trackIndex >= m_trackFilters.size()) {
            return filterMap;
        }

        // We can't return the vector as a map directly due to unique_ptr copy issues
        // Instead, we'll store references or rebuild the API to work with vectors
        // For now, return empty map and fix the UI to work directly with vector
        return filterMap;
    }

    // Bus filter methods
    void AudioSystem::setBusFilterEnabled(const std::string& filterName, bool enabled) {
        auto it = m_busFilters.find(filterName);
        if (it == m_busFilters.end()) {
            if (enabled) {
                FilterInstance instance;
                instance.initialize(filterName);
                instance.filterName = filterName;
                instance.enabled = true;
                m_busFilters[filterName] = std::move(instance);
                updateBusFilterParams();
            }
        } else {
            it->second.enabled = enabled;
            updateBusFilterParams();
        }
    }

    bool AudioSystem::isBusFilterEnabled(const std::string& filterName) const {
        auto it = m_busFilters.find(filterName);
        return it != m_busFilters.end() && it->second.enabled;
    }

    void AudioSystem::setBusFilterParameter(const std::string& filterName, int paramId,
                                            float value) {
        auto it = m_busFilters.find(filterName);
        if (it == m_busFilters.end()) {
            // Create filter if it doesn't exist
            FilterInstance instance;
            instance.initialize(filterName);
            instance.filterName = filterName;
            instance.enabled = true;
            m_busFilters[filterName] = std::move(instance);
            it = m_busFilters.find(filterName);
        }

        auto& instance = it->second;
        auto paramIt = instance.parameters.find(paramId);
        if (paramIt != instance.parameters.end()) {
            paramIt->second.value = value;
            paramIt->second.changed = true;
            instance.needsUpdate = true;
            instance.enabled = true;

            // Apply the parameter change immediately for realtime effect
            updateFilterParameters(instance);

            // If bus is playing, apply the parameter change to the voice
            if (m_busHandle) {
                // Use SoLoud's fadeFilterParameter for smooth realtime updates
                m_engine->get().fadeFilterParameter(m_busHandle, instance.slot, paramId, value,
                                                    0.05f);
            }
        }
    }

    float AudioSystem::getBusFilterParameter(const std::string& filterName, int paramId) const {
        auto it = m_busFilters.find(filterName);
        if (it != m_busFilters.end()) {
            auto paramIt = it->second.parameters.find(paramId);
            if (paramIt != it->second.parameters.end()) {
                return paramIt->second.value;
            }
        }
        return 0.0f;
    }

    const std::unordered_map<std::string, FilterInstance>& AudioSystem::getBusFilters() const {
        return m_busFilters;
    }

    void AudioSystem::updateBusFilterParams() {
        std::cout << "Updating bus filter parameters" << std::endl;

        // Ensure bus is playing first
        if (m_busHandle == 0) {
            m_masterBus->setVolume(m_busVolume);
            m_busHandle = m_engine->get().play(m_masterBus->get());
            m_engine->get().setVolume(m_busHandle, m_busVolume);
        }

        // Remove all filters from the bus
        for (int slot = 0; slot < 8; ++slot) {
            m_masterBus->get().setFilter(slot, nullptr);
            std::cout << "Cleared bus filter slot " << slot << std::endl;
        }

        // Apply enabled filters
        int filterSlot = 0;
        for (auto& [name, instance] : m_busFilters) {
            if (instance.enabled && instance.filter && filterSlot < 8) {
                // Set the filter on the bus
                m_masterBus->get().setFilter(filterSlot, instance.filter.get());
                instance.slot = filterSlot;

                // Apply all filter parameters immediately
                updateFilterParameters(instance);

                // Apply parameters to the bus voice with a small delay to ensure bus is ready
                for (const auto& [paramId, param] : instance.parameters) {
                    m_engine->get().fadeFilterParameter(m_busHandle, filterSlot, paramId,
                                                        param.value, 0.001f);
                }

                filterSlot++;
            } else if (!instance.enabled) {
                instance.slot = -1;
            }
        }

        // Clear any remaining filter slots
        for (int slot = filterSlot; slot < 8; ++slot) {
            m_masterBus->get().setFilter(slot, nullptr);
        }
    }

    bool AudioSystem::isSupportedFileExtension(const std::string& ext) { return ext == ".ogg"; }

} // namespace AudioTester