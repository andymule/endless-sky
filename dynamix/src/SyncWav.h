#pragma once
#include "soloud.h"
#include "soloud_wav.h"

namespace Dynamix {

    // Custom WavInstance that supports accurate seeking
    class SyncWavInstance : public SoLoud::WavInstance {
    public:
        SyncWavInstance(SoLoud::Wav* aParent);
        virtual SoLoud::result seek(SoLoud::time aSeconds, float* aScratch,
                                    unsigned int aScratchSize);
        virtual double getStreamPosition();

    private:
        double mSeekPosition = 0.0;
    };

    // Custom Wav that creates SyncWavInstance
    class SyncWav : public SoLoud::Wav {
    public:
        virtual SoLoud::AudioSourceInstance* createInstance();
    };

} // namespace Dynamix