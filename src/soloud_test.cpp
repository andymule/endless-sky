#include <iostream>
#include <thread>
#include <chrono>

// Include SoLoud header
#include "soloud.h"
#include "soloud_wav.h"

int main() {
    // Create SoLoud engine object
    SoLoud::Soloud soloud;
    
    // Create a wav object
    SoLoud::Wav sample;
    
    try {
        // Initialize SoLoud with the OpenAL backend
        SoLoud::result result = soloud.init(SoLoud::Soloud::CLIP_ROUNDOFF, SoLoud::Soloud::OPENAL);
        
        if (result != SoLoud::SO_NO_ERROR) {
            std::cerr << "SoLoud initialization error: " << result << std::endl;
            return 1;
        }
        
        std::cout << "SoLoud initialized successfully using OpenAL backend." << std::endl;
        
        // Load the alarm sound
        if (sample.load("alarm.wav") != SoLoud::SO_NO_ERROR) {
            std::cerr << "Error loading sound file" << std::endl;
            soloud.deinit();
            return 1;
        }
        
        std::cout << "Sound file loaded successfully." << std::endl;
        
        // Play the sound
        int handle = soloud.play(sample);
        std::cout << "Playing alarm sound..." << std::endl;
        
        // Wait while the sound is playing
        while (soloud.isValidVoiceHandle(handle) && soloud.getVoiceCount() > 0) {
            std::cout << "Sound playing... (press Ctrl+C to stop)" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // Cleanup
        soloud.deinit();
        std::cout << "SoLoud deinitialized." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 