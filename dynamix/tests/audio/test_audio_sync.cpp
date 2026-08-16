#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "AudioTestHarness.h"
#include "SignalAnalyzer.h"
#include "TestHelpers.h"

using namespace DynamixTest;

TEST_CASE("Multiple tracks play simultaneously", "[audio][sync]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    // Generate two different frequency sine waves
    auto sine440 = SignalAnalyzer::generateSineWave(440.0f, 1.0f, harness.getSampleRate());
    auto sine880 = SignalAnalyzer::generateSineWave(880.0f, 1.0f, harness.getSampleRate());

    int track0 = harness.loadTrackFromMemory(sine440, sine440.size() / 2);
    int track1 = harness.loadTrackFromMemory(sine880, sine880.size() / 2);

    REQUIRE(track0 >= 0);
    REQUIRE(track1 >= 0);
    REQUIRE(harness.getTrackCount() == 2);

    harness.playAllTracks();

    SECTION("Both tracks produce audio") {
        auto audio = harness.processSeconds(0.5f);

        float rms = SignalAnalyzer::calculateRMS(audio);
        REQUIRE(rms > 0.0f);

        // Spectrum should show peaks at both frequencies
        auto spectrum = SignalAnalyzer::calculateSpectrum(audio);
        // Note: This is a simplified check
        REQUIRE(spectrum.size() > 0);
    }

    SECTION("Mixed signal contains both frequencies") {
        auto audio = harness.processSeconds(0.5f);

        // One channel at a time: the spectrum treats its input as a single
        // stream, so an interleaved buffer reads as a detuned signal.
        auto mono = SignalAnalyzer::extractChannel(audio, harness.getChannels());
        auto spectrum = SignalAnalyzer::calculateSpectrum(mono);

        REQUIRE(SignalAnalyzer::hasFrequencyPeak(spectrum, 440.0f, harness.getSampleRate(), 0.1f));
        REQUIRE(SignalAnalyzer::hasFrequencyPeak(spectrum, 880.0f, harness.getSampleRate(), 0.1f));
    }
}

TEST_CASE("Track positions remain synchronized", "[audio][sync]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine1 = SignalAnalyzer::generateSineWave(440.0f, 5.0f, harness.getSampleRate());
    auto sine2 = SignalAnalyzer::generateSineWave(880.0f, 5.0f, harness.getSampleRate());

    REQUIRE(harness.loadTrackFromMemory(sine1, sine1.size() / 2) == 0);
    REQUIRE(harness.loadTrackFromMemory(sine2, sine2.size() / 2) == 1);

    harness.playAllTracks();

    SECTION("Tracks start at same position") {
        harness.processSeconds(0.1f);

        double pos0 = harness.getTrackPosition(0);
        double pos1 = harness.getTrackPosition(1);

        // Positions should be very close
        REQUIRE(std::abs(pos0 - pos1) < 0.01);
    }

    SECTION("Tracks remain synced after extended playback") {
        // Process 3 seconds of audio
        for (int i = 0; i < 30; ++i) {
            harness.processSeconds(0.1f);
        }

        double pos0 = harness.getTrackPosition(0);
        double pos1 = harness.getTrackPosition(1);

        // Positions should still be close (within 10ms)
        REQUIRE(std::abs(pos0 - pos1) < 0.01);
    }
}

TEST_CASE("Individual track volume control", "[audio][sync]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine440 = SignalAnalyzer::generateSineWave(440.0f, 1.0f, harness.getSampleRate());
    auto sine880 = SignalAnalyzer::generateSineWave(880.0f, 1.0f, harness.getSampleRate());

    REQUIRE(harness.loadTrackFromMemory(sine440, sine440.size() / 2) == 0);
    REQUIRE(harness.loadTrackFromMemory(sine880, sine880.size() / 2) == 1);

    harness.playAllTracks();

    SECTION("Muting one track reduces output level") {
        // Both tracks at full volume
        harness.setTrackVolume(0, 1.0f);
        harness.setTrackVolume(1, 1.0f);
        auto fullVolume = harness.processSeconds(0.2f);
        float fullRMS = SignalAnalyzer::calculateRMS(fullVolume);

        // Reset
        harness.clearAllTracks();
        REQUIRE(harness.loadTrackFromMemory(sine440, sine440.size() / 2) == 0);
        REQUIRE(harness.loadTrackFromMemory(sine880, sine880.size() / 2) == 1);
        harness.playAllTracks();

        // Mute one track
        harness.setTrackVolume(0, 0.0f);
        harness.setTrackVolume(1, 1.0f);
        auto oneTrack = harness.processSeconds(0.2f);
        float oneRMS = SignalAnalyzer::calculateRMS(oneTrack);

        // One track should be quieter
        REQUIRE(oneRMS < fullRMS);
    }
}
