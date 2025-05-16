#include "AudioSystem.h"
#include <iostream>

AudioSystem::AudioSystem() : m_isInitialized(false) {}

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
    {
        return true;
    }

    // Initialize SoLoud with SDL2 backend
    SoLoud::result result = m_soloud.init(SoLoud::Soloud::CLIP_ROUNDOFF, SoLoud::Soloud::SDL2);
    if (result != SoLoud::SO_NO_ERROR)
    {
        std::cerr << "Failed to initialize SoLoud: " << result << std::endl;
        return false;
    }

    // Set up the master bus
    m_masterBus.setVolume(1.0f);
    m_soloud.setGlobalVolume(1.0f);

    m_isInitialized = true;
    return true;
}

bool AudioSystem::loadDirectory(const std::filesystem::path& directory)
{
    if (!m_isInitialized)
    {
        std::cerr << "Audio system not initialized" << std::endl;
        return false;
    }

    // Clear existing tracks
    m_tracks.clear();
    m_trackVolumes.clear(); // Clear track volumes

    // Load all audio files from the directory
    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.is_regular_file())
        {
            auto extension = entry.path().extension().string();
            if (extension == ".wav" || extension == ".ogg" || extension == ".mp3" ||
                extension == ".flac")
            {
                auto track = std::make_unique<SoLoud::Wav>();
                if (track->load(entry.path().string().c_str()) == SoLoud::SO_NO_ERROR)
                {
                    m_tracks.push_back(std::move(track));
                    m_trackVolumes.push_back(1.0f); // Initialize volume to 1.0
                }
            }
        }
    }

    return !m_tracks.empty();
}

void AudioSystem::playAll()
{
    if (!m_isInitialized || m_tracks.empty())
    {
        return;
    }

    // Clear existing voice handles
    m_voiceHandles.clear();

    // Play all tracks and store their voice handles
    for (size_t i = 0; i < m_tracks.size(); ++i)
    {
        unsigned int handle = m_soloud.play(*m_tracks[i]);
        m_voiceHandles[i] = handle;
    }
}

void AudioSystem::stopAll()
{
    if (!m_isInitialized)
    {
        return;
    }

    m_soloud.stopAll();
    m_voiceHandles.clear();
}

void AudioSystem::pauseAll()
{
    if (!m_isInitialized)
    {
        return;
    }

    m_soloud.setPauseAll(true);
}

void AudioSystem::resumeAll()
{
    if (!m_isInitialized)
    {
        return;
    }

    m_soloud.setPauseAll(false);
}

float AudioSystem::getPlaybackPosition()
{
    if (!m_isInitialized || m_tracks.empty())
    {
        return 0.0f;
    }

    // Get the position from the first track
    return m_soloud.getStreamPosition(m_soloud.getActiveVoiceCount() - 1);
}

void AudioSystem::setPlaybackPosition(float position)
{
    if (!m_isInitialized || m_tracks.empty())
    {
        return;
    }

    // Set position for all tracks
    for (size_t i = 0; i < m_soloud.getActiveVoiceCount(); ++i)
    {
        m_soloud.seek(m_soloud.getActiveVoiceCount() - 1 - i, position);
    }
}

void AudioSystem::setTrackVolume(size_t trackIndex, float volume)
{
    if (!m_isInitialized || trackIndex >= m_tracks.size())
    {
        return;
    }

    // Clamp volume between 0 and 1
    volume = std::max(0.0f, std::min(1.0f, volume));
    m_trackVolumes[trackIndex] = volume;

    // Set volume on the Wav object for future playback
    if (m_tracks[trackIndex])
    {
        m_tracks[trackIndex]->setVolume(volume);
    }

    // Set volume on the currently playing instance if it exists
    auto it = m_voiceHandles.find(trackIndex);
    if (it != m_voiceHandles.end())
    {
        m_soloud.setVolume(it->second, volume);
    }
}

float AudioSystem::getTrackVolume(size_t trackIndex) const
{
    if (!m_isInitialized || trackIndex >= m_trackVolumes.size())
    {
        return 0.0f;
    }
    return m_trackVolumes[trackIndex];
}