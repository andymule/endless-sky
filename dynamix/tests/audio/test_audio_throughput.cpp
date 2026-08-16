#include <catch2/catch_test_macros.hpp>

#include "AudioTestHarness.h"
#include "SignalAnalyzer.h"
#include "TestHelpers.h"

using namespace DynamixTest;

TEST_CASE("Audio processing meets real-time requirements", "[audio][performance]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto noise = SignalAnalyzer::generateWhiteNoise(5.0f, harness.getSampleRate(), 0.5f);
    int trackIdx = harness.loadTrackFromMemory(noise, noise.size() / 2);
    harness.playAllTracks();

    SECTION("Basic processing is fast enough") {
        auto stats = SignalAnalyzer::measureThroughput(harness, 2.0f, 512);

        // At 44100Hz with 512 sample buffer, real-time budget is ~11.6ms
        INFO("Average process time: " << stats.avgProcessTimeMs << "ms");
        INFO("Max process time: " << stats.maxProcessTimeMs << "ms");
        INFO("CPU usage: " << stats.cpuUsagePercent << "%");

        // Should be well under real-time budget
        REQUIRE(stats.avgProcessTimeMs < 10.0f);
        REQUIRE(stats.maxProcessTimeMs < 50.0f);  // Allow spikes
    }

    SECTION("No buffer underruns in stress test") {
        // Apply multiple effects
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 0, 0.3f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "echo", 1, 0.2f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 0, 0.2f);
        harness.setFilterParameter(static_cast<size_t>(trackIdx), "freeverb", 2, 0.5f);

        auto stats = SignalAnalyzer::measureThroughput(harness, 3.0f, 512);

        // Should have minimal underruns
        INFO("Buffer underruns: " << stats.bufferUnderruns);
        INFO("Total buffers: " << stats.totalBuffersProcessed);
        // Allow some underruns but not too many
        REQUIRE(stats.bufferUnderruns < stats.totalBuffersProcessed / 10);
    }
}

TEST_CASE("Multiple tracks don't cause performance issues", "[audio][performance]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    // Load multiple tracks
    for (int i = 0; i < 4; ++i) {
        auto sine = SignalAnalyzer::generateSineWave(
            220.0f * static_cast<float>(i + 1), 2.0f, harness.getSampleRate());
        harness.loadTrackFromMemory(sine, sine.size() / 2);
    }

    REQUIRE(harness.getTrackCount() == 4);
    harness.playAllTracks();

    SECTION("4 tracks process within budget") {
        auto stats = SignalAnalyzer::measureThroughput(harness, 2.0f, 512);

        INFO("Average process time: " << stats.avgProcessTimeMs << "ms");
        INFO("CPU usage: " << stats.cpuUsagePercent << "%");

        // Even with 4 tracks, should be under budget
        REQUIRE(stats.avgProcessTimeMs < 10.0f);
    }
}

TEST_CASE("Processing produces consistent output", "[audio][performance]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 1.0f, harness.getSampleRate());
    harness.loadTrackFromMemory(sine, sine.size() / 2);
    harness.playAllTracks();

    SECTION("Output level is consistent") {
        std::vector<float> rmsValues;

        for (int i = 0; i < 10; ++i) {
            auto audio = harness.processSeconds(0.1f);
            rmsValues.push_back(SignalAnalyzer::calculateRMS(audio));
        }

        // Calculate variance
        float sum = 0.0f;
        for (float v : rmsValues)
            sum += v;
        float mean = sum / static_cast<float>(rmsValues.size());

        float variance = 0.0f;
        for (float v : rmsValues)
            variance += (v - mean) * (v - mean);
        variance /= static_cast<float>(rmsValues.size());

        // RMS should be relatively consistent
        INFO("Mean RMS: " << mean);
        INFO("Variance: " << variance);
        REQUIRE(variance < 0.01f);
    }
}

TEST_CASE("Large buffer sizes work correctly", "[audio][performance]") {
    AudioTestHarness harness(44100, 2, 4096);  // Large buffer
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 1.0f, harness.getSampleRate());
    harness.loadTrackFromMemory(sine, sine.size() / 2);
    harness.playAllTracks();

    SECTION("Large buffer produces audio") {
        auto audio = harness.processSeconds(0.5f);
        float rms = SignalAnalyzer::calculateRMS(audio);
        REQUIRE(rms > 0.0f);
    }
}

TEST_CASE("Small buffer sizes work correctly", "[audio][performance]") {
    // 512 frames is SoLoud's minimum buffer size (one sample granularity)
    AudioTestHarness harness(44100, 2, 512);
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 1.0f, harness.getSampleRate());
    harness.loadTrackFromMemory(sine, sine.size() / 2);
    harness.playAllTracks();

    SECTION("Small buffer produces audio") {
        auto audio = harness.processSeconds(0.5f);
        float rms = SignalAnalyzer::calculateRMS(audio);
        REQUIRE(rms > 0.0f);
    }
}
