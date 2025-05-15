#include "../include/AdaptiveMusic.h"

// Include SoLoud headers
#include "../../../extern/soloud/include/soloud.h"
#include "../../../extern/soloud/include/soloud_wav.h"

// Include SDL for initialization (already included in Endless Sky)
#include <SDL.h>

#include <iostream>

AdaptiveMusic::AdaptiveMusic()
    : mEngine(new SoLoud::Soloud())
    , mMusic(new SoLoud::Wav())
    , mMusicHandle(-1)
    , mInitialized(false)
{
}

AdaptiveMusic::~AdaptiveMusic()
{
    // Cleanup resources
    if (mInitialized)
    {
        mEngine->deinit();
        
        // Don't quit SDL here as Endless Sky manages its lifecycle
    }
}

bool AdaptiveMusic::Initialize()
{
    if (mInitialized)
        return true;
    
    // Don't initialize SDL here as Endless Sky already does it
    
    // Initialize SoLoud
    auto result = mEngine->init();
    if (result != SoLoud::SO_NO_ERROR)
    {
        std::cerr << "Failed to initialize SoLoud: " << result << std::endl;
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
    if (!mInitialized)
    {
        std::cerr << "AdaptiveMusic not initialized" << std::endl;
        return false;
    }
    
    // Load the music file
    auto result = mMusic->load(filename.c_str());
    if (result != SoLoud::SO_NO_ERROR)
    {
        std::cerr << "Failed to load music file '" << filename << "': " << result << std::endl;
        return false;
    }
    
    // Set the music to loop by default
    mMusic->setLooping(true);
    
    return true;
}

bool AdaptiveMusic::PlayMusicLooped(float volume)
{
    if (!mInitialized)
    {
        std::cerr << "AdaptiveMusic not initialized" << std::endl;
        return false;
    }
    
    // Stop any currently playing music
    if (mMusicHandle != -1 && mEngine->isValidVoiceHandle(mMusicHandle))
    {
        mEngine->stop(mMusicHandle);
    }
    
    // Play the music with looping
    mMusicHandle = mEngine->play(*mMusic, volume);
    
    return mEngine->isValidVoiceHandle(mMusicHandle);
}

void AdaptiveMusic::StopMusic()
{
    if (mInitialized && mMusicHandle != -1 && mEngine->isValidVoiceHandle(mMusicHandle))
    {
        mEngine->stop(mMusicHandle);
        mMusicHandle = -1;
    }
}

void AdaptiveMusic::SetVolume(float volume)
{
    if (mInitialized && mMusicHandle != -1 && mEngine->isValidVoiceHandle(mMusicHandle))
    {
        mEngine->setVolume(mMusicHandle, volume);
    }
}

void AdaptiveMusic::PauseMusic()
{
    if (mInitialized && mMusicHandle != -1 && mEngine->isValidVoiceHandle(mMusicHandle))
    {
        mEngine->setPause(mMusicHandle, true);
    }
}

void AdaptiveMusic::ResumeMusic()
{
    if (mInitialized && mMusicHandle != -1 && mEngine->isValidVoiceHandle(mMusicHandle))
    {
        mEngine->setPause(mMusicHandle, false);
    }
}

void AdaptiveMusic::Update()
{
    // This method can be used for future adaptive music features
    // like crossfading between tracks or dynamic adjustments
} 