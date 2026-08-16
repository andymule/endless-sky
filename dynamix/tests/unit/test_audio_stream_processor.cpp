#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "AudioStreamProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using Catch::Matchers::WithinAbs;
using Dynamix::AudioStreamProcessor;

namespace {

std::vector<float> makeSine(float frequency, int frames, int sampleRate, float amplitude = 0.5f) {
    std::vector<float> samples(static_cast<size_t>(frames));
    for (int i = 0; i < frames; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        samples[static_cast<size_t>(i)] =
            amplitude * std::sin(2.0f * static_cast<float>(M_PI) * frequency * t);
    }
    return samples;
}

float zeroCrossingFrequency(const std::vector<float>& samples, int sampleRate) {
    if (samples.size() < 16) {
        return 0.0f;
    }
    int crossings = 0;
    for (size_t i = 1; i < samples.size(); ++i) {
        if ((samples[i - 1] < 0.0f && samples[i] >= 0.0f) ||
            (samples[i - 1] > 0.0f && samples[i] <= 0.0f)) {
            ++crossings;
        }
    }
    const float duration = static_cast<float>(samples.size() - 1) / static_cast<float>(sampleRate);
    return (static_cast<float>(crossings) * 0.5f) / duration;
}

std::vector<float> stretchSine(float tempo, int sampleRate, float outputSeconds) {
    AudioStreamProcessor processor;
    REQUIRE(processor.configure(sampleRate, 1));

    const int outputFrames = static_cast<int>(outputSeconds * static_cast<float>(sampleRate));
    const int inputNeeded =
        processor.inputLatency() + static_cast<int>(outputFrames * tempo) + sampleRate;
    auto source = makeSine(440.0f, inputNeeded, sampleRate);

    std::vector<float> output(static_cast<size_t>(outputFrames), 0.0f);
    double phase = 0.0;
    int inPos = 0;

    const int primeFrames = std::max(1, processor.inputLatency());
    REQUIRE(inPos + primeFrames <= static_cast<int>(source.size()));
    const float* primePtr = source.data() + inPos;
    processor.prime(&primePtr, primeFrames, tempo);
    inPos += primeFrames;

    const int chunk = 512;
    int outPos = 0;
    while (outPos < outputFrames) {
        const int outChunk = std::min(chunk, outputFrames - outPos);
        const int inFrames = AudioStreamProcessor::inputFramesFor(outChunk, phase, tempo);
        REQUIRE(inPos + inFrames <= static_cast<int>(source.size()));

        const float* inPtr = source.data() + inPos;
        float* outPtr = output.data() + outPos;
        processor.process(&inPtr, inFrames, &outPtr, outChunk);
        inPos += inFrames;
        outPos += outChunk;
    }

    return output;
}

} // namespace

TEST_CASE("AudioStreamProcessor input/output ratio tracks tempo", "[unit][granular]") {
    double phase = 0.0;
    int totalIn = 0;
    constexpr int chunks = 100;
    constexpr int outChunk = 512;
    constexpr float tempo = 1.5f;

    for (int i = 0; i < chunks; ++i) {
        totalIn += AudioStreamProcessor::inputFramesFor(outChunk, phase, tempo);
    }

    const int expected = static_cast<int>(std::lround(chunks * outChunk * tempo));
    REQUIRE(std::abs(totalIn - expected) <= 2);
}

TEST_CASE("Zero-crossing detector sees 440Hz sines", "[unit][granular]") {
    auto sine = makeSine(440.0f, 44100, 44100);
    REQUIRE_THAT(zeroCrossingFrequency(sine, 44100), WithinAbs(440.0f, 2.0f));
}

TEST_CASE("AudioStreamProcessor preserves pitch while changing tempo", "[unit][granular]") {
    constexpr int sampleRate = 44100;

    auto measure = [&](float tempo) {
        auto audio = stretchSine(tempo, sampleRate, 1.5f);
        AudioStreamProcessor probe;
        probe.configure(sampleRate, 1);
        const size_t skip = static_cast<size_t>((probe.latencyMs() / 1000.0f + 0.15f) * sampleRate);
        REQUIRE(audio.size() > skip + 4096);
        std::vector<float> steady(audio.begin() + static_cast<std::ptrdiff_t>(skip), audio.end());

        double sumSq = 0.0;
        float peak = 0.0f;
        for (float s : steady) {
            sumSq += static_cast<double>(s) * s;
            peak = std::max(peak, std::abs(s));
        }
        const float rms = static_cast<float>(std::sqrt(sumSq / static_cast<double>(steady.size())));
        const float freq = zeroCrossingFrequency(steady, sampleRate);
        INFO("tempo=" << tempo << " latencyMs=" << probe.latencyMs() << " skip=" << skip
                      << " rms=" << rms << " peak=" << peak << " freq=" << freq);
        REQUIRE(rms > 0.05f);
        return freq;
    };

    SECTION("1.0x keeps 440Hz") {
        REQUIRE_THAT(measure(1.0f), WithinAbs(440.0f, 20.0f));
    }
    SECTION("2.0x keeps 440Hz") {
        REQUIRE_THAT(measure(2.0f), WithinAbs(440.0f, 25.0f));
    }
    SECTION("0.5x keeps 440Hz") {
        REQUIRE_THAT(measure(0.5f), WithinAbs(440.0f, 25.0f));
    }
}

TEST_CASE("Global grain tempo is independent of instance lifetime", "[unit][granular]") {
    AudioStreamProcessor::setGlobalTempo(1.25f);
    REQUIRE_THAT(AudioStreamProcessor::getGlobalTempo(), WithinAbs(1.25f, 0.0001f));
    REQUIRE(AudioStreamProcessor::isGlobalStretchActive());

    AudioStreamProcessor::setGlobalTempo(1.0f);
    REQUIRE_FALSE(AudioStreamProcessor::isGlobalStretchActive());
}
