#include <catch2/catch_test_macros.hpp>

#include "AudioState.h"
#include "EventSystem.h"
#include "SongManager.h"
#include "AudioTestHarness.h"
#include "TestHelpers.h"

using namespace Dynamix;
using namespace DynamixTest;

TEST_CASE("Event system state structure", "[integration][events]") {
    SECTION("StateSnapshot holds track information") {
        StateSnapshot state;
        state.masterTempo = 1.5f;
        state.granularTempo = 0.8f;

        TrackStateExtended track;
        track.file = "test.ogg";
        track.volume = 0.7f;
        state.tracks.push_back(track);

        REQUIRE(state.tracks.size() == 1);
        REQUIRE(state.tracks[0].file == "test.ogg");
        REQUIRE(approxEqual(state.tracks[0].volume, 0.7f));
    }

    SECTION("StateSnapshot holds effect information") {
        StateSnapshot state;
        state.masterTempo = 1.0f;
        state.granularTempo = 1.0f;

        TrackStateExtended track;
        track.file = "test.ogg";
        track.volume = 1.0f;

        EffectState echo;
        echo.parameters["0"] = 0.5f;
        echo.parameters["1"] = 0.3f;
        track.effects["echo"] = echo;

        EffectState reverb;
        reverb.parameters["0"] = 0.4f;
        reverb.parameters["2"] = 0.8f;
        track.effects["freeverb"] = reverb;

        state.tracks.push_back(track);

        REQUIRE(state.tracks[0].effects.size() == 2);
        REQUIRE(state.tracks[0].effects.count("echo") == 1);
        REQUIRE(state.tracks[0].effects.count("freeverb") == 1);
    }
}

TEST_CASE("MasterBusState structure", "[integration][events]") {
    MasterBusState state;
    state.volume = 0.9f;
    state.masterTempo = 1.2f;
    state.granularTempo = 0.9f;

    EffectState effect;
    effect.parameters["0"] = 0.3f;
    state.effects["echo"] = effect;

    REQUIRE(approxEqual(state.volume, 0.9f));
    REQUIRE(approxEqual(state.masterTempo, 1.2f));
    REQUIRE(state.effects.size() == 1);
}

TEST_CASE("Song events can be retrieved", "[integration][events]") {
    TempTestDirectory tempDir;
    tempDir.createSongFolder("event_song");
    tempDir.writeSongJson("event_song", createMinimalSongJson({"track.ogg"}));
    tempDir.writeMasterJson(createMinimalMasterJson());

    SongManager manager;
    manager.loadSongsFromDirectory(tempDir.path().string());

    // Add events
    SongEvent event1;
    event1.name = "Slow";
    event1.fadeTime = 2.0f;
    event1.state.masterTempo = 1.0f;
    event1.state.granularTempo = 1.0f;

    SongEvent event2;
    event2.name = "Fast";
    event2.fadeTime = 1.0f;
    event2.state.masterTempo = 1.5f;
    event2.state.granularTempo = 0.8f;

    manager.addSongEvent("event_song", event1);
    manager.addSongEvent("event_song", event2);

    SECTION("Events can be found by name") {
        REQUIRE(manager.hasSongEvent("event_song", "Slow"));
        REQUIRE(manager.hasSongEvent("event_song", "Fast"));
        REQUIRE_FALSE(manager.hasSongEvent("event_song", "NonExistent"));
    }

    SECTION("Events have correct properties") {
        const Song* song = manager.findSongByFolder("event_song");
        REQUIRE(song != nullptr);

        for (const auto& event : song->events) {
            if (event.name == "Slow") {
                REQUIRE(approxEqual(event.fadeTime, 2.0f));
                REQUIRE(approxEqual(event.state.masterTempo, 1.0f));
            } else if (event.name == "Fast") {
                REQUIRE(approxEqual(event.fadeTime, 1.0f));
                REQUIRE(approxEqual(event.state.masterTempo, 1.5f));
            }
        }
    }
}

TEST_CASE("Master events can be retrieved", "[integration][events]") {
    TempTestDirectory tempDir;
    tempDir.writeMasterJson(createMinimalMasterJson());

    SongManager manager;
    manager.loadSongsFromDirectory(tempDir.path().string());

    MasterEvent event1;
    event1.name = "Loud";
    event1.fadeTime = 1.0f;
    event1.state.volume = 1.0f;
    event1.state.masterTempo = 1.0f;
    event1.state.granularTempo = 1.0f;

    MasterEvent event2;
    event2.name = "Quiet";
    event2.fadeTime = 2.0f;
    event2.state.volume = 0.5f;
    event2.state.masterTempo = 0.8f;
    event2.state.granularTempo = 1.0f;

    manager.addMasterEvent(event1);
    manager.addMasterEvent(event2);

    REQUIRE(manager.hasMasterEvent("Loud"));
    REQUIRE(manager.hasMasterEvent("Quiet"));
    REQUIRE_FALSE(manager.hasMasterEvent("NonExistent"));
}

TEST_CASE("Event deletion works correctly", "[integration][events]") {
    TempTestDirectory tempDir;
    tempDir.createSongFolder("delete_test");
    tempDir.writeSongJson("delete_test", createMinimalSongJson({"track.ogg"}));
    tempDir.writeMasterJson(createMinimalMasterJson());

    SongManager manager;
    manager.loadSongsFromDirectory(tempDir.path().string());

    SongEvent newEvent;
    newEvent.name = "ToDelete";
    newEvent.fadeTime = 1.0f;
    newEvent.state.masterTempo = 1.0f;
    newEvent.state.granularTempo = 1.0f;

    manager.addSongEvent("delete_test", newEvent);
    REQUIRE(manager.hasSongEvent("delete_test", "ToDelete"));

    manager.deleteSongEvent("delete_test", "ToDelete");
    REQUIRE_FALSE(manager.hasSongEvent("delete_test", "ToDelete"));
}

TEST_CASE("Event overwrite works correctly", "[integration][events]") {
    TempTestDirectory tempDir;
    tempDir.createSongFolder("overwrite_test");
    tempDir.writeSongJson("overwrite_test", createMinimalSongJson({"track.ogg"}));
    tempDir.writeMasterJson(createMinimalMasterJson());

    SongManager manager;
    manager.loadSongsFromDirectory(tempDir.path().string());

    SongEvent event1;
    event1.name = "TestEvent";
    event1.fadeTime = 1.0f;
    event1.state.masterTempo = 1.0f;
    event1.state.granularTempo = 1.0f;

    manager.addSongEvent("overwrite_test", event1);

    SongEvent event2;
    event2.name = "TestEvent";
    event2.fadeTime = 3.0f;
    event2.state.masterTempo = 2.0f;  // Different tempo
    event2.state.granularTempo = 0.5f;

    manager.overwriteSongEvent("overwrite_test", event2);

    const Song* song = manager.findSongByFolder("overwrite_test");
    REQUIRE(song != nullptr);

    const SongEvent* event = nullptr;
    for (const auto& e : song->events) {
        if (e.name == "TestEvent") {
            event = &e;
            break;
        }
    }
    REQUIRE(event != nullptr);
    REQUIRE(approxEqual(event->fadeTime, 3.0f));
    REQUIRE(approxEqual(event->state.masterTempo, 2.0f));
}
