#include "AudioController.h"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    std::cout << "=== Testing Event System ===" << std::endl;

    AudioTester::AudioController controller;

    // Initialize the controller
    if (!controller.initialize()) {
        std::cerr << "Failed to initialize controller" << std::endl;
        return 1;
    }

    // Load songs from sound_staging directory
    controller.loadSongsFromDirectory("sound_staging");

    // Display loaded songs
    const auto* songManager = controller.getSongManager();
    if (songManager) {
        const auto& songs = songManager->getSongs();
        std::cout << "\nLoaded " << songs.size() << " songs:" << std::endl;

        for (const auto& song : songs) {
            std::cout << "  Song: " << song.name << " (" << song.events.size() << " events)"
                      << std::endl;
            for (const auto& event : song.events) {
                std::cout << "    - " << event.name << std::endl;
            }
        }

        const auto& masterBus = songManager->getMasterBus();
        std::cout << "\nMaster Bus: " << masterBus.name << " (" << masterBus.events.size()
                  << " events)" << std::endl;
        for (const auto& event : masterBus.events) {
            std::cout << "  - " << event.name << std::endl;
        }
    }

    // Test event triggering
    std::cout << "\n=== Testing Event Triggering ===" << std::endl;

    // Trigger a song event
    std::cout << "Triggering song event: Test Song -> Intro" << std::endl;
    controller.triggerSongEvent("Test Song", "Intro");

    // Simulate some time passing
    for (int i = 0; i < 10; ++i) {
        controller.updateEvents(0.1f); // 100ms per update
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << ".";
    }
    std::cout << std::endl;

    // Trigger another event
    std::cout << "Triggering song event: Test Song -> Verse" << std::endl;
    controller.triggerSongEvent("Test Song", "Verse");

    // Simulate more time passing
    for (int i = 0; i < 15; ++i) {
        controller.updateEvents(0.1f);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << ".";
    }
    std::cout << std::endl;

    // Test master event
    std::cout << "Triggering master event: Underwater" << std::endl;
    controller.triggerMasterEvent("Underwater");

    // Simulate final time passing
    for (int i = 0; i < 30; ++i) {
        controller.updateEvents(0.1f);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << ".";
    }
    std::cout << std::endl;

    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}