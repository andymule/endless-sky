#pragma once

#include "signalsmith-stretch/signalsmith-stretch.h"
#include <atomic>
#include <vector>

namespace Dynamix {

    /**
     * Real-time pitch-preserving time stretcher (Ableton Warp-style).
     *
     * Tempo is the ratio of source frames consumed per output frame:
     *   2.0 = twice as fast, original pitch
     *   0.5 = half speed, original pitch
     *
     * Each playing voice owns an instance. All voices share a global tempo
     * so layered stems stay in time. Processing happens on the audio thread
     * with preallocated buffers — no worker thread, no ring buffers.
     */
    class AudioStreamProcessor {
    public:
        static constexpr float kMinTempo = 0.5f;
        static constexpr float kMaxTempo = 2.0f;
        static constexpr float kBypassEpsilon = 0.001f;

        static void setGlobalTempo(float tempo);
        static float getGlobalTempo();
        static bool isGlobalStretchActive();

        AudioStreamProcessor() = default;

        bool configure(int sampleRate, int channels);
        void reset();

        int inputLatency() const;
        int outputLatency() const;
        float latencyMs() const;
        static float typicalLatencyMs(int sampleRate = 44100);

        int sampleRate() const { return m_sampleRate; }
        int channels() const { return m_channels; }
        bool isConfigured() const { return m_configured; }

        /**
         * Convert output frame count to input frames at `tempo`, keeping a
         * running fractional phase so the ratio stays exact over time.
         */
        static int inputFramesFor(int outputFrames, double& phase, float tempo);

        void process(const float* const* input, int inputFrames, float* const* output,
                     int outputFrames);

        /** Prime internal buffers after reset/seek so the first process() is aligned. */
        void prime(const float* const* input, int inputFrames, float playbackRateHint);

    private:
        bool m_configured = false;
        int m_sampleRate = 0;
        int m_channels = 0;
        signalsmith::stretch::SignalsmithStretch<float> m_stretch;
    };

} // namespace Dynamix
