#include "SyncWav.h"

namespace AudioTester {

    SyncWavInstance::SyncWavInstance(SoLoud::Wav* aParent) : SoLoud::WavInstance(aParent) {
        // Initialize with current stream position
        mSeekPosition = 0.0;
    }

    SoLoud::result SyncWavInstance::seek(SoLoud::time aSeconds, float* aScratch,
                                         unsigned int aScratchSize) {
        // Store the seek position for accurate tracking
        mSeekPosition = aSeconds;

        // Call parent implementation
        return SoLoud::WavInstance::seek(aSeconds, aScratch, aScratchSize);
    }

    double SyncWavInstance::getStreamPosition() {
        // Return our tracked position for more accurate sync
        return mSeekPosition;
    }

    SoLoud::AudioSourceInstance* SyncWav::createInstance() { return new SyncWavInstance(this); }

} // namespace AudioTester