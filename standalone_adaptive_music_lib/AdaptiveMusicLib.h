#pragma once

#include <string>
#include <vector>
#include <map>
#include <filesystem>

// Forward declarations for SoLoud
namespace SoLoud {
    class Soloud;
    class WavStream;
}

// Add alias for filesystem
namespace fs = std::filesystem;

class AdaptiveMusicLib {
public:
    AdaptiveMusicLib();
    ~AdaptiveMusicLib();
    
    // Initialize the audio engine
    bool Initialize();
    
    // Load and play a single music file in a loop
    bool PlayMusic(const std::string& filename, float volume = 1.0f);
    
    // Load and play multiple music stems synchronized
    bool PlayMultiStemMusic(const std::vector<std::string>& filenames, float volume = 1.0f);
    
    // Load and play stems from a directory
    bool PlayStemsFromDirectory(const std::string& directory, float volume = 1.0f);
    
    // Stop all currently playing music
    void StopMusic();
    
    // Pause all music
    void PauseMusic();
    
    // Resume all music
    void ResumeMusic();
    
    // Mute/unmute a specific stem (by index)
    void MuteStem(int stemIndex, bool mute = true);
    
    // Solo a specific stem (mute all others)
    void SoloStem(int stemIndex);
    
    // Set the volume of all music
    void SetVolume(float volume);
    
    // Set the volume of a specific stem
    void SetStemVolume(int stemIndex, float volume);
    
    // Get number of loaded stems
    int GetStemCount() const;
    
    // Update function to call each frame (if needed)
    void Update();
    
private:
    // Clear all current music resources
    void ClearMusic();
    
    // Helper method to check if a directory exists
    bool DirectoryExists(const std::string& path) const;

private:
    SoLoud::Soloud* mSoloud;
    std::vector<SoLoud::WavStream*> mStems;
    std::vector<unsigned int> mStemHandles;
    std::vector<float> mStemVolumes;
    float mMasterVolume;
    bool mInitialized;
}; 