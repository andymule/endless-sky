#include <catch2/catch_test_macros.hpp>

#include "AudioState.h"
#include "SongManager.h"
#include "TestHelpers.h"

using namespace Dynamix;
using namespace DynamixTest;

TEST_CASE("State survives complete save-reload cycle", "[roundtrip][persistence]") {
    TempTestDirectory tempDir;
    tempDir.createSongFolder("persist_test");
    tempDir.writeSongJson("persist_test", createMinimalSongJson({"track1.ogg"}));
    tempDir.writeMasterJson(createMinimalMasterJson());

    // Create comprehensive test event
    SongEvent originalEvent;
    originalEvent.name = "CompleteState";
    originalEvent.fadeTime = 2.5f;
    originalEvent.state.masterTempo = 1.35f;
    originalEvent.state.granularTempo = 0.85f;

    TrackStateExtended track;
    track.file = "track1.ogg";
    track.volume = 0.72f;

    // Add multiple effects with various parameters
    EffectState echo;
    echo.parameters["0"] = 0.55f;
    echo.parameters["1"] = 0.28f;
    echo.parameters["2"] = 0.42f;
    track.effects["echo"] = echo;

    EffectState freeverb;
    freeverb.parameters["0"] = 0.38f;
    freeverb.parameters["2"] = 0.72f;
    freeverb.parameters["3"] = 0.55f;
    track.effects["freeverb"] = freeverb;

    originalEvent.state.tracks.push_back(track);

    SECTION("State is preserved exactly") {
        // Save
        SongManager manager1;
        manager1.loadSongsFromDirectory(tempDir.path().string());
        manager1.addSongEvent("persist_test", originalEvent);
        manager1.saveSongJson("persist_test");

        // Reload
        SongManager manager2;
        manager2.loadSongsFromDirectory(tempDir.path().string());

        const Song* song = manager2.findSongByFolder("persist_test");
        REQUIRE(song != nullptr);

        const SongEvent* event = nullptr;
        for (const auto& e : song->events) {
            if (e.name == "CompleteState") {
                event = &e;
                break;
            }
        }
        REQUIRE(event != nullptr);

        // Compare states
        REQUIRE(statesEqual(originalEvent.state, event->state, 0.001f));
    }
}

TEST_CASE("Master bus state survives save-reload", "[roundtrip][persistence]") {
    TempTestDirectory tempDir;
    tempDir.writeMasterJson(createMinimalMasterJson());

    MasterEvent originalEvent;
    originalEvent.name = "TestMaster";
    originalEvent.fadeTime = 1.5f;
    originalEvent.state.volume = 0.88f;
    originalEvent.state.masterTempo = 1.15f;
    originalEvent.state.granularTempo = 0.92f;

    EffectState echo;
    echo.parameters["0"] = 0.45f;
    echo.parameters["1"] = 0.32f;
    originalEvent.state.effects["echo"] = echo;

    SECTION("Master state is preserved") {
        SongManager manager1;
        manager1.loadSongsFromDirectory(tempDir.path().string());
        manager1.addMasterEvent(originalEvent);
        manager1.saveMasterJson();

        SongManager manager2;
        manager2.loadSongsFromDirectory(tempDir.path().string());

        REQUIRE(manager2.hasMasterEvent("TestMaster"));

        // Get the master event
        const MasterBus& masterBus = manager2.getMasterBus();

        const MasterEvent* foundEvent = nullptr;
        for (const auto& event : masterBus.events) {
            if (event.name == "TestMaster") {
                foundEvent = &event;
                break;
            }
        }
        REQUIRE(foundEvent != nullptr);

        REQUIRE(masterStatesEqual(originalEvent.state, foundEvent->state, 0.001f));
    }
}

TEST_CASE("Multiple songs saved and loaded correctly", "[roundtrip][persistence]") {
    TempTestDirectory tempDir;

    // Create multiple songs
    for (int i = 0; i < 3; ++i) {
        std::string songName = "song" + std::to_string(i);
        tempDir.createSongFolder(songName);
        tempDir.writeSongJson(songName, createMinimalSongJson({"track.ogg"}));
    }
    tempDir.writeMasterJson(createMinimalMasterJson());

    // Configure each song differently
    SongManager manager1;
    manager1.loadSongsFromDirectory(tempDir.path().string());

    for (int i = 0; i < 3; ++i) {
        std::string songName = "song" + std::to_string(i);

        SongEvent newEvent;
        newEvent.name = "ConfiguredEvent";
        newEvent.fadeTime = 1.0f + static_cast<float>(i);
        newEvent.state.masterTempo = 1.0f + static_cast<float>(i) * 0.2f;
        newEvent.state.granularTempo = 1.0f - static_cast<float>(i) * 0.1f;

        TrackStateExtended track;
        track.file = "track.ogg";
        track.volume = 0.5f + static_cast<float>(i) * 0.2f;
        newEvent.state.tracks.push_back(track);

        manager1.addSongEvent(songName, newEvent);
        manager1.saveSongJson(songName);
    }

    // Reload and verify each song
    SongManager manager2;
    manager2.loadSongsFromDirectory(tempDir.path().string());

    for (int i = 0; i < 3; ++i) {
        std::string songName = "song" + std::to_string(i);
        const Song* song = manager2.findSongByFolder(songName);
        REQUIRE(song != nullptr);

        const SongEvent* event = nullptr;
        for (const auto& e : song->events) {
            if (e.name == "ConfiguredEvent") {
                event = &e;
                break;
            }
        }
        REQUIRE(event != nullptr);

        INFO("Checking song: " << songName);
        REQUIRE(approxEqual(event->state.masterTempo, 1.0f + static_cast<float>(i) * 0.2f));
        REQUIRE(approxEqual(event->state.granularTempo, 1.0f - static_cast<float>(i) * 0.1f));
        REQUIRE(approxEqual(event->fadeTime, 1.0f + static_cast<float>(i)));
    }
}

TEST_CASE("Empty effects are handled correctly", "[roundtrip][persistence]") {
    TempTestDirectory tempDir;
    tempDir.createSongFolder("empty_effects");
    tempDir.writeSongJson("empty_effects", createMinimalSongJson({"track.ogg"}));
    tempDir.writeMasterJson(createMinimalMasterJson());

    SongEvent newEvent;
    newEvent.name = "NoEffects";
    newEvent.fadeTime = 1.0f;
    newEvent.state.masterTempo = 1.0f;
    newEvent.state.granularTempo = 1.0f;

    TrackStateExtended track;
    track.file = "track.ogg";
    track.volume = 1.0f;
    // No effects added
    newEvent.state.tracks.push_back(track);

    SongManager manager1;
    manager1.loadSongsFromDirectory(tempDir.path().string());
    manager1.addSongEvent("empty_effects", newEvent);
    manager1.saveSongJson("empty_effects");

    SongManager manager2;
    manager2.loadSongsFromDirectory(tempDir.path().string());

    const Song* song = manager2.findSongByFolder("empty_effects");
    REQUIRE(song != nullptr);

    const SongEvent* event = nullptr;
    for (const auto& e : song->events) {
        if (e.name == "NoEffects") {
            event = &e;
            break;
        }
    }
    REQUIRE(event != nullptr);
    REQUIRE(event->state.tracks.size() == 1);
    REQUIRE(event->state.tracks[0].effects.empty());
}

TEST_CASE("Extreme values are handled correctly", "[roundtrip][persistence]") {
    TempTestDirectory tempDir;
    tempDir.createSongFolder("extreme_test");
    tempDir.writeSongJson("extreme_test", createMinimalSongJson({"track.ogg"}));
    tempDir.writeMasterJson(createMinimalMasterJson());

    SongEvent newEvent;
    newEvent.name = "Extreme";
    newEvent.fadeTime = 60.0f;  // Max fade time
    newEvent.state.masterTempo = 4.0f;  // Max tempo
    newEvent.state.granularTempo = 0.1f;  // Min granular tempo (if supported)

    TrackStateExtended track;
    track.file = "track.ogg";
    track.volume = 2.0f;  // Max volume
    newEvent.state.tracks.push_back(track);

    SongManager manager1;
    manager1.loadSongsFromDirectory(tempDir.path().string());
    manager1.addSongEvent("extreme_test", newEvent);
    manager1.saveSongJson("extreme_test");

    SongManager manager2;
    manager2.loadSongsFromDirectory(tempDir.path().string());

    const Song* song = manager2.findSongByFolder("extreme_test");
    REQUIRE(song != nullptr);

    const SongEvent* event = nullptr;
    for (const auto& e : song->events) {
        if (e.name == "Extreme") {
            event = &e;
            break;
        }
    }
    REQUIRE(event != nullptr);
    REQUIRE(approxEqual(event->state.masterTempo, 4.0f));
    REQUIRE(approxEqual(event->state.tracks[0].volume, 2.0f));
    REQUIRE(approxEqual(event->fadeTime, 60.0f));
}
