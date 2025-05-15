#pragma once

#include <string>
#include <vector>
#include <map>
#include <filesystem>

// Forward declarations for SoLoud
namespace SoLoud {
    class Soloud;
    class WavStream;
    class Bus;
    class Filter;
    class BiquadResonantFilter;
    class EchoFilter;
    class LofiFilter;
    class FlangerFilter;
    class DCRemovalFilter;
    class FFTFilter;
    class BassboostFilter;
    class WaveShaperFilter;
    class RobotizeFilter;
    class FreeverbFilter;
    
    // Define filter types here to avoid including the full header
    namespace BiquadResonantFilterType {
        enum {
            LOWPASS = 0,
            HIGHPASS = 1,
            BANDPASS = 2
        };
    }
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
    
    // Set the panning of the stem bus (-1 = left, 0 = center, 1 = right)
    void SetStemBusPan(unsigned int busHandle, float pan);
    
    // Get number of loaded stems
    int GetStemCount() const;
    
    // Update function to call each frame (if needed)
    void Update();
    
    // Create a unified bus for all stems and return the handle
    unsigned int CreateStemBus(float volume = 1.0f);
    
    // Add filters to the bus
    void AddBiquadResonantFilter(unsigned int busHandle, float frequency = 1000.0f, float resonance = 2.0f, int filterType = 0);
    void AddEchoFilter(unsigned int busHandle, float delay = 0.3f, float decay = 0.7f, float filter = 0.0f);
    void AddLofiFilter(unsigned int busHandle, float sampleRate = 8000.0f, float bitDepth = 3.0f);
    void AddFlangerFilter(unsigned int busHandle, float delay = 0.005f, float frequency = 10.0f);
    void AddDCRemovalFilter(unsigned int busHandle);
    void AddFFTFilter(unsigned int busHandle);
    void AddBassboostFilter(unsigned int busHandle, float boost = 2.0f);
    void AddWaveShaperFilter(unsigned int busHandle, float amount = 1.0f);
    void AddRobotizeFilter(unsigned int busHandle, float frequency = 50.0f, int waveform = 0);
    void AddFreeverbFilter(unsigned int busHandle, float mix = 0.5f, float roomSize = 0.5f, float damp = 0.5f, float width = 1.0f);
    
    // Set filter parameters
    void SetFreeverbMix(unsigned int busHandle, float mix);
    void SetBiquadFilterFrequency(unsigned int busHandle, float frequency);
    void SetBassboostBoost(unsigned int busHandle, float boost);
    void SetFlangerParameters(unsigned int busHandle, float delay, float frequency);
    void SetLofiParameters(unsigned int busHandle, float sampleRate, float bitDepth);
    
    // Direct SoLoud fade/oscillate methods
    
    // Oscillate the pan of the stem bus
    void OscillateStemBusPan(unsigned int busHandle, float minPan, float maxPan, float frequency);
    
    // Oscillate filter parameters directly using SoLoud's oscillator
    void OscillateFilterParameter(unsigned int busHandle, int filterIndex, unsigned int parameterIndex, float minValue, float maxValue, float frequency);
    
    // Older custom methods (can be replaced with direct SoLoud methods)
    void FadeFreeverbMix(unsigned int busHandle, float startMix, float endMix, float durationSeconds);
    void OscillateFreeverbMix(unsigned int busHandle, float minMix, float maxMix, float frequency);
    void OscillateBiquadFilterFrequency(unsigned int busHandle, float minFreq, float maxFreq, float frequency);
    
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
    
    // Bus for effect processing
    SoLoud::Bus* mStemBus;
    unsigned int mStemBusHandle;
    
    // Effect filters
    SoLoud::BiquadResonantFilter* mBiquadFilter;
    SoLoud::EchoFilter* mEchoFilter;
    SoLoud::LofiFilter* mLofiFilter;
    SoLoud::FlangerFilter* mFlangerFilter;
    SoLoud::DCRemovalFilter* mDCFilter;
    SoLoud::FFTFilter* mFFTFilter;
    SoLoud::BassboostFilter* mBassboostFilter;
    SoLoud::WaveShaperFilter* mWaveShaperFilter;
    SoLoud::RobotizeFilter* mRobotizeFilter;
    SoLoud::FreeverbFilter* mFreeverbFilter;
    
    // Fade data
    struct FadeData {
        unsigned int busHandle;
        float startValue;
        float endValue;
        float duration;
        float elapsedTime;
        bool active;
    };
    FadeData mFreeverbFade;
    
    // Oscillator data
    struct OscillatorData {
        unsigned int busHandle;
        float minValue;
        float maxValue;
        float frequency;
        float time;
        bool active;
    };
    OscillatorData mFreeverbOsc;
    OscillatorData mBiquadOsc;
}; 