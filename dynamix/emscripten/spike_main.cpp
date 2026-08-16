/**
 * Minimal WASM spike for Dynamix browser viability.
 *
 * Verifies the two riskiest native dependencies compile and run under
 * Emscripten:
 *   1. Signalsmith Stretch via AudioStreamProcessor
 *   2. SoLoud NULL backend + SyncWav granular playback path
 *
 * Exit codes are stable so CI / `node spike.js` can assert success.
 */

#include "AudioStreamProcessor.h"
#include "SyncWav.h"

#include "soloud.h"
#include "soloud_bus.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

constexpr int kSampleRate = 44100;
constexpr int kChannels = 2;
constexpr int kBufferSize = 2048;

std::vector<float> generateSine(float frequencyHz, float seconds) {
    const size_t frames = static_cast<size_t>(kSampleRate * seconds);
    std::vector<float> samples(frames * static_cast<size_t>(kChannels));
    for (size_t frame = 0; frame < frames; ++frame) {
        const float value =
            std::sin(2.0f * static_cast<float>(M_PI) * frequencyHz *
                     static_cast<float>(frame) / static_cast<float>(kSampleRate));
        for (int channel = 0; channel < kChannels; ++channel) {
            samples[frame * static_cast<size_t>(kChannels) + static_cast<size_t>(channel)] =
                value;
        }
    }
    return samples;
}

float calculateRMS(const std::vector<float>& samples) {
    if (samples.empty()) {
        return 0.0f;
    }
    double sumSquares = 0.0;
    for (float sample : samples) {
        sumSquares += static_cast<double>(sample) * static_cast<double>(sample);
    }
    return static_cast<float>(std::sqrt(sumSquares / static_cast<double>(samples.size())));
}

bool loadSineIntoSyncWav(Dynamix::SyncWav& wav, const std::vector<float>& interleaved,
                         size_t numFrames) {
    const size_t frames = std::min(numFrames, interleaved.size() / static_cast<size_t>(kChannels));
    if (frames == 0) {
        return false;
    }

    std::vector<short> planar(frames * static_cast<size_t>(kChannels));
    for (size_t frame = 0; frame < frames; ++frame) {
        for (size_t channel = 0; channel < static_cast<size_t>(kChannels); ++channel) {
            const float sample = std::clamp(
                interleaved[frame * static_cast<size_t>(kChannels) + channel], -1.0f, 1.0f);
            planar[channel * frames + frame] = static_cast<short>(sample * 32767.0f);
        }
    }

    return wav.loadRawWave16(planar.data(), static_cast<unsigned int>(planar.size()),
                             static_cast<float>(kSampleRate),
                             static_cast<unsigned int>(kChannels)) == SoLoud::SO_NO_ERROR;
}

int testAudioStreamProcessor() {
    Dynamix::AudioStreamProcessor processor;
    if (!processor.configure(kSampleRate, kChannels)) {
        std::fprintf(stderr, "AudioStreamProcessor::configure failed\n");
        return 1;
    }

    const int outputFrames = 512;
    const float tempo = 2.0f;
    double phase = 0.0;
    const int inputFrames =
        Dynamix::AudioStreamProcessor::inputFramesFor(outputFrames, phase, tempo);

    std::vector<float> channel0(static_cast<size_t>(inputFrames), 0.0f);
    std::vector<float> channel1(static_cast<size_t>(inputFrames), 0.0f);
    for (int frame = 0; frame < inputFrames; ++frame) {
        const float value =
            std::sin(2.0f * static_cast<float>(M_PI) * 440.0f * static_cast<float>(frame) /
                     static_cast<float>(kSampleRate));
        channel0[static_cast<size_t>(frame)] = value;
        channel1[static_cast<size_t>(frame)] = value;
    }

    const float* inputs[2] = {channel0.data(), channel1.data()};
    std::vector<float> out0(static_cast<size_t>(outputFrames), 0.0f);
    std::vector<float> out1(static_cast<size_t>(outputFrames), 0.0f);
    float* outputs[2] = {out0.data(), out1.data()};

    const int primeFrames = std::max(1, processor.inputLatency());
    std::vector<float> prime0(static_cast<size_t>(primeFrames), 0.0f);
    std::vector<float> prime1(static_cast<size_t>(primeFrames), 0.0f);
    for (int frame = 0; frame < primeFrames; ++frame) {
        const float value =
            std::sin(2.0f * static_cast<float>(M_PI) * 440.0f * static_cast<float>(frame) /
                     static_cast<float>(kSampleRate));
        prime0[static_cast<size_t>(frame)] = value;
        prime1[static_cast<size_t>(frame)] = value;
    }
    const float* primeInputs[2] = {prime0.data(), prime1.data()};
    processor.prime(primeInputs, primeFrames, tempo);

    std::vector<float> interleaved;
    interleaved.reserve(static_cast<size_t>(outputFrames * kChannels * 4));
    for (int pass = 0; pass < 4; ++pass) {
        processor.process(inputs, inputFrames, outputs, outputFrames);
        for (int frame = 0; frame < outputFrames; ++frame) {
            interleaved.push_back(out0[static_cast<size_t>(frame)]);
            interleaved.push_back(out1[static_cast<size_t>(frame)]);
        }
    }

    const float rms = calculateRMS(interleaved);
    if (rms < 0.001f) {
        std::fprintf(stderr, "AudioStreamProcessor produced near-silent output (rms=%f)\n", rms);
        return 2;
    }

    return 0;
}

int testSoloudSyncWavGranular() {
    Dynamix::AudioStreamProcessor::setGlobalTempo(2.0f);

    SoLoud::Soloud engine;
    if (engine.init(SoLoud::Soloud::CLIP_ROUNDOFF, SoLoud::Soloud::NULLDRIVER,
                    static_cast<unsigned int>(kSampleRate),
                    static_cast<unsigned int>(kBufferSize),
                    static_cast<unsigned int>(kChannels)) != SoLoud::SO_NO_ERROR) {
        std::fprintf(stderr, "SoLoud NULLDRIVER init failed\n");
        return 10;
    }

    SoLoud::Bus bus;
    const unsigned int busHandle = engine.play(bus);

    Dynamix::SyncWav wav;
    const auto sine = generateSine(440.0f, 4.0f);
    if (!loadSineIntoSyncWav(wav, sine, sine.size() / static_cast<size_t>(kChannels))) {
        std::fprintf(stderr, "Failed to load sine into SyncWav\n");
        return 11;
    }
    wav.setLooping(true);

    const unsigned int voiceHandle = bus.play(wav);
    if (voiceHandle == 0) {
        std::fprintf(stderr, "Failed to start SyncWav voice\n");
        return 12;
    }

    std::vector<float> capture;
    constexpr size_t totalFrames = 44100;
    capture.reserve(totalFrames * static_cast<size_t>(kChannels));

    const size_t chunkFrames = static_cast<size_t>(kBufferSize);
    std::vector<float> chunk(chunkFrames * static_cast<size_t>(kChannels));
    for (size_t frame = 0; frame < totalFrames; frame += chunkFrames) {
        const size_t frames = std::min(chunkFrames, totalFrames - frame);
        engine.mix(chunk.data(), static_cast<unsigned int>(frames));
        capture.insert(capture.end(), chunk.begin(),
                       chunk.begin() + static_cast<std::ptrdiff_t>(frames * static_cast<size_t>(kChannels)));
    }

    engine.stop(busHandle);
    engine.deinit();
    Dynamix::AudioStreamProcessor::setGlobalTempo(1.0f);

    if (calculateRMS(capture) < 0.01f) {
        std::fprintf(stderr, "SoLoud mix produced near-silent output\n");
        return 13;
    }

    return 0;
}

} // namespace

int main() {
    const int stretchResult = testAudioStreamProcessor();
    if (stretchResult != 0) {
        std::fprintf(stderr, "FAIL: Signalsmith stretch spike (code %d)\n", stretchResult);
        return stretchResult;
    }

    const int soloudResult = testSoloudSyncWavGranular();
    if (soloudResult != 0) {
        std::fprintf(stderr, "FAIL: SoLoud/SyncWav spike (code %d)\n", soloudResult);
        return soloudResult;
    }

    std::printf("PASS: dynamix emscripten wasm spike\n");
    return 0;
}
