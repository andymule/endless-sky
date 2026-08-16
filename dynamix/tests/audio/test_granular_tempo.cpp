#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "AudioTestHarness.h"
#include "SignalAnalyzer.h"
#include "TestHelpers.h"

#include <cstddef>

using namespace DynamixTest;
using Catch::Matchers::WithinAbs;

namespace {

std::vector<float> leftChannel(const std::vector<float>& interleaved, int channels) {
    std::vector<float> left;
    if (channels <= 0 || interleaved.empty()) {
        return left;
    }
    left.reserve(interleaved.size() / static_cast<size_t>(channels));
    for (size_t i = 0; i < interleaved.size(); i += static_cast<size_t>(channels)) {
        left.push_back(interleaved[i]);
    }
    return left;
}

std::vector<float> dropSeconds(const std::vector<float>& interleaved, int channels, int sampleRate,
                               float seconds) {
    const size_t skip = static_cast<size_t>(seconds * static_cast<float>(sampleRate)) *
                        static_cast<size_t>(channels);
    if (skip >= interleaved.size()) {
        return {};
    }
    return {interleaved.begin() + static_cast<std::ptrdiff_t>(skip), interleaved.end()};
}

} // namespace

TEST_CASE("Grain tempo changes speed without changing pitch", "[audio][granular]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 6.0f, harness.getSampleRate());
    REQUIRE(harness.loadTrackFromMemory(sine, sine.size() / 2) >= 0);
    harness.playAllTracks();

    auto measurePitch = [&](float grainTempo) {
        harness.setGranularTempo(grainTempo);
        harness.setMasterTempo(1.0f);
        auto audio = harness.processSeconds(1.2f);
        auto steady = dropSeconds(audio, harness.getChannels(), harness.getSampleRate(), 0.3f);
        auto left = leftChannel(steady, harness.getChannels());
        REQUIRE(SignalAnalyzer::calculateRMS(left) > 0.01f);
        return SignalAnalyzer::findDominantFrequency(left, harness.getSampleRate());
    };

    SECTION("1.0x grain is original pitch") {
        REQUIRE_THAT(measurePitch(1.0f), WithinAbs(440.0f, 30.0f));
    }

    SECTION("2.0x grain keeps pitch") {
        REQUIRE_THAT(measurePitch(2.0f), WithinAbs(440.0f, 40.0f));
    }

    SECTION("0.5x grain keeps pitch") {
        REQUIRE_THAT(measurePitch(0.5f), WithinAbs(440.0f, 40.0f));
    }
}

TEST_CASE("Tape speed changes pitch independently of grain tempo", "[audio][tempo]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 4.0f, harness.getSampleRate());
    REQUIRE(harness.loadTrackFromMemory(sine, sine.size() / 2) >= 0);
    harness.playAllTracks();
    harness.setGranularTempo(1.0f);

    SECTION("2x tape doubles pitch") {
        harness.setMasterTempo(2.0f);
        auto audio = harness.processSeconds(0.6f);
        auto left = leftChannel(audio, harness.getChannels());
        float freq = SignalAnalyzer::findDominantFrequency(left, harness.getSampleRate());
        REQUIRE_THAT(freq, WithinAbs(880.0f, 80.0f));
    }

    SECTION("0.5x tape halves pitch") {
        harness.setMasterTempo(0.5f);
        auto audio = harness.processSeconds(1.0f);
        auto left = leftChannel(audio, harness.getChannels());
        float freq = SignalAnalyzer::findDominantFrequency(left, harness.getSampleRate());
        REQUIRE_THAT(freq, WithinAbs(220.0f, 40.0f));
    }
}

TEST_CASE("Tape speed and grain tempo stack independently", "[audio][granular][tempo]") {
    AudioTestHarness harness;
    REQUIRE(harness.initialize());

    auto sine = SignalAnalyzer::generateSineWave(440.0f, 8.0f, harness.getSampleRate());
    REQUIRE(harness.loadTrackFromMemory(sine, sine.size() / 2) >= 0);
    harness.playAllTracks();

    SECTION("2x grain + 1x tape keeps pitch and produces audio") {
        harness.setGranularTempo(2.0f);
        harness.setMasterTempo(1.0f);
        auto audio = harness.processSeconds(1.0f);
        auto steady = dropSeconds(audio, harness.getChannels(), harness.getSampleRate(), 0.3f);
        auto left = leftChannel(steady, harness.getChannels());
        REQUIRE(SignalAnalyzer::calculateRMS(left) > 0.01f);
        REQUIRE_THAT(SignalAnalyzer::findDominantFrequency(left, harness.getSampleRate()),
                     WithinAbs(440.0f, 40.0f));
    }

    SECTION("1x grain + 2x tape doubles pitch") {
        harness.setGranularTempo(1.0f);
        harness.setMasterTempo(2.0f);
        auto audio = harness.processSeconds(0.6f);
        auto left = leftChannel(audio, harness.getChannels());
        REQUIRE_THAT(SignalAnalyzer::findDominantFrequency(left, harness.getSampleRate()),
                     WithinAbs(880.0f, 80.0f));
    }

    SECTION("2x grain + 2x tape doubles pitch (grain does not add more pitch)") {
        harness.setGranularTempo(2.0f);
        harness.setMasterTempo(2.0f);
        auto audio = harness.processSeconds(1.0f);
        auto steady = dropSeconds(audio, harness.getChannels(), harness.getSampleRate(), 0.3f);
        auto left = leftChannel(steady, harness.getChannels());
        REQUIRE(SignalAnalyzer::calculateRMS(left) > 0.01f);
        REQUIRE_THAT(SignalAnalyzer::findDominantFrequency(left, harness.getSampleRate()),
                     WithinAbs(880.0f, 80.0f));
    }
}
