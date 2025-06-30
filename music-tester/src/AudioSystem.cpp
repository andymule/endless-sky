#include "AudioSystem.h"
#include "Logger.h"
#include "TrackManager.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <vector>

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
            LOG_ERROR("Failed to initialize SoLoud");
            return false;
        }

        m_masterBus = std::make_unique<AudioBus>(m_engine->get(), m_busVolume);

        // Play the bus once and store the handle
        m_busHandle = m_engine->get().play(m_masterBus->get());
        m_engine->get().setVolume(m_busHandle, m_busVolume);

        // Initialize granular tempo processor
        m_granularProcessor = std::make_unique<AudioStreamProcessor>();
        if (!m_granularProcessor->initialize(44100, 2, 3.0, 2.0)) {
            LOG_ERROR("Failed to initialize granular processor in AudioSystem");
            // Don't fail completely - granular tempo just won't be available
        } else if (!m_granularProcessor->start()) {
            LOG_ERROR("Failed to start granular processor in AudioSystem");
            // Don't fail completely - granular tempo just won't be available
        }

        // Create granular intercept filter
        m_granularFilter = std::make_unique<GranularInterceptFilter>(m_granularProcessor.get(),
                                                                     &m_granularEnabled);

        // Master bus is ready

        m_isInitialized = true;
        return true;
    }

    void AudioSystem::cleanup() {
        if (m_isInitialized) {
            m_engine->get().stopAll();
            if (m_granularProcessor) {
                m_granularProcessor->stop();
            }
            m_engine->deinitialize();
            m_isInitialized = false;
        }
    }

    void AudioSystem::loadTrack(const std::string& path) {
        if (!m_isInitialized) {
            return;
        }

        std::filesystem::path filePath(path);
        std::string extension = filePath.extension().string();

        // More efficient case-insensitive comparison for .ogg extension
        bool isSupported = false;
        if (extension.length() == 4 &&
            (extension[0] == '.' || extension[0] == 'O' || extension[0] == 'o') &&
            (extension[1] == 'o' || extension[1] == 'O') &&
            (extension[2] == 'g' || extension[2] == 'G') &&
            (extension[3] == 'g' || extension[3] == 'G')) {
            isSupported = true;
        }

        if (!isSupported) {
            LOG_ERROR("Unsupported file format: " + extension);
            return;
        }

        TrackManager::TrackInfo trackInfo;

        // Use SoLoud's built-in loading (which already loads OGG into RAM)
        trackInfo.wav = std::make_unique<SyncWav>();
        SoLoud::result result = trackInfo.wav->load(path.c_str());
        if (result != SoLoud::SO_NO_ERROR) {
            LOG_ERROR("Failed to load: " + path + " (error: " + std::to_string(result) + ")");
            return;
        }

        trackInfo.duration = trackInfo.wav->getLength();
        trackInfo.wav->setLooping(true);
        LOG_INFO("Loaded: " + path + " (duration: " + std::to_string(trackInfo.duration) + "s)");

        m_trackManager.addTrack(std::move(trackInfo));
        m_trackFilters.resize(m_trackManager.getTrackCount());
        calculateMasterDuration();
    }

    void AudioSystem::playTrack(size_t index) {
        if (!m_isInitialized || index >= m_trackManager.getTrackCount())
            return;

        // Stop the track if it's already playing
        stopTrack(index);

        // Play the track through the bus
        unsigned int handle = m_masterBus->get().play(*m_trackManager.getTrack(index).wav);
        m_trackManager.getTrack(index).handle = handle;
        m_trackManager.getTrack(index).isPlaying = true;

        // Apply current global playback rate
        if (m_globalPlaybackRate != 1.0f) {
            m_engine->get().setRelativePlaySpeed(handle, m_globalPlaybackRate);
        }
    }

    void AudioSystem::stopTrack(size_t index) {
        if (!m_isInitialized || index >= m_trackManager.getTrackCount())
            return;

        if (m_trackManager.getTrack(index).isPlaying &&
            m_trackManager.getTrack(index).handle != 0) {
            m_engine->get().stop(m_trackManager.getTrack(index).handle);
            m_trackManager.getTrack(index).handle = 0;
            m_trackManager.getTrack(index).isPlaying = false;
        }
    }

    void AudioSystem::pauseTrack(size_t index) {
        if (!m_isInitialized || index >= m_trackManager.getTrackCount())
            return;

        if (m_trackManager.getTrack(index).isPlaying &&
            m_trackManager.getTrack(index).handle != 0) {
            m_engine->get().setPause(m_trackManager.getTrack(index).handle, true);
            m_trackManager.getTrack(index).isPaused = true;
        }
    }

    void AudioSystem::resumeTrack(size_t index) {
        if (!m_isInitialized || index >= m_trackManager.getTrackCount())
            return;

        if (m_trackManager.getTrack(index).isPaused && m_trackManager.getTrack(index).handle != 0) {
            m_engine->get().setPause(m_trackManager.getTrack(index).handle, false);
            m_trackManager.getTrack(index).isPaused = false;
        }
    }

    void AudioSystem::setTrackVolume(size_t index, float volume) {
        if (!m_isInitialized || index >= m_trackManager.getTrackCount())
            return;

        // Store volume in TrackInfo (single source of truth)
        m_trackManager.getTrack(index).volume = volume;

        if (m_trackManager.getTrack(index).isPlaying &&
            m_trackManager.getTrack(index).handle != 0) {
            m_engine->get().setVolume(m_trackManager.getTrack(index).handle, volume);
        }
    }

    float AudioSystem::getTrackVolume(size_t index) const {
        if (!m_isInitialized || index >= m_trackManager.getTrackCount())
            return 1.0f; // Default volume if track doesn't exist

        return m_trackManager.getTrack(index).volume; // Return from single source of truth
    }

    void AudioSystem::setTrackLooping(size_t index, bool looping) {
        if (!m_isInitialized || index >= m_trackManager.getTrackCount())
            return;

        m_trackManager.getTrack(index).wav->setLooping(looping);
    }

    void AudioSystem::removeTrack(size_t index) {
        if (!m_isInitialized || index >= m_trackManager.getTrackCount())
            return;

        // Stop the track if it's playing
        if (m_trackManager.getTrack(index).isPlaying &&
            m_trackManager.getTrack(index).handle != 0) {
            m_engine->get().stop(m_trackManager.getTrack(index).handle);
        }

        // Remove the track from the tracks vector
        m_trackManager.removeTrack(index);

        // Remove the corresponding filter data
        if (index < m_trackFilters.size()) {
            m_trackFilters.erase(m_trackFilters.begin() + index);
        }

        // Recalculate master duration since track count changed
        calculateMasterDuration();

        LOG_INFO("Removed track " + std::to_string(index) + " from audio system");
    }

    void AudioSystem::setBusVolume(float volume) {
        m_busVolume = volume;
        if (m_busHandle)
            m_engine->get().setVolume(m_busHandle, volume);
    }

    float AudioSystem::getBusVolume() const { return m_busVolume; }

    void AudioSystem::setGlobalPlaybackRate(float rate) {
        if (!m_isInitialized) {
            return;
        }

        // Store user tape speed for dual tape speed calculation
        m_userTapeSpeed = rate;

        // Update the dual tape speed system
        updateDualTapeSpeed();
    }

    // Granular tempo control (pitch-preserving)
    void AudioSystem::setGranularTempo(float tempo) {
        // Store granular tempo for dual tape speed calculation
        m_granularTempo = tempo;

        // Update the dual tape speed system
        updateDualTapeSpeed();

        // Auto-enable granular processing when tempo != 1.0
        bool shouldEnable = std::abs(tempo - 1.0f) > 0.001f;
        setGranularEnabled(shouldEnable);
    }

    float AudioSystem::getGranularTempo() const {
        // Return the stored granular tempo value, not the processor's tempo
        return m_granularTempo;
    }

    float AudioSystem::getGranularLatencyMs() const {
        if (m_granularProcessor) {
            return m_granularProcessor->getLatencyMs();
        }
        return 0.0f;
    }

    bool AudioSystem::isGranularProcessorReady() const {
        return m_granularProcessor && m_granularProcessor->isReady();
    }

    void AudioSystem::setGranularEnabled(bool enabled) {
        if (!m_granularProcessor || !m_granularProcessor->isReady() || !m_granularFilter) {
            return;
        }

        if (enabled != m_granularEnabled) {
            m_granularEnabled = enabled;

            if (enabled) {
                // Apply granular filter to master bus to intercept audio
                m_masterBus->get().setFilter(0, m_granularFilter.get());
                LOG_INFO("Granular tempo processing enabled - filter applied to master bus");
            } else {
                // Remove granular filter from master bus
                m_masterBus->get().setFilter(0, nullptr);
                LOG_INFO("Granular tempo processing disabled - filter removed from master bus");
            }
        }
    }

    bool AudioSystem::isGranularEnabled() const { return m_granularEnabled; }

    void AudioSystem::processMasterOutput(float* buffer, unsigned int samples,
                                          unsigned int channels) {
        // This method will be called from updateSync() for now
        // In the future, this could be hooked into SoLoud's audio pipeline more directly
        if (!m_granularProcessor || !m_granularProcessor->isReady() || !m_granularEnabled) {
            return;
        }

        // Skip processing if granular tempo is 1.0 (no change needed)
        if (std::abs(m_granularTempo - 1.0f) < 0.001f) {
            return;
        }

        const size_t totalSamples = samples * channels;

        // Ensure capture buffer is large enough
        if (m_captureBuffer.size() < totalSamples) {
            m_captureBuffer.resize(totalSamples);
        }

        // Copy input audio to our buffer
        std::memcpy(m_captureBuffer.data(), buffer, totalSamples * sizeof(float));

        // Feed audio to granular processor
        size_t samplesWritten =
            m_granularProcessor->feedInput(m_captureBuffer.data(), totalSamples);

        // Try to read processed audio
        size_t processedSamples = m_granularProcessor->readOutput(buffer, totalSamples);

        if (processedSamples < totalSamples) {
            // Not enough processed audio available - fill remaining with silence or original
            // This can happen during startup or tempo changes
            if (processedSamples > 0) {
                // Fill remaining with silence to avoid artifacts
                std::memset(buffer + processedSamples, 0,
                            (totalSamples - processedSamples) * sizeof(float));
            }
            // If no processed audio available, keep original (buffer already contains it)
        }
    }

    void AudioSystem::testGranularProcessing() {
        // Simple test: generate sine wave audio and process it through granular processor
        // This tests that the basic processing pipeline works
        const size_t testSamples = 512; // Small test buffer
        const size_t channels = 2;
        const size_t totalSamples = testSamples * channels;

        // Ensure test buffer exists
        static std::vector<float> testBuffer(totalSamples);
        static size_t sampleCount = 0;

        // Generate simple sine wave test audio (very quiet)
        const float frequency = 440.0f; // A4 note
        const float sampleRate = 44100.0f;
        const float amplitude = 0.01f; // Very quiet for testing

        for (size_t i = 0; i < testSamples; ++i) {
            float value = amplitude * std::sin(2.0f * M_PI * frequency * sampleCount / sampleRate);
            testBuffer[i * channels] = value;     // Left channel
            testBuffer[i * channels + 1] = value; // Right channel
            sampleCount++;
        }

        // Feed test audio to granular processor
        size_t fed = m_granularProcessor->feedInput(testBuffer.data(), totalSamples);

        // Try to read processed audio (discard for now - just testing the flow)
        std::vector<float> outputBuffer(totalSamples);
        size_t read = m_granularProcessor->readOutput(outputBuffer.data(), totalSamples);

        // Optional: print debug info occasionally
        static int debugCounter = 0;
        if (++debugCounter % 1000 == 0) { // Every ~23 seconds at 44.1kHz
            LOG_INFO("Granular test: fed " + std::to_string(fed) + " samples, read " +
                     std::to_string(read) + ", granularTempo " + std::to_string(m_granularTempo));
        }
    }

    // Granular Intercept Filter Implementation
    GranularInterceptFilter::GranularInterceptFilter(AudioStreamProcessor* processor,
                                                     bool* enabledFlag)
        : m_processor(processor), m_enabledFlag(enabledFlag) {}

    SoLoud::FilterInstance* GranularInterceptFilter::createInstance() {
        return new GranularInterceptFilterInstance(m_processor, m_enabledFlag);
    }

    GranularInterceptFilterInstance::GranularInterceptFilterInstance(
        AudioStreamProcessor* processor, bool* enabledFlag)
        : m_processor(processor), m_enabledFlag(enabledFlag), m_lastPitchCompensation(1.0f) {
        // No additional initialization needed for simplified approach
    }

    void GranularInterceptFilterInstance::filterChannel(float* aBuffer, unsigned int aSamples,
                                                        float aSamplerate, double aTime,
                                                        unsigned int aChannel,
                                                        unsigned int aChannels) {
        // Safety checks
        if (!m_processor || !m_enabledFlag || !*m_enabledFlag) {
            return; // Pass through unchanged
        }

        // Skip processing if granular tempo is 1.0 (no pitch compensation needed)
        float pitchCompensation = m_processor->getPitchCompensation();
        if (std::abs(pitchCompensation - 1.0f) < 0.001f) {
            return; // Pass through unchanged - no pitch compensation needed
        }

        // Only process stereo audio (2 channels)
        if (aChannels != 2) {
            return; // Pass through unchanged for non-stereo audio
        }

        // Process each channel independently with identical settings
        // This ensures both left and right channels get exactly the same processing
        // The stereo-configured Signalsmith processor will handle each channel correctly

        // Simple approach: process this channel directly with the mono method
        // The AudioStreamProcessor uses a stereo-configured processor internally
        // which ensures consistent processing across channels
        bool success = m_processor->processPitchCompensation(aBuffer, aBuffer, aSamples);

        // If processing failed, aBuffer remains unchanged (pass-through)
        if (!success) {
            // Already logged in processPitchCompensation, just pass through
        }
    }

    void AudioSystem::addFilterToTrack(size_t trackIndex, const std::string& filterName) {
        if (!m_isInitialized || trackIndex >= m_trackFilters.size())
            return;

        // Check if filter already exists
        auto& trackFilters = m_trackFilters[trackIndex];
        if (trackFilters.filters.find(filterName) != trackFilters.filters.end())
            return;

        // Find the next available position
        int nextPosition = 0;
        for (const auto& [name, instance] : trackFilters.filters) {
            if (instance.position >= nextPosition) {
                nextPosition = instance.position + 1;
            }
        }

        // Create new filter
        FilterInstance instance;
        initializeFilter(instance, filterName);
        if (instance.filter) {
            instance.enabled = true;
            instance.position = nextPosition;
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
            // Find the next available position
            int nextPosition = 0;
            for (const auto& [name, instance] : trackFilters.filters) {
                if (instance.position >= nextPosition) {
                    nextPosition = instance.position + 1;
                }
            }

            FilterInstance instance;
            initializeFilter(instance, filterName);
            // Always start with wet = 0.0 for new filters
            int wetId = getWetParameterId(filterName);
            if (wetId >= 0) {
                instance.parameters[wetId].value = 0.0f;
            }
            instance.enabled = false; // Not enabled until wet > 0
            instance.position = nextPosition;
            trackFilters.filters[filterName] = std::move(instance);
            it = trackFilters.filters.find(filterName);
        }

        auto& instance = it->second;
        auto paramIt = instance.parameters.find(paramId);
        if (paramIt != instance.parameters.end()) {
            auto& param = paramIt->second;
            if (param.value != value) {
                // Auto enable/disable logic based on wet parameter
                bool wasEnabled = instance.enabled;
                if (isWetParameter(filterName, paramId)) {
                    instance.enabled = (value > 0.0f); // Auto enable/disable based on wet > 0
                } else if (!instance.enabled && value != param.value) {
                    // If setting non-wet parameter and filter is disabled, auto-enable if wet > 0
                    auto wetParamIt = instance.parameters.find(getWetParameterId(filterName));
                    if (wetParamIt != instance.parameters.end() &&
                        wetParamIt->second.value > 0.0f) {
                        instance.enabled = true;
                    }
                }

                // Apply parameter change
                if (trackIndex < m_trackManager.getTrackCount() &&
                    m_trackManager.getTrack(trackIndex).isPlaying && instance.slot >= 0 &&
                    instance.enabled) {
                    SoLoud::handle voiceHandle = m_trackManager.getTrack(trackIndex).handle;

                    // For problematic filters (freeverb, robotize, lofi, flanger, bassboost), use
                    // immediate parameter setting for WET parameter to override the broken
                    // initialization in SoLoud
                    if ((filterName == "freeverb" || filterName == "robotize" ||
                         filterName == "lofi" || filterName == "flanger") &&
                        paramId == 0) {
                        // Apply WET parameter immediately without fade for these filters
                        m_engine->get().setFilterParameter(
                            voiceHandle, static_cast<unsigned int>(instance.slot),
                            static_cast<unsigned int>(paramId), value);
                    } else if (filterName == "freeverb") {
                        // For Freeverb, apply ALL parameters immediately to force the Revmodel to
                        // update This is needed because FreeverbFilterInstance constructor doesn't
                        // apply parameters to Revmodel
                        m_engine->get().setFilterParameter(
                            voiceHandle, static_cast<unsigned int>(instance.slot),
                            static_cast<unsigned int>(paramId), value);
                    } else {
                        // Use normal fade for other parameters
                        m_engine->get().fadeFilterParameter(
                            voiceHandle, static_cast<unsigned int>(instance.slot),
                            static_cast<unsigned int>(paramId), value,
                            FILTER_PARAM_TRANSITION_TIME);
                    }
                }

                // Store the new value
                param.value = value;

                // If enabled state changed, need to reapply filters
                if (wasEnabled != instance.enabled) {
                    applyFiltersToTrack(trackIndex);
                }

                // For robotize, we need to call setParams() on the filter instance for Frequency
                // and Waveform parameters to ensure real-time updates work correctly
                if (filterName == "robotize" && (paramId == 1 || paramId == 2)) {
                    updateFilterInstance(instance, filterName);

                    // Force track restart to apply the new parameters to the actual SoLoud filter
                    // instance
                    if (trackIndex < m_trackManager.getTrackCount() &&
                        m_trackManager.getTrack(trackIndex).isPlaying) {
                        applyFiltersToTrack(trackIndex);
                    }
                }

                // For echo filter, we need to call setParams() on the filter instance for Delay
                // and Decay parameters to ensure real-time updates work correctly
                if (filterName == "echo" && (paramId == 1 || paramId == 2)) {
                    updateFilterInstance(instance, filterName);
                }
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
        else if (filterName == "waveshaper")
            instance.filter = std::make_unique<SoLoud::WaveShaperFilter>();
        else if (filterName == "robotize")
            instance.filter = std::make_unique<SoLoud::RobotizeFilter>();
        else if (filterName == "freeverb")
            instance.filter = std::make_unique<SoLoud::FreeverbFilter>();

        if (instance.filter) {

            // Initialize parameters using FilterManager definitions (eliminates duplication)
            // Get parameter definitions from FilterManager for this specific filter
            const auto& availableFilters = FilterManager::getAvailableFilters();
            for (const auto& availableFilterName : availableFilters) {
                if (availableFilterName == filterName) {
                    // Found the filter, get its parameter definitions
                    for (int paramId = 0; paramId < 8;
                         ++paramId) { // SoLoud supports max 8 parameters
                        std::string paramName =
                            m_filterManager.getParameterName(filterName, paramId);
                        if (!paramName.empty()) {
                            // Determine parameter type based on original definitions
                            ParameterType paramType = ParameterType::FLOAT; // Default
                            if (filterName == "biquad" && paramId == 1) {
                                paramType = ParameterType::INT; // Filter Type
                            } else if (filterName == "robotize" && paramId == 2) {
                                paramType = ParameterType::INT; // Waveform
                            } else if (filterName == "freeverb" && paramId == 1) {
                                paramType = ParameterType::INT; // Freeze
                            }

                            // Parameter exists, create it with FilterManager values
                            instance.parameters[paramId] = {
                                m_filterManager.getParameterDefault(filterName, paramName),
                                m_filterManager.getParameterMin(filterName, paramName),
                                m_filterManager.getParameterMax(filterName, paramName), paramName,
                                paramType};
                        }
                    }
                    break; // Found the filter, no need to continue
                }
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

        // Use FilterManager to apply parameters (eliminates hardcoded filter-specific logic)

        // Build parameter values vector for FilterManager using only existing parameters
        std::vector<float> paramValues;
        for (const auto& [paramId, param] : instance.parameters) {
            paramValues.push_back(param.value);
        }

        // Apply parameters using FilterManager (respects filter-specific quirks)
        if (!paramValues.empty()) {
            m_filterManager.applyAllParameters(instance.filter.get(), filterName, paramValues);
        }
    }

    void AudioSystem::applyFiltersToTrack(size_t trackIndex) {
        if (!m_isInitialized || trackIndex >= m_trackManager.getTrackCount())
            return;

        // Check if track is currently playing
        if (trackIndex >= m_trackManager.getTrackCount() ||
            !m_trackManager.getTrack(trackIndex).isPlaying) {
            // Track not playing, just apply filters to the source for next play
            for (int slot = 0; slot < 8; ++slot) {
                m_trackManager.getTrack(trackIndex).wav->setFilter(slot, nullptr);
            }

            int filterSlot = 0;
            for (auto& [name, instance] : m_trackFilters[trackIndex].filters) {
                if (instance.enabled && instance.filter && filterSlot < 8) {
                    m_trackManager.getTrack(trackIndex)
                        .wav->setFilter(filterSlot, instance.filter.get());
                    // Set initial parameters immediately to ensure clean state
                    updateFilterInstance(instance, name);
                    instance.slot = filterSlot;
                    filterSlot++;
                }
            }
            return;
        }

        // Track is playing - need to restart it with new filters
        unsigned int handle = m_trackManager.getTrack(trackIndex).handle;
        float position = m_engine->get().getStreamPosition(handle);
        float volume = m_engine->get().getVolume(handle);

        // Stop current voice
        m_engine->get().stop(handle);
        m_trackManager.getTrack(trackIndex).handle = 0;
        m_trackManager.getTrack(trackIndex).isPlaying = false;

        // Clear all filters from the track source first
        for (int slot = 0; slot < 8; ++slot) {
            m_trackManager.getTrack(trackIndex).wav->setFilter(slot, nullptr);
        }

        // Apply enabled filters to the track source with clean initialization
        int filterSlot = 0;
        for (auto& [name, instance] : m_trackFilters[trackIndex].filters) {
            if (instance.enabled && instance.filter && filterSlot < 8) {
                // Don't re-initialize the filter unless it's truly broken
                // Just ensure parameters are correctly applied
                m_trackManager.getTrack(trackIndex)
                    .wav->setFilter(filterSlot, instance.filter.get());
                updateFilterInstance(instance, name);
                instance.slot = filterSlot;
                filterSlot++;
            }
        }

        // Restart the track with clean filters
        unsigned int newHandle = m_masterBus->get().play(*m_trackManager.getTrack(trackIndex).wav);
        m_trackManager.getTrack(trackIndex).handle = newHandle;
        m_trackManager.getTrack(trackIndex).isPlaying = true;
        m_engine->get().setVolume(newHandle, volume);
        m_engine->get().seek(newHandle, position);

        // CRITICAL: Reapply global playback rate after restart
        if (m_globalPlaybackRate != 1.0f) {
            m_engine->get().setRelativePlaySpeed(newHandle, m_globalPlaybackRate);
        }

        // Apply all parameters to all filters after the track is restarted
        filterSlot = 0;
        for (auto& [name, instance] : m_trackFilters[trackIndex].filters) {
            if (instance.enabled && instance.filter && filterSlot < 8) {
                // Apply all parameters for this filter, including WET parameter
                for (const auto& [paramId, param] : instance.parameters) {
                    // For problematic filters (freeverb, robotize, lofi, flanger),
                    // ensure WET parameter is applied immediately
                    if ((name == "freeverb" || name == "robotize" || name == "lofi" ||
                         name == "flanger") &&
                        paramId == 0) {
                        // Apply WET parameter immediately without fade for these filters
                        m_engine->get().setFilterParameter(newHandle, filterSlot, paramId,
                                                           param.value);
                    } else if (name == "freeverb") {
                        // For Freeverb, apply ALL parameters immediately to force the Revmodel to
                        // update This is needed because FreeverbFilterInstance constructor doesn't
                        // apply parameters to Revmodel
                        m_engine->get().setFilterParameter(newHandle, filterSlot, paramId,
                                                           param.value);
                    } else {
                        // Use normal fade for other parameters
                        m_engine->get().fadeFilterParameter(newHandle, filterSlot, paramId,
                                                            param.value,
                                                            FILTER_PARAM_TRANSITION_TIME);
                    }
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

    std::vector<std::string> AudioSystem::getFiltersInSignalChainOrder(size_t trackIndex) const {
        std::vector<std::string> result;

        if (!m_isInitialized || trackIndex >= m_trackFilters.size())
            return result;

        const auto& filters = m_trackFilters[trackIndex].filters;
        const auto& signalChainOrder = FilterManager::getFiltersInSignalChainOrder();

        // Add ALL filters in their hard-coded signal chain order (enabled and disabled)
        for (const auto& filterName : signalChainOrder) {
            result.push_back(filterName);
        }

        // Then add any remaining available filters that aren't in the signal chain order
        const auto& availableFilters = FilterManager::getAvailableFilters();
        for (const auto& filterName : availableFilters) {
            if (std::find(signalChainOrder.begin(), signalChainOrder.end(), filterName) ==
                signalChainOrder.end()) {
                result.push_back(filterName);
            }
        }

        return result;
    }

    // Bus filter methods
    void AudioSystem::setBusFilterEnabled(const std::string& filterName, bool enabled) {
        auto it = m_busFilters.find(filterName);
        if (it == m_busFilters.end()) {
            if (enabled) {
                // Find the next available position
                int nextPosition = 0;
                for (const auto& [name, instance] : m_busFilters) {
                    if (instance.position >= nextPosition) {
                        nextPosition = instance.position + 1;
                    }
                }

                FilterInstance instance;
                initializeFilter(instance, filterName);
                int wetId = getWetParameterId(filterName);
                if (wetId >= 0) {
                    instance.parameters[wetId].value = 0.0f;
                }
                instance.enabled = false;
                instance.position = nextPosition;
                m_busFilters[filterName] = std::move(instance);
                it = m_busFilters.find(filterName);
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
            // Find the next available position
            int nextPosition = 0;
            for (const auto& [name, instance] : m_busFilters) {
                if (instance.position >= nextPosition) {
                    nextPosition = instance.position + 1;
                }
            }

            FilterInstance instance;
            initializeFilter(instance, filterName);
            int wetId = getWetParameterId(filterName);
            if (wetId >= 0) {
                instance.parameters[wetId].value = 0.0f;
            }
            instance.enabled = false;
            instance.position = nextPosition;
            m_busFilters[filterName] = std::move(instance);
            it = m_busFilters.find(filterName);
        }

        auto& instance = it->second;
        auto paramIt = instance.parameters.find(paramId);
        if (paramIt != instance.parameters.end()) {
            auto& param = paramIt->second;
            if (param.value != value) {
                // Auto enable/disable logic based on wet parameter
                bool wasEnabled = instance.enabled;
                if (isWetParameter(filterName, paramId)) {
                    instance.enabled = (value > 0.0f); // Auto enable/disable based on wet > 0
                } else if (!instance.enabled && value != param.value) {
                    // If setting non-wet parameter and filter is disabled, auto-enable if wet > 0
                    auto wetParamIt = instance.parameters.find(getWetParameterId(filterName));
                    if (wetParamIt != instance.parameters.end() &&
                        wetParamIt->second.value > 0.0f) {
                        instance.enabled = true;
                    }
                }

                // Store the new value
                param.value = value;

                // Apply the parameter change immediately for realtime effect
                if (instance.enabled) {
                    updateFilterInstance(instance, filterName);
                }

                // For robotize, we need to force bus filter reapplication for Frequency and
                // Waveform parameters to ensure real-time updates work correctly
                if (filterName == "robotize" && (paramId == 1 || paramId == 2)) {
                    updateBusFilterParams();
                }

                // For echo filter, we need to call setParams() on the filter instance for Delay
                // and Decay parameters to ensure real-time updates work correctly
                if (filterName == "echo" && (paramId == 1 || paramId == 2)) {
                    updateFilterInstance(instance, filterName);
                }

                // If bus is playing and filter is enabled, apply the parameter change to the voice
                if (m_busHandle && instance.enabled) {
                    // For problematic filters (freeverb, robotize, lofi, flanger), use
                    // immediate parameter setting for WET parameter to override the broken
                    // initialization in SoLoud
                    if ((filterName == "freeverb" || filterName == "robotize" ||
                         filterName == "lofi" || filterName == "flanger") &&
                        paramId == 0) {
                        // Apply WET parameter immediately without fade for these filters
                        m_engine->get().setFilterParameter(m_busHandle, instance.slot, paramId,
                                                           value);
                    } else if (filterName == "freeverb") {
                        // For Freeverb, apply ALL parameters immediately to force the Revmodel to
                        // update This is needed because FreeverbFilterInstance constructor doesn't
                        // apply parameters to Revmodel
                        m_engine->get().setFilterParameter(m_busHandle, instance.slot, paramId,
                                                           value);
                    } else {
                        // Use normal fade for other parameters
                        m_engine->get().fadeFilterParameter(m_busHandle, instance.slot, paramId,
                                                            value, FILTER_PARAM_TRANSITION_TIME);
                    }
                }

                // If enabled state changed, need to reapply bus filters
                if (wasEnabled != instance.enabled) {
                    updateBusFilterParams();
                }
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

    std::vector<std::string> AudioSystem::getBusFiltersInSignalChainOrder() const {
        std::vector<std::string> result;

        const auto& signalChainOrder = FilterManager::getFiltersInSignalChainOrder();

        // Add ALL filters in their hard-coded signal chain order (enabled and disabled)
        for (const auto& filterName : signalChainOrder) {
            result.push_back(filterName);
        }

        // Then add any remaining available filters that aren't in the signal chain order
        const auto& availableFilters = FilterManager::getAvailableFilters();
        for (const auto& filterName : availableFilters) {
            if (std::find(signalChainOrder.begin(), signalChainOrder.end(), filterName) ==
                signalChainOrder.end()) {
                result.push_back(filterName);
            }
        }

        return result;
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
                    // For problematic filters (freeverb, robotize, lofi, flanger),
                    // ensure WET parameter is applied immediately to override the broken
                    // initialization in SoLoud
                    if ((name == "freeverb" || name == "robotize" || name == "lofi" ||
                         name == "flanger") &&
                        paramId == 0) {
                        // Apply WET parameter immediately without fade for these filters
                        m_engine->get().setFilterParameter(m_busHandle, filterSlot, paramId,
                                                           param.value);
                    } else if (name == "freeverb") {
                        // For Freeverb, apply ALL parameters immediately to force the Revmodel to
                        // update This is needed because FreeverbFilterInstance constructor doesn't
                        // apply parameters to Revmodel
                        m_engine->get().setFilterParameter(m_busHandle, filterSlot, paramId,
                                                           param.value);
                    } else {
                        // Use normal fade for other parameters
                        m_engine->get().fadeFilterParameter(m_busHandle, filterSlot, paramId,
                                                            param.value, BUS_PARAM_FADE_TIME);
                    }
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

    // Synchronization methods
    /**
     * Updates audio synchronization across all tracks.
     *
     * This method maintains perfect synchronization between multiple audio tracks
     * by detecting and correcting drift. It uses a master track as the reference
     * clock and periodically checks all other tracks for timing discrepancies.
     *
     * The synchronization process:
     * 1. Updates global time based on master track position
     * 2. Checks for sync issues every 100ms (SYNC_CHECK_INTERVAL)
     * 3. Detects tracks that have drifted beyond tolerance (1ms)
     * 4. Corrects drifted tracks by seeking to the correct position
     *
     * This ensures all tracks stay perfectly synchronized even during long playback
     * sessions where small timing differences can accumulate.
     */
    void AudioSystem::updateSync() {
        // DEBUG: Simple heartbeat to confirm updateSync is being called
        static int heartbeatCount = 0;
        heartbeatCount++;
        if (heartbeatCount % 1000 == 0) { // Log every 1000th call
            LOG_INFO("UPDATE SYNC HEARTBEAT: call #" + std::to_string(heartbeatCount));
        }

        // Early exit if system not ready or no tracks playing
        if (!m_isInitialized || !m_syncState.isPlaying || m_trackManager.getTrackCount() == 0) {
            // DEBUG: Log why updateSync is exiting early
            static int earlyExitCount = 0;
            earlyExitCount++;
            if (earlyExitCount % 1000 == 0) { // Log every 1000th early exit to avoid spam
                LOG_INFO("UPDATE SYNC EARLY EXIT: initialized=" + std::to_string(m_isInitialized) +
                         ", isPlaying=" + std::to_string(m_syncState.isPlaying) +
                         ", trackCount=" + std::to_string(m_trackManager.getTrackCount()));
            }
            return;
        }

        // Update global time based on master track position
        // The master track serves as the reference clock for all other tracks
        if (m_syncState.masterTrackIndex < m_trackManager.getTrackCount() &&
            m_trackManager.getTrack(m_syncState.masterTrackIndex).isPlaying) {
            double oldGlobalTime = m_syncState.globalTime;
            m_syncState.globalTime = m_engine->get().getStreamPosition(
                m_trackManager.getTrack(m_syncState.masterTrackIndex).handle);

            // DEBUG: Log if global time update seems unusual
            double timeAdvance = m_syncState.globalTime - oldGlobalTime;
            if (timeAdvance > 0.1) { // If time advanced more than 100ms in one update
                LOG_INFO("GLOBAL TIME JUMP: " + std::to_string(oldGlobalTime) + "s -> " +
                         std::to_string(m_syncState.globalTime) + "s " +
                         "(advance: " + std::to_string(timeAdvance * 1000.0) + "ms)");
            }
        } else {
            // DEBUG: Log when master track is not available
            static int masterUnavailableCount = 0;
            masterUnavailableCount++;
            if (masterUnavailableCount % 100 == 0) { // Log every 100th occurrence
                LOG_INFO("MASTER TRACK UNAVAILABLE: index=" +
                         std::to_string(m_syncState.masterTrackIndex) +
                         ", trackCount=" + std::to_string(m_trackManager.getTrackCount()) +
                         ", isPlaying=" + std::to_string(m_syncState.isPlaying));
            }
        }

        // PERFORMANCE OPTIMIZATION: Check for sync issues every 100ms
        // This prevents excessive checking while still catching drift quickly
        double currentTime = m_syncState.globalTime;

        // FIXED: Handle case where currentTime < lastSyncCheck (track looping/seeking)
        double timeSinceLastCheck = currentTime - m_syncState.lastSyncCheck;

        // If time went backwards (track looped/seeking), reset and do a sync check
        if (timeSinceLastCheck < 0) {
            LOG_INFO("TIME WENT BACKWARDS: currentTime=" + std::to_string(currentTime) +
                     ", lastCheck=" + std::to_string(m_syncState.lastSyncCheck) +
                     ", resetting sync check");
            m_syncState.lastSyncCheck = currentTime;
            // Continue to sync check below
        }
        // Otherwise, check if enough time has passed for next sync check
        else if (timeSinceLastCheck < SyncState::SYNC_CHECK_INTERVAL) {
            // DEBUG: Log when sync check is skipped due to interval
            static int intervalSkipCount = 0;
            intervalSkipCount++;
            if (intervalSkipCount % 1000 == 0) { // Log every 1000th skip
                LOG_INFO("SYNC CHECK SKIPPED: currentTime=" + std::to_string(currentTime) +
                         ", lastCheck=" + std::to_string(m_syncState.lastSyncCheck) +
                         ", timeSinceLast=" + std::to_string(timeSinceLastCheck) +
                         ", interval=" + std::to_string(SyncState::SYNC_CHECK_INTERVAL));
            }
            return;
        } else {
            // Normal case: enough time has passed, update lastSyncCheck
            m_syncState.lastSyncCheck = currentTime;
        }

        // If we get here, it's time for a sync check
        m_syncState.lastSyncCheck = currentTime;

        // Check each track for drift and correct if needed
        // This is the core synchronization logic
        checkAndCorrectSync();
    }

    /**
     * Checks all tracks for synchronization drift and corrects any issues.
     *
     * This method iterates through all tracks (except the master) and checks
     * if they have drifted beyond the tolerance threshold. If drift is detected,
     * the track is corrected by seeking to the correct position.
     *
     * The master track is skipped since it serves as the reference clock.
     * Only playing tracks are checked to avoid unnecessary processing.
     */
    void AudioSystem::checkAndCorrectSync() {
        // DEBUG: Log comprehensive sync state
        static int debugCounter = 0;
        debugCounter++;

        // Only log every 10th check to avoid spam (every 1 second)
        if (debugCounter % 10 == 0) {
            LOG_INFO("=== SYNC DEBUG CHECK ===");
            LOG_INFO("Master track: " + std::to_string(m_syncState.masterTrackIndex) +
                     " (global time: " + std::to_string(m_syncState.globalTime) + "s)");

            for (size_t i = 0; i < m_trackManager.getTrackCount(); ++i) {
                const auto& track = m_trackManager.getTrack(i);
                double trackTime = getTrackCurrentTime(i);
                double timeDiff = std::abs(m_syncState.globalTime - trackTime);
                bool isDrifting = timeDiff > SyncState::DRIFT_TOLERANCE;

                LOG_INFO("Track " + std::to_string(i) + ": " +
                         "playing=" + std::to_string(track.isPlaying) +
                         ", position=" + std::to_string(trackTime) + "s" +
                         ", diff=" + std::to_string(timeDiff * 1000.0) + "ms" +
                         ", drifting=" + std::to_string(isDrifting) +
                         (i == m_syncState.masterTrackIndex ? " [MASTER]" : ""));
            }
            LOG_INFO("=== END SYNC DEBUG ===");
        }

        for (size_t i = 0; i < m_trackManager.getTrackCount(); ++i) {
            // Skip the master track - it's our reference clock
            if (i == m_syncState.masterTrackIndex) {
                continue;
            }

            // Only check tracks that are currently playing
            if (m_trackManager.getTrack(i).isPlaying && isTrackDrifting(i)) {
                LOG_INFO("Track " + std::to_string(i) + " drifting, correcting...");
                correctTrackSync(i, m_syncState.globalTime);
            }
        }
    }

    /**
     * Corrects synchronization drift for a specific track.
     *
     * This method compares the track's current position with the target time
     * (from the master track). If the difference exceeds the drift tolerance
     * (1ms), the track is seeked to the correct position.
     *
     * The correction uses our custom seek method for accurate positioning
     * and updates the track's expected position to prevent future drift.
     *
     * @param trackIndex Index of the track to correct
     * @param targetTime Target time position from master track
     */
    void AudioSystem::correctTrackSync(size_t trackIndex, double targetTime) {
        // Safety check: ensure track exists and is playing
        if (trackIndex >= m_trackManager.getTrackCount() ||
            !m_trackManager.getTrack(trackIndex).isPlaying) {
            return;
        }

        // Get current position of this track from SoLoud
        double currentTime = getTrackCurrentTime(trackIndex);
        double timeDiff = std::abs(targetTime - currentTime);

        // Only correct if drift is significant (>1ms tolerance)
        // This prevents unnecessary seeking for minor timing differences
        if (timeDiff > SyncState::DRIFT_TOLERANCE) {
            // Use our custom seek method for accurate positioning
            // This ensures the track jumps to exactly the right position
            m_engine->get().seek(m_trackManager.getTrack(trackIndex).handle, targetTime);
            m_trackManager.getTrack(trackIndex).expectedPosition = targetTime;
            LOG_INFO("Track " + std::to_string(trackIndex) +
                     " synced: " + std::to_string(timeDiff) + "s correction");
        }
    }

    /**
     * Gets the current playback time for a specific track.
     *
     * This method queries SoLoud for the current stream position of the track.
     * It includes safety checks to ensure the track exists and is playing.
     *
     * @param trackIndex Index of the track to query
     * @return Current playback time in seconds, or 0.0 if track not available
     */
    double AudioSystem::getTrackCurrentTime(size_t trackIndex) const {
        if (trackIndex >= m_trackManager.getTrackCount() ||
            !m_trackManager.getTrack(trackIndex).isPlaying) {
            return 0.0;
        }
        return m_engine->get().getStreamPosition(m_trackManager.getTrack(trackIndex).handle);
    }

    /**
     * Checks if a track has drifted beyond the synchronization tolerance.
     *
     * This method compares the track's current position with the master track's
     * position. If the difference exceeds the drift tolerance (1ms), the track
     * is considered to be drifting and needs correction.
     *
     * The tolerance is set to 1ms to catch drift early while avoiding
     * unnecessary corrections for minor timing differences.
     *
     * @param trackIndex Index of the track to check
     * @return true if track is drifting, false otherwise
     */
    bool AudioSystem::isTrackDrifting(size_t trackIndex) const {
        if (trackIndex >= m_trackManager.getTrackCount() ||
            !m_trackManager.getTrack(trackIndex).isPlaying) {
            return false;
        }

        // Get current positions
        double trackTime = getTrackCurrentTime(trackIndex);
        double masterTime = m_syncState.globalTime;
        double timeDiff = std::abs(masterTime - trackTime);

        // DEBUG: Log drift detection details for significant differences
        if (timeDiff > 0.0005) { // Log if difference > 0.5ms
            LOG_INFO("DRIFT DETECTION: Track " + std::to_string(trackIndex) + " - Master: " +
                     std::to_string(masterTime) + "s, " + "Track: " + std::to_string(trackTime) +
                     "s, " + "Diff: " + std::to_string(timeDiff * 1000.0) + "ms, " +
                     "Tolerance: " + std::to_string(SyncState::DRIFT_TOLERANCE * 1000.0) + "ms");
        }

        // Check if difference exceeds tolerance (1ms)
        return timeDiff > SyncState::DRIFT_TOLERANCE;
    }

    // Dual tape speed architecture implementation
    /**
     * Updates the dual tape speed architecture for granular tempo control.
     *
     * This method implements the dual tape speed architecture that allows independent
     * control of tempo and pitch. The architecture works as follows:
     *
     * 1. User Tape Speed: Direct playback rate control (0.1x - 4.0x)
     * 2. Granular Tempo: Pitch-preserving tempo multiplier (0.5x - 2.0x)
     * 3. Internal Tape Speed: Hidden calculation = userTapeSpeed * granularTempo
     * 4. Pitch Compensation: Hidden calculation = 1.0 / granularTempo
     *
     * This eliminates complex buffering by maintaining perfect sample ratios while
     * achieving independent pitch and tempo control. SoLoud handles time changes
     * through tape speed, while Signalsmith Stretch handles pitch compensation.
     *
     * The method only updates when values actually change to avoid unnecessary
     * processing and logging.
     */
    void AudioSystem::updateDualTapeSpeed() {
        if (!m_isInitialized) {
            return;
        }

        // Calculate the hidden internal values that make the architecture work
        float newInternalTapeSpeed = calculateInternalTapeSpeed();
        float newPitchCompensation = calculatePitchCompensation();

        // PERFORMANCE OPTIMIZATION: Only update if values actually changed
        // This prevents unnecessary processing and reduces log spam
        bool valuesChanged = (std::abs(newInternalTapeSpeed - m_internalTapeSpeed) > 0.0001f) ||
                             (std::abs(newPitchCompensation - m_pitchCompensation) > 0.0001f);

        // Update the hidden internal values
        m_internalTapeSpeed = newInternalTapeSpeed;
        m_pitchCompensation = newPitchCompensation;

        // Apply internal tape speed to all currently playing tracks
        // This is what SoLoud actually uses for playback rate control
        for (size_t i = 0; i < m_trackManager.getTrackCount(); ++i) {
            if (m_trackManager.getTrack(i).isPlaying && m_trackManager.getTrack(i).handle != 0) {
                m_engine->get().setRelativePlaySpeed(m_trackManager.getTrack(i).handle,
                                                     m_internalTapeSpeed);
            }
        }

        // Store for future tracks that get loaded
        m_globalPlaybackRate = m_internalTapeSpeed;

        // Update Signalsmith Stretch with pitch compensation
        // This handles the pitch preservation part of the architecture
        if (m_granularProcessor && m_granularProcessor->isReady()) {
            // Set pitch compensation in the AudioStreamProcessor
            // This counteracts the tempo change to preserve pitch
            m_granularProcessor->setPitchCompensation(m_pitchCompensation);

            // Only log when values actually change to reduce log spam
            if (valuesChanged) {
                LOG_INFO("Dual tape speed update: userSpeed=" + std::to_string(m_userTapeSpeed) +
                         ", granularTempo=" + std::to_string(m_granularTempo) +
                         ", internalSpeed=" + std::to_string(m_internalTapeSpeed) +
                         ", pitchComp=" + std::to_string(m_pitchCompensation));
            }
        }
    }

    /**
     * Calculates the internal tape speed for the dual tape speed architecture.
     *
     * This is the hidden calculation that combines user tape speed and granular tempo:
     * Internal Tape Speed = userTapeSpeed * granularTempo
     *
     * The internal tape speed is what SoLoud actually uses for playback rate control.
     * By combining both user controls, we achieve the desired tempo while maintaining
     * the architecture's ability to preserve pitch through separate compensation.
     *
     * @return The calculated internal tape speed multiplier
     */
    float AudioSystem::calculateInternalTapeSpeed() const {
        return m_userTapeSpeed * m_granularTempo;
    }

    /**
     * Calculates the pitch compensation factor for the dual tape speed architecture.
     *
     * This is the hidden calculation that preserves pitch during tempo changes:
     * Pitch Compensation = 1.0 / granularTempo
     *
     * When granular tempo changes the playback speed, this compensation factor
     * is applied by Signalsmith Stretch to counteract the pitch shift. This allows
     * independent control of tempo and pitch.
     *
     * The calculation includes a safety check to prevent division by zero.
     *
     * @return The calculated pitch compensation factor
     */
    float AudioSystem::calculatePitchCompensation() const {
        return (m_granularTempo != 0.0f) ? (1.0f / m_granularTempo) : 1.0f;
    }

    void AudioSystem::setFilterParameterByName(size_t trackIndex, const std::string& filterName,
                                               const std::string& paramName, float value) {
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

        auto& filterInstance = it->second;

        // Use FilterManager to validate and get parameter ID
        if (!m_filterManager.isValidParameter(filterName, paramName, value)) {
            LOG_ERROR("Invalid parameter value: " + filterName + "." + paramName + " = " +
                      std::to_string(value));
            return;
        }

        int paramId = m_filterManager.getParameterId(filterName, paramName);
        if (paramId == -1) {
            LOG_ERROR("Invalid parameter: " + filterName + "." + paramName);
            return;
        }

        // Use the existing integer-based method
        setFilterParameter(trackIndex, filterName, paramId, value);
    }

    float AudioSystem::getFilterParameterByName(size_t trackIndex, const std::string& filterName,
                                                const std::string& paramName) const {
        if (!m_isInitialized || trackIndex >= m_trackFilters.size()) {
            return 0.0f;
        }

        const auto& trackFilters = m_trackFilters[trackIndex];
        auto it = trackFilters.filters.find(filterName);
        if (it == trackFilters.filters.end()) {
            return 0.0f;
        }

        // Use FilterManager to get parameter ID
        int paramId = m_filterManager.getParameterId(filterName, paramName);
        if (paramId == -1) {
            return 0.0f;
        }

        // Use the existing integer-based method
        return getFilterParameter(trackIndex, filterName, paramId);
    }

    void AudioSystem::setBusFilterParameterByName(const std::string& filterName,
                                                  const std::string& paramName, float value) {
        if (!m_isInitialized) {
            return;
        }

        auto it = m_busFilters.find(filterName);
        if (it == m_busFilters.end()) {
            // Initialize the filter if it doesn't exist
            FilterInstance instance;
            initializeFilter(instance, filterName);
            if (instance.filter) {
                instance.enabled = true; // Enable the filter by default
                m_busFilters[filterName] = std::move(instance);
                it = m_busFilters.find(filterName);
            } else {
                return;
            }
        }

        auto& filterInstance = it->second;

        // Use FilterManager to validate and get parameter ID
        if (!m_filterManager.isValidParameter(filterName, paramName, value)) {
            LOG_ERROR("Invalid parameter value: " + filterName + "." + paramName + " = " +
                      std::to_string(value));
            return;
        }

        int paramId = m_filterManager.getParameterId(filterName, paramName);
        if (paramId == -1) {
            LOG_ERROR("Invalid parameter: " + filterName + "." + paramName);
            return;
        }

        // Use the existing integer-based method
        setBusFilterParameter(filterName, paramId, value);
    }

    float AudioSystem::getBusFilterParameterByName(const std::string& filterName,
                                                   const std::string& paramName) const {
        if (!m_isInitialized) {
            return 0.0f;
        }

        auto it = m_busFilters.find(filterName);
        if (it == m_busFilters.end()) {
            return 0.0f;
        }

        // Use FilterManager to get parameter ID
        int paramId = m_filterManager.getParameterId(filterName, paramName);
        if (paramId == -1) {
            return 0.0f;
        }

        // Use the existing integer-based method
        return getBusFilterParameter(filterName, paramId);
    }

    // Helper methods for wet-based effect automation
    bool AudioSystem::isWetParameter(const std::string& filterName, int paramId) const {
        // Use FilterManager to check if this is a wet parameter
        return m_filterManager.getParameterName(filterName, paramId) == "wet";
    }

    int AudioSystem::getWetParameterId(const std::string& filterName) const {
        // Use FilterManager to get wet parameter ID
        return m_filterManager.getParameterId(filterName, "wet");
    }

    bool AudioSystem::shouldAutoEnableFilter(const std::string& filterName, int paramId,
                                             float value) const {
        // Auto-enable if setting wet parameter > 0, or if filter has no wet parameter
        int wetParamId = getWetParameterId(filterName);
        if (wetParamId == -1) {
            return true; // Filter has no wet parameter, always enable when parameters are set
        }

        if (paramId == wetParamId) { // Wet parameter
            return value > 0.0f;
        }

        return false; // Don't auto-enable for non-wet parameters of new filters
    }

    bool AudioSystem::hasPausedTracks() const {
        for (size_t i = 0; i < m_trackManager.getTrackCount(); ++i) {
            if (m_trackManager.getTrack(i).isPaused) {
                return true;
            }
        }
        return false;
    }

    void AudioSystem::calculateMasterDuration() {
        if (m_trackManager.getTrackCount() == 0) {
            m_syncState.masterDuration = 0.0;
            m_syncState.masterTrackIndex = 0;
            return;
        }

        // Find the shortest track duration (master clock)
        double shortestDuration = std::numeric_limits<double>::max();
        size_t shortestIndex = 0;

        for (size_t i = 0; i < m_trackManager.getTrackCount(); ++i) {
            if (m_trackManager.getTrack(i).duration < shortestDuration) {
                shortestDuration = m_trackManager.getTrack(i).duration;
                shortestIndex = i;
            }
        }

        m_syncState.masterDuration = shortestDuration;
        m_syncState.masterTrackIndex = shortestIndex;

        LOG_INFO("Master duration set to: " + std::to_string(m_syncState.masterDuration) +
                 "s (track " + std::to_string(shortestIndex) + ")");
    }

    void AudioSystem::playAllTracks() {
        if (!m_isInitialized || m_trackManager.getTrackCount() == 0) {
            return;
        }

        // Only reset state if this is the very first play
        if (!m_hasEverPlayed) {
            // Stop all tracks first (only on first play)
            stopAllTracks();

            // Start all tracks simultaneously
            for (size_t i = 0; i < m_trackManager.getTrackCount(); ++i) {
                playTrack(i);
            }

            m_syncState.isPlaying = true;
            m_syncState.globalTime = 0.0;
            m_syncState.lastSyncCheck = 0.0;
            m_hasEverPlayed = true;

            LOG_INFO("Started synchronized playback of " +
                     std::to_string(m_trackManager.getTrackCount()) + " tracks");
        } else {
            // Tracks have been played before - this should not happen in normal pause/resume flow
            // But if it does, just resume from current state
            LOG_INFO("playAllTracks called but tracks have been played before - resuming instead");
            resumeAllTracks();
        }
    }

    void AudioSystem::stopAllTracks() {
        if (!m_isInitialized) {
            return;
        }

        for (size_t i = 0; i < m_trackManager.getTrackCount(); ++i) {
            stopTrack(i);
        }

        m_syncState.isPlaying = false;
        m_syncState.globalTime = 0.0;
    }

    void AudioSystem::pauseAllTracks() {
        if (!m_isInitialized) {
            return;
        }

        // Store current global time before pausing
        if (m_syncState.isPlaying &&
            m_syncState.masterTrackIndex < m_trackManager.getTrackCount() &&
            m_trackManager.getTrack(m_syncState.masterTrackIndex).isPlaying) {
            m_syncState.globalTime = m_engine->get().getStreamPosition(
                m_trackManager.getTrack(m_syncState.masterTrackIndex).handle);
        }

        for (size_t i = 0; i < m_trackManager.getTrackCount(); ++i) {
            pauseTrack(i);
        }

        m_syncState.isPlaying = false;
        // Don't reset globalTime - preserve it for resume
    }

    void AudioSystem::resumeAllTracks() {
        if (!m_isInitialized || m_trackManager.getTrackCount() == 0) {
            return;
        }

        // Resume all tracks from their paused positions
        for (size_t i = 0; i < m_trackManager.getTrackCount(); ++i) {
            resumeTrack(i);
        }

        m_syncState.isPlaying = true;
        // globalTime is already set from pause, just update lastSyncCheck
        m_syncState.lastSyncCheck = m_syncState.globalTime;

        LOG_INFO("Resumed synchronized playback of " +
                 std::to_string(m_trackManager.getTrackCount()) + " tracks from position " +
                 std::to_string(m_syncState.globalTime));
    }
} // namespace AudioTester