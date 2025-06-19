#pragma once

#include "CircularBuffer.h"
#include "signalsmith-stretch/signalsmith-stretch.h"
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace AudioTester {

    /**
     * AudioStreamProcessor - Real-time audio processing with granular time stretching
     *
     * This class processes audio streams through Signalsmith Stretch BEFORE feeding
     * them to SoLoud, avoiding the broken SoLoud filter system. It uses ring buffers
     * and a separate processing thread for real-time performance.
     */
    class AudioStreamProcessor {
    public:
        AudioStreamProcessor();
        ~AudioStreamProcessor();

        /**
         * Initialize the processor
         * @param sampleRate Audio sample rate (e.g., 44100)
         * @param channels Number of audio channels (1=mono, 2=stereo)
         * @param inputBufferSeconds Input buffer size in seconds (default: 5)
         * @param outputBufferSeconds Output buffer size in seconds (default: 3)
         * @return true on success, false on failure
         */
        bool initialize(int sampleRate, int channels, double inputBufferSeconds = 5.0,
                        double outputBufferSeconds = 3.0);

        /**
         * Start the processing thread
         * @return true on success, false on failure
         */
        bool start();

        /**
         * Stop the processing thread
         */
        void stop();

        /**
         * Set target tempo multiplier
         * @param tempo Tempo multiplier (1.0 = normal, 0.5 = half speed, 2.0 = double speed)
         */
        void setTempo(float tempo);

        /**
         * Get current tempo setting
         * @return Current tempo multiplier
         */
        float getTempo() const;

        /**
         * Feed input audio samples to the processor
         * @param samples Interleaved audio samples
         * @param count Number of samples (total, not per channel)
         * @return Number of samples actually consumed
         */
        size_t feedInput(const float* samples, size_t count);

        /**
         * Read processed output samples
         * @param samples Buffer to store interleaved output samples
         * @param count Number of samples to read (total, not per channel)
         * @return Number of samples actually read
         */
        size_t readOutput(float* samples, size_t count);

        /**
         * Seek to a specific time position
         * @param timeSeconds Time position in seconds
         */
        void seek(double timeSeconds);

        /**
         * Get current processing latency in samples
         * @return Latency in samples
         */
        int getLatencySamples() const;

        /**
         * Get current processing latency in milliseconds
         * @return Latency in milliseconds
         */
        float getLatencyMs() const;

        /**
         * Check if processor is initialized and ready
         * @return true if ready to process audio
         */
        bool isReady() const;

        /**
         * Get input buffer status
         * @return Available space in input buffer (samples)
         */
        size_t getInputBufferSpace() const;

        /**
         * Get output buffer status
         * @return Available data in output buffer (samples)
         */
        size_t getOutputBufferAvailable() const;

    private:
        // Core configuration
        int m_sampleRate = 0;
        int m_channels = 0;
        bool m_initialized = false;
        bool m_running = false;

        // Ring buffer management
        std::unique_ptr<CircularBuffer<float>> m_inputBuffer;
        std::unique_ptr<CircularBuffer<float>> m_outputBuffer;

        // Signalsmith Stretch instance
        std::unique_ptr<signalsmith::stretch::SignalsmithStretch<float>> m_stretcher;

        // Processing thread
        std::unique_ptr<std::thread> m_processingThread;
        std::atomic<bool> m_shouldStop;

        // Tempo control
        std::atomic<float> m_targetTempo;
        float m_currentTempo;

        // Processing parameters
        static constexpr size_t PROCESSING_CHUNK_SIZE = 1024;
        static constexpr int TEMPO_CHANGE_THRESHOLD_MS = 1; // Minimum time between tempo changes

        // Working buffers for processing
        std::vector<std::vector<float>> m_inputChannelBuffers;
        std::vector<std::vector<float>> m_outputChannelBuffers;
        std::vector<float*> m_inputChannelPointers;
        std::vector<float*> m_outputChannelPointers;
        std::vector<float> m_interleavedWorkBuffer;

        // Processing thread methods
        void processingLoop();
        void handleTempoChange();
        void processChunk();
        size_t getOutputChunkSize() const;

        // Buffer management
        void setupChannelBuffers();
        void deinterleaveInput(const float* interleavedInput, size_t samples);
        void interleaveOutput(float* interleavedOutput, size_t samples);

        // Utility methods
        bool isTempoChangeNeeded() const;
        void updateCurrentTempo();
    };

} // namespace AudioTester