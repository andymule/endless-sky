#include "AudioSystem.h"
#include "ErrorHandling.h"
#include <filesystem>
#include <iostream>

namespace AudioTester {

    // Time in seconds for smooth parameter transitions
    constexpr SoLoud::time FILTER_PARAM_TRANSITION_TIME = 0.05;

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

        std::cout << "Initializing AudioSystem..." << std::endl;
        if (!m_engine->initialize()) {
            std::cerr << "Failed to initialize SoLoud" << std::endl;
            return false;
        }

        m_masterBus = std::make_unique<AudioBus>(m_engine->get(), m_busVolume);

        // Play the bus once and store the handle
        m_busHandle = m_engine->get().play(m_masterBus->get());
        m_masterBus->setVolume(m_busVolume);
        m_engine->get().setVolume(m_busHandle, m_busVolume);

        m_isInitialized = true;
        std::cout << "AudioSystem initialized successfully" << std::endl;
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
            std::cout << "Cannot load file - AudioSystem not initialized" << std::endl;
            return;
        }

        std::cout << "Loading audio file: " << path << std::endl;
        auto wav = std::make_unique<SoLoud::Wav>();
        SoLoud::result result = wav->load(path.c_str());
        if (result == SoLoud::SO_NO_ERROR) {
            std::cout << "Successfully loaded: " << path << std::endl;
            m_tracks.push_back(std::move(wav));
            m_trackFilters.resize(m_tracks.size());
            // Enable looping by default for the newly loaded track
            m_tracks.back()->setLooping(true);
        } else {
            std::cout << "Failed to load: " << path << " (error: " << result << ")" << std::endl;
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
        std::cout << "[AudioSystem] Setting bus volume to: " << volume << std::endl;
        m_busVolume = volume;
        m_masterBus->setVolume(volume);
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
            std::cout << "Set filter parameter failed - invalid track or not initialized"
                      << std::endl;
            return;
        }

        std::cout << "Setting filter parameter - track: " << trackIndex << " filter: " << filterName
                  << " param: " << paramId << " value: " << value << std::endl;

        auto& trackFilters = m_trackFilters[trackIndex];
        auto it = trackFilters.filters.find(filterName);
        if (it == trackFilters.filters.end()) {
            // Initialize the filter if it doesn't exist
            std::cout << "Initializing new filter: " << filterName << std::endl;
            FilterInstance instance;
            initializeFilter(instance, filterName);
            if (instance.filter) {
                instance.enabled = true; // Enable the filter by default
                trackFilters.filters[filterName] = std::move(instance);
                it = trackFilters.filters.find(filterName);
            } else {
                std::cout << "Failed to initialize filter: " << filterName << std::endl;
                return;
            }
        }

        auto& instance = it->second;
        auto paramIt = instance.parameters.find(paramId);
        if (paramIt != instance.parameters.end()) {
            auto& param = paramIt->second;
            if (param.value != value) {
                std::cout << "Parameter value changed from " << param.value << " to " << value
                          << std::endl;

                // Get the voice handle for this track
                auto voiceIt = m_trackHandles.find(trackIndex);
                if (voiceIt != m_trackHandles.end() && instance.slot >= 0) {
                    SoLoud::handle voiceHandle = voiceIt->second;

                    // Use SoLoud's built-in parameter fading on the track's voice handle
                    m_engine->get().fadeFilterParameter(
                        voiceHandle, static_cast<unsigned int>(instance.slot),
                        static_cast<unsigned int>(paramId), value, FILTER_PARAM_TRANSITION_TIME);
                }

                // Store the new value
                param.value = value;
                param.changed = true;
                instance.needsUpdate = true;
                instance.enabled = true; // Ensure filter is enabled when parameters change

                // DON'T call updateFilterInstance for realtime changes - this causes conflicts
                // updateFilterInstance should only be used for initial setup
            }
        } else {
            std::cout << "Parameter " << paramId << " not found in filter " << filterName
                      << std::endl;
        }
    }

    void AudioSystem::initializeFilter(FilterInstance& instance, const std::string& filterName) {
        std::cout << "Initializing filter: " << filterName << std::endl;

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
            std::cout << "Filter created successfully" << std::endl;

            // Initialize parameters with their ranges based on filter type
            if (filterName == "biquad") {
                // Type, Frequency, Resonance
                instance.parameters[1] = {0.0f, 0.0f, 5.0f, "Type", false};
                instance.parameters[2] = {1000.0f, 20.0f, 20000.0f, "Frequency", false};
                instance.parameters[3] = {1.0f, 0.1f, 10.0f, "Resonance", false};
            } else if (filterName == "echo") {
                // Wet, Delay, Decay, Filter
                instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet", false};
                instance.parameters[1] = {0.3f, 0.001f, 1.0f, "Delay", false};
                instance.parameters[2] = {0.7f, 0.001f, 1.0f, "Decay", false};
                instance.parameters[3] = {0.0f, 0.0f, 0.999f, "Filter", false};
            } else if (filterName == "lofi") {
                // Wet, Sample rate
                instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet", false};
                instance.parameters[1] = {0.5f, 0.0f, 1.0f, "Sample Rate", false};
            } else if (filterName == "flanger") {
                // Wet, Delay
                instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet", false};
                instance.parameters[1] = {0.5f, 0.0f, 1.0f, "Delay", false};
            } else if (filterName == "dcremoval") {
                // Only one parameter: Length (in seconds)
                instance.parameters[0] = {0.1f, 0.01f, 10.0f, "Length", false};
            } else if (filterName == "bassboost") {
                // Boost
                instance.parameters[1] = {0.5f, 0.0f, 1.0f, "Boost", false};
            } else if (filterName == "waveshaper") {
                // Amount
                instance.parameters[1] = {0.5f, 0.0f, 1.0f, "Amount", false};
            } else if (filterName == "robotize") {
                // Wet, Frequency, Waveform
                instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet", false};
                instance.parameters[1] = {30.0f, 0.1f, 100.0f, "Frequency", false};
                instance.parameters[2] = {0.0f, 0.0f, 6.0f, "Waveform", false};
            } else if (filterName == "freeverb") {
                // Wet, Room size, Damp, Width
                instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet", false};
                instance.parameters[1] = {0.5f, 0.0f, 1.0f, "Room Size", false};
                instance.parameters[2] = {0.5f, 0.0f, 1.0f, "Damp", false};
                instance.parameters[3] = {0.5f, 0.0f, 1.0f, "Width", false};
            }

            // Apply initial parameters
            updateFilterInstance(instance, filterName);
        } else {
            std::cout << "Failed to create filter: " << filterName << std::endl;
        }
    }

    void AudioSystem::updateFilterInstance(FilterInstance& instance,
                                           const std::string& filterName) {
        if (!instance.filter) {
            std::cout << "Filter update skipped - no filter instance" << std::endl;
            return;
        }

        if (!instance.enabled) {
            std::cout << "Filter update skipped - filter disabled" << std::endl;
            return;
        }

        std::cout << "Updating filter: " << filterName << std::endl;

        // Update all changed parameters for the correct filter type
        if (filterName == "biquad") {
            auto* f = dynamic_cast<SoLoud::BiquadResonantFilter*>(instance.filter.get());
            if (f) {
                float p1 = instance.parameters[1].value; // Type
                float p2 = instance.parameters[2].value; // Frequency
                float p3 = instance.parameters[3].value; // Resonance
                std::cout << "Setting biquad params: type=" << p1 << " freq=" << p2 << " res=" << p3
                          << std::endl;
                f->setParams(static_cast<int>(p1), p2, p3);
            }
        } else if (filterName == "echo") {
            auto* f = dynamic_cast<SoLoud::EchoFilter*>(instance.filter.get());
            if (f) {
                float p1 = instance.parameters[0].value;     // Wet
                float p2 = instance.parameters[1].value;     // Delay
                float p3 = instance.parameters[2].value;     // Decay
                float p4 = instance.parameters[3].value;     // Filter
                float delay = std::max(0.001f, p2);          // Minimum 1ms delay
                float decay = std::max(0.001f, p3);          // Minimum 0.1% decay
                float filter = std::clamp(p4, 0.0f, 0.999f); // Filter between 0 and 0.999
                f->setParams(delay, decay, filter);
            }
        } else if (filterName == "lofi") {
            auto* f = dynamic_cast<SoLoud::LofiFilter*>(instance.filter.get());
            if (f) {
                float p1 = instance.parameters[0].value; // Wet
                float p2 = instance.parameters[1].value; // Sample rate
                f->setParams(p1, p2);
            }
        } else if (filterName == "flanger") {
            auto* f = dynamic_cast<SoLoud::FlangerFilter*>(instance.filter.get());
            if (f) {
                float p1 = instance.parameters[0].value; // Wet
                float p2 = instance.parameters[1].value; // Delay
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
                float p1 = instance.parameters[0].value;   // Wet
                float p2 = instance.parameters[1].value;   // Frequency
                float p3 = instance.parameters[2].value;   // Waveform
                float freq = std::clamp(p2, 0.1f, 100.0f); // Frequency between 0.1 and 100 Hz
                int wave = static_cast<int>(std::clamp(p3, 0.0f, 6.0f)); // Waveform between 0 and 6
                f->setParams(freq, wave);
            }
        } else if (filterName == "freeverb") {
            auto* f = dynamic_cast<SoLoud::FreeverbFilter*>(instance.filter.get());
            if (f) {
                float p1 = instance.parameters[0].value; // Wet
                float p2 = instance.parameters[1].value; // Room size
                float p3 = instance.parameters[2].value; // Damp
                float p4 = instance.parameters[3].value; // Width
                f->setParams(p1, p2, p3, p4);
            }
        }

        // Mark all parameters as not changed
        for (auto& [paramId, param] : instance.parameters) {
            param.changed = false;
            std::cout << "Parameter " << paramId << " marked as unchanged" << std::endl;
        }
        instance.needsUpdate = false;
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

        // No need for additional parameter setting - filters are already properly initialized
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
                instance.filterName = filterName;
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
            initializeFilter(instance, filterName);
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
            updateFilterInstance(instance, filterName);

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
                updateFilterInstance(instance, name);

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