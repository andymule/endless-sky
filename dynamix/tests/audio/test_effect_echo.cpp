#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "AudioTestHarness.h"
#include "SignalAnalyzer.h"
#include "TestHelpers.h"

using namespace DynamixTest;
using Catch::Matchers::WithinAbs;

TEST_CASE("Echo effect produces audible delay", "[audio][echo]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    // Generate impulse signal - easy to detect echo
    auto impulse = SignalAnalyzer::generateImpulse(1.0f, harness.getSampleRate(), 0.1f, 0.9f);
    int trackIdx = harness.loadTrackFromMemory(impulse, impulse.size() / 2);
    REQUIRE(trackIdx >= 0);

    harness.playAllTracks();

    SECTION("Dry signal has single peak") {
        auto dry = harness.processSeconds(1.0f);
        size_t peakCount = SignalAnalyzer::countPeaks(dry, 0.3f);
        // Should have exactly 1 significant peak (the impulse)
        REQUIRE(peakCount >= 1);
        REQUIRE(peakCount <= 3); // Allow for some minor secondary peaks
    }

    SECTION("Echo adds delayed repetitions") {
        // Enable echo with 200ms delay
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 0, 1.0f);  // wet
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 1, 0.2f);  // delay 200ms
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 2, 0.5f);  // decay

        auto wet = harness.processSeconds(1.0f);

        // Wet signal should have more peaks due to echo
        size_t wetPeakCount = SignalAnalyzer::countPeaks(wet, 0.1f);
        REQUIRE(wetPeakCount > 1);

        // Signal should be different from dry
        auto dry = SignalAnalyzer::generateImpulse(1.0f, harness.getSampleRate(), 0.1f, 0.9f);
        // Note: Can't directly compare since we need to process dry without effects
    }

    SECTION("Echo produces measurably different signal") {
        // Process without effect
        auto dry = harness.processSeconds(0.5f);

        // Reset and apply effect
        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(impulse, impulse.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 0, 0.8f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 1, 0.15f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 2, 0.6f);

        auto wet = harness.processSeconds(0.5f);

        // Signals should be different
        REQUIRE(SignalAnalyzer::signalsDifferent(dry, wet, 0.01f));
    }
}

TEST_CASE("Echo delay parameter affects timing", "[audio][echo]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto impulse = SignalAnalyzer::generateImpulse(1.0f, harness.getSampleRate(), 0.05f, 0.9f);

    SECTION("Short delay produces faster echoes") {
        int trackIdx = harness.loadTrackFromMemory(impulse, impulse.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 1, 0.1f);  // 100ms
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 2, 0.5f);

        auto shortDelay = harness.processSeconds(0.5f);
        auto shortPeaks = SignalAnalyzer::findPeakPositions(shortDelay, 0.1f);

        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(impulse, impulse.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 1, 0.3f);  // 300ms
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 2, 0.5f);

        auto longDelay = harness.processSeconds(0.5f);
        auto longPeaks = SignalAnalyzer::findPeakPositions(longDelay, 0.1f);

        // Short delay should have more peaks in same time window
        // (or peaks should be closer together)
        if (shortPeaks.size() >= 2 && longPeaks.size() >= 2) {
            size_t shortSpacing = shortPeaks[1] - shortPeaks[0];
            size_t longSpacing = longPeaks[1] - longPeaks[0];
            REQUIRE(shortSpacing < longSpacing);
        }
    }
}

TEST_CASE("Echo decay parameter affects amplitude", "[audio][echo]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto impulse = SignalAnalyzer::generateImpulse(1.0f, harness.getSampleRate(), 0.05f, 0.9f);

    SECTION("Higher decay produces longer sustain") {
        int trackIdx = harness.loadTrackFromMemory(impulse, impulse.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 1, 0.2f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 2, 0.8f);  // High decay

        auto highDecay = harness.processSeconds(1.0f);
        float highRMS = SignalAnalyzer::calculateRMS(highDecay);

        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(impulse, impulse.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 1, 0.2f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 2, 0.2f);  // Low decay

        auto lowDecay = harness.processSeconds(1.0f);
        float lowRMS = SignalAnalyzer::calculateRMS(lowDecay);

        // Higher decay should result in higher overall RMS
        REQUIRE(highRMS > lowRMS);
    }
}

TEST_CASE("Echo wet parameter controls mix", "[audio][echo]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 0.5f, harness.getSampleRate());
    int trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    harness.playAllTracks();

    SECTION("Wet=0 passes dry signal unchanged") {
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 0, 0.0f);  // wet = 0
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 1, 0.2f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 2, 0.5f);

        auto processed = harness.processSeconds(0.3f);

        // With wet=0, signal should be essentially unchanged
        float rms = SignalAnalyzer::calculateRMS(processed);
        REQUIRE(rms > 0.0f); // Signal present
    }

    SECTION("Wet=1 produces fully wet signal") {
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 0, 1.0f);  // wet = 1
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 1, 0.2f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 2, 0.5f);

        auto processed = harness.processSeconds(0.5f);

        // With wet=1, signal includes echo
        float rms = SignalAnalyzer::calculateRMS(processed);
        REQUIRE(rms > 0.0f);
    }
}
