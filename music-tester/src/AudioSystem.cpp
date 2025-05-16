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

    // Load all audio files from the directory
    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.is_regular_file())
        {
            auto extension = entry.path().extension().string();
            // Convert to lowercase for comparison
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            if (extension == ".wav" || extension == ".flac" || extension == ".ogg" ||
                extension == ".mp3")
            {
                auto track = std::make_unique<SoLoud::Wav>();
                SoLoud::result result = track->load(entry.path().string().c_str());
                if (result == SoLoud::SO_NO_ERROR)
                {
                    m_tracks.push_back(std::move(track));
                    std::cout << "Loaded: " << entry.path().filename() << std::endl;
                }
                else
                {
                    std::cerr << "Failed to load: " << entry.path().filename()
                              << " (Error: " << result << ")" << std::endl;
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

    // Play all tracks
    for (const auto& track : m_tracks)
    {
        m_soloud.play(*track);
    }
}

void AudioSystem::stopAll()
{
    if (!m_isInitialized)
    {
        return;
    }

    m_soloud.stopAll();
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