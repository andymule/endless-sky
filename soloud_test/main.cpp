#include <iostream>
#include <thread>
#include <chrono>
#include <string>

// Include SDL2 header with correct paths for different platforms
#ifdef __APPLE__
// For macOS with Homebrew, SDL2 headers are at /opt/homebrew/include/SDL2
#include <SDL.h>  // When included this way, CMake's include path will find it
#elif defined(_WIN32)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

// Include SoLoud headers with correct path
#include "../extern/soloud/include/soloud.h"
#include "../extern/soloud/include/soloud_wav.h"

// Helper function to convert SoLoud error codes to strings
std::string soLoudErrorToString(SoLoud::result error) {
    switch (error) {
        case SoLoud::INVALID_PARAMETER: return "Invalid parameter";
        case SoLoud::FILE_NOT_FOUND: return "File not found";
        case SoLoud::FILE_LOAD_FAILED: return "File load failed";
        case SoLoud::DLL_NOT_FOUND: return "DLL not found";
        case SoLoud::OUT_OF_MEMORY: return "Out of memory";
        case SoLoud::NOT_IMPLEMENTED: return "Not implemented";
        case SoLoud::UNKNOWN_ERROR: return "Unknown error";
        default: return "Error code " + std::to_string(error);
    }
}

int main(int argc, char* argv[]) {
    // Print startup message
    std::cout << "SoLoud Test Program\n" 
              << "-------------------\n" 
              << "Initializing SDL2 audio subsystem..." << std::endl;
    
    // Initialize SDL2 (required for SDL2 static backend)
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL2 initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    std::cout << "SDL2 initialized successfully." << std::endl;
    
    // Create SoLoud engine object
    SoLoud::Soloud soloud;
    
    // Create a wav object
    SoLoud::Wav sample;
    
    try {
        // Initialize SoLoud (will use SDL2 static backend)
        std::cout << "Initializing SoLoud..." << std::endl;
        SoLoud::result result = soloud.init();
        
        if (result != SoLoud::SO_NO_ERROR) {
            std::cerr << "SoLoud initialization failed: " << soLoudErrorToString(result) << std::endl;
            SDL_Quit();
            return 1;
        }
        
        std::cout << "SoLoud initialized successfully with backend: " 
                  << soloud.getBackendString() << std::endl;
        
        // Print audio info
        unsigned int sampleRate = soloud.getBackendSamplerate();
        std::cout << "Backend sample rate: " << sampleRate << " Hz" << std::endl;
        
        // Try to load the alarm sound
        std::cout << "Loading alarm.wav..." << std::endl;
        result = sample.load("alarm.wav");
        if (result != SoLoud::SO_NO_ERROR) {
            std::cerr << "Failed to load sound file: " << soLoudErrorToString(result) << std::endl;
            soloud.deinit();
            SDL_Quit();
            return 1;
        }
        
        std::cout << "Sound file loaded successfully." << std::endl;
        
        // Play the sound
        std::cout << "Playing alarm sound..." << std::endl;
        int handle = soloud.play(sample, 1.0f);
        
        if (!soloud.isValidVoiceHandle(handle)) {
            std::cerr << "Failed to play sound." << std::endl;
            soloud.deinit();
            SDL_Quit();
            return 1;
        }
        
        // Wait while the sound is playing (max 5 seconds)
        std::cout << "Waiting for sound to finish (max 5 seconds)..." << std::endl;
        int counter = 0;
        while (soloud.isValidVoiceHandle(handle) && counter < 5) {
            std::cout << "Sound playing... (" << counter + 1 << "/5)" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            counter++;
        }
        
        // Cleanup
        std::cout << "Cleaning up resources..." << std::endl;
        soloud.deinit();
        std::cout << "SoLoud deinitialized." << std::endl;
        
        // Cleanup SDL
        SDL_Quit();
        std::cout << "SDL2 deinitialized." << std::endl;
        
        std::cout << "Test completed successfully!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    return 0;
} 