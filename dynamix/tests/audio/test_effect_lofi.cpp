#include <catch2/catch_test_macros.hpp>

#include "AudioTestHarness.h"
#include "SignalAnalyzer.h"
#include "TestHelpers.h"

using namespace DynamixTest;

TEST_CASE("LoFi effect produces bit crushing", "[audio][lofi]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 0.5f, harness.getSampleRate());
    int trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    REQUIRE(trackIdx >= 0);

    harness.playAllTracks();

    SECTION("Bit crushing produces quantization") {
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 0, 1.0f);      // wet
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 1, 8000.0f);   // sample rate
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 2, 4.0f);      // 4-bit

        auto processed = harness.processSeconds(0.3f);

        // Check for quantization
        REQUIRE(SignalAnalyzer::isBitCrushed(processed, 4));
    }

    SECTION("Higher bit depth produces less quantization") {
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 1, 8000.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 2, 8.0f);  // 8-bit

        auto highBit = harness.processSeconds(0.3f);

        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 1, 8000.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 2, 2.0f);  // 2-bit

        auto lowBit = harness.processSeconds(0.3f);

        // Both should be bit-crushed, but differently
        REQUIRE(SignalAnalyzer::isBitCrushed(highBit, 8));
        REQUIRE(SignalAnalyzer::isBitCrushed(lowBit, 2));
    }
}

TEST_CASE("LoFi sample rate reduction", "[audio][lofi]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    // Use high frequency content to test sample rate reduction
    auto noise = SignalAnalyzer::generateWhiteNoise(0.5f, harness.getSampleRate(), 0.5f);

    SECTION("Lower sample rate reduces high frequencies") {
        int trackIdx = harness.loadTrackFromMemory(noise, noise.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 1, 4000.0f);  // Low sample rate
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 2, 16.0f);

        auto lowSR = harness.processSeconds(0.3f);
        float lowHFRatio = SignalAnalyzer::measureHighFrequencyRatio(
            lowSR, harness.getSampleRate(), 2000.0f);

        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(noise, noise.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 1, 16000.0f);  // High sample rate
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 2, 16.0f);

        auto highSR = harness.processSeconds(0.3f);
        float highHFRatio = SignalAnalyzer::measureHighFrequencyRatio(
            highSR, harness.getSampleRate(), 2000.0f);

        // Lower sample rate should have less high frequency content
        REQUIRE(lowHFRatio < highHFRatio);
    }
}

TEST_CASE("LoFi produces measurable change", "[audio][lofi]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 0.3f, harness.getSampleRate());
    int trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    harness.playAllTracks();

    auto dry = harness.processSeconds(0.3f);

    harness.clearAllTracks();
    trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    harness.playAllTracks();
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 0, 1.0f);
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 1, 4000.0f);
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "lofi", 2, 4.0f);

    auto wet = harness.processSeconds(0.3f);

    REQUIRE(SignalAnalyzer::signalsDifferent(dry, wet, 0.01f));
}
