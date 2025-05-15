#include <iostream>
#include <thread>
#include <chrono>
#include "AdaptiveMusicLib.h"

// Define SoLoud filter parameter indices based on SoLoud documentation
namespace FilterParamIndex {
    // BiquadResonantFilter
    const unsigned int BIQUAD_TYPE = 0;
    const unsigned int BIQUAD_FREQUENCY = 1;
    const unsigned int BIQUAD_RESONANCE = 2;
    
    // FreeverbFilter
    const unsigned int FREEVERB_WET = 0;
    const unsigned int FREEVERB_ROOM_SIZE = 1;
    const unsigned int FREEVERB_DAMP = 2;
    const unsigned int FREEVERB_WIDTH = 3;
}

int main()
{
    std::cout << "AdaptiveMusicLib Panning + Reverb Test Program" << std::endl;
    
    // Create audio library instance
    AdaptiveMusicLib audio;
    
    // Initialize the audio system
    std::cout << "Initializing audio system..." << std::endl;
    if (!audio.Initialize())
    {
        std::cerr << "Failed to initialize audio system" << std::endl;
        return 1;
    }
    
    // Play stems directly first (no bus, no filtering)
    std::cout << "Loading and playing stems directly (without bus)..." << std::endl;
    if (!audio.PlayStemsFromDirectory("mainmenu", 0.8f))
    {
        std::cerr << "Failed to play stems. Trying alternative path..." << std::endl;
        // Try with full path as fallback
        if (!audio.PlayStemsFromDirectory("../sounds/music/mainmenu", 0.8f))
        {
            std::cerr << "Failed to play stems with alternative path" << std::endl;
            return 1;
        }
    }
    
    // Let it play for a few seconds
    std::cout << "Playing with no effects for 5 seconds..." << std::endl;
    auto startTime = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::steady_clock::now() - startTime).count() < 5)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Now create a bus and route the stems through it
    std::cout << "Creating stem bus and routing stems through it..." << std::endl;
    unsigned int busHandle = audio.CreateStemBus(1.0f); // Set to full volume
    if (!busHandle)
    {
        std::cerr << "Failed to create stem bus" << std::endl;
        return 1;
    }
    
    // Hard pan the bus to the right
    std::cout << "Hard panning bus to the right side..." << std::endl;
    audio.SetStemBusPan(busHandle, 1.0f); // 1.0 = full right
    
    // Add heavy reverb to the bus
    std::cout << "Adding maximum reverb effect..." << std::endl;
    audio.AddFreeverbFilter(busHandle, 1.0f, 1.0f, 0.5f, 1.0f);
    
    // Let it play for a while with the hard panning and reverb
    std::cout << "Playing with right pan + reverb for 10 seconds..." << std::endl;
    startTime = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::steady_clock::now() - startTime).count() < 10)
    {
        audio.Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Now switch to the left side but keep reverb
    std::cout << "Switching to left pan + reverb for 10 seconds..." << std::endl;
    audio.SetStemBusPan(busHandle, -1.0f); // -1.0 = full left
    
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