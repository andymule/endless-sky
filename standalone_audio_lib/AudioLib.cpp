#include "AudioLib.h"
#include <iostream>

// Include SoLoud headers
#include "../extern/soloud/include/soloud.h"
#include "../extern/soloud/include/soloud_wavstream.h"

AudioLib::AudioLib()
    : mSoloud(nullptr), mMusic(nullptr), mMusicHandle(0), mInitialized(false)
{
}

AudioLib::~AudioLib()
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

bool AudioLib::Initialize()
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
    
    mInitialized = true;
    return true;
}

bool AudioLib::PlayMusic(const std::string& filename, float volume)
{
    if(!mInitialized || !mSoloud)
        return false;
    
    // Clean up previous music if any
    if(mMusic)
    {
        delete mMusic;
        mMusic = nullptr;
    }
    
    // Create and load the music
    mMusic = new SoLoud::WavStream();
    if(!mMusic)
        return false;
    
    // Set looping
    mMusic->setLooping(true);
    
    // Load the music file
    int result = mMusic->load(filename.c_str());
    if(result != SoLoud::SO_NO_ERROR)
    {
        delete mMusic;
        mMusic = nullptr;
        return false;
    }
    
    // Play the music
    mMusicHandle = mSoloud->play(*mMusic, volume);
    
    return mSoloud->isValidVoiceHandle(mMusicHandle);
}

void AudioLib::StopMusic()
{
    if(mInitialized && mSoloud && mSoloud->isValidVoiceHandle(mMusicHandle))
    {
        mSoloud->stop(mMusicHandle);
        mMusicHandle = 0;
    }
}

void AudioLib::PauseMusic()
{
    if(mInitialized && mSoloud && mSoloud->isValidVoiceHandle(mMusicHandle))
    {
        mSoloud->setPause(mMusicHandle, true);
    }
}

void AudioLib::ResumeMusic()
{
    if(mInitialized && mSoloud && mSoloud->isValidVoiceHandle(mMusicHandle))
    {
        mSoloud->setPause(mMusicHandle, false);
    }
}

void AudioLib::SetVolume(float volume)
{
    if(mInitialized && mSoloud && mSoloud->isValidVoiceHandle(mMusicHandle))
    {
        mSoloud->setVolume(mMusicHandle, volume);
    }
}

void AudioLib::Update()
{
    // Nothing needed for basic functionality
} 