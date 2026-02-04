#include <catch2/catch_test_macros.hpp>

#include "AudioTestHarness.h"
#include "SignalAnalyzer.h"
#include "SongManager.h"
#include "TestHelpers.h"

using namespace Dynamix;
using namespace DynamixTest;

TEST_CASE("Complete song creation workflow", "[integration][workflow]") {
    TempTestDirectory tempDir;

    SECTION("Create song folder and load") {
        // Step 1: Create song folder structure
        tempDir.createSongFolder("test_song");
        tempDir.writeSongJson("test_song", createMinimalSongJson({"track1.ogg"}));
        tempDir.writeMasterJson(createMinimalMasterJson());

        // Step 2: Load via SongManager
        SongManager manager;
        manager.loadSongsFromDirectory(tempDir.path().string());

        // Step 3: Verify song loaded
        const auto& songs = manager.getSongs();
        REQUIRE(songs.size() == 1);
        REQUIRE(songs[0].folderPath.filename().string() == "test_song");

        const Song* song = manager.findSongByFolder("test_song");
        REQUIRE(song != nullptr);
        REQUIRE(song->events.size() >= 1);
    }

    SECTION("Add event with effects and save") {
        tempDir.createSongFolder("test_song");
        tempDir.writeSongJson("test_song", createMinimalSongJson({"track1.ogg"}));
        tempDir.writeMasterJson(createMinimalMasterJson());

        SongManager manager;
        manager.loadSongsFromDirectory(tempDir.path().string());

        // Create event with effects
        SongEvent newEvent;
        newEvent.name = "WithEffects";
        newEvent.fadeTime = 2.0f;
        newEvent.state.masterTempo = 1.2f;
        newEvent.state.granularTempo = 0.9f;

        TrackStateExtended track;
        track.file = "track1.ogg";
        track.volume = 0.8f;

        EffectState echoEffect;
        echoEffect.parameters["0"] = 0.6f;
        echoEffect.parameters["1"] = 0.25f;
        echoEffect.parameters["2"] = 0.4f;
        track.effects["echo"] = echoEffect;

        newEvent.state.tracks.push_back(track);

        // Add event
        manager.addSongEvent("test_song", newEvent);

        // Save
        manager.saveSongJson("test_song");

        // Reload and verify
        SongManager manager2;
        manager2.loadSongsFromDirectory(tempDir.path().string());

        const Song* song = manager2.findSongByFolder("test_song");
        REQUIRE(song != nullptr);

        bool found = false;
        for (const auto& event : song->events) {
            if (event.name == "WithEffects") {
                found = true;
                REQUIRE(approxEqual(event.fadeTime, 2.0f));
                REQUIRE(approxEqual(event.state.masterTempo, 1.2f));
                REQUIRE(event.state.tracks.size() == 1);
                REQUIRE(event.state.tracks[0].effects.count("echo") == 1);
            }
        }
        REQUIRE(found);
    }
}

TEST_CASE("Song with multiple events", "[integration][workflow]") {
    TempTestDirectory tempDir;
    tempDir.createSongFolder("multi_event");
    tempDir.writeSongJson("multi_event", createMinimalSongJson({"track1.ogg"}));
    tempDir.writeMasterJson(createMinimalMasterJson());

    SongManager manager;
    manager.loadSongsFromDirectory(tempDir.path().string());

    // Add multiple events
    for (int i = 0; i < 5; ++i) {
        SongEvent newEvent;
        newEvent.name = "Event" + std::to_string(i);
        newEvent.fadeTime = 1.0f;
        newEvent.state.masterTempo = 1.0f + static_cast<float>(i) * 0.1f;
        newEvent.state.granularTempo = 1.0f;

        TrackStateExtended track;
        track.file = "track1.ogg";
        track.volume = 0.5f + static_cast<float>(i) * 0.1f;
        newEvent.state.tracks.push_back(track);

        manager.addSongEvent("multi_event", newEvent);
    }

    manager.saveSongJson("multi_event");

    // Reload and verify all events
    SongManager manager2;
    manager2.loadSongsFromDirectory(tempDir.path().string());

    const Song* song = manager2.findSongByFolder("multi_event");
    REQUIRE(song != nullptr);
    REQUIRE(song->events.size() >= 5);

    for (int i = 0; i < 5; ++i) {
        REQUIRE(manager2.hasSongEvent("multi_event", "Event" + std::to_string(i)));
    }
}

TEST_CASE("Master bus event workflow", "[integration][workflow]") {
    TempTestDirectory tempDir;
    tempDir.writeMasterJson(createMinimalMasterJson());

    SongManager manager;
    manager.loadSongsFromDirectory(tempDir.path().string());

    SECTION("Add master event with effects") {
        MasterEvent newEvent;
        newEvent.name = "WithReverb";
        newEvent.fadeTime = 3.0f;
        newEvent.state.volume = 0.9f;
        newEvent.state.masterTempo = 1.0f;
        newEvent.state.granularTempo = 1.0f;

        EffectState freeverbEffect;
        freeverbEffect.parameters["0"] = 0.4f;
        freeverbEffect.parameters["2"] = 0.7f;
        newEvent.state.effects["freeverb"] = freeverbEffect;

        manager.addMasterEvent(newEvent);
        manager.saveMasterJson();

        // Reload and verify
        SongManager manager2;
        manager2.loadSongsFromDirectory(tempDir.path().string());

        REQUIRE(manager2.hasMasterEvent("WithReverb"));
    }
}
