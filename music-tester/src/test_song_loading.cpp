#include "SongManager.h"
#include <iostream>

int main() {
    std::cout << "=== Testing Song Manager ===" << std::endl;

    AudioTester::SongManager songManager;

    // Test loading songs from sound_staging directory
    songManager.loadSongsFromDirectory("sound_staging");

    // Display loaded songs
    const auto& songs = songManager.getSongs();
    std::cout << "\nLoaded " << songs.size() << " songs:" << std::endl;

    for (const auto& song : songs) {
        std::cout << "  Song: " << song.name << std::endl;
        std::cout << "    Path: " << song.folderPath << std::endl;
        std::cout << "    Events: " << song.events.size() << std::endl;

        for (const auto& event : song.events) {
            std::cout << "      - " << event.name << " (fade: " << event.fadeTime << "s)"
                      << std::endl;
            std::cout << "        Tempo: " << event.state.masterTempo << " / "
                      << event.state.granularTempo << std::endl;
            std::cout << "        Tracks: " << event.state.tracks.size() << std::endl;

            for (size_t i = 0; i < event.state.tracks.size(); ++i) {
                const auto& track = event.state.tracks[i];
                std::cout << "          [" << i << "] " << track.file << " (vol: " << track.volume
                          << ", effects: " << track.effects.size() << ")" << std::endl;
            }
        }
        std::cout << std::endl;
    }

    // Display master bus
    const auto& masterBus = songManager.getMasterBus();
    if (!masterBus.events.empty()) {
        std::cout << "Master Bus: " << masterBus.name << std::endl;
        std::cout << "  Events: " << masterBus.events.size() << std::endl;

        for (const auto& event : masterBus.events) {
            std::cout << "    - " << event.name << " (fade: " << event.fadeTime << "s)"
                      << std::endl;
            std::cout << "      Tempo: " << event.state.masterTempo << " / "
                      << event.state.granularTempo << std::endl;
            std::cout << "      Volume: " << event.state.volume << std::endl;
            std::cout << "      Effects: " << event.state.effects.size() << std::endl;
        }
    } else {
        std::cout << "No master bus events loaded." << std::endl;
    }

    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}