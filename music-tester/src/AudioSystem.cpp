#include "AudioSystem.h"
#include <iostream>

namespace AudioTester {

    // Time in seconds for smooth parameter transitions
    constexpr SoLoud::time FILTER_PARAM_TRANSITION_TIME = 0.05;

    // Filter parameter constants
    constexpr float MIN_DELAY = 0.001f;           // Minimum 1ms delay
    constexpr float MIN_DECAY = 0.001f;           // Minimum 0.1% decay
    constexpr float MAX_FILTER_VALUE = 0.999f;    // Maximum filter value
    constexpr float MIN_FREQUENCY = 0.1f;         // Minimum frequency in Hz
    constexpr float MAX_FREQUENCY = 100.0f;       // Maximum frequency in Hz
    constexpr float BUS_PARAM_FADE_TIME = 0.001f; // Fast fade for bus parameters

    const std::vector<std::string> AudioSystem::AVAILABLE_FILTERS = {
        "biquad",    "echo",       "lofi",     "flanger", "dcremoval",
        "bassboost", "waveshaper", "robotize", "freeverb"};

    AudioSystem::AudioSystem() {
        m_engine = std::make_unique<SoloudEngine>();
        m_isInitialized = false;
    }

    AudioSystem::~AudioSystem() {
        if (m_isInitialized) {
            cleanup();
        }
    }

    bool AudioSystem::initialize() {
        if (m_isInitialized)
            return true;

        if (!m_engine->initialize()) {
            std::cerr << "Failed to initialize SoLoud" << std::endl;
            return false;
        }

        m_masterBus = std::make_unique<AudioBus>(m_engine->get(), m_busVolume);

        // Play the bus once and store the handle
        m_busHandle = m_engine->get().play(m_masterBus->get());
        m_engine->get().setVolume(m_busHandle, m_busVolume);

        m_isInitialized = true;
        return true;
    }

    void AudioSystem::cleanup() {
        if (m_isInitialized) {
            m_engine->get().stopAll();
            m_engine->deinitialize();
            m_isInitialized = false;
        }
    }

    void AudioSystem::loadAudioFile(const std::string& path) {
        if (!m_isInitialized) {
            std::cerr << "Cannot load file - AudioSystem not initialized" << std::endl;
            return;
        }
        auto wav = std::make_unique<SoLoud::Wav>();
        SoLoud::result result = wav->load(path.c_str());
        if (result == SoLoud::SO_NO_ERROR) {
            m_tracks.push_back(std::move(wav));
            m_trackFilters.resize(m_tracks.size());
            // Enable looping by default for the newly loaded track
            m_tracks.back()->setLooping(true);
        } else {
            std::cerr << "Failed to load: " << path << " (error: " << result << ")" << std::endl;
        }
    }

    void AudioSystem::playTrack(size_t index) {
        if (!m_isInitialized || index >= m_tracks.size())
            return;

        // Stop the track if it's already playing
        stopTrack(index);

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
        m_busVolume = volume;
        if (m_busHandle)
            m_engine->get().setVolume(m_busHandle, volume);
    }

    float AudioSystem::getBusVolume() const { return m_busVolume; }

    void AudioSystem::addFilterToTrack(size_t trackIndex, const std::string& filterName) {
        if (!m_isInitialized || trackIndex >= m_trackFilters.size())
            return;

        // Check if filter already exists
        auto& trackFilters = m_trackFilters[trackIndex];
        if (trackFilters.filters.find(filterName) != trackFilters.filters.end())
            return;

        // Create new filter
        FilterInstance instance;
        initializeFilter(instance, filterName);
        if (instance.filter) {
            instance.enabled = true;
            trackFilters.filters[filterName] = std::move(instance);
            applyFiltersToTrack(trackIndex);
        }
    }

    void AudioSystem::removeFilterFromTrack(size_t trackIndex, const std::string& filterName) {
        if (!m_isInitialized || trackIndex >= m_trackFilters.size())
            return;

        auto& trackFilters = m_trackFilters[trackIndex];
        auto it = trackFilters.filters.find(filterName);
        if (it != trackFilters.filters.end()) {
            trackFilters.filters.erase(it);
            applyFiltersToTrack(trackIndex);
        }
    }

    void AudioSystem::setFilterParameter(size_t trackIndex, const std::string& filterName,
                                         int paramId, float value) {
        if (!m_isInitialized || trackIndex >= m_trackFilters.size()) {
            return;
        }

        auto& trackFilters = m_trackFilters[trackIndex];
        auto it = trackFilters.filters.find(filterName);
        if (it == trackFilters.filters.end()) {
            // Initialize the filter if it doesn't exist
            FilterInstance instance;
            initializeFilter(instance, filterName);
            if (instance.filter) {
                instance.enabled = true; // Enable the filter by default
                trackFilters.filters[filterName] = std::move(instance);
                it = trackFilters.filters.find(filterName);
            } else {
                return;
            }
        }

        auto& instance = it->second;
        auto paramIt = instance.parameters.find(paramId);
        if (paramIt != instance.parameters.end()) {
            auto& param = paramIt->second;
            if (param.value != value) {
                // Get the voice handle for this track
                auto voiceIt = m_trackHandles.find(trackIndex);
                if (voiceIt != m_trackHandles.end() && instance.slot >= 0) {
                    SoLoud::handle voiceHandle = voiceIt->second;

                    // Apply parameter change using SoLoud's built-in parameter fading
                    // This ensures each filter instance gets its own parameter value
                    m_engine->get().fadeFilterParameter(
                        voiceHandle, static_cast<unsigned int>(instance.slot),
                        static_cast<unsigned int>(paramId), value, FILTER_PARAM_TRANSITION_TIME);
                }

                // Store the new value
                param.value = value;
                instance.enabled = true; // Ensure filter is enabled when parameters change

                // DON'T call updateFilterInstance for realtime changes - this causes conflicts
                // updateFilterInstance should only be used for initial setup
            }
        }
    }

    void AudioSystem::initializeFilter(FilterInstance& instance, const std::string& filterName) {

        if (filterName == "biquad")
            instance.filter = std::make_unique<SoLoud::BiquadResonantFilter>();
        else if (filterName == "echo")
            instance.filter = std::make_unique<SoLoud::EchoFilter>();
        else if (filterName == "lofi")
            instance.filter = std::make_unique<SoLoud::LofiFilter>();
        else if (filterName == "flanger")
            instance.filter = std::make_unique<SoLoud::FlangerFilter>();
        else if (filterName == "dcremoval")
            instance.filter = std::make_unique<SoLoud::DCRemovalFilter>();
        else if (filterName == "bassboost")
            instance.filter = std::make_unique<SoLoud::BassboostFilter>();
        else if (filterName == "waveshaper")
            instance.filter = std::make_unique<SoLoud::WaveShaperFilter>();
        else if (filterName == "robotize")
            instance.filter = std::make_unique<SoLoud::RobotizeFilter>();
        else if (filterName == "freeverb")
            instance.filter = std::make_unique<SoLoud::FreeverbFilter>();

        if (instance.filter) {

            // Initialize parameters with their ranges based on filter type
            if (filterName == "biquad") {
                // WET, Type, Frequency, Resonance
                instance.parameters[0] = {1.0f, 0.0f, 1.0f, "Wet Mix"};
                instance.parameters[1] = {0.0f, 0.0f, 2.0f, "Filter Type"};
                instance.parameters[2] = {1000.0f, 20.0f, 8000.0f, "Frequency (Hz)"};
                instance.parameters[3] = {2.0f, 0.1f, 10.0f, "Resonance"};
            } else if (filterName == "echo") {
                // WET, Delay, Decay, Filter
                instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet Mix"};
                instance.parameters[1] = {0.3f, MIN_DELAY, 1.0f, "Delay (s)"};
                instance.parameters[2] = {0.7f, MIN_DECAY, 1.0f, "Decay"};
                instance.parameters[3] = {0.0f, 0.0f, MAX_FILTER_VALUE, "Filter"};
            } else if (filterName == "lofi") {
                // WET, Sample rate, Bit depth
                instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet Mix"};
                instance.parameters[1] = {0.5f, 0.0f, 1.0f, "Sample Rate"};
                instance.parameters[2] = {0.5f, 0.0f, 1.0f, "Bit Depth"};
            } else if (filterName == "flanger") {
                // WET, Delay, Freq
                instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet Mix"};
                instance.parameters[1] = {0.005f, 0.001f, 0.1f, "Delay (s)"};
                instance.parameters[2] = {10.0f, 0.1f, 100.0f, "Frequency (Hz)"};
            } else if (filterName == "dcremoval") {
                // Only one parameter: Length (in seconds) - no WET parameter for this filter
                instance.parameters[0] = {0.1f, 0.01f, 10.0f, "Length (s)"};
            } else if (filterName == "bassboost") {
                // WET, Boost
                instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet Mix"};
                instance.parameters[1] = {0.5f, 0.0f, 1.0f, "Boost"};
            } else if (filterName == "waveshaper") {
                // WET, Amount
                instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet Mix"};
                instance.parameters[1] = {0.5f, -1.0f, 1.0f, "Distortion"};
            } else if (filterName == "robotize") {
                // WET, Frequency, Waveform
                instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet Mix"};
                instance.parameters[1] = {30.0f, MIN_FREQUENCY, MAX_FREQUENCY, "Frequency (Hz)"};
                instance.parameters[2] = {0.0f, 0.0f, 6.0f, "Waveform"};
            } else if (filterName == "freeverb") {
                // WET, Freeze, Room size, Damp, Width
                instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet Mix"};
                instance.parameters[1] = {0.0f, 0.0f, 1.0f, "Freeze"};
                instance.parameters[2] = {0.5f, 0.0f, 1.0f, "Room Size"};
                instance.parameters[3] = {0.5f, 0.0f, 1.0f, "Damping"};
                instance.parameters[4] = {0.5f, 0.0f, 1.0f, "Width"};
            }

            // Apply initial parameters
            updateFilterInstance(instance, filterName);
        }
    }

    void AudioSystem::updateFilterInstance(FilterInstance& instance,
                                           const std::string& filterName) {
        if (!instance.filter || !instance.enabled) {
            return;
        }

        // Update all changed parameters for the correct filter type
        if (filterName == "biquad") {
            auto* f = dynamic_cast<SoLoud::BiquadResonantFilter*>(instance.filter.get());
            if (f) {
                float p1 = instance.parameters[1].value; // Type
                float p2 = instance.parameters[2].value; // Frequency
                float p3 = instance.parameters[3].value; // Resonance
                f->setParams(static_cast<int>(p1), p2, p3);
            }
        } else if (filterName == "echo") {
            auto* f = dynamic_cast<SoLoud::EchoFilter*>(instance.filter.get());
            if (f) {
                float p1 = instance.parameters[1].value;               // Delay
                float p2 = instance.parameters[2].value;               // Decay
                float p3 = instance.parameters[3].value;               // Filter
                float delay = std::max(MIN_DELAY, p1);                 // Minimum 1ms delay
                float decay = std::max(MIN_DECAY, p2);                 // Minimum 0.1% decay
                float filter = std::clamp(p3, 0.0f, MAX_FILTER_VALUE); // Filter between 0 and 0.999
                f->setParams(delay, decay, filter);
            }
        } else if (filterName == "lofi") {
            auto* f = dynamic_cast<SoLoud::LofiFilter*>(instance.filter.get());
            if (f) {
                float p1 = instance.parameters[1].value; // Sample rate
                float p2 = instance.parameters[2].value; // Bit depth
                f->setParams(p1, p2);
            }
        } else if (filterName == "flanger") {
            auto* f = dynamic_cast<SoLoud::FlangerFilter*>(instance.filter.get());
            if (f) {
                float p1 = instance.parameters[1].value; // Delay
                float p2 = instance.parameters[2].value; // Frequency
                f->setParams(p1, p2);
            }
        } else if (filterName == "dcremoval") {
            auto* f = dynamic_cast<SoLoud::DCRemovalFilter*>(instance.filter.get());
            if (f) {
                float length = instance.parameters[0].value;
                f->setParams(length);
            }
        } else if (filterName == "bassboost") {
            auto* f = dynamic_cast<SoLoud::BassboostFilter*>(instance.filter.get());
            if (f) {
                float p1 = instance.parameters[1].value; // Boost
                f->setParams(p1);
            }
        } else if (filterName == "waveshaper") {
            auto* f = dynamic_cast<SoLoud::WaveShaperFilter*>(instance.filter.get());
            if (f) {
                float p1 = instance.parameters[1].value; // Amount
                f->setParams(p1);
            }
        } else if (filterName == "robotize") {
            auto* f = dynamic_cast<SoLoud::RobotizeFilter*>(instance.filter.get());
            if (f) {
                float p1 = instance.parameters[1].value; // Frequency
                float p2 = instance.parameters[2].value; // Waveform
                float freq = std::clamp(p1, MIN_FREQUENCY,
                                        MAX_FREQUENCY); // Frequency between 0.1 and 100 Hz
                int wave = static_cast<int>(std::clamp(p2, 0.0f, 6.0f)); // Waveform between 0 and 6
                f->setParams(freq, wave);
            }
        } else if (filterName == "freeverb") {
            auto* f = dynamic_cast<SoLoud::FreeverbFilter*>(instance.filter.get());
            if (f) {
                float p1 = instance.parameters[1].value; // Freeze
                float p2 = instance.parameters[2].value; // Room size
                float p3 = instance.parameters[3].value; // Damp
                float p4 = instance.parameters[4].value; // Width
                f->setParams(p1, p2, p3, p4);
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
            for (auto& [name, instance] : m_trackFilters[trackIndex].filters) {
                if (instance.enabled && instance.filter && filterSlot < 8) {
                    m_tracks[trackIndex]->setFilter(filterSlot, instance.filter.get());
                    // Set initial parameters immediately to ensure clean state
                    updateFilterInstance(instance, name);
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

        // Apply enabled filters to the track source with clean initialization
        int filterSlot = 0;
        for (auto& [name, instance] : m_trackFilters[trackIndex].filters) {
            if (instance.enabled && instance.filter && filterSlot < 8) {
                // Don't re-initialize the filter unless it's truly broken
                // Just ensure parameters are correctly applied
                m_tracks[trackIndex]->setFilter(filterSlot, instance.filter.get());
                updateFilterInstance(instance, name);
                instance.slot = filterSlot;
                filterSlot++;
            }
        }

        // Restart the track with clean filters
        unsigned int newHandle = m_masterBus->get().play(*m_tracks[trackIndex]);
        m_trackHandles[trackIndex] = newHandle;
        m_engine->get().setVolume(newHandle, volume);
        m_engine->get().seek(newHandle, position);

        // Apply all parameters to all filters after the track is restarted
        filterSlot = 0;
        for (auto& [name, instance] : m_trackFilters[trackIndex].filters) {
            if (instance.enabled && instance.filter && filterSlot < 8) {
                // Apply all parameters for this filter
                for (const auto& [paramId, param] : instance.parameters) {
                    m_engine->get().fadeFilterParameter(newHandle, filterSlot, paramId, param.value,
                                                        FILTER_PARAM_TRANSITION_TIME);
                }
                filterSlot++;
            }
        }
    }

    bool AudioSystem::isFilterEnabled(size_t trackIndex, const std::string& filterName) const {
        if (!m_isInitialized || trackIndex >= m_trackFilters.size())
            return false;

        auto it = m_trackFilters[trackIndex].filters.find(filterName);
        return it != m_trackFilters[trackIndex].filters.end() && it->second.enabled;
    }

    float AudioSystem::getFilterParameter(size_t trackIndex, const std::string& filterName,
                                          int paramId) const {
        if (!m_isInitialized || trackIndex >= m_trackFilters.size())
            return 0.0f;

        auto it = m_trackFilters[trackIndex].filters.find(filterName);
        if (it != m_trackFilters[trackIndex].filters.end() && it->second.enabled) {
            auto paramIt = it->second.parameters.find(paramId);
            if (paramIt != it->second.parameters.end()) {
                return paramIt->second.value;
            }
        }
        return 0.0f;
    }

    void AudioSystem::setFilterEnabled(size_t trackIndex, const std::string& filterName,
                                       bool enabled) {
        if (!m_isInitialized || trackIndex >= m_trackFilters.size())
            return;

        auto& trackFilters = m_trackFilters[trackIndex];
        auto it = trackFilters.filters.find(filterName);
        if (it == trackFilters.filters.end()) {
            if (enabled) {
                // Create new filter instance
                FilterInstance instance;
                initializeFilter(instance, filterName);
                instance.enabled = true;
                trackFilters.filters[filterName] = std::move(instance);
                applyFiltersToTrack(trackIndex);
            }
        } else {
            it->second.enabled = enabled;
            applyFiltersToTrack(trackIndex);
        }
    }

    const std::unordered_map<std::string, FilterInstance>&
    AudioSystem::getFilters(size_t trackIndex) const {
        static const std::unordered_map<std::string, FilterInstance> empty;
        if (!m_isInitialized || trackIndex >= m_trackFilters.size())
            return empty;
        return m_trackFilters[trackIndex].filters;
    }

    // Bus filter methods
    void AudioSystem::setBusFilterEnabled(const std::string& filterName, bool enabled) {
        auto it = m_busFilters.find(filterName);
        if (it == m_busFilters.end()) {
            if (enabled) {
                FilterInstance instance;
                initializeFilter(instance, filterName);
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
            initializeFilter(instance, filterName);
            instance.enabled = true;
            m_busFilters[filterName] = std::move(instance);
            it = m_busFilters.find(filterName);
        }

        auto& instance = it->second;
        auto paramIt = instance.parameters.find(paramId);
        if (paramIt != instance.parameters.end()) {
            paramIt->second.value = value;
            instance.enabled = true;

            // Apply the parameter change immediately for realtime effect
            updateFilterInstance(instance, filterName);

            // If bus is playing, apply the parameter change to the voice
            if (m_busHandle) {
                // Apply parameter change using SoLoud's built-in parameter fading
                m_engine->get().fadeFilterParameter(m_busHandle, instance.slot, paramId, value,
                                                    FILTER_PARAM_TRANSITION_TIME);
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

        // Ensure bus is playing first
        if (m_busHandle == 0) {
            m_masterBus->setVolume(m_busVolume);
            m_busHandle = m_engine->get().play(m_masterBus->get());
            m_engine->get().setVolume(m_busHandle, m_busVolume);
        }

        // Remove all filters from the bus
        for (int slot = 0; slot < 8; ++slot) {
            m_masterBus->get().setFilter(slot, nullptr);
        }

        // Apply enabled filters
        int filterSlot = 0;
        for (auto& [name, instance] : m_busFilters) {
            if (instance.enabled && instance.filter && filterSlot < 8) {
                // Set the filter on the bus
                m_masterBus->get().setFilter(filterSlot, instance.filter.get());
                instance.slot = filterSlot;

                // Apply all filter parameters immediately
                updateFilterInstance(instance, name);

                // Apply parameters to the bus voice with a small delay to ensure bus is ready
                for (const auto& [paramId, param] : instance.parameters) {
                    m_engine->get().fadeFilterParameter(m_busHandle, filterSlot, paramId,
                                                        param.value, BUS_PARAM_FADE_TIME);
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