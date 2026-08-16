#include "AudioStreamProcessor.h"
#include <algorithm>
#include <cmath>

namespace Dynamix {

    namespace {
        std::atomic<float> g_globalTempo{1.0f};
    }

    void AudioStreamProcessor::setGlobalTempo(float tempo) {
        g_globalTempo.store(std::clamp(tempo, kMinTempo, kMaxTempo), std::memory_order_relaxed);
    }

    float AudioStreamProcessor::getGlobalTempo() {
        return g_globalTempo.load(std::memory_order_relaxed);
    }

    bool AudioStreamProcessor::isGlobalStretchActive() {
        return std::abs(getGlobalTempo() - 1.0f) > kBypassEpsilon;
    }

    bool AudioStreamProcessor::configure(int sampleRate, int channels) {
        if (sampleRate <= 0 || channels <= 0 || channels > 8) {
            m_configured = false;
            return false;
        }

        m_sampleRate = sampleRate;
        m_channels = channels;

        // splitComputation spreads FFT work across callbacks so a 512-frame
        // SoLoud block doesn't hitch. presetDefault is the high-quality Warp preset
        // (~120ms analysis window, similar to Ableton Complex).
        m_stretch.presetDefault(channels, static_cast<float>(sampleRate), true);
        m_stretch.setTransposeSemitones(0.0f);
        m_stretch.reset();
        m_configured = true;
        return true;
    }

    void AudioStreamProcessor::reset() {
        if (m_configured) {
            m_stretch.reset();
        }
    }

    int AudioStreamProcessor::inputLatency() const {
        return m_configured ? m_stretch.inputLatency() : 0;
    }

    int AudioStreamProcessor::outputLatency() const {
        return m_configured ? m_stretch.outputLatency() : 0;
    }

    float AudioStreamProcessor::latencyMs() const {
        if (!m_configured || m_sampleRate <= 0) {
            return 0.0f;
        }
        return (static_cast<float>(inputLatency() + outputLatency()) / m_sampleRate) * 1000.0f;
    }

    float AudioStreamProcessor::typicalLatencyMs(int sampleRate) {
        static int cachedRate = 0;
        static float cachedMs = 0.0f;
        if (cachedRate != sampleRate) {
            AudioStreamProcessor probe;
            probe.configure(sampleRate, 2);
            cachedRate = sampleRate;
            cachedMs = probe.latencyMs();
        }
        return cachedMs;
    }

    int AudioStreamProcessor::inputFramesFor(int outputFrames, double& phase, float tempo) {
        if (outputFrames <= 0) {
            return 0;
        }
        if (tempo <= 0.0f) {
            tempo = 1.0f;
        }
        phase += static_cast<double>(outputFrames) * static_cast<double>(tempo);
        int inputFrames = static_cast<int>(phase);
        phase -= static_cast<double>(inputFrames);
        if (inputFrames < 1) {
            inputFrames = 1;
            phase = 0.0;
        }
        return inputFrames;
    }

    void AudioStreamProcessor::process(const float* const* input, int inputFrames,
                                       float* const* output, int outputFrames) {
        if (!m_configured || inputFrames <= 0 || outputFrames <= 0) {
            return;
        }
        m_stretch.process(input, inputFrames, output, outputFrames);
    }

    void AudioStreamProcessor::prime(const float* const* input, int inputFrames,
                                     float playbackRateHint) {
        if (!m_configured || inputFrames <= 0) {
            return;
        }
        m_stretch.seek(input, inputFrames, static_cast<double>(playbackRateHint));
    }

} // namespace Dynamix
