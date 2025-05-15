#pragma once

#include <string>

// Include SoLoud headers
#include "soloud.h"
#include "soloud_wav.h"

/**
 * AdaptiveMusic class for integrating with Endless Sky's audio system.
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
     * Load a music file (MP3, WAV, etc.).
     * @param filename The path to the music file
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
    // SoLoud engine
    SoLoud::Soloud* mSoloud;
    
    // Music file
    SoLoud::Wav* mMusic;
    
    // Handle to the currently playing music
    unsigned int mMusicHandle;
    
    // Currently playing music file path
    std::string mCurrentMusic;
    
    // Whether the engine is initialized
    bool mInitialized;
}; 