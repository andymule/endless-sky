#include <catch2/catch_test_macros.hpp>

#include "AudioTestHarness.h"
#include "SignalAnalyzer.h"
#include "TestHelpers.h"

using namespace DynamixTest;

TEST_CASE("Robotize effect produces pitch modulation", "[audio][robotize]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 1.0f, harness.getSampleRate());
    int trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    REQUIRE(trackIdx >= 0);

    harness.playAllTracks();

    SECTION("Robotize produces modulation artifacts") {
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "robotize", 0, 0.8f);  // wet
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "robotize", 1, 5.0f);  // freq
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "robotize", 2, 0);     // waveform

        auto processed = harness.processSeconds(0.5f);

        // Signal should have RMS > 0
        float rms = SignalAnalyzer::calculateRMS(processed);
        REQUIRE(rms > 0.0f);
    }

    SECTION("Different frequencies produce different modulations") {
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "robotize", 0, 0.8f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "robotize", 1, 3.0f);  // Low freq
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "robotize", 2, 0);

        auto lowFreq = harness.processSeconds(0.5f);

        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "robotize", 0, 0.8f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "robotize", 1, 8.0f);  // High freq
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "robotize", 2, 0);

        auto highFreq = harness.processSeconds(0.5f);

        // Different modulation frequencies should produce different signals
        REQUIRE(SignalAnalyzer::signalsDifferent(lowFreq, highFreq, 0.01f));
    }
}

TEST_CASE("Robotize produces measurable change", "[audio][robotize]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 0.3f, harness.getSampleRate());
    int trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    harness.playAllTracks();

    auto dry = harness.processSeconds(0.3f);

    harness.clearAllTracks();
    trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    harness.playAllTracks();
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "robotize", 0, 0.8f);
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "robotize", 1, 5.0f);
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "robotize", 2, 0);

    auto wet = harness.processSeconds(0.3f);

    REQUIRE(SignalAnalyzer::signalsDifferent(dry, wet, 0.01f));
}
