#pragma once

#include <string>

// Forward declarations for SoLoud
namespace SoLoud {
    class Soloud;
    class WavStream;
}

class AudioLib {
public:
    AudioLib();
    ~AudioLib();
    
    // Initialize the audio engine
    bool Initialize();
    
    // Load and play a music file in a loop
    bool PlayMusic(const std::string& filename, float volume = 1.0f);
    
    // Stop the currently playing music
    void StopMusic();
    
    // Pause the currently playing music
    void PauseMusic();
    
    // Resume the currently playing music
    void ResumeMusic();
    
    // Set the volume of the currently playing music
    void SetVolume(float volume);
    
    // Update function to call each frame (if needed)
    void Update();
    
private:
    SoLoud::Soloud* mSoloud;
    SoLoud::WavStream* mMusic;
    unsigned int mMusicHandle;
    bool mInitialized;
}; 