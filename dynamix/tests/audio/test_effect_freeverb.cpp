#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "AudioTestHarness.h"
#include "SignalAnalyzer.h"
#include "TestHelpers.h"

using namespace DynamixTest;

TEST_CASE("Freeverb effect produces reverb tail", "[audio][freeverb]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    // Generate impulse for reverb testing
    auto impulse = SignalAnalyzer::generateImpulse(2.0f, harness.getSampleRate(), 0.05f, 0.9f);
    int trackIdx = harness.loadTrackFromMemory(impulse, impulse.size() / 2);
    REQUIRE(trackIdx >= 0);

    harness.playAllTracks();

    SECTION("Dry signal decays quickly") {
        auto dry = harness.processSeconds(1.0f);

        // Calculate RMS in last 500ms - should be near zero for dry impulse
        size_t lastHalfSamples = static_cast<size_t>(0.5f * harness.getSampleRate() * 2);
        std::vector<float> lastHalf(dry.end() - lastHalfSamples, dry.end());
        float tailRMS = SignalAnalyzer::calculateRMS(lastHalf);

        // Dry impulse should have very little energy in the tail
        REQUIRE(tailRMS < 0.05f);
    }

    SECTION("Reverb produces sustained tail") {
        const size_t lastHalfSamples =
            static_cast<size_t>(0.5f * static_cast<float>(harness.getSampleRate() *
                                                          harness.getChannels()));
        auto tailRMS = [&](const std::vector<float>& audio) {
            return SignalAnalyzer::calculateRMS(
                std::vector<float>(audio.end() - lastHalfSamples, audio.end()));
        };

        // Measure the dry tail first: a single impulse spread over 500ms is
        // quiet in absolute terms, so compare against dry instead of a
        // hand-picked level.
        const float dryTail = tailRMS(harness.processSeconds(2.0f));

        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(impulse, impulse.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 0, 1.0f);  // wet
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 2, 0.9f);  // large room
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 3, 0.3f);  // low damp

        const float wetTail = tailRMS(harness.processSeconds(2.0f));

        REQUIRE(wetTail > 0.0001f);
        REQUIRE(wetTail > dryTail * 10.0f);
    }
}

TEST_CASE("Freeverb room size affects decay", "[audio][freeverb]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto impulse = SignalAnalyzer::generateImpulse(2.0f, harness.getSampleRate(), 0.05f, 0.9f);

    SECTION("Large room produces longer decay") {
        int trackIdx = harness.loadTrackFromMemory(impulse, impulse.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 2, 0.95f);  // Large room
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 3, 0.3f);

        auto largeRoom = harness.processSeconds(1.5f);
        float largeRMS = SignalAnalyzer::calculateRMS(largeRoom);

        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(impulse, impulse.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 2, 0.2f);  // Small room
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 3, 0.3f);

        auto smallRoom = harness.processSeconds(1.5f);
        float smallRMS = SignalAnalyzer::calculateRMS(smallRoom);

        // Large room should have more sustained energy
        REQUIRE(largeRMS > smallRMS);
    }
}

TEST_CASE("Freeverb damping affects brightness", "[audio][freeverb]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    // Use white noise for frequency content analysis
    auto noise = SignalAnalyzer::generateWhiteNoise(0.5f, harness.getSampleRate(), 0.5f);

    SECTION("High damping reduces high frequencies") {
        int trackIdx = harness.loadTrackFromMemory(noise, noise.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 2, 0.8f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 3, 0.9f);  // High damp

        auto highDamp = harness.processSeconds(0.5f);
        float highDampRatio = SignalAnalyzer::measureHighFrequencyRatio(
            highDamp, harness.getSampleRate(), 4000.0f);

        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(noise, noise.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 2, 0.8f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 3, 0.1f);  // Low damp

        auto lowDamp = harness.processSeconds(0.5f);
        float lowDampRatio = SignalAnalyzer::measureHighFrequencyRatio(
            lowDamp, harness.getSampleRate(), 4000.0f);

        // High damping should have less high frequency content
        REQUIRE(highDampRatio < lowDampRatio);
    }
}

TEST_CASE("Freeverb produces measurable change", "[audio][freeverb]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 0.3f, harness.getSampleRate());
    int trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    harness.playAllTracks();

    // Process without effect
    auto dry = harness.processSeconds(0.3f);

    // Reset and apply effect
    harness.clearAllTracks();
    trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    harness.playAllTracks();
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 0, 0.8f);
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 2, 0.7f);
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 3, 0.5f);

    auto wet = harness.processSeconds(0.3f);

    // Signals should be different
    REQUIRE(SignalAnalyzer::signalsDifferent(dry, wet, 0.01f));
}
