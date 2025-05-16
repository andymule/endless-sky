#include <iostream>
#include <thread>
#include <chrono>
#include "AdaptiveMusicLib.h"
#include "../extern/soloud/include/soloud.h"
#include "../extern/soloud/include/soloud_freeverbfilter.h"

// Define SoLoud filter parameter indices based on SoLoud documentation
namespace FilterParamIndex {
    // FreeverbFilter
    const unsigned int FREEVERB_WET = 0;
    const unsigned int FREEVERB_ROOM_SIZE = 1;
    const unsigned int FREEVERB_DAMP = 2;
    const unsigned int FREEVERB_WIDTH = 3;
}

int main()
{
    std::cout << "AdaptiveMusicLib Direct Stem Effect Test Program" << std::endl;
    
    // Create audio library instance
    AdaptiveMusicLib audio;
    
    // Initialize the audio system
    std::cout << "Initializing audio system..." << std::endl;
    if (!audio.Initialize())
    {
        std::cerr << "Failed to initialize audio system" << std::endl;
        return 1;
    }
    
    // Load stems directly
    std::cout << "Loading stems directly..." << std::endl;
    if (!audio.PlayStemsFromDirectory("mainmenu", 0.8f))
    {
        std::cerr << "Failed to play stems. Trying alternative path..." << std::endl;
        if (!audio.PlayStemsFromDirectory("../sounds/music/mainmenu", 0.8f))
        {
            std::cerr << "Failed to play stems with alternative path" << std::endl;
            return 1;
        }
    }
    
    // Access SoLoud engine directly
    SoLoud::Soloud* soloud = (SoLoud::Soloud*)audio.GetSoloudEngine();
    if (!soloud)
    {
        std::cerr << "Failed to get SoLoud engine" << std::endl;
        return 1;
    }
    
    // Create the filter
    SoLoud::FreeverbFilter reverb;
    reverb.setParams(1.0f, 1.0f, 0.0f, 1.0f);
    
    // For direct stem panning
    int numStems = audio.GetStemCount();
    std::vector<unsigned int> stemHandles;
    
    // Get all stems handles
    for (int i = 0; i < numStems; i++)
    {
        // The handle list is private but we know the stem indices 0 to numStems-1
        unsigned int handle = audio.GetStemHandle(i);
        if (handle)
        {
            stemHandles.push_back(handle);
        }
    }
    
    // Let it play for a few seconds without effects
    std::cout << "Playing with no effects for 5 seconds..." << std::endl;
    auto startTime = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::steady_clock::now() - startTime).count() < 5)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Apply pan to all stems directly (not using a bus)
    std::cout << "Panning all stems to the right..." << std::endl;
    for (unsigned int handle : stemHandles)
    {
        if (soloud->isValidVoiceHandle(handle))
        {
            soloud->setPan(handle, 1.0f); // 1.0 = full right
        }
    }
    
    // Let it play with panning for a few seconds
    std::cout << "Playing with pan right for 5 seconds..." << std::endl;
    startTime = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::steady_clock::now() - startTime).count() < 5)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Apply global reverb filter
    std::cout << "Adding reverb as global filter..." << std::endl;
    soloud->setGlobalFilter(0, &reverb);
    
    // Let it play with panning and reverb
    std::cout << "Playing with pan right + reverb for 10 seconds..." << std::endl;
    startTime = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::steady_clock::now() - startTime).count() < 10)
    {
        audio.Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Apply pan to the other side
    std::cout << "Panning all stems to the left..." << std::endl;
    for (unsigned int handle : stemHandles)
    {
        if (soloud->isValidVoiceHandle(handle))
        {
            soloud->setPan(handle, -1.0f); // -1.0 = full left
        }
    }
    
    // Let it play with opposite panning and reverb
    std::cout << "Playing with pan left + reverb for 10 seconds..." << std::endl;
    startTime = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::steady_clock::now() - startTime).count() < 10)
    {
        audio.Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Stop music
    std::cout << "Stopping music..." << std::endl;
    audio.StopMusic();
    
    std::cout << "Test completed successfully" << std::endl;
    return 0;
} 