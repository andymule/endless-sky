#include "MasterTempoProcessor.h"
#include <algorithm>
#include <cmath>

namespace AudioTester {

    MasterTempoProcessor::MasterTempoProcessor() {
        // Constructor - initialization happens in initialize()
    }

    MasterTempoProcessor::~MasterTempoProcessor() {
        // Destructor - cleanup happens automatically with smart pointers
    }

    bool MasterTempoProcessor::initialize(int sampleRate, int channels, int blockSize) {
        if (sampleRate <= 0 || channels <= 0 || channels > 8 || blockSize <= 0) {
            return false;
        }

        try {
            // Store configuration
            m_sampleRate = sampleRate;
            m_channels = channels;
            m_blockSize = blockSize;

            // Create the Signalsmith Stretch instance
            m_stretcher = std::make_unique<signalsmith::stretch::SignalsmithStretch<float>>();

            // Configure the stretcher with optimal settings for real-time use
            // presetDefault gives good quality/performance balance
            m_stretcher->presetDefault(channels, sampleRate);

            // Initialize buffers
            m_inputBuffers.resize(channels);
            m_outputBuffers.resize(channels);
            m_inputPointers.resize(channels);
            m_outputPointers.resize(channels);

            // Pre-allocate buffers with reasonable initial size
            for (int ch = 0; ch < channels; ++ch) {
                m_inputBuffers[ch].resize(blockSize * 2); // Extra room for variable sizes
                m_outputBuffers[ch].resize(blockSize * 2);
                m_inputPointers[ch] = m_inputBuffers[ch].data();
                m_outputPointers[ch] = m_outputBuffers[ch].data();
            }

            m_inputBufferSize = blockSize * 2;
            m_outputBufferSize = blockSize * 2;

            // Reset stretcher state
            m_stretcher->reset();

            m_initialized = true;
            return true;

        } catch (const std::exception& e) {
            m_initialized = false;
            return false;
        }
    }

    bool MasterTempoProcessor::process(const float* const* inputBuffer, float** outputBuffer,
                                       int inputSamples, int outputSamples) {
        if (!m_initialized || !inputBuffer || !outputBuffer || inputSamples <= 0 ||
            outputSamples <= 0) {
            return false;
        }

        // Update tempo smoothing
        updateTempoSmoothing();

        // If disabled or tempo is exactly 1.0, pass through unprocessed
        {
            std::lock_guard<std::mutex> lock(m_parameterMutex);
            if (!m_enabled || std::abs(m_currentTempo - 1.0f) < 0.001f) {
                // Simple passthrough - copy input to output
                int samplesToCopy = std::min(inputSamples, outputSamples);
                for (int ch = 0; ch < m_channels; ++ch) {
                    if (inputBuffer[ch] && outputBuffer[ch]) {
                        std::copy(inputBuffer[ch], inputBuffer[ch] + samplesToCopy,
                                  outputBuffer[ch]);

                        // Zero remaining output if outputSamples > inputSamples
                        if (outputSamples > samplesToCopy) {
                            std::fill(outputBuffer[ch] + samplesToCopy,
                                      outputBuffer[ch] + outputSamples, 0.0f);
                        }
                    }
                }
                return true;
            }
        }

        // Ensure buffers are large enough
        if (!resizeBuffers(inputSamples, outputSamples)) {
            return false;
        }

        try {
            // Copy input data to our internal buffers
            for (int ch = 0; ch < m_channels; ++ch) {
                if (inputBuffer[ch]) {
                    std::copy(inputBuffer[ch], inputBuffer[ch] + inputSamples,
                              m_inputBuffers[ch].begin());
                } else {
                    // If input channel is null, fill with silence
                    std::fill(m_inputBuffers[ch].begin(), m_inputBuffers[ch].begin() + inputSamples,
                              0.0f);
                }
            }

            // Process through Signalsmith Stretch
            m_stretcher->process(m_inputPointers.data(), inputSamples, m_outputPointers.data(),
                                 outputSamples);

            // Copy processed data to output buffers
            for (int ch = 0; ch < m_channels; ++ch) {
                if (outputBuffer[ch]) {
                    std::copy(m_outputBuffers[ch].begin(),
                              m_outputBuffers[ch].begin() + outputSamples, outputBuffer[ch]);
                }
            }

            return true;

        } catch (const std::exception& e) {
            // On error, pass through unprocessed audio
            int samplesToCopy = std::min(inputSamples, outputSamples);
            for (int ch = 0; ch < m_channels; ++ch) {
                if (inputBuffer[ch] && outputBuffer[ch]) {
                    std::copy(inputBuffer[ch], inputBuffer[ch] + samplesToCopy, outputBuffer[ch]);
                    if (outputSamples > samplesToCopy) {
                        std::fill(outputBuffer[ch] + samplesToCopy,
                                  outputBuffer[ch] + outputSamples, 0.0f);
                    }
                }
            }
            return false;
        }
    }

    void MasterTempoProcessor::setTempo(float tempo) {
        // Clamp tempo to reasonable range
        tempo = std::clamp(tempo, 0.1f, 4.0f);

        std::lock_guard<std::mutex> lock(m_parameterMutex);
        m_targetTempo = tempo;
    }

    float MasterTempoProcessor::getTempo() const {
        std::lock_guard<std::mutex> lock(m_parameterMutex);
        return m_targetTempo;
    }

    void MasterTempoProcessor::setEnabled(bool enabled) {
        std::lock_guard<std::mutex> lock(m_parameterMutex);
        m_enabled = enabled;
    }

    bool MasterTempoProcessor::isEnabled() const {
        std::lock_guard<std::mutex> lock(m_parameterMutex);
        return m_enabled;
    }

    int MasterTempoProcessor::getLatencySamples() const {
        if (!m_initialized || !m_stretcher) {
            return 0;
        }

        // Signalsmith Stretch reports latency through inputLatency() and outputLatency()
        return m_stretcher->inputLatency() + m_stretcher->outputLatency();
    }

    float MasterTempoProcessor::getLatencyMs() const {
        if (m_sampleRate <= 0) {
            return 0.0f;
        }

        return (static_cast<float>(getLatencySamples()) / static_cast<float>(m_sampleRate)) *
               1000.0f;
    }

    void MasterTempoProcessor::reset() {
        if (m_stretcher) {
            m_stretcher->reset();
        }

        // Reset tempo smoothing
        std::lock_guard<std::mutex> lock(m_parameterMutex);
        m_currentTempo = m_targetTempo;
    }

    bool MasterTempoProcessor::isInitialized() const { return m_initialized; }

    // Private helper methods

    void MasterTempoProcessor::updateTempoSmoothing() {
        if (!m_smoothTempoChanges) {
            std::lock_guard<std::mutex> lock(m_parameterMutex);
            m_currentTempo = m_targetTempo;
            return;
        }

        std::lock_guard<std::mutex> lock(m_parameterMutex);

        // Smooth tempo changes to avoid audio glitches
        float tempoDiff = m_targetTempo - m_currentTempo;
        if (std::abs(tempoDiff) > 0.001f) {
            m_currentTempo += tempoDiff * m_tempoSmoothingRate;

            // Snap to target when very close
            if (std::abs(m_targetTempo - m_currentTempo) < 0.001f) {
                m_currentTempo = m_targetTempo;
            }
        }
    }

    bool MasterTempoProcessor::resizeBuffers(int inputSamples, int outputSamples) {
        size_t neededInputSize = static_cast<size_t>(inputSamples);
        size_t neededOutputSize = static_cast<size_t>(outputSamples);

        bool needResize = false;

        if (neededInputSize > m_inputBufferSize) {
            m_inputBufferSize = neededInputSize * 2; // Extra room for growth
            needResize = true;
        }

        if (neededOutputSize > m_outputBufferSize) {
            m_outputBufferSize = neededOutputSize * 2; // Extra room for growth
            needResize = true;
        }

        if (needResize) {
            try {
                for (int ch = 0; ch < m_channels; ++ch) {
                    m_inputBuffers[ch].resize(m_inputBufferSize);
                    m_outputBuffers[ch].resize(m_outputBufferSize);
                }
                setupChannelPointers();
            } catch (const std::exception& e) {
                return false;
            }
        }

        return true;
    }

    void MasterTempoProcessor::setupChannelPointers() {
        for (int ch = 0; ch < m_channels; ++ch) {
            m_inputPointers[ch] = m_inputBuffers[ch].data();
            m_outputPointers[ch] = m_outputBuffers[ch].data();
        }
    }

    int MasterTempoProcessor::calculateOutputSamples(int inputSamples, float tempo) const {
        if (tempo <= 0.0f) {
            return inputSamples;
        }

        // For time stretching: output samples = input samples / tempo
        // tempo > 1.0 = faster = fewer output samples
        // tempo < 1.0 = slower = more output samples
        return static_cast<int>(std::round(static_cast<float>(inputSamples) / tempo));
    }

} // namespace AudioTester