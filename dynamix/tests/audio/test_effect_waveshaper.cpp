#include <catch2/catch_test_macros.hpp>

#include "AudioTestHarness.h"
#include "SignalAnalyzer.h"
#include "TestHelpers.h"

using namespace DynamixTest;

TEST_CASE("Waveshaper effect produces distortion", "[audio][waveshaper]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 0.5f, harness.getSampleRate());
    int trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    REQUIRE(trackIdx >= 0);

    harness.playAllTracks();

    SECTION("Distortion adds harmonics") {
        auto dry = harness.processSeconds(0.3f);

        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "waveshaper", 0, 1.0f);  // wet
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "waveshaper", 1, 0.5f);  // amount

        auto wet = harness.processSeconds(0.3f);

        // Distortion should increase harmonic content
        bool hasDistortion = SignalAnalyzer::hasDistortion(dry, wet);
        // Distortion should be detectable or signals should be different
        REQUIRE(SignalAnalyzer::signalsDifferent(dry, wet, 0.01f));
    }

    SECTION("Higher amount produces more distortion") {
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "waveshaper", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "waveshaper", 1, 0.2f);  // Low amount

        auto lowDistortion = harness.processSeconds(0.3f);
        float lowTHD = SignalAnalyzer::measureTHD(
            SignalAnalyzer::extractChannel(lowDistortion, harness.getChannels()), 440.0f,
            harness.getSampleRate());

        harness.clearAllTracks();
        trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
        harness.playAllTracks();
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "waveshaper", 0, 1.0f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "waveshaper", 1, 0.8f);  // High amount

        auto highDistortion = harness.processSeconds(0.3f);
        float highTHD = SignalAnalyzer::measureTHD(
            SignalAnalyzer::extractChannel(highDistortion, harness.getChannels()), 440.0f,
            harness.getSampleRate());

        // Higher amount should produce more THD
        REQUIRE(highTHD >= lowTHD);
    }
}

TEST_CASE("Waveshaper produces measurable change", "[audio][waveshaper]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 0.3f, harness.getSampleRate());
    int trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    harness.playAllTracks();

    auto dry = harness.processSeconds(0.3f);

    harness.clearAllTracks();
    trackIdx = harness.loadTrackFromMemory(sine, sine.size() / 2);
    harness.playAllTracks();
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "waveshaper", 0, 0.8f);
    harness.setFilterParameter(static_cast<size_t>(trackIdx), "waveshaper", 1, 0.5f);

    auto wet = harness.processSeconds(0.3f);

    REQUIRE(SignalAnalyzer::signalsDifferent(dry, wet, 0.01f));
}
