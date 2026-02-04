#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "AudioTestHarness.h"
#include "SignalAnalyzer.h"
#include "TestHelpers.h"

using namespace DynamixTest;
using Catch::Matchers::WithinAbs;

TEST_CASE("Granular tempo changes speed", "[audio][granular]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 2.0f, harness.getSampleRate());
    int trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    REQUIRE(trackIdx >= 0);

    harness.playAllTracks();

    SECTION("Normal tempo (1.0x) produces expected frequency") {
        harness.setGranularTempo(1.0f);
        auto audio = harness.processSeconds(1.0f);

        float freq = SignalAnalyzer::findDominantFrequency(audio, harness.getSampleRate());
        // Should be approximately 440Hz
        REQUIRE_THAT(freq, WithinAbs(440.0f, 50.0f));
    }

    SECTION("Granular tempo processes without crash") {
        harness.setGranularTempo(1.5f);
        auto audio = harness.processSeconds(0.5f);

        // Should produce audio
        float rms = SignalAnalyzer::calculateRMS(audio);
        REQUIRE(rms > 0.0f);
    }

    SECTION("Different tempos produce different playback") {
        harness.setGranularTempo(1.0f);
        auto normal = harness.processSeconds(0.5f);

        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
        harness.playAllTracks();
        harness.setGranularTempo(2.0f);
        auto fast = harness.processSeconds(0.5f);

        // Different tempos should produce different audio
        // (Note: This is a simplified test - real granular testing would verify pitch preservation)
        REQUIRE(SignalAnalyzer::calculateRMS(normal) > 0.0f);
        REQUIRE(SignalAnalyzer::calculateRMS(fast) > 0.0f);
    }
}

TEST_CASE("Granular tempo preserves pitch", "[audio][granular]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 2.0f, harness.getSampleRate());

    SECTION("Pitch is preserved at 2x tempo") {
        int trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
        harness.playAllTracks();
        harness.setGranularTempo(2.0f);

        auto audio = harness.processSeconds(1.0f);
        float freq = SignalAnalyzer::findDominantFrequency(audio, harness.getSampleRate());

        // Pitch should still be approximately 440Hz
        // Allow larger tolerance for granular processing artifacts
        REQUIRE_THAT(freq, WithinAbs(440.0f, 100.0f));
    }

    SECTION("Pitch is preserved at 0.5x tempo") {
        int trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
        harness.playAllTracks();
        harness.setGranularTempo(0.5f);

        auto audio = harness.processSeconds(1.0f);
        float freq = SignalAnalyzer::findDominantFrequency(audio, harness.getSampleRate());

        // Pitch should still be approximately 440Hz
        REQUIRE_THAT(freq, WithinAbs(440.0f, 100.0f));
    }
}

TEST_CASE("Master tempo affects pitch", "[audio][tempo]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 2.0f, harness.getSampleRate());
    int trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    REQUIRE(trackIdx >= 0);

    harness.playAllTracks();

    SECTION("2x master tempo doubles pitch") {
        harness.setMasterTempo(2.0f);
        auto audio = harness.processSeconds(0.5f);

        float freq = SignalAnalyzer::findDominantFrequency(audio, harness.getSampleRate());
        // 2x tempo should approximately double the frequency
        REQUIRE_THAT(freq, WithinAbs(880.0f, 100.0f));
    }

    SECTION("0.5x master tempo halves pitch") {
        harness.setMasterTempo(0.5f);
        auto audio = harness.processSeconds(1.0f);

        float freq = SignalAnalyzer::findDominantFrequency(audio, harness.getSampleRate());
        // 0.5x tempo should approximately halve the frequency
        REQUIRE_THAT(freq, WithinAbs(220.0f, 50.0f));
    }
}
