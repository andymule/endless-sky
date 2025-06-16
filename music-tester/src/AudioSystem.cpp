#include "AudioSystem.h"
#include <iostream>
#include <chrono>

const std::vector<std::string> AudioSystem::AVAILABLE_FILTERS = {
    "biquad", "echo", "lofi", "flanger", "dcremoval", "bassboost", "waveshaper", "robotize", "freeverb"
};

// Time in seconds for smooth parameter transitions
constexpr SoLoud::time FILTER_PARAM_TRANSITION_TIME = 0.05;

AudioSystem::AudioSystem() : m_isInitialized(false)
{
}

AudioSystem::~AudioSystem()
{
    if (m_isInitialized)
    {
        m_soloud.deinit();
    }
}

bool AudioSystem::initialize()
{
    if (m_isInitialized)
        return true;

    std::cout << "Initializing AudioSystem..." << std::endl;
    SoLoud::result result = m_soloud.init();
    if (result != SoLoud::SO_NO_ERROR)
    {
        std::cerr << "Failed to initialize SoLoud: " << result << std::endl;
        return false;
    }

    // Initialize the master bus properly before use
    // Clear all filter slots to ensure clean state
    for (int i = 0; i < 8; ++i) {
        // Don't call setFilter on uninitialized bus
    }
    
    // Play the bus once and store the handle
    m_busHandle = m_soloud.play(m_masterBus);
    m_masterBus.setVolume(m_busVolume);
    m_soloud.setVolume(m_busHandle, m_busVolume);

    m_isInitialized = true;
    std::cout << "AudioSystem initialized successfully" << std::endl;

    // Auto-load tracks from the default folder
    std::filesystem::path defaultFolder = "sound_staging";
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
    std::cout << "Looking for sound_staging in: " << std::filesystem::absolute(defaultFolder) << std::endl;
    
    if (std::filesystem::exists(defaultFolder))
    {
        std::cout << "Default folder found, contents:" << std::endl;
        for (const auto& entry : std::filesystem::directory_iterator(defaultFolder))
        {
            std::cout << "  " << entry.path().filename() << std::endl;
        }
        std::cout << "Loading tracks..." << std::endl;
        bool loaded = loadDirectory(defaultFolder);
        std::cout << "Load result: " << (loaded ? "success" : "failed") << std::endl;
        std::cout << "Number of tracks loaded: " << m_tracks.size() << std::endl;
    }
    else
    {
        std::cout << "Default folder not found at: " << std::filesystem::absolute(defaultFolder) << std::endl;
        std::cout << "Directory contents:" << std::endl;
        for (const auto& entry : std::filesystem::directory_iterator("."))
        {
            std::cout << "  " << entry.path().filename() << std::endl;
        }
    }

    return true;
}

void AudioSystem::initializeFilter(FilterInstance& instance, const std::string& filterName)
{
    std::cout << "Initializing filter: " << filterName << std::endl;

    // Create the appropriate filter type
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

    if (instance.filter)
    {
        std::cout << "Filter created successfully" << std::endl;
        
        // Initialize parameters with their ranges based on filter type
        if (filterName == "biquad")
        {
            // Type, Frequency, Resonance
            instance.parameters[0] = {0.0f, 0.0f, 5.0f, "Type", false};  // 0-5 for different filter types
            instance.parameters[1] = {1000.0f, 20.0f, 20000.0f, "Frequency", false};  // 20Hz-20kHz
            instance.parameters[2] = {1.0f, 0.1f, 10.0f, "Resonance", false};  // Q factor
        }
        else if (filterName == "echo")
        {
            // Wet, Delay, Decay, Filter
            instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet", false};  // 0-1 wet/dry mix
            instance.parameters[1] = {0.3f, 0.001f, 1.0f, "Delay", false};  // 0.001-1 delay time
            instance.parameters[2] = {0.7f, 0.001f, 1.0f, "Decay", false};  // 0.001-1 decay amount
            instance.parameters[3] = {0.0f, 0.0f, 0.999f, "Filter", false};  // 0-0.999 filter amount
        }
        else if (filterName == "lofi")
        {
            // Wet, Sample rate
            instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet", false};  // 0-1 wet/dry mix
            instance.parameters[1] = {0.5f, 0.0f, 1.0f, "Sample Rate", false};  // 0-1 sample rate reduction
        }
        else if (filterName == "flanger")
        {
            // Wet, Delay
            instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet", false};  // 0-1 wet/dry mix
            instance.parameters[1] = {0.5f, 0.0f, 1.0f, "Delay", false};  // 0-1 delay time
        }
        else if (filterName == "dcremoval")
        {
            // Only one parameter: Length (in seconds)
            instance.parameters[0] = {0.1f, 0.01f, 10.0f, "Length", false}; // 0.01-10 seconds
        }
        else if (filterName == "bassboost")
        {
            // Boost
            instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Boost", false};  // 0-1 boost amount
        }
        else if (filterName == "waveshaper")
        {
            // Amount
            instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Amount", false};  // 0-1 distortion amount
        }
        else if (filterName == "robotize")
        {
            // Wet, Frequency, Waveform
            instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet", false};  // 0-1 wet/dry mix
            instance.parameters[1] = {30.0f, 0.1f, 100.0f, "Frequency", false};  // 0.1-100 Hz
            instance.parameters[2] = {0.0f, 0.0f, 6.0f, "Waveform", false};  // 0-6 waveform type
        }
        else if (filterName == "freeverb")
        {
            // Wet, Room size, Damp, Width
            instance.parameters[0] = {0.5f, 0.0f, 1.0f, "Wet", false};  // 0-1 wet/dry mix
            instance.parameters[1] = {0.5f, 0.0f, 1.0f, "Room Size", false};  // 0-1 room size
            instance.parameters[2] = {0.5f, 0.0f, 1.0f, "Damp", false};  // 0-1 damping
            instance.parameters[3] = {0.5f, 0.0f, 1.0f, "Width", false};  // 0-1 stereo width
        }

        // Apply initial parameters
        updateFilterInstance(instance, filterName);
    }
    else
    {
        std::cout << "Failed to create filter: " << filterName << std::endl;
    }
}

void AudioSystem::updateFilterInstance(FilterInstance& instance, const std::string& filterName)
{
    if (!instance.filter)
    {
        std::cout << "Filter update skipped - no filter instance" << std::endl;
        return;
    }

    if (!instance.enabled)
    {
        std::cout << "Filter update skipped - filter disabled" << std::endl;
        return;
    }

    std::cout << "Updating filter: " << filterName << std::endl;

    // Update all changed parameters for the correct filter type
    if (filterName == "biquad")
    {
        auto* f = dynamic_cast<SoLoud::BiquadResonantFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;  // Type
            float p2 = instance.parameters[1].value;  // Frequency
            float p3 = instance.parameters[2].value;  // Resonance
            std::cout << "Setting biquad params: type=" << p1 << " freq=" << p2 << " res=" << p3 << std::endl;
            f->setParams(static_cast<int>(p1), p2, p3);  // Type, Frequency, Resonance
        }
    }
    else if (filterName == "echo")
    {
        auto* f = dynamic_cast<SoLoud::EchoFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;  // Wet
            float p2 = instance.parameters[1].value;  // Delay
            float p3 = instance.parameters[2].value;  // Decay
            float p4 = instance.parameters[3].value;  // Filter
            float delay = std::max(0.001f, p2); // Minimum 1ms delay
            float decay = std::max(0.001f, p3); // Minimum 0.1% decay
            float filter = std::clamp(p4, 0.0f, 0.999f); // Filter between 0 and 0.999
            f->setParams(delay, decay, filter);
        }
    }
    else if (filterName == "lofi")
    {
        auto* f = dynamic_cast<SoLoud::LofiFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;  // Wet
            float p2 = instance.parameters[1].value;  // Sample rate
            f->setParams(p1, p2);  // Wet, Sample rate
        }
    }
    else if (filterName == "flanger")
    {
        auto* f = dynamic_cast<SoLoud::FlangerFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;  // Wet
            float p2 = instance.parameters[1].value;  // Delay
            f->setParams(p1, p2);  // Wet, Delay
        }
    }
    else if (filterName == "dcremoval")
    {
        auto* f = dynamic_cast<SoLoud::DCRemovalFilter*>(instance.filter.get());
        if (f)
        {
            float length = instance.parameters[0].value;
            f->setParams(length);
        }
    }
    else if (filterName == "bassboost")
    {
        auto* f = dynamic_cast<SoLoud::BassboostFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;  // Boost
            f->setParams(p1);  // Only takes boost parameter
        }
    }
    else if (filterName == "waveshaper")
    {
        auto* f = dynamic_cast<SoLoud::WaveShaperFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;  // Amount
            f->setParams(p1);  // Only takes amount parameter
        }
    }
    else if (filterName == "robotize")
    {
        auto* f = dynamic_cast<SoLoud::RobotizeFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;  // Wet
            float p2 = instance.parameters[1].value;  // Frequency
            float p3 = instance.parameters[2].value;  // Waveform
            float freq = std::clamp(p2, 0.1f, 100.0f); // Frequency between 0.1 and 100 Hz
            int wave = static_cast<int>(std::clamp(p3, 0.0f, 6.0f)); // Waveform between 0 and 6
            f->setParams(freq, wave);
        }
    }
    else if (filterName == "freeverb")
    {
        auto* f = dynamic_cast<SoLoud::FreeverbFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;  // Wet
            float p2 = instance.parameters[1].value;  // Room size
            float p3 = instance.parameters[2].value;  // Damp
            float p4 = instance.parameters[3].value;  // Width
            f->setParams(p1, p2, p3, p4);  // Wet, Room size, Damp, Width
        }
    }

    // Mark all parameters as unchanged
    for (auto& param : instance.parameters)
    {
        param.second.changed = false;
    }
}

bool AudioSystem::loadDirectory(const std::filesystem::path& directory)
{
    if (!m_isInitialized)
    {
        std::cout << "Cannot load directory - AudioSystem not initialized" << std::endl;
        return false;
    }

    std::cout << "Loading directory: " << std::filesystem::absolute(directory) << std::endl;

    // Clear existing tracks
    m_tracks.clear();
    m_trackVolumes.clear();
    m_trackFilters.clear();
    m_voiceHandles.clear();

    // Load all audio files from the directory
    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.is_regular_file())
        {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            std::cout << "Found file: " << entry.path().filename() << " (extension: " << ext << ")" << std::endl;
            
            if (ext == ".wav" || ext == ".ogg")
            {
                auto wav = std::make_unique<SoLoud::Wav>();
                SoLoud::result result = wav->load(entry.path().string().c_str());
                if (result == SoLoud::SO_NO_ERROR)
                {
                    std::cout << "Successfully loaded: " << entry.path().filename() << std::endl;
                    m_tracks.push_back(std::move(wav));
                    m_trackVolumes.push_back(1.0f);
                    m_trackFilters.push_back(TrackFilters());
                    // Enable looping by default for the newly loaded track
                    m_tracks.back()->setLooping(true);
                }
                else
                {
                    std::cout << "Failed to load: " << entry.path().filename() << " (error: " << result << ")" << std::endl;
                }
            }
        }
    }

    std::cout << "Finished loading directory. Total tracks: " << m_tracks.size() << std::endl;
    return !m_tracks.empty();
}

void AudioSystem::setMasterEnabled(bool enabled)
{
    if (m_masterEnabled == enabled)
        return;

    m_masterEnabled = enabled;
    if (enabled)
    {
        playAll();
    }
    else
    {
        stopAll();
    }
}

void AudioSystem::playAll()
{
    if (!m_isInitialized || m_tracks.empty() || !m_masterEnabled)
        return;

    if (m_timeBasedSync)
    {
        startTimeBasedSync();
    }
    else
    {
        // Play all tracks through the bus
        for (size_t i = 0; i < m_tracks.size(); ++i)
        {
            // Apply all filters to the audio source before playing
            int filterSlot = 0;
            for (auto& [name, instance] : m_trackFilters[i].filters)
            {
                if (instance.filter)
                {
                    m_tracks[i]->setFilter(filterSlot, instance.filter.get());
                    instance.slot = filterSlot;
                    filterSlot++;
                }
            }

            // Clear any remaining filter slots
            for (int slot = filterSlot; slot < 8; ++slot)
            {
                m_tracks[i]->setFilter(slot, nullptr);
            }

            // Play the track
            m_voiceHandles[i] = m_soloud.play(*m_tracks[i]);
            m_soloud.setVolume(m_voiceHandles[i], m_trackVolumes[i]);
            
            // Set looping for all tracks
            m_tracks[i]->setLooping(true);
        }
    }
}

void AudioSystem::stopAll()
{
    if (!m_isInitialized)
        return;

    m_soloud.stopAudioSource(m_masterBus);
    m_soloud.stopAll();
    m_voiceHandles.clear();
    m_busHandle = 0;
}

void AudioSystem::pauseAll()
{
    if (!m_isInitialized)
        return;

    m_soloud.setPauseAll(true);
}

void AudioSystem::resumeAll()
{
    if (!m_isInitialized)
        return;

    m_soloud.setPauseAll(false);
}

float AudioSystem::getPlaybackPosition()
{
    if (!m_isInitialized || m_tracks.empty())
        return 0.0f;

    return m_soloud.getStreamPosition(m_voiceHandles[0]);
}

void AudioSystem::setPlaybackPosition(float position)
{
    if (!m_isInitialized || m_tracks.empty())
        return;

    if (m_timeBasedSync)
    {
        m_syncOffset = position;
        m_syncStartTime = std::chrono::steady_clock::now();
        resyncTracks();
    }
    else
    {
        // Original position setting logic
        for (size_t i = 0; i < m_tracks.size(); ++i)
        {
            m_soloud.seek(m_voiceHandles[i], position);
        }
    }
}

void AudioSystem::setTrackVolume(size_t trackIndex, float volume)
{
    if (!m_isInitialized || trackIndex >= m_trackVolumes.size())
        return;

    m_trackVolumes[trackIndex] = volume;
    auto it = m_voiceHandles.find(trackIndex);
    if (it != m_voiceHandles.end())
    {
        m_soloud.setVolume(it->second, volume);
    }
}

float AudioSystem::getTrackVolume(size_t trackIndex) const
{
    if (trackIndex >= m_trackVolumes.size())
        return 0.0f;
    return m_trackVolumes[trackIndex];
}

void AudioSystem::setFilterEnabled(size_t trackIndex, const std::string& filterName, bool enabled)
{
    if (trackIndex >= m_trackFilters.size())
        return;

    auto& trackFilter = m_trackFilters[trackIndex];
    auto it = trackFilter.filters.find(filterName);
    if (it == trackFilter.filters.end())
    {
        // Initialize the filter if it doesn't exist
        initializeFilter(trackFilter.filters[filterName], filterName);
        it = trackFilter.filters.find(filterName);
    }

    if (it != trackFilter.filters.end())
    {
        auto& instance = it->second;
        instance.enabled = enabled;
        instance.needsUpdate = true;

        // Get the current voice handle for this track
        auto voiceIt = m_voiceHandles.find(trackIndex);
        if (voiceIt != m_voiceHandles.end())
        {
            unsigned int voiceHandle = voiceIt->second;
            
            if (enabled)
            {
                // Find an available filter slot
                int slot = 0;
                while (slot < 8 && m_soloud.getFilterParameter(voiceHandle, slot, 0) != 0)
                    slot++;

                if (slot < 8)
                {
                    instance.slot = slot;
                    // Set the filter instance
                    m_soloud.setGlobalFilter(slot, instance.filter.get());
                    // Enable it and set initial parameters
                    m_soloud.setFilterParameter(voiceHandle, slot, 0, 1.0f); // Enable
                    updateFilterInstance(instance, filterName);
                    std::cout << "Applied filter " << filterName << " to track " << trackIndex << " in slot " << slot << std::endl;
                }
            }
            else
            {
                // Remove the filter from its slot
                if (instance.slot >= 0)
                {
                    m_soloud.setFilterParameter(voiceHandle, instance.slot, 0, 0.0f); // Disable the filter
                    m_soloud.setGlobalFilter(instance.slot, nullptr); // Remove the filter instance
                    instance.slot = -1;
                    std::cout << "Removed filter " << filterName << " from track " << trackIndex << std::endl;
                }
            }
        }
    }
}

void AudioSystem::setFilterParameter(size_t trackIndex, const std::string& filterName, int paramId, float value)
{
    if (trackIndex >= m_trackFilters.size())
        return;

    auto& trackFilter = m_trackFilters[trackIndex];
    auto it = trackFilter.filters.find(filterName);
    if (it != trackFilter.filters.end())
    {
        auto& instance = it->second;
        auto paramIt = instance.parameters.find(paramId);
        if (paramIt != instance.parameters.end())
        {
            float oldValue = paramIt->second.value;
            paramIt->second.value = std::clamp(value, paramIt->second.min, paramIt->second.max);
            paramIt->second.changed = true;
            instance.needsUpdate = true;

            std::cout << "Setting filter parameter - track: " << trackIndex 
                      << " filter: " << filterName 
                      << " param: " << paramId 
                      << " value: " << value << std::endl;

            if (oldValue != paramIt->second.value)
            {
                std::cout << "Parameter value changed from " << oldValue << " to " << paramIt->second.value << std::endl;
                
                // Get the current voice handle for this track
                auto voiceIt = m_voiceHandles.find(trackIndex);
                if (voiceIt != m_voiceHandles.end() && instance.slot >= 0)
                {
                    unsigned int voiceHandle = voiceIt->second;
                    // Update the filter instance first
                    updateFilterInstance(instance, filterName);
                    // Then update the parameter
                    m_soloud.setFilterParameter(voiceHandle, instance.slot, paramId + 1, paramIt->second.value);
                    std::cout << "Updated filter parameter" << std::endl;
                }
            }
            else
            {
                std::cout << "Parameter value unchanged" << std::endl;
            }
        }
    }
}

bool AudioSystem::isFilterEnabled(size_t trackIndex, const std::string& filterName) const
{
    if (!m_isInitialized || trackIndex >= m_trackFilters.size())
        return false;

    const auto& trackFilters = m_trackFilters[trackIndex];
    auto it = trackFilters.filters.find(filterName);
    return it != trackFilters.filters.end() && it->second.enabled;
}

const std::unordered_map<std::string, FilterInstance>& AudioSystem::getFilters(size_t trackIndex) const
{
    static const std::unordered_map<std::string, FilterInstance> empty;
    if (!m_isInitialized || trackIndex >= m_trackFilters.size())
        return empty;
    return m_trackFilters[trackIndex].filters;
}

void AudioSystem::updateFilterParams(size_t trackIndex)
{
    if (!m_isInitialized || trackIndex >= m_tracks.size())
    {
        std::cout << "Filter params update skipped - track " << trackIndex << " invalid" << std::endl;
        return;
    }

    std::cout << "Updating filter params for track " << trackIndex << std::endl;

    // Save current playback position if playing
    float position = 0.0f;
    auto it = m_voiceHandles.find(trackIndex);
    if (it != m_voiceHandles.end())
    {
        position = m_soloud.getStreamPosition(it->second);
        // Stop the current voice
        m_soloud.stop(it->second);
        m_voiceHandles.erase(it);
    }

    // Remove all filters from the track
    for (int slot = 0; slot < 8; ++slot)
    {
        m_tracks[trackIndex]->setFilter(slot, nullptr);
        std::cout << "Cleared track filter slot " << slot << std::endl;
    }

    // Apply enabled filters
    int filterSlot = 0;
    for (auto& [name, instance] : m_trackFilters[trackIndex].filters)
    {
        if (instance.enabled && instance.filter)
        {
            std::cout << "Applying filter " << name << " to slot " << filterSlot << std::endl;
            m_tracks[trackIndex]->setFilter(filterSlot, instance.filter.get());
            instance.slot = filterSlot;
            filterSlot++;
        }
        else if (!instance.enabled)
        {
            if (instance.slot >= 0)
            {
                m_tracks[trackIndex]->setFilter(instance.slot, nullptr);
                instance.slot = -1;
            }
        }
    }

    // Clear any remaining filter slots
    for (int slot = filterSlot; slot < 8; ++slot)
    {
        m_tracks[trackIndex]->setFilter(slot, nullptr);
    }

    // Restart the track at the previous position if it was playing
    if (m_isInitialized && m_masterBus.getActiveVoiceCount() > 0)
    {
        unsigned int handle = m_masterBus.playClocked(m_soloud.getStreamPosition(0), *m_tracks[trackIndex], m_trackVolumes[trackIndex]);
        m_voiceHandles[trackIndex] = handle;
        m_soloud.seek(handle, position);
        // Fade parameters for all enabled filters
        for (auto& [name, instance] : m_trackFilters[trackIndex].filters)
        {
            if (instance.enabled && instance.filter && instance.slot >= 0)
            {
                for (const auto& [paramId, param] : instance.parameters)
                {
                    m_soloud.fadeFilterParameter(handle, static_cast<unsigned int>(instance.slot), 
                                               static_cast<unsigned int>(paramId), 
                                               param.value, 
                                               FILTER_PARAM_TRANSITION_TIME);
                }
            }
        }
    }
}

void AudioSystem::setBusFilterEnabled(const std::string& filterName, bool enabled)
{
    auto it = m_busFilters.find(filterName);
    if (it == m_busFilters.end())
    {
        FilterInstance instance;
        initializeFilter(instance, filterName);
        if (instance.filter)
        {
            instance.enabled = enabled;
            m_busFilters[filterName] = std::move(instance);
            updateBusFilterParams();
        }
    }
    else if (it->second.enabled != enabled)
    {
        it->second.enabled = enabled;
        updateBusFilterParams();
    }
}

bool AudioSystem::isBusFilterEnabled(const std::string& filterName) const
{
    auto it = m_busFilters.find(filterName);
    return it != m_busFilters.end() && it->second.enabled;
}

void AudioSystem::setBusFilterParameter(const std::string& filterName, int paramId, float value)
{
    auto it = m_busFilters.find(filterName);
    if (it == m_busFilters.end())
    {
        FilterInstance instance;
        initializeFilter(instance, filterName);
        if (instance.filter)
        {
            instance.enabled = true;  // Enable the filter by default
            m_busFilters[filterName] = std::move(instance);
            it = m_busFilters.find(filterName);
        }
        else
        {
            std::cout << "Failed to initialize filter: " << filterName << std::endl;
            return;
        }
    }

    auto& instance = it->second;
    auto paramIt = instance.parameters.find(paramId);
    if (paramIt != instance.parameters.end())
    {
        auto& param = paramIt->second;
        if (param.value != value)
        {
            std::cout << "Setting bus filter parameter - filter: " << filterName 
                      << " param: " << paramId 
                      << " value: " << value << std::endl;
            
            // Get the bus handle
            if (m_busHandle && instance.slot >= 0)
            {
                // Use SoLoud's built-in parameter fading
                m_soloud.fadeFilterParameter(m_busHandle, static_cast<unsigned int>(instance.slot), 
                                          static_cast<unsigned int>(paramId), value, FILTER_PARAM_TRANSITION_TIME);
            }
            
            // Store the new value
            param.value = value;
            param.changed = true;
            instance.needsUpdate = true;
            instance.enabled = true;  // Ensure filter is enabled when parameters change
            
            // Update the filter instance parameters
            updateFilterInstance(instance, filterName);
        }
    }
    else
    {
        std::cout << "Parameter " << paramId << " not found in bus filter " << filterName << std::endl;
    }
}

float AudioSystem::getBusFilterParameter(const std::string& filterName, int paramId)
{
    auto it = m_busFilters.find(filterName);
    if (it == m_busFilters.end() || !it->second.enabled)
        return 0.0f;

    if (m_busHandle)
    {
        return m_soloud.getFilterParameter(m_busHandle, it->second.slot, paramId);
    }
    return 0.0f;
}

const std::unordered_map<std::string, FilterInstance>& AudioSystem::getBusFilters() const
{
    return m_busFilters;
}

void AudioSystem::updateBusFilterParams()
{
    std::cout << "Updating bus filter parameters" << std::endl;

    // SAFETY CHECK: Only clear filters if bus handle is valid
    if (m_busHandle == 0) {
        std::cout << "Bus handle invalid, skipping filter update" << std::endl;
        return;
    }

    // Remove all filters from the bus - but do it safely
    for (int slot = 0; slot < 8; ++slot)
    {
        try {
            m_masterBus.setFilter(slot, nullptr);
            std::cout << "Cleared bus filter slot " << slot << std::endl;
        } catch (...) {
            std::cout << "Failed to clear bus filter slot " << slot << std::endl;
        }
    }

    // Apply enabled filters
    int filterSlot = 0;
    for (auto& [name, instance] : m_busFilters)
    {
        if (instance.enabled && instance.filter)
        {
            // Only set the filter on the bus
            m_masterBus.setFilter(filterSlot, instance.filter.get());
            // Fade each parameter to its value
            if (m_busHandle)
            {
                for (const auto& [paramId, param] : instance.parameters)
                {
                    m_soloud.fadeFilterParameter(m_busHandle, static_cast<unsigned int>(filterSlot), static_cast<unsigned int>(paramId), param.value, FILTER_PARAM_TRANSITION_TIME);
                }
            }
            instance.slot = filterSlot;  // Store the slot number
            filterSlot++;
        }
        else if (!instance.enabled && instance.slot >= 0)
        {
            // Remove filter if it was previously enabled
            m_masterBus.setFilter(instance.slot, nullptr);
            instance.slot = -1;
        }
    }

    // Clear any remaining filter slots
    for (int slot = filterSlot; slot < 8; ++slot)
    {
        m_masterBus.setFilter(slot, nullptr);
    }
}

void AudioSystem::setBusVolume(float volume)
{
    std::cout << "[AudioSystem] Setting bus volume to: " << volume << std::endl;
    m_busVolume = volume;
    m_masterBus.setVolume(volume);
    if (m_busHandle)
        m_soloud.setVolume(m_busHandle, volume);
}

float AudioSystem::getLongestTrackLength() const
{
    if (!m_isInitialized || m_tracks.empty())
        return 0.0f;

    float longestLength = 0.0f;
    for (const auto& track : m_tracks)
    {
        float length = track->getLength();
        if (length > longestLength)
            longestLength = length;
    }
    return longestLength;
}

float AudioSystem::getShortestTrackLength() const
{
    if (m_tracks.empty())
        return 0.0f;

    float shortestLength = std::numeric_limits<float>::max();
    for (size_t i = 0; i < m_tracks.size(); ++i)
    {
        float length = m_tracks[i]->getLength();
        if (length < shortestLength)
        {
            shortestLength = length;
            m_shortestTrackIndex = i;
        }
    }
    return shortestLength;
}

void AudioSystem::startTimeBasedSync()
{
    if (m_timeBasedSync)
        return;

    m_timeBasedSync = true;
    m_syncStartTime = std::chrono::steady_clock::now();
    m_syncOffset = 0.0f;

    // Find the shortest track to use as reference
    getShortestTrackLength();

    // Start all tracks
    playAll();
}

void AudioSystem::stopTimeBasedSync()
{
    m_timeBasedSync = false;
}

void AudioSystem::updateTimeBasedSync()
{
    if (!m_timeBasedSync || m_tracks.empty())
        return;

    auto currentTime = std::chrono::steady_clock::now();
    float elapsedTime = std::chrono::duration<float>(currentTime - m_syncStartTime).count() + m_syncOffset;

    // Get the reference position from the shortest track
    float referencePosition = m_soloud.getStreamPosition(m_voiceHandles[m_shortestTrackIndex]);
    float targetPosition = elapsedTime;

    // Check if we need to resync
    if (std::abs(referencePosition - targetPosition) > SYNC_THRESHOLD)
    {
        resyncTracks();
    }
}

float AudioSystem::getNearestKeyframe(float position) const
{
    // Round to nearest keyframe interval
    return std::round(position / KEYFRAME_INTERVAL) * KEYFRAME_INTERVAL;
}

void AudioSystem::fastForwardToPosition(size_t trackIndex, float targetPosition)
{
    if (trackIndex >= m_tracks.size())
        return;

    // Get the nearest keyframe to avoid audio glitches
    float keyframePosition = getNearestKeyframe(targetPosition);
    
    // Stop the current playback
    m_soloud.stop(m_voiceHandles[trackIndex]);
    
    // Set the new position using seek
    m_soloud.seek(m_voiceHandles[trackIndex], keyframePosition);
    
    // Restart playback
    m_voiceHandles[trackIndex] = m_soloud.play(*m_tracks[trackIndex]);
    m_soloud.setVolume(m_voiceHandles[trackIndex], m_trackVolumes[trackIndex]);
}

void AudioSystem::resyncTracks()
{
    if (!m_timeBasedSync || m_tracks.empty())
        return;

    auto currentTime = std::chrono::steady_clock::now();
    float elapsedTime = std::chrono::duration<float>(currentTime - m_syncStartTime).count() + m_syncOffset;
    float targetPosition = getNearestKeyframe(elapsedTime);

    // Update all tracks to the target position
    for (size_t i = 0; i < m_tracks.size(); ++i)
    {
        if (i != m_shortestTrackIndex) // Skip the reference track
        {
            fastForwardToPosition(i, targetPosition);
        }
    }

    // Update the sync offset to account for any drift
    m_syncOffset = targetPosition - elapsedTime;
}

float AudioSystem::getFilterParameter(size_t trackIndex, const std::string& filterName, int paramIndex)
{
    if (trackIndex >= m_trackFilters.size())
        return 0.0f;

    const auto& trackFilter = m_trackFilters[trackIndex];
    auto it = trackFilter.filters.find(filterName);
    if (it == trackFilter.filters.end() || !it->second.enabled)
        return 0.0f;

    auto voiceIt = m_voiceHandles.find(trackIndex);
    if (voiceIt == m_voiceHandles.end())
        return 0.0f;

    return m_soloud.getFilterParameter(voiceIt->second, it->second.slot, paramIndex);
}