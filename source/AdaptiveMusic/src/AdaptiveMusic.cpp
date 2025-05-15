#include "../include/AdaptiveMusic.h"

// Include SoLoud headers for direct integration
#include "soloud.h"
#include "soloud_wav.h"  // Using Wav instead of WavStream to avoid stb_vorbis dependency

// Include Logger for consistent logging
#include "../../Logger.h"

#include <filesystem>
#include <string>

AdaptiveMusic::AdaptiveMusic()
    : mSoloud(nullptr), mMusic(nullptr), mMusicHandle(0), mInitialized(false)
{
}

AdaptiveMusic::~AdaptiveMusic()
{
    // Clean up resources
    StopMusic();
    
    if(mMusic)
    {
        delete mMusic;
        mMusic = nullptr;
    }
    
    if(mSoloud)
    {
        mSoloud->deinit();
        delete mSoloud;
        mSoloud = nullptr;
    }
}

bool AdaptiveMusic::Initialize()
{
    if(mInitialized)
        return true;
    
    Logger::LogError("AdaptiveMusic: Initializing audio system");
    
    mSoloud = new SoLoud::Soloud();
    if(!mSoloud)
        return false;
    
    int result = mSoloud->init(SoLoud::Soloud::CLIP_ROUNDOFF);
    if(result != SoLoud::SO_NO_ERROR)
    {
        Logger::LogError("AdaptiveMusic: Failed to initialize SoLoud");
        delete mSoloud;
        mSoloud = nullptr;
        return false;
    }
    
    mInitialized = true;
    return true;
}

bool AdaptiveMusic::IsInitialized() const
{
    return mInitialized;
}

bool AdaptiveMusic::LoadMusic(const std::string& filename)
{
    if(!mInitialized || !mSoloud)
    {
        Logger::LogError("AdaptiveMusic: Not initialized");
        return false;
    }
    
    // Check if file exists
    if(!std::filesystem::exists(filename))
    {
        Logger::LogError("AdaptiveMusic: File not found: " + filename);
        return false;
    }
    
    // Cleanup previous music if any
    if(mMusic)
    {
        delete mMusic;
        mMusic = nullptr;
    }
    
    // Load the music file
    mMusic = new SoLoud::Wav();
    if(!mMusic)
        return false;
    
    int result = mMusic->load(filename.c_str());
    if(result != SoLoud::SO_NO_ERROR)
    {
        Logger::LogError("AdaptiveMusic: Failed to load music file: " + filename);
        delete mMusic;
        mMusic = nullptr;
        return false;
    }
    
    mCurrentMusic = filename;
    Logger::LogError("AdaptiveMusic: Loaded music file: " + filename);
    return true;
}

bool AdaptiveMusic::PlayMusicLooped(float volume)
{
    if(!mInitialized || !mSoloud || !mMusic)
    {
        Logger::LogError("AdaptiveMusic: Not initialized or no music loaded");
        return false;
    }
    
    // Set looping
    mMusic->setLooping(true);
    
    // Play the music
    mMusicHandle = mSoloud->play(*mMusic, volume);
    
    if(mSoloud->isValidVoiceHandle(mMusicHandle))
    {
        Logger::LogError("AdaptiveMusic: Playing music");
        return true;
    }
    
    Logger::LogError("AdaptiveMusic: Failed to play music");
    return false;
}

void AdaptiveMusic::StopMusic()
{
    if(mInitialized && mSoloud && mSoloud->isValidVoiceHandle(mMusicHandle))
    {
        mSoloud->stop(mMusicHandle);
        mMusicHandle = 0;
    }
}

void AdaptiveMusic::SetVolume(float volume)
{
    if(mInitialized && mSoloud && mSoloud->isValidVoiceHandle(mMusicHandle))
    {
        mSoloud->setVolume(mMusicHandle, volume);
    }
}

void AdaptiveMusic::PauseMusic()
{
    if(mInitialized && mSoloud && mSoloud->isValidVoiceHandle(mMusicHandle))
    {
        mSoloud->setPause(mMusicHandle, true);
    }
}

void AdaptiveMusic::ResumeMusic()
{
    if(mInitialized && mSoloud && mSoloud->isValidVoiceHandle(mMusicHandle))
    {
        mSoloud->setPause(mMusicHandle, false);
    }
}

void AdaptiveMusic::Update()
{
    // No updates needed for simple playback
} 