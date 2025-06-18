#pragma once

#include "signalsmith-stretch/signalsmith-stretch.h"
#include <memory>
#include <mutex>
#include <vector>

namespace AudioTester {

    /**
     * MasterTempoProcessor - Real-time tempo stretching for master output
     *
     * This class handles real-time tempo modification of the final audio stream
     * using Signalsmith Stretch. It processes audio after SoLoud's mix but before
     * final output to speakers.
     *
     * Features:
     * - Real-time tempo control (0.1x to 4.0x range)
     * - High-quality granular synthesis via Signalsmith Stretch
     * - Thread-safe parameter updates
     * - Automatic buffer management
     * - Low-latency processing (~20-40ms)
     */
    class MasterTempoProcessor {
    public:
        MasterTempoProcessor();
        ~MasterTempoProcessor();

        /**
         * Initialize the processor
         * @param sampleRate Audio sample rate (e.g., 44100)
         * @param channels Number of audio channels (1=mono, 2=stereo)
         * @param blockSize Preferred processing block size (default: 1024)
         * @return true on success, false on failure
         */
        bool initialize(int sampleRate, int channels, int blockSize = 1024);

        /**
         * Process audio with current tempo scaling
         * @param inputBuffer Input audio samples [channel][sample]
         * @param outputBuffer Output audio samples [channel][sample]
         * @param inputSamples Number of input samples per channel
         * @param outputSamples Number of output samples per channel (may differ from input)
         * @return true on success, false on error
         */
        bool process(const float* const* inputBuffer, float** outputBuffer, int inputSamples,
                     int outputSamples);

        /**
         * Set master tempo multiplier
         * @param tempo Tempo multiplier (1.0 = normal, 0.5 = half speed, 2.0 = double speed)
         *              Valid range: 0.1 to 4.0
         */
        void setTempo(float tempo);

        /**
         * Get current tempo multiplier
         * @return Current tempo setting
         */
        float getTempo() const;

        /**
         * Enable/disable tempo processing
         * @param enabled If false, audio passes through unprocessed
         */
        void setEnabled(bool enabled);

        /**
         * Check if processor is enabled
         * @return true if enabled, false if bypassed
         */
        bool isEnabled() const;

        /**
         * Get current processing latency in samples
         * @return Latency in samples (depends on block size and stretch algorithm)
         */
        int getLatencySamples() const;

        /**
         * Get current processing latency in milliseconds
         * @return Latency in milliseconds
         */
        float getLatencyMs() const;

        /**
         * Reset internal buffers and state
         * Call this when seeking or when audio stream is discontinuous
         */
        void reset();

        /**
         * Check if processor is properly initialized
         * @return true if ready to process audio
         */
        bool isInitialized() const;

    private:
        // Core stretcher instance (one per channel for stereo independence)
        std::unique_ptr<signalsmith::stretch::SignalsmithStretch<float>> m_stretcher;

        // Configuration
        int m_sampleRate = 0;
        int m_channels = 0;
        int m_blockSize = 1024;
        bool m_initialized = false;

        // Processing state
        mutable std::mutex m_parameterMutex;
        float m_targetTempo = 1.0f;
        float m_currentTempo = 1.0f;
        bool m_enabled = true;

        // Buffer management
        std::vector<std::vector<float>> m_inputBuffers;  // [channel][sample]
        std::vector<std::vector<float>> m_outputBuffers; // [channel][sample]
        std::vector<float*> m_inputPointers;             // Pointers for stretcher
        std::vector<float*> m_outputPointers;            // Pointers for stretcher

        // Internal processing
        size_t m_inputBufferSize = 0;
        size_t m_outputBufferSize = 0;

        // Performance monitoring
        bool m_smoothTempoChanges = true;
        float m_tempoSmoothingRate = 0.1f; // How fast tempo changes are applied

        // Helper methods
        void updateTempoSmoothing();
        bool resizeBuffers(int inputSamples, int outputSamples);
        void setupChannelPointers();
        int calculateOutputSamples(int inputSamples, float tempo) const;
    };

} // namespace AudioTester