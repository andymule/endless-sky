#include <iostream>
#include <thread>
#include <chrono>
#include "AudioLib.h"

int main()
{
    std::cout << "AudioLib Test Program" << std::endl;
    
    // Create audio library instance
    AudioLib audio;
    
    // Initialize the audio system
    std::cout << "Initializing audio system..." << std::endl;
    if (!audio.Initialize())
    {
        std::cerr << "Failed to initialize audio system" << std::endl;
        return 1;
    }
    
    // Play music
    std::cout << "Loading and playing music..." << std::endl;
    if (!audio.PlayMusic("main_menu.mp3", 0.5f))
    {
        std::cerr << "Failed to play music" << std::endl;
        return 1;
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