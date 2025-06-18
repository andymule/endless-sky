#pragma once

#include "MasterTempoProcessor.h"
#include "soloud.h"
#include <memory>
#include <vector>

namespace AudioTester {

    // Forward declaration
    class GranularTempoFilterInstance;

    /**
     * GranularTempoFilter - A SoLoud filter that provides pitch-preserving tempo stretching
     *
     * This filter wraps the MasterTempoProcessor to provide granular synthesis-based
     * tempo stretching that maintains the original pitch while changing playback speed.
     */
    class GranularTempoFilter : public SoLoud::Filter {
    public:
        GranularTempoFilter();
        virtual ~GranularTempoFilter();

        // SoLoud Filter interface
        virtual SoLoud::FilterInstance* createInstance() override;
        virtual int getParamCount() override;
        virtual const char* getParamName(unsigned int aParamIndex) override;
        virtual unsigned int getParamType(unsigned int aParamIndex) override;
        virtual float getParamMax(unsigned int aParamIndex) override;
        virtual float getParamMin(unsigned int aParamIndex) override;

        // Granular tempo control
        void setTempo(float tempo);
        float getTempo() const;
        void setEnabled(bool enabled);
        bool isEnabled() const;

        // Filter parameters (for SoLoud integration)
        enum Params {
            TEMPO = 0,   // Tempo multiplier (0.1 to 2.0)
            ENABLED = 1, // Enable/disable (0.0 or 1.0)
            PARAM_COUNT
        };

    private:
        float m_tempo = 1.0f;
        bool m_enabled = false;
    };

    /**
     * GranularTempoFilterInstance - Per-voice instance of the granular tempo filter
     */
    class GranularTempoFilterInstance : public SoLoud::FilterInstance {
    public:
        GranularTempoFilterInstance(GranularTempoFilter* aParent);
        virtual ~GranularTempoFilterInstance();

        // SoLoud FilterInstance interface
        virtual SoLoud::result initParams(int aNumParams) override;
        virtual void updateParams(SoLoud::time aTime) override;
        virtual void filter(float* aBuffer, unsigned int aSamples, unsigned int aBufferSize,
                            unsigned int aChannels, float aSamplerate, SoLoud::time aTime) override;

    private:
        void resizeBuffers(unsigned int aSamples, unsigned int aChannels);
        int calculateOutputSamples(int inputSamples, float tempo) const;

        GranularTempoFilter* m_parent;
        std::unique_ptr<MasterTempoProcessor> m_processor;

        // Audio processing buffers
        std::vector<std::vector<float>> m_inputBuffers;
        std::vector<std::vector<float>> m_outputBuffers;
        std::vector<float*> m_inputPointers;
        std::vector<float*> m_outputPointers;

        // State tracking
        unsigned int m_lastChannels = 0;
        unsigned int m_lastSampleRate = 0;
        bool m_initialized = false;

        // Parameter smoothing
        float m_currentTempo = 1.0f;
        bool m_currentEnabled = false;
    };

} // namespace AudioTester