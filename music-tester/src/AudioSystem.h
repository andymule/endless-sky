#pragma once

#include "soloud.h"
#include "soloud_bus.h"
#include "soloud_wav.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class AudioSystem
{
  public:
    AudioSystem();
    ~AudioSystem();

    // Initialize the audio system
    bool initialize();

    // Load all audio files from a directory
    bool loadDirectory(const std::filesystem::path& directory);

    // Play all loaded tracks
    void playAll();

    // Stop all tracks
    void stopAll();

    // Pause all tracks
    void pauseAll();

    // Resume all tracks
    void resumeAll();

    // Get the current playback position
    float getPlaybackPosition();

    // Set the playback position
    void setPlaybackPosition(float position);

  private:
    SoLoud::Soloud m_soloud;
    SoLoud::Bus m_masterBus;
    std::vector<std::unique_ptr<SoLoud::Wav>> m_tracks;
    bool m_isInitialized;
};