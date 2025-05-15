#pragma once

#include <string>
#include <memory>

// Forward declarations to avoid exposing SoLoud internals
namespace SoLoud {
    class Soloud;
    class Wav;
}

/**
 * AdaptiveMusic class for integrating SoLoud audio with Endless Sky.
 * This provides a simplified interface to play and control background music.
 */
class AdaptiveMusic {
public:
    /**
     * Constructor. Creates an AdaptiveMusic instance.
     */
    AdaptiveMusic();
    
    /**
     * Destructor. Cleans up all resources.
     */
    ~AdaptiveMusic();
    
    /**
     * Initialize the audio engine.
     * @return true if initialization was successful, false otherwise
     */
    bool Initialize();
    
    /**
     * Check if the audio engine is initialized.
     * @return true if initialized, false otherwise
     */
    bool IsInitialized() const;
    
    /**
     * Load a wave file.
     * @param filename The path to the wave file
     * @return true if the file was loaded successfully, false otherwise
     */
    bool LoadMusic(const std::string& filename);
    
    /**
     * Play the loaded music in a loop.
     * @param volume The volume level (0.0 to 1.0)
     * @return true if playback started successfully, false otherwise
     */
    bool PlayMusicLooped(float volume = 1.0f);
    
    /**
     * Stop the currently playing music.
     */
    void StopMusic();
    
    /**
     * Set the volume of the currently playing music.
     * @param volume The volume level (0.0 to 1.0)
     */
    void SetVolume(float volume);
    
    /**
     * Pause the currently playing music.
     */
    void PauseMusic();
    
    /**
     * Resume the paused music.
     */
    void ResumeMusic();
    
    /**
     * Update method to be called regularly
     * (e.g., once per frame in the game loop)
     */
    void Update();

private:
    // Private implementation details
    std::unique_ptr<SoLoud::Soloud> mEngine;
    std::unique_ptr<SoLoud::Wav> mMusic;
    int mMusicHandle = -1;
    bool mInitialized = false;
}; 