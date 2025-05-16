#include "AudioSystem.h"
#include <iostream>

const std::vector<std::string> AudioSystem::AVAILABLE_FILTERS = {
    "biquad", "echo", "lofi", "flanger", "dcremoval", "bassboost", "waveshaper", "robotize", "freeverb"
};

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

    SoLoud::result result = m_soloud.init();
    if (result != SoLoud::SO_NO_ERROR)
    {
        std::cerr << "Failed to initialize SoLoud: " << result << std::endl;
        return false;
    }

    m_isInitialized = true;
    return true;
}

void AudioSystem::initializeFilter(FilterInstance& instance, const std::string& filterName)
{
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
        // Initialize parameters with their ranges
        int paramCount = instance.filter->getParamCount();
        for (int i = 0; i < paramCount; ++i)
        {
            FilterParameter param;
            param.name = instance.filter->getParamName(i);
            param.min = instance.filter->getParamMin(i);
            param.max = instance.filter->getParamMax(i);
            param.value = (param.min + param.max) * 0.5f; // Default to middle of range
            instance.parameters[i] = param;
        }
    }
}

void AudioSystem::updateFilterInstance(FilterInstance& instance, const std::string& filterName)
{
    if (!instance.filter || !instance.enabled || !instance.needsUpdate)
        return;

    // Update all changed parameters for the correct filter type
    if (filterName == "biquad")
    {
        auto* f = dynamic_cast<SoLoud::BiquadResonantFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;
            float p2 = instance.parameters[1].value;
            float p3 = instance.parameters[2].value;
            f->setParams(p1, p2, p3);
        }
    }
    else if (filterName == "echo")
    {
        auto* f = dynamic_cast<SoLoud::EchoFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;
            float p2 = instance.parameters[1].value;
            f->setParams(p1, p2);
        }
    }
    else if (filterName == "lofi")
    {
        auto* f = dynamic_cast<SoLoud::LofiFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;
            float p2 = instance.parameters[1].value;
            f->setParams(p1, p2);
        }
    }
    else if (filterName == "flanger")
    {
        auto* f = dynamic_cast<SoLoud::FlangerFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;
            float p2 = instance.parameters[1].value;
            f->setParams(p1, p2);
        }
    }
    else if (filterName == "dcremoval")
    {
        auto* f = dynamic_cast<SoLoud::DCRemovalFilter*>(instance.filter.get());
        if (f)
        {
            f->setParams(); // No params
        }
    }
    else if (filterName == "bassboost")
    {
        auto* f = dynamic_cast<SoLoud::BassboostFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;
            f->setParams(p1);
        }
    }
    else if (filterName == "waveshaper")
    {
        auto* f = dynamic_cast<SoLoud::WaveShaperFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;
            f->setParams(p1);
        }
    }
    else if (filterName == "robotize")
    {
        auto* f = dynamic_cast<SoLoud::RobotizeFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;
            float p2 = instance.parameters[1].value;
            f->setParams(p1, p2);
        }
    }
    else if (filterName == "freeverb")
    {
        auto* f = dynamic_cast<SoLoud::FreeverbFilter*>(instance.filter.get());
        if (f)
        {
            float p1 = instance.parameters[0].value;
            float p2 = instance.parameters[1].value;
            float p3 = instance.parameters[2].value;
            f->setParams(0.0f, p1, p2, p3);
        }
    }

    // Mark all parameters as not changed
    for (auto& [paramId, param] : instance.parameters)
        param.changed = false;
    instance.needsUpdate = false;
}

bool AudioSystem::loadDirectory(const std::filesystem::path& directory)
{
    if (!m_isInitialized)
        return false;

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
            
            if (ext == ".wav" || ext == ".flac")
            {
                auto wav = std::make_unique<SoLoud::Wav>();
                SoLoud::result result = wav->load(entry.path().string().c_str());
                if (result == SoLoud::SO_NO_ERROR)
                {
                    m_tracks.push_back(std::move(wav));
                    m_trackVolumes.push_back(1.0f);
                    m_trackFilters.push_back(TrackFilters());
                }
            }
        }
    }

    return !m_tracks.empty();
}

void AudioSystem::playAll()
{
    if (!m_isInitialized)
        return;

    stopAll();

    for (size_t i = 0; i < m_tracks.size(); ++i)
    {
        // Remove all filters first
        for (int slot = 0; slot < 8; ++slot)
            m_tracks[i]->setFilter(slot, nullptr);

        // Apply enabled filters
        int filterSlot = 0;
        for (auto& [name, instance] : m_trackFilters[i].filters)
        {
            if (instance.enabled && instance.filter)
            {
                updateFilterInstance(instance, name);
                m_tracks[i]->setFilter(filterSlot++, instance.filter.get());
            }
        }

        unsigned int handle = m_soloud.play(*m_tracks[i], m_trackVolumes[i]);
        m_voiceHandles[i] = handle;
    }
}

void AudioSystem::stopAll()
{
    if (!m_isInitialized)
        return;

    m_soloud.stopAll();
    m_voiceHandles.clear();
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
    if (!m_isInitialized)
        return;

    for (const auto& handle : m_voiceHandles)
    {
        m_soloud.seek(handle.second, position);
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

void AudioSystem::setFilterParameter(size_t trackIndex, const std::string& filterName, int paramId, float value)
{
    if (!m_isInitialized || trackIndex >= m_trackFilters.size())
        return;

    auto& trackFilters = m_trackFilters[trackIndex];
    auto it = trackFilters.filters.find(filterName);
    if (it == trackFilters.filters.end())
    {
        // Initialize the filter if it doesn't exist
        FilterInstance instance;
        initializeFilter(instance, filterName);
        if (instance.filter)
        {
            trackFilters.filters[filterName] = std::move(instance);
            it = trackFilters.filters.find(filterName);
        }
        else
            return;
    }

    auto& instance = it->second;
    auto paramIt = instance.parameters.find(paramId);
    if (paramIt != instance.parameters.end())
    {
        auto& param = paramIt->second;
        if (param.value != value)
        {
            param.value = value;
            param.changed = true;
            instance.needsUpdate = true;
            updateFilterParams(trackIndex);
        }
    }
}

float AudioSystem::getFilterParameter(size_t trackIndex, const std::string& filterName, int paramId) const
{
    if (!m_isInitialized || trackIndex >= m_trackFilters.size())
        return 0.0f;

    const auto& trackFilters = m_trackFilters[trackIndex];
    auto it = trackFilters.filters.find(filterName);
    if (it != trackFilters.filters.end())
    {
        const auto& instance = it->second;
        auto paramIt = instance.parameters.find(paramId);
        if (paramIt != instance.parameters.end())
            return paramIt->second.value;
    }
    return 0.0f;
}

void AudioSystem::setFilterEnabled(size_t trackIndex, const std::string& filterName, bool enabled)
{
    if (!m_isInitialized || trackIndex >= m_trackFilters.size())
        return;

    auto& trackFilters = m_trackFilters[trackIndex];
    auto it = trackFilters.filters.find(filterName);
    if (it == trackFilters.filters.end())
    {
        // Initialize the filter if it doesn't exist
        FilterInstance instance;
        initializeFilter(instance, filterName);
        if (instance.filter)
        {
            instance.enabled = enabled;
            trackFilters.filters[filterName] = std::move(instance);
            updateFilterParams(trackIndex);
        }
    }
    else if (it->second.enabled != enabled)
    {
        it->second.enabled = enabled;
        updateFilterParams(trackIndex);
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
        return;

    // Remove all filters first
    for (int slot = 0; slot < 8; ++slot)
        m_tracks[trackIndex]->setFilter(slot, nullptr);

    // Apply enabled filters
    int filterSlot = 0;
    for (auto& [name, instance] : m_trackFilters[trackIndex].filters)
    {
        if (instance.enabled && instance.filter)
        {
            updateFilterInstance(instance, name);
            m_tracks[trackIndex]->setFilter(filterSlot++, instance.filter.get());
        }
    }

    // If the track is currently playing, we need to restart it to apply the new filters
    auto it = m_voiceHandles.find(trackIndex);
    if (it != m_voiceHandles.end())
    {
        float currentPos = m_soloud.getStreamPosition(it->second);
        m_soloud.stop(it->second);
        unsigned int handle = m_soloud.play(*m_tracks[trackIndex], m_trackVolumes[trackIndex]);
        m_soloud.seek(handle, currentPos);
        m_voiceHandles[trackIndex] = handle;
    }
}