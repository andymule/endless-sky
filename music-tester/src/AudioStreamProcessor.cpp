#include "AudioStreamProcessor.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>

namespace AudioTester {

    AudioStreamProcessor::AudioStreamProcessor()
        : m_shouldStop(false), m_targetTempo(1.0f), m_currentTempo(1.0f),
          m_targetPitchCompensation(1.0f), m_currentPitchCompensation(1.0f) {}

    AudioStreamProcessor::~AudioStreamProcessor() { stop(); }

    bool AudioStreamProcessor::initialize(int sampleRate, int channels, double inputBufferSeconds,
                                          double outputBufferSeconds) {
        if (sampleRate <= 0 || channels <= 0 || channels > 8) {
            std::cerr << "AudioStreamProcessor: Invalid parameters" << std::endl;
            return false;
        }

        // Stop any existing processing
        stop();

        try {
            m_sampleRate = sampleRate;
            m_channels = channels;

            // Calculate buffer sizes in samples (interleaved)
            size_t inputBufferSize =
                static_cast<size_t>(sampleRate * channels * inputBufferSeconds);
            size_t outputBufferSize =
                static_cast<size_t>(sampleRate * channels * outputBufferSeconds);

            // Create ring buffers
            m_inputBuffer = std::make_unique<CircularBuffer<float>>(inputBufferSize);
            m_outputBuffer = std::make_unique<CircularBuffer<float>>(outputBufferSize);

            // Create Signalsmith Stretch instance for tempo processing
            m_stretcher = std::make_unique<signalsmith::stretch::SignalsmithStretch<float>>();

            // Configure stretcher with optimal settings for real-time use
            m_stretcher->presetDefault(channels, sampleRate);

            // Create stereo pitch processor for proper channel synchronization
            m_pitchProcessor = std::make_unique<signalsmith::stretch::SignalsmithStretch<float>>();

            // Configure pitch processor with high-quality settings for optimal pitch shifting
            // Use larger block sizes for better frequency resolution and quality
            int optimalBlockSamples = 2048;   // Larger blocks = better frequency resolution
            int optimalIntervalSamples = 512; // 4:1 overlap ratio for high quality

            // Use manual configuration for maximum control over quality
            m_pitchProcessor->configure(
                2, optimalBlockSamples, optimalIntervalSamples,
                true); // Enable split computation for smoother real-time processing

            // Configure for pitch-only processing with tonality limit for better timbre
            // preservation
            m_pitchProcessor->setTransposeSemitones(0.0); // Start with no pitch change

            // Set tonality limit to preserve timbre - frequencies above 8kHz won't be pitch-shifted
            // This reduces artifacts and preserves the natural character of the sound
            float tonalityLimit = 8000.0f / sampleRate; // Normalize to sample rate
            m_pitchProcessor->setTransposeSemitones(0.0, tonalityLimit);

            // Enable formant compensation for more natural-sounding pitch shifts
            // This helps preserve the vocal characteristics and timbre when pitch shifting
            m_pitchProcessor->setFormantFactor(1.0f); // Start with no formant shift

            // Set formant base frequency for typical musical content (around 200Hz fundamental)
            float formantBase = 200.0f / sampleRate; // Normalize to sample rate
            m_pitchProcessor->setFormantBase(formantBase);

            // Reset to ensure clean initial state
            m_pitchProcessor->reset();

            // Setup channel buffers for processing
            setupChannelBuffers();

            // Reset state
            m_currentTempo = 1.0f;
            m_targetTempo.store(1.0f);
            m_currentPitchCompensation = 1.0f;
            m_targetPitchCompensation.store(1.0f);
            m_smoothPitchCompensation = 1.0f;
            m_initialized = true;

            std::cout << "AudioStreamProcessor initialized: " << sampleRate << "Hz, " << channels
                      << " channels" << std::endl;
            std::cout << "Input buffer: " << inputBufferSeconds << "s (" << inputBufferSize
                      << " samples)" << std::endl;
            std::cout << "Output buffer: " << outputBufferSeconds << "s (" << outputBufferSize
                      << " samples)" << std::endl;

            return true;

        } catch (const std::exception& e) {
            std::cerr << "AudioStreamProcessor initialization failed: " << e.what() << std::endl;
            m_initialized = false;
            return false;
        }
    }

    bool AudioStreamProcessor::start() {
        if (!m_initialized || m_running) {
            return false;
        }

        try {
            m_shouldStop.store(false);
            m_processingThread =
                std::make_unique<std::thread>(&AudioStreamProcessor::processingLoop, this);
            m_running = true;

            std::cout << "AudioStreamProcessor started" << std::endl;
            return true;

        } catch (const std::exception& e) {
            std::cerr << "Failed to start AudioStreamProcessor: " << e.what() << std::endl;
            return false;
        }
    }

    void AudioStreamProcessor::stop() {
        if (m_running && m_processingThread) {
            m_shouldStop.store(true);

            if (m_processingThread->joinable()) {
                m_processingThread->join();
            }

            m_processingThread.reset();
            m_running = false;

            std::cout << "AudioStreamProcessor stopped" << std::endl;
        }
    }

    void AudioStreamProcessor::setTempo(float tempo) {
        // Clamp tempo to reasonable range
        tempo = std::clamp(tempo, 0.1f, 4.0f);
        m_targetTempo.store(tempo);
    }

    float AudioStreamProcessor::getTempo() const { return m_targetTempo.load(); }

    size_t AudioStreamProcessor::feedInput(const float* samples, size_t count) {
        if (!m_initialized || !samples) {
            return 0;
        }

        return m_inputBuffer->write(samples, count);
    }

    size_t AudioStreamProcessor::readOutput(float* samples, size_t count) {
        if (!m_initialized || !samples) {
            return 0;
        }

        return m_outputBuffer->read(samples, count);
    }

    void AudioStreamProcessor::seek(double timeSeconds) {
        if (!m_initialized || !m_stretcher) {
            return;
        }

        // Clear buffers on seek
        m_inputBuffer->clear();
        m_outputBuffer->clear();

        // Reset stretcher state for clean seeking
        m_stretcher->reset();

        std::cout << "AudioStreamProcessor seek to " << timeSeconds << "s" << std::endl;
    }

    int AudioStreamProcessor::getLatencySamples() const {
        if (!m_initialized || !m_stretcher) {
            return 0;
        }

        // Signalsmith Stretch latency plus buffer latency
        int stretchLatency = m_stretcher->inputLatency() + m_stretcher->outputLatency();
        int bufferLatency = static_cast<int>(PROCESSING_CHUNK_SIZE);

        return stretchLatency + bufferLatency;
    }

    float AudioStreamProcessor::getLatencyMs() const {
        if (m_sampleRate <= 0) {
            return 0.0f;
        }

        return (static_cast<float>(getLatencySamples()) / m_channels) / m_sampleRate * 1000.0f;
    }

    bool AudioStreamProcessor::isReady() const { return m_initialized && m_running; }

    size_t AudioStreamProcessor::getInputBufferSpace() const {
        return m_inputBuffer ? m_inputBuffer->space() : 0;
    }

    size_t AudioStreamProcessor::getOutputBufferAvailable() const {
        return m_outputBuffer ? m_outputBuffer->available() : 0;
    }

    // Private methods

    void AudioStreamProcessor::processingLoop() {
        std::cout << "AudioStreamProcessor thread started" << std::endl;

        while (!m_shouldStop.load()) {
            // Check for tempo changes
            if (isTempoChangeNeeded()) {
                handleTempoChange();
            }

            // Process a chunk if we have enough input and output space
            if (m_inputBuffer->available() >= PROCESSING_CHUNK_SIZE * m_channels &&
                m_outputBuffer->space() >= getOutputChunkSize()) {
                processChunk();
            } else {
                // Sleep briefly to avoid busy waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        std::cout << "AudioStreamProcessor thread stopped" << std::endl;
    }

    void AudioStreamProcessor::handleTempoChange() {
        float newTempo = m_targetTempo.load();

        if (std::abs(newTempo - m_currentTempo) > 0.001f) {
            std::cout << "AudioStreamProcessor tempo change: " << m_currentTempo << " -> "
                      << newTempo << std::endl;

            updateCurrentTempo();

            // Prepare stretcher for new tempo
            if (m_stretcher) {
                // For tempo stretching: playback rate = 1/tempo
                float playbackRate = 1.0f / m_currentTempo;

                // Call seek with current settings to update internal state
                // We use empty input for this configuration call
                setupChannelBuffers(); // Ensure buffers are ready
                m_stretcher->seek(m_inputChannelPointers.data(), 0, playbackRate);
            }
        }
    }

    void AudioStreamProcessor::processChunk() {
        if (!m_stretcher) {
            return;
        }

        const size_t inputSamples = PROCESSING_CHUNK_SIZE;
        const size_t outputSamples = getOutputChunkSize() / m_channels;
        const size_t interleavedInputSamples = inputSamples * m_channels;

        // Ensure our work buffer is large enough
        if (m_interleavedWorkBuffer.size() < interleavedInputSamples) {
            m_interleavedWorkBuffer.resize(interleavedInputSamples);
        }

        // Read interleaved input from ring buffer
        size_t samplesRead =
            m_inputBuffer->read(m_interleavedWorkBuffer.data(), interleavedInputSamples);
        if (samplesRead != interleavedInputSamples) {
            // Not enough input data - should not happen due to check in processingLoop
            return;
        }

        // Deinterleave input into channel buffers
        deinterleaveInput(m_interleavedWorkBuffer.data(), inputSamples);

        try {
            // Process through Signalsmith Stretch
            m_stretcher->process(m_inputChannelPointers.data(), static_cast<int>(inputSamples),
                                 m_outputChannelPointers.data(), static_cast<int>(outputSamples));

            // Interleave output and write to ring buffer
            const size_t interleavedOutputSamples = outputSamples * m_channels;
            if (m_interleavedWorkBuffer.size() < interleavedOutputSamples) {
                m_interleavedWorkBuffer.resize(interleavedOutputSamples);
            }

            interleaveOutput(m_interleavedWorkBuffer.data(), outputSamples);

            // Write processed output to ring buffer
            size_t samplesWritten =
                m_outputBuffer->write(m_interleavedWorkBuffer.data(), interleavedOutputSamples);
            if (samplesWritten != interleavedOutputSamples) {
                std::cerr << "AudioStreamProcessor: Output buffer overflow" << std::endl;
            }

        } catch (const std::exception& e) {
            std::cerr << "AudioStreamProcessor processing error: " << e.what() << std::endl;
        }
    }

    size_t AudioStreamProcessor::getOutputChunkSize() const {
        // For time stretching: output samples = input samples / tempo
        // We need to account for interleaved samples
        float tempo = m_currentTempo;
        if (tempo <= 0.0f)
            tempo = 1.0f;

        size_t outputSamples =
            static_cast<size_t>(std::round(static_cast<float>(PROCESSING_CHUNK_SIZE) / tempo));
        return outputSamples * m_channels; // Return interleaved sample count
    }

    void AudioStreamProcessor::setupChannelBuffers() {
        // Resize channel buffers
        m_inputChannelBuffers.resize(m_channels);
        m_outputChannelBuffers.resize(m_channels);
        m_inputChannelPointers.resize(m_channels);
        m_outputChannelPointers.resize(m_channels);

        // Each channel buffer needs to handle the maximum chunk size
        const size_t maxChunkSize =
            std::max(PROCESSING_CHUNK_SIZE, getOutputChunkSize() / m_channels + 1);

        for (int ch = 0; ch < m_channels; ++ch) {
            m_inputChannelBuffers[ch].resize(maxChunkSize);
            m_outputChannelBuffers[ch].resize(maxChunkSize);
            m_inputChannelPointers[ch] = m_inputChannelBuffers[ch].data();
            m_outputChannelPointers[ch] = m_outputChannelBuffers[ch].data();
        }
    }

    void AudioStreamProcessor::deinterleaveInput(const float* interleavedInput, size_t samples) {
        for (size_t sample = 0; sample < samples; ++sample) {
            for (int ch = 0; ch < m_channels; ++ch) {
                m_inputChannelBuffers[ch][sample] = interleavedInput[sample * m_channels + ch];
            }
        }
    }

    void AudioStreamProcessor::interleaveOutput(float* interleavedOutput, size_t samples) {
        for (size_t sample = 0; sample < samples; ++sample) {
            for (int ch = 0; ch < m_channels; ++ch) {
                interleavedOutput[sample * m_channels + ch] = m_outputChannelBuffers[ch][sample];
            }
        }
    }

    bool AudioStreamProcessor::isTempoChangeNeeded() const {
        return std::abs(m_targetTempo.load() - m_currentTempo) > 0.001f;
    }

    void AudioStreamProcessor::updateCurrentTempo() { m_currentTempo = m_targetTempo.load(); }

    // Pitch compensation methods for dual tape speed architecture
    void AudioStreamProcessor::setPitchCompensation(float pitchFactor) {
        // Clamp pitch compensation to reasonable range
        pitchFactor = std::clamp(pitchFactor, 0.25f, 4.0f);
        m_targetPitchCompensation.store(pitchFactor);
    }

    float AudioStreamProcessor::getPitchCompensation() const {
        return m_targetPitchCompensation.load();
    }

    void AudioStreamProcessor::updateSmoothPitchCompensation() {
        float targetPitch = m_targetPitchCompensation.load();

        // Smooth interpolation to target pitch compensation (eliminates clicks)
        float diff = targetPitch - m_smoothPitchCompensation;
        if (std::abs(diff) > 0.001f) {
            m_smoothPitchCompensation += diff * PITCH_SMOOTHING_FACTOR;

            // Update pitch processor with smooth value and tonality limit
            if (m_pitchProcessor) {
                // Convert linear pitch factor to semitones for Signalsmith
                // semitones = 12 * log2(pitchFactor)
                float semitones = 12.0f * std::log2(m_smoothPitchCompensation);

                // Apply tonality limit to preserve timbre above 8kHz
                float tonalityLimit = 8000.0f / m_sampleRate; // Normalize to sample rate
                m_pitchProcessor->setTransposeSemitones(semitones, tonalityLimit);

                // Update formant compensation to counteract the pitch shift for more natural sound
                // This preserves the original formant structure while changing pitch
                m_pitchProcessor->setFormantFactor(1.0f / m_smoothPitchCompensation,
                                                   true); // compensatePitch=true

                // Only log significant changes to reduce console spam
                static float lastLoggedPitch = 1.0f;
                if (std::abs(m_smoothPitchCompensation - lastLoggedPitch) > 0.05f) {
                    std::cout << "AudioStreamProcessor pitch compensation updated: "
                              << m_smoothPitchCompensation << " (tonality limit: " << tonalityLimit
                              << ")" << std::endl;
                    lastLoggedPitch = m_smoothPitchCompensation;
                }
            }
        }
    }

    bool AudioStreamProcessor::processPitchCompensation(const float* inputSamples,
                                                        float* outputSamples, size_t sampleCount) {
        if (!m_initialized || !m_pitchProcessor || !inputSamples || !outputSamples) {
            return false;
        }

        // Update smooth pitch compensation to eliminate clicks
        updateSmoothPitchCompensation();

        // Skip processing if no pitch compensation needed (within threshold)
        if (std::abs(m_smoothPitchCompensation - 1.0f) < 0.001f) {
            // Simple copy for no processing needed
            std::memcpy(outputSamples, inputSamples, sampleCount * sizeof(float));
            return true;
        }

        try {
            // Process single channel through stereo-configured processor
            // Create stereo input/output with the same data in both channels
            // This ensures the stereo processor gets consistent input and both channels stay in
            // sync

            // Resize working buffers if needed (for stereo processing)
            size_t stereoSamples = sampleCount * 2; // Interleaved stereo
            if (m_interleavedWorkBuffer.size() < stereoSamples) {
                m_interleavedWorkBuffer.resize(stereoSamples);
            }

            // Create interleaved stereo input (duplicate mono to both channels)
            for (size_t i = 0; i < sampleCount; ++i) {
                m_interleavedWorkBuffer[i * 2] = inputSamples[i];     // Left
                m_interleavedWorkBuffer[i * 2 + 1] = inputSamples[i]; // Right (same as left)
            }

            // Setup channel buffers for stereo processing
            if (m_inputChannelBuffers[0].size() < sampleCount) {
                m_inputChannelBuffers[0].resize(sampleCount);
                m_inputChannelBuffers[1].resize(sampleCount);
                m_outputChannelBuffers[0].resize(sampleCount);
                m_outputChannelBuffers[1].resize(sampleCount);
            }

            // Deinterleave to channel buffers
            for (size_t i = 0; i < sampleCount; ++i) {
                m_inputChannelBuffers[0][i] = m_interleavedWorkBuffer[i * 2];     // Left
                m_inputChannelBuffers[1][i] = m_interleavedWorkBuffer[i * 2 + 1]; // Right
            }

            // Set up channel pointers
            m_inputChannelPointers[0] = m_inputChannelBuffers[0].data();
            m_inputChannelPointers[1] = m_inputChannelBuffers[1].data();
            m_outputChannelPointers[0] = m_outputChannelBuffers[0].data();
            m_outputChannelPointers[1] = m_outputChannelBuffers[1].data();

            // Process stereo (both channels will be identical due to identical input)
            m_pitchProcessor->process(m_inputChannelPointers.data(), static_cast<int>(sampleCount),
                                      m_outputChannelPointers.data(),
                                      static_cast<int>(sampleCount));

            // Use left channel output (both channels should be identical)
            std::memcpy(outputSamples, m_outputChannelBuffers[0].data(),
                        sampleCount * sizeof(float));

            return true;

        } catch (const std::exception& e) {
            std::cerr << "Signalsmith pitch compensation error: " << e.what() << std::endl;
            // Fall back to simple copy on error to maintain audio flow
            std::memcpy(outputSamples, inputSamples, sampleCount * sizeof(float));
            return true;
        }
    }

    void AudioStreamProcessor::resetPitchProcessor() {
        if (m_pitchProcessor) {
            m_pitchProcessor->reset();
            m_smoothPitchCompensation = 1.0f;
            m_currentPitchCompensation = 1.0f;
            m_targetPitchCompensation.store(1.0f);
            std::cout << "AudioStreamProcessor pitch processor reset" << std::endl;
        }
    }

} // namespace AudioTester
