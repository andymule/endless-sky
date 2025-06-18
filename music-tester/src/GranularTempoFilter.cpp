#include "GranularTempoFilter.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace AudioTester {

    // GranularTempoFilter implementation
    GranularTempoFilter::GranularTempoFilter() {
        // No need to set param count - handled by SoLoud
    }

    GranularTempoFilter::~GranularTempoFilter() = default;

    SoLoud::FilterInstance* GranularTempoFilter::createInstance() {
        return new GranularTempoFilterInstance(this);
    }

    void GranularTempoFilter::setTempo(float tempo) {
        m_tempo = std::clamp(tempo, 0.1f, 2.0f);
        // Note: Parameter updates are handled through the AudioSystem
    }

    float GranularTempoFilter::getTempo() const { return m_tempo; }

    void GranularTempoFilter::setEnabled(bool enabled) {
        m_enabled = enabled;
        // Note: Parameter updates are handled through the AudioSystem
    }

    bool GranularTempoFilter::isEnabled() const { return m_enabled; }

    int GranularTempoFilter::getParamCount() { return PARAM_COUNT; }

    const char* GranularTempoFilter::getParamName(unsigned int aParamIndex) {
        switch (aParamIndex) {
            case TEMPO:
                return "Tempo";
            case ENABLED:
                return "Enabled";
            default:
                return nullptr;
        }
    }

    unsigned int GranularTempoFilter::getParamType(unsigned int aParamIndex) {
        switch (aParamIndex) {
            case TEMPO:
                return SoLoud::Filter::FLOAT_PARAM;
            case ENABLED:
                return SoLoud::Filter::BOOL_PARAM;
            default:
                return SoLoud::Filter::FLOAT_PARAM;
        }
    }

    float GranularTempoFilter::getParamMax(unsigned int aParamIndex) {
        switch (aParamIndex) {
            case TEMPO:
                return 2.0f;
            case ENABLED:
                return 1.0f;
            default:
                return 1.0f;
        }
    }

    float GranularTempoFilter::getParamMin(unsigned int aParamIndex) {
        switch (aParamIndex) {
            case TEMPO:
                return 0.1f;
            case ENABLED:
                return 0.0f;
            default:
                return 0.0f;
        }
    }

    // GranularTempoFilterInstance implementation
    GranularTempoFilterInstance::GranularTempoFilterInstance(GranularTempoFilter* aParent)
        : m_parent(aParent) {
        m_processor = std::make_unique<MasterTempoProcessor>();

        // Initialize parameters using the parent filter's parameter count
        if (aParent) {
            initParams(aParent->getParamCount());
        }
    }

    GranularTempoFilterInstance::~GranularTempoFilterInstance() = default;

    SoLoud::result GranularTempoFilterInstance::initParams(int aNumParams) {
        // Call parent initParams first
        SoLoud::result result = SoLoud::FilterInstance::initParams(aNumParams);
        if (result != SoLoud::SO_NO_ERROR) {
            return result;
        }

        // Initialize parameters with default values
        if (aNumParams >= GranularTempoFilter::TEMPO + 1) {
            mParam[GranularTempoFilter::TEMPO] = 1.0f;
        }
        if (aNumParams >= GranularTempoFilter::ENABLED + 1) {
            mParam[GranularTempoFilter::ENABLED] = 0.0f;
        }

        return SoLoud::SO_NO_ERROR;
    }

    void GranularTempoFilterInstance::updateParams(SoLoud::time aTime) {
        static int updateCount = 0;
        if (updateCount++ % 1000 == 0) {
            std::cout << "updateParams called - mNumParams: " << mNumParams
                      << ", mParam[0]: " << (mNumParams > 0 ? mParam[0] : -999.0f)
                      << ", mParam[1]: " << (mNumParams > 1 ? mParam[1] : -999.0f) << std::endl;
        }

        // Update our local state from filter parameters
        if (mNumParams >= GranularTempoFilter::TEMPO + 1) {
            float newTempo = mParam[GranularTempoFilter::TEMPO];
            if (std::abs(newTempo - m_currentTempo) > 0.001f) {
                std::cout << "Tempo parameter changed from " << m_currentTempo << " to " << newTempo
                          << std::endl;
                m_currentTempo = newTempo;
                if (m_processor) {
                    m_processor->setTempo(m_currentTempo);
                }
            }
        }

        if (mNumParams >= GranularTempoFilter::ENABLED + 1) {
            bool newEnabled = mParam[GranularTempoFilter::ENABLED] > 0.5f;
            if (newEnabled != m_currentEnabled) {
                std::cout << "Enabled parameter changed from " << m_currentEnabled << " to "
                          << newEnabled << std::endl;
                m_currentEnabled = newEnabled;
                if (m_processor) {
                    m_processor->setEnabled(m_currentEnabled);
                }
            }
        }
    }

    void GranularTempoFilterInstance::filter(float* aBuffer, unsigned int aSamples,
                                             unsigned int aBufferSize, unsigned int aChannels,
                                             float aSamplerate, SoLoud::time aTime) {
        // Call updateParams to get the latest parameter values from SoLoud
        updateParams(aTime);

        static int callCount = 0;
        if (callCount++ % 1000 == 0) {
            std::cout << "GranularTempoFilter::filter() called - enabled: " << m_currentEnabled
                      << ", tempo: " << m_currentTempo << std::endl;
        }

        // Early exit if disabled or tempo is 1.0
        if (!m_currentEnabled || std::abs(m_currentTempo - 1.0f) < 0.001f) {
            // Pass through unprocessed
            return;
        }

        // Initialize processor if needed
        if (!m_initialized || m_lastChannels != aChannels ||
            m_lastSampleRate != static_cast<unsigned int>(aSamplerate)) {
            if (m_processor->initialize(static_cast<int>(aSamplerate), static_cast<int>(aChannels),
                                        static_cast<int>(aSamples))) {
                m_initialized = true;
                m_lastChannels = aChannels;
                m_lastSampleRate = static_cast<unsigned int>(aSamplerate);
            } else {
                // Initialization failed, pass through
                return;
            }
        }

        // Resize buffers if needed
        resizeBuffers(aSamples, aChannels);

        // For granular processing, we need to handle the variable input/output relationship
        // For now, we'll use a 1:1 mapping for simplicity
        int outputSamples = static_cast<int>(aSamples);
        int inputSamples = calculateOutputSamples(outputSamples, m_currentTempo);

        // Ensure we don't exceed the buffer size
        inputSamples = std::min(inputSamples, static_cast<int>(aSamples));

        // De-interleave input data into channel buffers
        for (unsigned int ch = 0; ch < aChannels; ++ch) {
            for (int i = 0; i < inputSamples; ++i) {
                m_inputBuffers[ch][i] = aBuffer[i * aChannels + ch];
            }
            m_inputPointers[ch] = m_inputBuffers[ch].data();
            m_outputPointers[ch] = m_outputBuffers[ch].data();
        }

        // Process through the granular tempo processor (Signalsmith Stretch)
        bool success = m_processor->process(const_cast<const float* const*>(m_inputPointers.data()),
                                            m_outputPointers.data(), inputSamples, outputSamples);

        if (success) {
            // Interleave output data back into the buffer
            for (unsigned int ch = 0; ch < aChannels; ++ch) {
                for (int i = 0; i < outputSamples; ++i) {
                    aBuffer[i * aChannels + ch] = m_outputBuffers[ch][i];
                }
            }
        }
        // If processing failed, the original buffer data remains unchanged (passthrough)
    }

    void GranularTempoFilterInstance::resizeBuffers(unsigned int aSamples, unsigned int aChannels) {
        // Resize buffers to accommodate the maximum possible samples needed
        size_t maxSamples = static_cast<size_t>(aSamples * 2); // Extra room for tempo stretching

        if (m_inputBuffers.size() != aChannels) {
            m_inputBuffers.resize(aChannels);
            m_outputBuffers.resize(aChannels);
            m_inputPointers.resize(aChannels);
            m_outputPointers.resize(aChannels);
        }

        for (unsigned int ch = 0; ch < aChannels; ++ch) {
            if (m_inputBuffers[ch].size() < maxSamples) {
                m_inputBuffers[ch].resize(maxSamples);
                m_outputBuffers[ch].resize(maxSamples);
            }
        }
    }

    int GranularTempoFilterInstance::calculateOutputSamples(int inputSamples, float tempo) const {
        if (tempo <= 0.0f) {
            return inputSamples;
        }

        // For time stretching: output samples = input samples / tempo
        // tempo > 1.0 = faster = fewer output samples
        // tempo < 1.0 = slower = more output samples
        return static_cast<int>(std::round(static_cast<float>(inputSamples) / tempo));
    }

} // namespace AudioTester