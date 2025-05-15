#include <iostream>
#include <thread>
#include <chrono>
#include "AdaptiveMusicLib.h"

int main()
{
    std::cout << "AdaptiveMusicLib Test Program" << std::endl;
    
    // Create audio library instance
    AdaptiveMusicLib audio;
    
    // Initialize the audio system
    std::cout << "Initializing audio system..." << std::endl;
    if (!audio.Initialize())
    {
        std::cerr << "Failed to initialize audio system" << std::endl;
        return 1;
    }
    
    // Play stems from the directory
    std::cout << "Loading and playing music stems..." << std::endl;
    if (!audio.PlayStemsFromDirectory("mainmenu", 0.5f))
    {
        std::cerr << "Failed to play stems. Trying alternative path..." << std::endl;
        // Try with full path as fallback
        if (!audio.PlayStemsFromDirectory("../sounds/music/mainmenu", 0.5f))
        {
            std::cerr << "Failed to play stems with alternative path" << std::endl;
            return 1;
        }
    }
    
    // Wait for 5 seconds
    std::cout << "Playing music for 5 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Stop music
    std::cout << "Stopping music..." << std::endl;
    audio.StopMusic();
    
    std::cout << "Test completed successfully" << std::endl;
    return 0;
} 