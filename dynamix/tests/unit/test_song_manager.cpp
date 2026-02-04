#include <catch2/catch_test_macros.hpp>

#include "AudioTestHarness.h"
#include "SongManager.h"
#include "TestHelpers.h"

#include <fstream>

using namespace Dynamix;
using namespace DynamixTest;

TEST_CASE("SongManager loads songs from directory", "[unit][song]") {
    TempTestDirectory tempDir;

    SECTION("Empty directory loads without error") {
        SongManager manager;
        // Should not throw
        REQUIRE_NOTHROW(manager.loadSongsFromDirectory(tempDir.path().string()));
    }

    SECTION("Directory with valid song folder loads correctly") {
        tempDir.createSongFolder("test_song");
        tempDir.writeSongJson("test_song", createMinimalSongJson({"track1.ogg"}));
        tempDir.writeMasterJson(createMinimalMasterJson());

        SongManager manager;
        manager.loadSongsFromDirectory(tempDir.path().string());

        const auto& songs = manager.getSongs();
        REQUIRE(songs.size() == 1);
        REQUIRE(songs[0].folderPath.filename().string() == "test_song");
    }

    SECTION("Multiple songs load correctly") {
        tempDir.createSongFolder("song1");
        tempDir.createSongFolder("song2");
        tempDir.writeSongJson("song1", createMinimalSongJson({"track1.ogg"}));
        tempDir.writeSongJson("song2", createMinimalSongJson({"track2.ogg"}));
        tempDir.writeMasterJson(createMinimalMasterJson());

        SongManager manager;
        manager.loadSongsFromDirectory(tempDir.path().string());

        const auto& songs = manager.getSongs();
        REQUIRE(songs.size() == 2);
    }
}

TEST_CASE("SongManager handles invalid JSON gracefully", "[unit][song]") {
    TempTestDirectory tempDir;

    SECTION("Invalid JSON syntax is handled without crash") {
        tempDir.createSongFolder("bad_song");
        // Write invalid JSON
        std::ofstream file(tempDir.path() / "bad_song" / "_song.json");
        file << "{ this is not valid json }";
        file.close();
        tempDir.writeMasterJson(createMinimalMasterJson());

        SongManager manager;
        // Should not throw, just log error and skip
        REQUIRE_NOTHROW(manager.loadSongsFromDirectory(tempDir.path().string()));
    }

    SECTION("Missing _master.json logs warning but continues") {
        tempDir.createSongFolder("test_song");
        tempDir.writeSongJson("test_song", createMinimalSongJson({"track1.ogg"}));
        // No master JSON

        SongManager manager;
        REQUIRE_NOTHROW(manager.loadSongsFromDirectory(tempDir.path().string()));
    }
}

TEST_CASE("SongManager event CRUD operations", "[unit][song]") {
    TempTestDirectory tempDir;
    tempDir.createSongFolder("test_song");
    tempDir.writeSongJson("test_song", createMinimalSongJson({"track1.ogg"}));
    tempDir.writeMasterJson(createMinimalMasterJson());

    SongManager manager;
    manager.loadSongsFromDirectory(tempDir.path().string());

    SECTION("Song has default event after loading") {
        const Song* song = manager.findSongByFolder("test_song");
        REQUIRE(song != nullptr);
        REQUIRE(song->events.size() >= 1);
    }

    SECTION("Can add new event to song") {
        SongEvent newEvent;
        newEvent.name = "NewEvent";
        newEvent.fadeTime = 2.0f;
        newEvent.state.masterTempo = 1.0f;
        newEvent.state.granularTempo = 1.0f;

        manager.addSongEvent("test_song", newEvent);

        const Song* song = manager.findSongByFolder("test_song");
        REQUIRE(song != nullptr);
        
        bool found = false;
        for (const auto& event : song->events) {
            if (event.name == "NewEvent") {
                found = true;
                REQUIRE(event.fadeTime == 2.0f);
            }
        }
        REQUIRE(found);
    }

    SECTION("Can check if event exists") {
        REQUIRE(manager.hasSongEvent("test_song", "Default"));
        REQUIRE_FALSE(manager.hasSongEvent("test_song", "NonExistent"));
        REQUIRE_FALSE(manager.hasSongEvent("non_existent_song", "Default"));
    }

    SECTION("Can delete event from song") {
        SongEvent newEvent;
        newEvent.name = "ToDelete";
        newEvent.fadeTime = 1.0f;
        manager.addSongEvent("test_song", newEvent);
        REQUIRE(manager.hasSongEvent("test_song", "ToDelete"));

        manager.deleteSongEvent("test_song", "ToDelete");
        REQUIRE_FALSE(manager.hasSongEvent("test_song", "ToDelete"));
    }
}

TEST_CASE("SongManager saves and reloads correctly", "[unit][song]") {
    TempTestDirectory tempDir;
    tempDir.createSongFolder("test_song");
    tempDir.writeSongJson("test_song", createMinimalSongJson({"track1.ogg"}));
    tempDir.writeMasterJson(createMinimalMasterJson());

    SECTION("Saved JSON can be reloaded") {
        // First load
        SongManager manager1;
        manager1.loadSongsFromDirectory(tempDir.path().string());

        // Add event with effects
        SongEvent newEvent;
        newEvent.name = "TestEvent";
        newEvent.fadeTime = 2.5f;
        newEvent.state.masterTempo = 1.5f;
        newEvent.state.granularTempo = 0.8f;
        TrackStateExtended track;
        track.file = "track1.ogg";
        track.volume = 0.7f;
        EffectState echoEffect;
        echoEffect.parameters["0"] = 0.5f;
        echoEffect.parameters["1"] = 0.3f;
        track.effects["echo"] = echoEffect;
        newEvent.state.tracks.push_back(track);

        manager1.addSongEvent("test_song", newEvent);
        manager1.saveSongJson("test_song");

        // Second load
        SongManager manager2;
        manager2.loadSongsFromDirectory(tempDir.path().string());

        const Song* song = manager2.findSongByFolder("test_song");
        REQUIRE(song != nullptr);

        bool found = false;
        for (const auto& event : song->events) {
            if (event.name == "TestEvent") {
                found = true;
                REQUIRE(approxEqual(event.fadeTime, 2.5f));
                REQUIRE(approxEqual(event.state.masterTempo, 1.5f));
                REQUIRE(approxEqual(event.state.granularTempo, 0.8f));
                REQUIRE(event.state.tracks.size() == 1);
                REQUIRE(approxEqual(event.state.tracks[0].volume, 0.7f));
                REQUIRE(event.state.tracks[0].effects.count("echo") == 1);
            }
        }
        REQUIRE(found);
    }
}

TEST_CASE("SongManager master bus operations", "[unit][song]") {
    TempTestDirectory tempDir;
    tempDir.writeMasterJson(createMinimalMasterJson());

    SongManager manager;
    manager.loadSongsFromDirectory(tempDir.path().string());

    SECTION("Master bus has default event") {
        REQUIRE(manager.hasMasterEvent("Normal"));
    }

    SECTION("Can add master event") {
        MasterEvent newEvent;
        newEvent.name = "NewMaster";
        newEvent.fadeTime = 1.0f;
        newEvent.state.volume = 0.8f;
        newEvent.state.masterTempo = 1.0f;
        newEvent.state.granularTempo = 1.0f;

        manager.addMasterEvent(newEvent);
        REQUIRE(manager.hasMasterEvent("NewMaster"));
    }

    SECTION("Can delete master event") {
        MasterEvent newEvent;
        newEvent.name = "ToDelete";
        newEvent.fadeTime = 1.0f;
        manager.addMasterEvent(newEvent);
        REQUIRE(manager.hasMasterEvent("ToDelete"));

        manager.deleteMasterEvent("ToDelete");
        REQUIRE_FALSE(manager.hasMasterEvent("ToDelete"));
    }
}
