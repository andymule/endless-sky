#include "AdaptiveMusicLib.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

// Include SoLoud headers
#include "../extern/soloud/include/soloud.h"
#include "../extern/soloud/include/soloud_wavstream.h"

// Namespace alias to fix linter issues
namespace fs = std::filesystem;

AdaptiveMusicLib::AdaptiveMusicLib()
    : mSoloud(nullptr), mMasterVolume(1.0f), mInitialized(false)
{
}

AdaptiveMusicLib::~AdaptiveMusicLib()
{
    // Clean up resources
    StopMusic();
    ClearMusic();
    
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
    // This space intentionally left empty - SoLoud handles real-time audio without
    // requiring an update loop. This function is kept for API compatibility.
} 