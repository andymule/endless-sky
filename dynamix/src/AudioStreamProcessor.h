#pragma once

#include "CircularBuffer.h"
#include "signalsmith-stretch/signalsmith-stretch.h"
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace Dynamix {

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
         * Set pitch compensation factor for dual tape speed architecture
         * @param pitchFactor Pitch multiplier (1.0 = normal, 2.0 = octave up, 0.5 = octave down)
         */
        void setPitchCompensation(float pitchFactor);

        /**
         * Get current pitch compensation setting
         * @return Current pitch compensation factor
         */
        float getPitchCompensation() const;

        /**
         * Process audio with 1:1 sample ratio (pitch compensation only)
         * @param inputSamples Interleaved input samples
         * @param outputSamples Buffer for interleaved output samples
         * @param sampleCount Number of samples (total, not per channel)
         * @return true if processing succeeded
         */
        bool processPitchCompensation(const float* inputSamples, float* outputSamples,
                                      size_t sampleCount);

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

        // Reset pitch processor to eliminate any residual state
        void resetPitchProcessor();

    private:
        bool m_initialized = false;
        bool m_running = false;
        std::atomic<bool> m_shouldStop{false};

        int m_sampleRate = 0;
        int m_channels = 0;

        // Ring buffers for audio streaming (interleaved)
        std::unique_ptr<CircularBuffer<float>> m_inputBuffer;
        std::unique_ptr<CircularBuffer<float>> m_outputBuffer;

        // Signalsmith Stretch for time/pitch manipulation
        std::unique_ptr<signalsmith::stretch::SignalsmithStretch<float>> m_stretcher;

        // Separate mono pitch processor for per-channel filtering (avoids channel mismatch)
        std::unique_ptr<signalsmith::stretch::SignalsmithStretch<float>> m_pitchProcessor;

        // Channel buffers for processing
        std::vector<std::vector<float>> m_inputChannelBuffers;
        std::vector<std::vector<float>> m_outputChannelBuffers;
        std::vector<float*> m_inputChannelPointers;
        std::vector<float*> m_outputChannelPointers;

        // Working buffer for interleaved audio
        std::vector<float> m_interleavedWorkBuffer;

        // Processing thread
        std::unique_ptr<std::thread> m_processingThread;

        // Tempo control
        std::atomic<float> m_targetTempo{1.0f};
        float m_currentTempo = 1.0f;

        // Pitch compensation for dual tape speed architecture
        std::atomic<float> m_targetPitchCompensation{1.0f};
        float m_currentPitchCompensation = 1.0f;

        // Smooth transition for pitch compensation (eliminates clicks)
        float m_smoothPitchCompensation = 1.0f;
        static constexpr float PITCH_SMOOTHING_FACTOR =
            0.01f; // Adjust for smoother/faster transitions

        // Processing constants
        static constexpr size_t PROCESSING_CHUNK_SIZE = 512;

        // Private methods
        void processingLoop();
        void handleTempoChange();
        void processChunk();
        size_t getOutputChunkSize() const;
        void setupChannelBuffers();
        void deinterleaveInput(const float* interleavedInput, size_t samples);
        void interleaveOutput(float* interleavedOutput, size_t samples);
        bool isTempoChangeNeeded() const;
        void updateCurrentTempo();

        // Smooth pitch compensation update
        void updateSmoothPitchCompensation();
    };

} // namespace Dynamix