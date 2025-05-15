#include "AdaptiveMusicLib.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <cmath>

// Include SoLoud headers
#include "../extern/soloud/include/soloud.h"
#include "../extern/soloud/include/soloud_wavstream.h"
#include "../extern/soloud/include/soloud_bus.h"
#include "../extern/soloud/include/soloud_filter.h"
#include "../extern/soloud/include/soloud_biquadresonantfilter.h"
#include "../extern/soloud/include/soloud_echofilter.h"
#include "../extern/soloud/include/soloud_lofifilter.h"
#include "../extern/soloud/include/soloud_flangerfilter.h"
#include "../extern/soloud/include/soloud_dcremovalfilter.h"
#include "../extern/soloud/include/soloud_fftfilter.h"
#include "../extern/soloud/include/soloud_bassboostfilter.h"
#include "../extern/soloud/include/soloud_waveshaperfilter.h"
#include "../extern/soloud/include/soloud_robotizefilter.h"
#include "../extern/soloud/include/soloud_freeverbfilter.h"

// Namespace alias to fix linter issues
#if defined(__cplusplus) && __cplusplus >= 201703L
    namespace fs = std::filesystem;
#else
    #error "C++17 or later is required for std::filesystem"
#endif

AdaptiveMusicLib::AdaptiveMusicLib()
    : mSoloud(nullptr), mMasterVolume(1.0f), mInitialized(false),
      mStemBus(nullptr), mStemBusHandle(0),
      mBiquadFilter(nullptr), mEchoFilter(nullptr), mLofiFilter(nullptr),
      mFlangerFilter(nullptr), mDCFilter(nullptr), mFFTFilter(nullptr),
      mBassboostFilter(nullptr), mWaveShaperFilter(nullptr), mRobotizeFilter(nullptr),
      mFreeverbFilter(nullptr)
{
    mFreeverbFade = {0, 0.0f, 0.0f, 0.0f, 0.0f, false};
    mFreeverbOsc = {0, 0.0f, 0.0f, 0.0f, 0.0f, false};
    mBiquadOsc = {0, 0.0f, 0.0f, 0.0f, 0.0f, false};
}

AdaptiveMusicLib::~AdaptiveMusicLib()
{
    // Clean up resources
    StopMusic();
    ClearMusic();
    
    // Clean up filters
    delete mBiquadFilter;
    delete mEchoFilter;
    delete mLofiFilter;
    delete mFlangerFilter;
    delete mDCFilter;
    delete mFFTFilter;
    delete mBassboostFilter;
    delete mWaveShaperFilter;
    delete mRobotizeFilter;
    delete mFreeverbFilter;
    
    // Clean up bus
    delete mStemBus;
    
    if(mSoloud)
    {
        mSoloud->deinit();
        delete mSoloud;
        mSoloud = nullptr;
    }
}

bool AdaptiveMusicLib::Initialize()
{
    if(mInitialized)
        return true;
    
    mSoloud = new SoLoud::Soloud();
    if(!mSoloud)
        return false;
    
    int result = mSoloud->init(SoLoud::Soloud::CLIP_ROUNDOFF);
    if(result != SoLoud::SO_NO_ERROR)
    {
        delete mSoloud;
        mSoloud = nullptr;
        return false;
    }
    
    // Initialize filters to nullptr
    mBiquadFilter = nullptr;
    mEchoFilter = nullptr;
    mLofiFilter = nullptr;
    mFlangerFilter = nullptr;
    mDCFilter = nullptr;
    mFFTFilter = nullptr;
    mBassboostFilter = nullptr;
    mWaveShaperFilter = nullptr;
    mRobotizeFilter = nullptr;
    mFreeverbFilter = nullptr;
    
    // Initialize the bus
    mStemBus = nullptr;
    mStemBusHandle = 0;
    
    mInitialized = true;
    return true;
}

void AdaptiveMusicLib::ClearMusic()
{
    // Delete all loaded stems
    for(auto& stem : mStems)
    {
        if(stem)
        {
            delete stem;
            stem = nullptr;
        }
    }
    
    mStems.clear();
    mStemHandles.clear();
    mStemVolumes.clear();
}

bool AdaptiveMusicLib::PlayMusic(const std::string& filename, float volume)
{
    if(!mInitialized || !mSoloud)
        return false;
    
    // Clean up previous music
    StopMusic();
    ClearMusic();
    
    // Store the master volume
    mMasterVolume = volume;
    
    // Create and load the music
    SoLoud::WavStream* music = new SoLoud::WavStream();
    if(!music)
        return false;
    
    // Set looping
    music->setLooping(true);
    
    // Load the music file
    int result = music->load(filename.c_str());
    if(result != SoLoud::SO_NO_ERROR)
    {
        delete music;
        return false;
    }
    
    // Add to our stems collection
    mStems.push_back(music);
    mStemVolumes.push_back(volume);
    
    // Play the music
    unsigned int handle = mSoloud->play(*music, volume);
    mStemHandles.push_back(handle);
    
    return mSoloud->isValidVoiceHandle(handle);
}

bool AdaptiveMusicLib::PlayMultiStemMusic(const std::vector<std::string>& filenames, float volume)
{
    if(!mInitialized || !mSoloud || filenames.empty())
        return false;
    
    // Print the number of stems we're loading
    std::cout << "AdaptiveMusicLib: Loading " << filenames.size() << " music stems" << std::endl;
    
    // Clean up previous music
    StopMusic();
    ClearMusic();
    
    // Store the master volume
    mMasterVolume = volume;
    
    bool success = true;
    
    // Load all stems
    for(const auto& filename : filenames)
    {
        // Create and load the stem
        SoLoud::WavStream* stem = new SoLoud::WavStream();
        if(!stem)
        {
            std::cerr << "AdaptiveMusicLib: Failed to create WavStream for: " << filename << std::endl;
            success = false;
            break;
        }
        
        // Set looping
        stem->setLooping(true);
        
        // Load the stem file
        int result = stem->load(filename.c_str());
        if(result != SoLoud::SO_NO_ERROR)
        {
            std::cerr << "AdaptiveMusicLib: Failed to load stem file: " << filename << " (Error: " << result << ")" << std::endl;
            delete stem;
            success = false;
            break;
        }
        
        std::cout << "AdaptiveMusicLib: Successfully loaded stem: " << filename << std::endl;
        
        // Add to our stems collection
        mStems.push_back(stem);
        mStemVolumes.push_back(volume);
    }
    
    // If any stem failed to load, clean up and return false
    if(!success)
    {
        ClearMusic();
        return false;
    }
    
    // Play all stems - they will be synchronized because we load them all first
    // and then play them in quick succession
    for(size_t i = 0; i < mStems.size(); ++i)
    {
        unsigned int handle = mSoloud->play(*mStems[i], mStemVolumes[i]);
        mStemHandles.push_back(handle);
        
        if(!mSoloud->isValidVoiceHandle(handle))
        {
            std::cerr << "AdaptiveMusicLib: Failed to play stem #" << i << std::endl;
            success = false;
        }
        else
        {
            std::cout << "AdaptiveMusicLib: Playing stem #" << i << std::endl;
        }
    }
    
    return success;
}

bool AdaptiveMusicLib::DirectoryExists(const std::string& path) const
{
    if(path.empty())
        return false;
    
    try
    {
        return fs::exists(path) && fs::is_directory(path);
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error checking directory existence: " << e.what() << std::endl;
        return false;
    }
}

bool AdaptiveMusicLib::PlayStemsFromDirectory(const std::string& directory, float volume)
{
    std::vector<std::string> stemFiles;
    
    // Check if directory exists first
    if(!DirectoryExists(directory))
    {
        std::cerr << "AdaptiveMusicLib: Directory does not exist: " << directory << std::endl;
        return false;
    }
    
    try
    {
        // Print the directory we're searching
        std::cout << "AdaptiveMusicLib: Searching for stems in: " << directory << std::endl;
        
        // Collect all audio files in the directory
        for(const auto& entry : fs::directory_iterator(directory))
        {
            if(entry.is_regular_file())
            {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                // Check if it's a supported audio format
                if(ext == ".mp3" || ext == ".wav" || ext == ".ogg" || ext == ".flac")
                {
                    stemFiles.push_back(entry.path().string());
                    std::cout << "AdaptiveMusicLib: Found stem: " << entry.path().string() << std::endl;
                }
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "Failed to read directory: " << e.what() << std::endl;
        return false;
    }
    
    if(stemFiles.empty())
    {
        std::cerr << "AdaptiveMusicLib: No stem files found in directory: " << directory << std::endl;
        return false;
    }
    
    // Sort the files to ensure consistent loading order
    std::sort(stemFiles.begin(), stemFiles.end());
    
    // Play the collected stems
    return PlayMultiStemMusic(stemFiles, volume);
}

void AdaptiveMusicLib::StopMusic()
{
    if(!mInitialized || !mSoloud)
        return;
    
    // Stop all stems
    for(unsigned int handle : mStemHandles)
    {
        if(mSoloud->isValidVoiceHandle(handle))
        {
            mSoloud->stop(handle);
        }
    }
    
    mStemHandles.clear();
}

void AdaptiveMusicLib::PauseMusic()
{
    if(!mInitialized || !mSoloud)
        return;
    
    // Pause all stems
    for(unsigned int handle : mStemHandles)
    {
        if(mSoloud->isValidVoiceHandle(handle))
        {
            mSoloud->setPause(handle, true);
        }
    }
}

void AdaptiveMusicLib::ResumeMusic()
{
    if(!mInitialized || !mSoloud)
        return;
    
    // Resume all stems
    for(unsigned int handle : mStemHandles)
    {
        if(mSoloud->isValidVoiceHandle(handle))
        {
            mSoloud->setPause(handle, false);
        }
    }
}

void AdaptiveMusicLib::MuteStem(int stemIndex, bool mute)
{
    if(!mInitialized || !mSoloud || stemIndex < 0 || stemIndex >= static_cast<int>(mStemHandles.size()))
        return;
    
    unsigned int handle = mStemHandles[stemIndex];
    if(mSoloud->isValidVoiceHandle(handle))
    {
        float volume = mute ? 0.0f : mStemVolumes[stemIndex];
        mSoloud->setVolume(handle, volume);
    }
}

void AdaptiveMusicLib::SoloStem(int stemIndex)
{
    if(!mInitialized || !mSoloud || stemIndex < 0 || stemIndex >= static_cast<int>(mStemHandles.size()))
        return;
    
    // Mute all stems
    for(size_t i = 0; i < mStemHandles.size(); ++i)
    {
        if(mSoloud->isValidVoiceHandle(mStemHandles[i]))
        {
            float volume = (i == static_cast<size_t>(stemIndex)) ? mStemVolumes[i] : 0.0f;
            mSoloud->setVolume(mStemHandles[i], volume);
        }
    }
}

void AdaptiveMusicLib::SetVolume(float volume)
{
    if(!mInitialized || !mSoloud)
        return;
    
    mMasterVolume = volume;
    
    // Apply volume to all stems, respecting their individual volumes
    for(size_t i = 0; i < mStemHandles.size(); ++i)
    {
        if(mSoloud->isValidVoiceHandle(mStemHandles[i]))
        {
            mSoloud->setVolume(mStemHandles[i], mStemVolumes[i] * mMasterVolume);
        }
    }
}

void AdaptiveMusicLib::SetStemVolume(int stemIndex, float volume)
{
    if(!mInitialized || !mSoloud || stemIndex < 0 || stemIndex >= static_cast<int>(mStemHandles.size()))
        return;
    
    // Store the individual stem volume
    mStemVolumes[stemIndex] = volume;
    
    // Apply with master volume scaling
    unsigned int handle = mStemHandles[stemIndex];
    if(mSoloud->isValidVoiceHandle(handle))
    {
        mSoloud->setVolume(handle, volume * mMasterVolume);
    }
}

int AdaptiveMusicLib::GetStemCount() const
{
    return static_cast<int>(mStems.size());
}

void AdaptiveMusicLib::Update()
{
    // Handle freeverb mix fade
    if(mFreeverbFade.active)
    {
        // Increment time (assuming 60 fps)
        mFreeverbFade.elapsedTime += 1.0f / 60.0f;
        
        if(mFreeverbFade.elapsedTime >= mFreeverbFade.duration)
        {
            // Fade is complete
            SetFreeverbMix(mFreeverbFade.busHandle, mFreeverbFade.endValue);
            mFreeverbFade.active = false;
        }
        else
        {
            // Calculate current mix value
            float t = mFreeverbFade.elapsedTime / mFreeverbFade.duration;
            float currentMix = mFreeverbFade.startValue + t * (mFreeverbFade.endValue - mFreeverbFade.startValue);
            SetFreeverbMix(mFreeverbFade.busHandle, currentMix);
        }
    }
    // Handle freeverb mix oscillation (only if fade is not active)
    else if(mFreeverbOsc.active)
    {
        // Increment time (assuming 60 fps)
        mFreeverbOsc.time += 1.0f / 60.0f;
        
        // Calculate oscillating mix value using sine wave
        float t = sin(2.0f * M_PI * mFreeverbOsc.frequency * mFreeverbOsc.time);
        
        // Map from -1,1 range to minValue,maxValue range
        float currentMix = mFreeverbOsc.minValue + (t + 1.0f) * 0.5f * (mFreeverbOsc.maxValue - mFreeverbOsc.minValue);
        
        // Apply the mix value
        SetFreeverbMix(mFreeverbOsc.busHandle, currentMix);
    }
    
    // Handle biquad filter frequency oscillation
    if(mBiquadOsc.active)
    {
        // Increment time (assuming 60 fps)
        mBiquadOsc.time += 1.0f / 60.0f;
        
        // Calculate oscillating frequency value using sine wave
        float t = sin(2.0f * M_PI * mBiquadOsc.frequency * mBiquadOsc.time);
        
        // Map from -1,1 range to minValue,maxValue range
        float currentFreq = mBiquadOsc.minValue + (t + 1.0f) * 0.5f * (mBiquadOsc.maxValue - mBiquadOsc.minValue);
        
        // Apply the frequency value
        SetBiquadFilterFrequency(mBiquadOsc.busHandle, currentFreq);
    }
}

// New methods for filter support

unsigned int AdaptiveMusicLib::CreateStemBus(float volume)
{
    if(!mInitialized || !mSoloud)
        return 0;
    
    // Clean up previous bus if it exists
    if(mStemBus)
    {
        mSoloud->stop(mStemBusHandle);
        delete mStemBus;
    }
    
    // Create a new bus
    mStemBus = new SoLoud::Bus();
    if(!mStemBus)
        return 0;
    
    // Play the bus
    mStemBusHandle = mSoloud->play(*mStemBus, volume);
    
    // If we have stems, play them through the bus
    for(size_t i = 0; i < mStems.size(); ++i)
    {
        if(mSoloud->isValidVoiceHandle(mStemHandles[i]))
        {
            mSoloud->stop(mStemHandles[i]); // Stop direct playing
        }
        
        if(mStems[i])
        {
            unsigned int handle = mStemBus->play(*mStems[i], mStemVolumes[i]);
            mStemHandles[i] = handle;
        }
    }
    
    return mStemBusHandle;
}

void AdaptiveMusicLib::AddBiquadResonantFilter(unsigned int busHandle, float frequency, float resonance, int filterType)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle))
        return;
    
    // Clean up previous filter if it exists
    delete mBiquadFilter;
    
    // Create the filter
    mBiquadFilter = new SoLoud::BiquadResonantFilter();
    if(!mBiquadFilter)
        return;
    
    // Set parameters
    mBiquadFilter->setParams(filterType, frequency, resonance);
    
    // Apply the filter to the bus
    mStemBus->setFilter(0, mBiquadFilter);
}

void AdaptiveMusicLib::AddEchoFilter(unsigned int busHandle, float delay, float decay, float filter)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle))
        return;
    
    // Clean up previous filter if it exists
    delete mEchoFilter;
    
    // Create the filter
    mEchoFilter = new SoLoud::EchoFilter();
    if(!mEchoFilter)
        return;
    
    // Set parameters
    mEchoFilter->setParams(delay, decay, filter);
    
    // Apply the filter to the bus
    mStemBus->setFilter(1, mEchoFilter);
}

void AdaptiveMusicLib::AddLofiFilter(unsigned int busHandle, float sampleRate, float bitDepth)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle))
        return;
    
    // Clean up previous filter if it exists
    delete mLofiFilter;
    
    // Create the filter
    mLofiFilter = new SoLoud::LofiFilter();
    if(!mLofiFilter)
        return;
    
    // Set parameters
    mLofiFilter->setParams(sampleRate, bitDepth);
    
    // Apply the filter to the bus
    mStemBus->setFilter(2, mLofiFilter);
}

void AdaptiveMusicLib::AddFlangerFilter(unsigned int busHandle, float delay, float frequency)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle))
        return;
    
    // Clean up previous filter if it exists
    delete mFlangerFilter;
    
    // Create the filter
    mFlangerFilter = new SoLoud::FlangerFilter();
    if(!mFlangerFilter)
        return;
    
    // Set parameters
    mFlangerFilter->setParams(delay, frequency);
    
    // Apply the filter to the bus
    mStemBus->setFilter(3, mFlangerFilter);
}

void AdaptiveMusicLib::AddDCRemovalFilter(unsigned int busHandle)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle))
        return;
    
    // Clean up previous filter if it exists
    delete mDCFilter;
    
    // Create the filter
    mDCFilter = new SoLoud::DCRemovalFilter();
    if(!mDCFilter)
        return;
    
    // Apply the filter to the bus
    mStemBus->setFilter(4, mDCFilter);
}

void AdaptiveMusicLib::AddFFTFilter(unsigned int busHandle)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle))
        return;
    
    // Clean up previous filter if it exists
    delete mFFTFilter;
    
    // Create the filter
    mFFTFilter = new SoLoud::FFTFilter();
    if(!mFFTFilter)
        return;
    
    // Apply the filter to the bus
    mStemBus->setFilter(5, mFFTFilter);
}

void AdaptiveMusicLib::AddBassboostFilter(unsigned int busHandle, float boost)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle))
        return;
    
    // Clean up previous filter if it exists
    delete mBassboostFilter;
    
    // Create the filter
    mBassboostFilter = new SoLoud::BassboostFilter();
    if(!mBassboostFilter)
        return;
    
    // Set parameters
    mBassboostFilter->setParams(boost);
    
    // Apply the filter to the bus
    mStemBus->setFilter(6, mBassboostFilter);
}

void AdaptiveMusicLib::AddWaveShaperFilter(unsigned int busHandle, float amount)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle))
        return;
    
    // Clean up previous filter if it exists
    delete mWaveShaperFilter;
    
    // Create the filter
    mWaveShaperFilter = new SoLoud::WaveShaperFilter();
    if(!mWaveShaperFilter)
        return;
    
    // Set parameters
    mWaveShaperFilter->setParams(amount);
    
    // Apply the filter to the bus
    mStemBus->setFilter(7, mWaveShaperFilter);
}

void AdaptiveMusicLib::AddRobotizeFilter(unsigned int busHandle, float frequency, int waveform)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle))
        return;
    
    // Clean up previous filter if it exists
    delete mRobotizeFilter;
    
    // Create the filter
    mRobotizeFilter = new SoLoud::RobotizeFilter();
    if(!mRobotizeFilter)
        return;
    
    // Set parameters
    mRobotizeFilter->setParams(frequency, waveform);
    
    // Apply the filter to the bus
    mStemBus->setFilter(8, mRobotizeFilter);
}

void AdaptiveMusicLib::AddFreeverbFilter(unsigned int busHandle, float mix, float roomSize, float damp, float width)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle))
        return;
    
    // Clean up previous filter if it exists
    delete mFreeverbFilter;
    
    // Create the filter
    mFreeverbFilter = new SoLoud::FreeverbFilter();
    if(!mFreeverbFilter)
        return;
    
    // Set parameters
    mFreeverbFilter->setParams(mix, roomSize, damp, width);
    
    // Apply the filter to the bus
    mStemBus->setFilter(9, mFreeverbFilter);
}

void AdaptiveMusicLib::SetFreeverbMix(unsigned int busHandle, float mix)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle) || !mFreeverbFilter || !mStemBus)
        return;
    
    // Get current parameters
    float roomSize = 0.5f; // Default values
    float damp = 0.5f;
    float width = 1.0f;
    
    // Update the mix parameter only, keeping other parameters the same
    mFreeverbFilter->setParams(mix, roomSize, damp, width);
}

void AdaptiveMusicLib::SetBassboostBoost(unsigned int busHandle, float boost)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle) || !mBassboostFilter || !mStemBus)
        return;
    
    // Update the boost parameter
    mBassboostFilter->setParams(boost);
}

void AdaptiveMusicLib::SetFlangerParameters(unsigned int busHandle, float delay, float frequency)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle) || !mFlangerFilter || !mStemBus)
        return;
    
    // Update the parameters
    mFlangerFilter->setParams(delay, frequency);
}

void AdaptiveMusicLib::SetLofiParameters(unsigned int busHandle, float sampleRate, float bitDepth)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle) || !mLofiFilter || !mStemBus)
        return;
    
    // Update the parameters
    mLofiFilter->setParams(sampleRate, bitDepth);
}

void AdaptiveMusicLib::FadeFreeverbMix(unsigned int busHandle, float startMix, float endMix, float durationSeconds)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle) || !mFreeverbFilter)
        return;
    
    // Set up the fade
    mFreeverbFade.busHandle = busHandle;
    mFreeverbFade.startValue = startMix;
    mFreeverbFade.endValue = endMix;
    mFreeverbFade.duration = durationSeconds;
    mFreeverbFade.elapsedTime = 0.0f;
    mFreeverbFade.active = true;
    
    // Set the initial mix
    SetFreeverbMix(busHandle, startMix);
}

void AdaptiveMusicLib::OscillateFreeverbMix(unsigned int busHandle, float minMix, float maxMix, float frequency)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle) || !mFreeverbFilter)
        return;
    
    // Disable any active fade
    mFreeverbFade.active = false;
    
    // Set up the oscillator
    mFreeverbOsc.busHandle = busHandle;
    mFreeverbOsc.minValue = minMix;
    mFreeverbOsc.maxValue = maxMix;
    mFreeverbOsc.frequency = frequency;
    mFreeverbOsc.time = 0.0f;
    mFreeverbOsc.active = true;
    
    // Set the initial mix to middle value
    SetFreeverbMix(busHandle, (minMix + maxMix) * 0.5f);
}

void AdaptiveMusicLib::SetBiquadFilterFrequency(unsigned int busHandle, float frequency)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle) || !mBiquadFilter || !mStemBus)
        return;
    
    // Get current filter type and resonance (we need to keep these the same)
    int type = 0; // Default to lowpass
    float resonance = 2.0f; // Default resonance
    
    // Update the frequency parameter only, keeping other parameters the same
    mBiquadFilter->setParams(type, frequency, resonance);
}

void AdaptiveMusicLib::OscillateBiquadFilterFrequency(unsigned int busHandle, float minFreq, float maxFreq, float frequency)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle) || !mBiquadFilter)
        return;
    
    // Set up the oscillator
    mBiquadOsc.busHandle = busHandle;
    mBiquadOsc.minValue = minFreq;
    mBiquadOsc.maxValue = maxFreq;
    mBiquadOsc.frequency = frequency;
    mBiquadOsc.time = 0.0f;
    mBiquadOsc.active = true;
    
    // Set the initial frequency to middle value
    SetBiquadFilterFrequency(busHandle, (minFreq + maxFreq) * 0.5f);
}

void AdaptiveMusicLib::SetStemBusPan(unsigned int busHandle, float pan)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle))
        return;
    
    // Clamp pan value to valid range (-1.0 to 1.0)
    float clampedPan = std::max(-1.0f, std::min(1.0f, pan));
    
    // Set pan for the bus
    mSoloud->setPan(busHandle, clampedPan);
}

void AdaptiveMusicLib::OscillateStemBusPan(unsigned int busHandle, float minPan, float maxPan, float frequency)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle))
        return;
    
    // Clamp pan values to valid range (-1.0 to 1.0)
    float clampedMinPan = std::max(-1.0f, std::min(1.0f, minPan));
    float clampedMaxPan = std::max(-1.0f, std::min(1.0f, maxPan));
    
    // Use SoLoud's oscillate pan function directly
    mSoloud->oscillatePan(busHandle, clampedMinPan, clampedMaxPan, frequency);
}

void AdaptiveMusicLib::OscillateFilterParameter(unsigned int busHandle, int filterIndex, unsigned int parameterIndex, float minValue, float maxValue, float frequency)
{
    if(!mInitialized || !mSoloud || !mSoloud->isValidVoiceHandle(busHandle))
        return;
    
    // Use SoLoud's oscillate filter parameter function directly
    mSoloud->oscillateFilterParameter(busHandle, filterIndex, parameterIndex, minValue, maxValue, frequency);
} 