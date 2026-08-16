#pragma once

#include "AudioStreamProcessor.h"
#include "soloud.h"
#include "soloud_wav.h"
#include <vector>

namespace Dynamix {

    /**
     * Wav voice that time-stretches in getAudio() so tempo can change without
     * changing pitch. Tape speed remains SoLoud relativePlaySpeed on top.
     */
    class SyncWavInstance : public SoLoud::AudioSourceInstance {
    public:
        explicit SyncWavInstance(SoLoud::Wav* aParent);

        unsigned int getAudio(float* aBuffer, unsigned int aSamplesToRead,
                              unsigned int aBufferSize) override;
        SoLoud::result rewind() override;
        SoLoud::result seek(SoLoud::time aSeconds, float* aScratch,
                            unsigned int aScratchSize) override;
        bool hasEnded() override;

    private:
        static constexpr unsigned int kMaxChunk = 512;
        static constexpr unsigned int kScratchFrames = 16384;

        void ensureConfigured();
        void bindInputPointers();
        int readSourceFrames(float** dest, int frames, bool looping);
        unsigned int renderDry(float* aBuffer, unsigned int aSamplesToRead,
                               unsigned int aBufferSize);
        unsigned int renderStretched(float* aBuffer, unsigned int aSamplesToRead,
                                     unsigned int aBufferSize, float tempo);

        SoLoud::Wav* m_parent = nullptr;
        unsigned int m_sourceOffset = 0;
        double m_inputPhase = 0.0;
        bool m_primed = false;
        bool m_wasStretching = false;

        AudioStreamProcessor m_processor;
        std::vector<std::vector<float>> m_inputBuffers;
        std::vector<float*> m_inputPointers;
        std::vector<float*> m_outputPointers;
    };

    class SyncWav : public SoLoud::Wav {
    public:
        SoLoud::AudioSourceInstance* createInstance() override;
    };

} // namespace Dynamix
