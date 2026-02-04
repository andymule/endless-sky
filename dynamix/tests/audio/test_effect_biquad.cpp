#include <catch2/catch_test_macros.hpp>

#include "AudioTestHarness.h"
#include "SignalAnalyzer.h"
#include "TestHelpers.h"

using namespace DynamixTest;

TEST_CASE("Biquad filter produces frequency filtering", "[audio][biquad]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    // Use white noise to test frequency filtering
    auto noise = SignalAnalyzer::generateWhiteNoise(0.5f, harness.getSampleRate(), 0.5f);

    SECTION("Low-pass filter reduces high frequencies") {
        int trackIdx = harness.loadTrackFromMemory(noise, noise.size() / 2);
        harness.playAllTracks();

        auto dry = harness.processSeconds(0.3f);
        float dryHFRatio = SignalAnalyzer::measureHighFrequencyRatio(
            dry, harness.getSampleRate(), 2000.0f);

        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(noise, noise.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 0, 1.0f);    // wet
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 1, 0);       // type: lowpass
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 2, 500.0f);  // freq
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 3, 1.0f);    // resonance

        auto wet = harness.processSeconds(0.3f);
        float wetHFRatio = SignalAnalyzer::measureHighFrequencyRatio(
            wet, harness.getSampleRate(), 2000.0f);

        // Low-pass should significantly reduce high frequency content
        REQUIRE(wetHFRatio < dryHFRatio);
    }

    SECTION("Cutoff frequency affects filtering") {
        int trackIdx = harness.loadTrackFromMemory(noise, noise.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 1, 0);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 2, 500.0f);  // Low cutoff
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 3, 1.0f);

        auto lowCutoff = harness.processSeconds(0.3f);

        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(noise, noise.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 1, 0);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 2, 4000.0f);  // High cutoff
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 3, 1.0f);

        auto highCutoff = harness.processSeconds(0.3f);

        float lowHFRatio = SignalAnalyzer::measureHighFrequencyRatio(
            lowCutoff, harness.getSampleRate(), 2000.0f);
        float highHFRatio = SignalAnalyzer::measureHighFrequencyRatio(
            highCutoff, harness.getSampleRate(), 2000.0f);

        // Higher cutoff should allow more high frequency through
        REQUIRE(highHFRatio > lowHFRatio);
    }
}

TEST_CASE("Biquad filter types work correctly", "[audio][biquad]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto noise = SignalAnalyzer::generateWhiteNoise(0.5f, harness.getSampleRate(), 0.5f);

    SECTION("Different filter types produce different results") {
        // Low-pass (type 0)
        int trackIdx = harness.loadTrackFromMemory(noise, noise.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 1, 0);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 2, 1000.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 3, 2.0f);

        auto lowpass = harness.processSeconds(0.3f);

        // High-pass (type 1)
        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(noise, noise.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 1, 1);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 2, 1000.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 3, 2.0f);

        auto highpass = harness.processSeconds(0.3f);

        // Low-pass and high-pass should be different
        REQUIRE(SignalAnalyzer::signalsDifferent(lowpass, highpass, 0.01f));
    }
}

TEST_CASE("Biquad produces measurable change", "[audio][biquad]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto noise = SignalAnalyzer::generateWhiteNoise(0.3f, harness.getSampleRate(), 0.5f);
    int trackIdx = harness.loadTrackFromMemory(noise, noise.size() / 2);
    harness.playAllTracks();

    auto dry = harness.processSeconds(0.3f);

    harness.clearAllTracks();
    trackIdx = harness.loadTrackFromMemory(noise, noise.size() / 2);
    harness.playAllTracks();
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 0, 1.0f);
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 1, 0);
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 2, 500.0f);
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "biquad", 3, 5.0f);

    auto wet = harness.processSeconds(0.3f);

    REQUIRE(SignalAnalyzer::signalsDifferent(dry, wet, 0.01f));
}
