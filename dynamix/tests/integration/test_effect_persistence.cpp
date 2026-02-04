#include <catch2/catch_test_macros.hpp>

#include "AudioTestHarness.h"
#include "SongManager.h"
#include "TestHelpers.h"

using namespace Dynamix;
using namespace DynamixTest;

TEST_CASE("Effect parameters survive save/reload", "[integration][persistence]") {
    TempTestDirectory tempDir;
    tempDir.createSongFolder("effect_test");
    tempDir.writeSongJson("effect_test", createMinimalSongJson({"track1.ogg"}));
    tempDir.writeMasterJson(createMinimalMasterJson());

    // Original parameter values
    std::map<std::string, std::map<std::string, float>> originalEffects = {
        {"echo", {{"0", 0.65f}, {"1", 0.35f}, {"2", 0.45f}}},
        {"freeverb", {{"0", 0.55f}, {"2", 0.75f}, {"3", 0.55f}}},
        {"lofi", {{"0", 0.40f}, {"1", 6000.0f}, {"2", 6.0f}}}
    };

    SECTION("All effect parameters are preserved") {
        SongManager manager;
        manager.loadSongsFromDirectory(tempDir.path().string());

        // Create event with all effects
        SongEvent newEvent;
        newEvent.name = "AllEffects";
        newEvent.fadeTime = 1.5f;
        newEvent.state.masterTempo = 1.0f;
        newEvent.state.granularTempo = 1.0f;

        TrackStateExtended track;
        track.file = "track1.ogg";
        track.volume = 0.8f;

        for (const auto& [effectName, params] : originalEffects) {
            EffectState effect;
            for (const auto& [paramId, value] : params) {
                effect.parameters[paramId] = value;
            }
            track.effects[effectName] = effect;
        }
        newEvent.state.tracks.push_back(track);

        manager.addSongEvent("effect_test", newEvent);
        manager.saveSongJson("effect_test");

        // Reload
        SongManager manager2;
        manager2.loadSongsFromDirectory(tempDir.path().string());

        const Song* song = manager2.findSongByFolder("effect_test");
        REQUIRE(song != nullptr);

        // Find the event
        const SongEvent* foundEvent = nullptr;
        for (const auto& event : song->events) {
            if (event.name == "AllEffects") {
                foundEvent = &event;
                break;
            }
        }
        REQUIRE(foundEvent != nullptr);
        REQUIRE(foundEvent->state.tracks.size() == 1);

        // Verify each effect
        for (const auto& [effectName, params] : originalEffects) {
            INFO("Checking effect: " << effectName);
            REQUIRE(foundEvent->state.tracks[0].effects.count(effectName) == 1);

            const auto& loadedEffect = foundEvent->state.tracks[0].effects.at(effectName);
            for (const auto& [paramId, expectedValue] : params) {
                INFO("Checking param: " << paramId);
                REQUIRE(loadedEffect.parameters.count(paramId) == 1);
                REQUIRE(approxEqual(loadedEffect.parameters.at(paramId), expectedValue));
            }
        }
    }
}

TEST_CASE("Master bus effects survive save/reload", "[integration][persistence]") {
    TempTestDirectory tempDir;
    tempDir.writeMasterJson(createMinimalMasterJson());

    std::map<std::string, float> echoParams = {{"0", 0.50f}, {"1", 0.25f}, {"2", 0.55f}};
    std::map<std::string, float> freeverbParams = {{"0", 0.35f}, {"2", 0.65f}};

    SECTION("Master bus effect parameters are preserved") {
        SongManager manager;
        manager.loadSongsFromDirectory(tempDir.path().string());

        MasterEvent newEvent;
        newEvent.name = "BusEffects";
        newEvent.fadeTime = 2.0f;
        newEvent.state.volume = 0.85f;
        newEvent.state.masterTempo = 1.0f;
        newEvent.state.granularTempo = 1.0f;

        EffectState echo;
        for (const auto& [id, val] : echoParams) {
            echo.parameters[id] = val;
        }
        newEvent.state.effects["echo"] = echo;

        EffectState freeverb;
        for (const auto& [id, val] : freeverbParams) {
            freeverb.parameters[id] = val;
        }
        newEvent.state.effects["freeverb"] = freeverb;

        manager.addMasterEvent(newEvent);
        manager.saveMasterJson();

        // Reload
        SongManager manager2;
        manager2.loadSongsFromDirectory(tempDir.path().string());

        REQUIRE(manager2.hasMasterEvent("BusEffects"));

        // Get the master event
        const MasterBus& masterBus = manager2.getMasterBus();

        const MasterEvent* foundEvent = nullptr;
        for (const auto& event : masterBus.events) {
            if (event.name == "BusEffects") {
                foundEvent = &event;
                break;
            }
        }
        REQUIRE(foundEvent != nullptr);
        REQUIRE(approxEqual(foundEvent->state.volume, 0.85f));
        REQUIRE(foundEvent->state.effects.count("echo") == 1);
        REQUIRE(foundEvent->state.effects.count("freeverb") == 1);
    }
}

TEST_CASE("Volume settings survive save/reload", "[integration][persistence]") {
    TempTestDirectory tempDir;
    tempDir.createSongFolder("volume_test");
    tempDir.writeSongJson("volume_test", createMinimalSongJson({"track1.ogg", "track2.ogg"}));
    tempDir.writeMasterJson(createMinimalMasterJson());

    SongManager manager;
    manager.loadSongsFromDirectory(tempDir.path().string());

    SongEvent newEvent;
    newEvent.name = "VolumeMix";
    newEvent.fadeTime = 1.0f;
    newEvent.state.masterTempo = 1.0f;
    newEvent.state.granularTempo = 1.0f;

    TrackStateExtended track1;
    track1.file = "track1.ogg";
    track1.volume = 0.0f;  // Muted

    TrackStateExtended track2;
    track2.file = "track2.ogg";
    track2.volume = 1.5f;  // Boosted

    newEvent.state.tracks.push_back(track1);
    newEvent.state.tracks.push_back(track2);

    manager.addSongEvent("volume_test", newEvent);
    manager.saveSongJson("volume_test");

    // Reload
    SongManager manager2;
    manager2.loadSongsFromDirectory(tempDir.path().string());

    const Song* song = manager2.findSongByFolder("volume_test");
    REQUIRE(song != nullptr);

    const SongEvent* event = nullptr;
    for (const auto& e : song->events) {
        if (e.name == "VolumeMix") {
            event = &e;
            break;
        }
    }
    REQUIRE(event != nullptr);
    REQUIRE(event->state.tracks.size() == 2);

    // Find tracks by filename and verify volumes
    for (const auto& track : event->state.tracks) {
        if (track.file == "track1.ogg") {
            REQUIRE(approxEqual(track.volume, 0.0f));
        } else if (track.file == "track2.ogg") {
            REQUIRE(approxEqual(track.volume, 1.5f));
        }
    }
}
