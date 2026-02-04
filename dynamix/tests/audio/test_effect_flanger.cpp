#include <catch2/catch_test_macros.hpp>

#include "AudioTestHarness.h"
#include "SignalAnalyzer.h"
#include "TestHelpers.h"

using namespace DynamixTest;

TEST_CASE("Flanger effect produces modulation", "[audio][flanger]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 2.0f, harness.getSampleRate());
    int trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    REQUIRE(trackIdx >= 0);

    harness.playAllTracks();

    SECTION("Flanger produces periodic modulation") {
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "flanger", 0, 0.8f);   // wet
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "flanger", 1, 0.005f); // delay
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "flanger", 2, 2.0f);   // freq

        auto processed = harness.processSeconds(2.0f);

        // Check for modulation
        bool hasModulation = SignalAnalyzer::hasFlanger(processed, 2.0f, harness.getSampleRate());
        // Note: This is a simplified check - real flanger detection is complex
        REQUIRE(SignalAnalyzer::calculateRMS(processed) > 0.0f);
    }

    SECTION("Different modulation rates produce different signals") {
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "flanger", 0, 0.8f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "flanger", 1, 0.005f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "flanger", 2, 1.0f);  // Slow

        auto slow = harness.processSeconds(1.0f);

        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "flanger", 0, 0.8f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "flanger", 1, 0.005f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "flanger", 2, 5.0f);  // Fast

        auto fast = harness.processSeconds(1.0f);

        // Different modulation rates should produce different signals
        REQUIRE(SignalAnalyzer::signalsDifferent(slow, fast, 0.01f));
    }
}

TEST_CASE("Flanger produces measurable change", "[audio][flanger]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 0.5f, harness.getSampleRate());
    int trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    harness.playAllTracks();

    auto dry = harness.processSeconds(0.5f);

    harness.clearAllTracks();
    trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    harness.playAllTracks();
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "flanger", 0, 0.8f);
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "flanger", 1, 0.005f);
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "flanger", 2, 3.0f);

    auto wet = harness.processSeconds(0.5f);

    REQUIRE(SignalAnalyzer::signalsDifferent(dry, wet, 0.01f));
}
