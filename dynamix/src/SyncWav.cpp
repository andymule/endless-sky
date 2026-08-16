#include "SyncWav.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Dynamix {

    SyncWavInstance::SyncWavInstance(SoLoud::Wav* aParent) : m_parent(aParent) {
        if (!aParent) {
            return;
        }
        const unsigned int channels = std::max(1u, aParent->mChannels);
        const int sampleRate =
            static_cast<int>(aParent->mBaseSamplerate > 0.0f ? aParent->mBaseSamplerate : 44100.0f);
        m_processor.configure(sampleRate, static_cast<int>(channels));
        m_inputBuffers.assign(channels, std::vector<float>(kScratchFrames, 0.0f));
        m_inputPointers.resize(channels, nullptr);
        m_outputPointers.resize(channels, nullptr);
        bindInputPointers();
    }

    void SyncWavInstance::ensureConfigured() {
        if (!m_parent || m_processor.isConfigured()) {
            return;
        }
        const unsigned int channels = std::max(1u, mChannels != 0 ? mChannels : m_parent->mChannels);
        const int sampleRate = static_cast<int>(mBaseSamplerate > 0.0f ? mBaseSamplerate : 44100.0f);
        m_processor.configure(sampleRate, static_cast<int>(channels));
        m_inputBuffers.assign(channels, std::vector<float>(kScratchFrames, 0.0f));
        m_inputPointers.resize(channels, nullptr);
        m_outputPointers.resize(channels, nullptr);
        bindInputPointers();
    }

    void SyncWavInstance::bindInputPointers() {
        for (size_t ch = 0; ch < m_inputBuffers.size(); ++ch) {
            m_inputPointers[ch] = m_inputBuffers[ch].data();
        }
    }

    int SyncWavInstance::readSourceFrames(float** dest, int frames, bool looping) {
        if (!m_parent || !m_parent->mData || m_parent->mSampleCount == 0 || frames <= 0) {
            return 0;
        }

        const unsigned int sampleCount = m_parent->mSampleCount;
        const unsigned int channels = mChannels;
        int copied = 0;

        while (copied < frames) {
            if (m_sourceOffset >= sampleCount) {
                if (!looping) {
                    break;
                }
                m_sourceOffset = 0;
            }

            const unsigned int remaining = sampleCount - m_sourceOffset;
            const unsigned int toCopy = std::min(remaining, static_cast<unsigned int>(frames - copied));
            for (unsigned int ch = 0; ch < channels; ++ch) {
                const float* src = m_parent->mData + (ch * sampleCount) + m_sourceOffset;
                std::memcpy(dest[ch] + copied, src, toCopy * sizeof(float));
            }
            m_sourceOffset += toCopy;
            copied += static_cast<int>(toCopy);
        }

        if (copied < frames) {
            for (unsigned int ch = 0; ch < channels; ++ch) {
                std::memset(dest[ch] + copied, 0, static_cast<size_t>(frames - copied) * sizeof(float));
            }
        }

        return copied;
    }

    unsigned int SyncWavInstance::renderDry(float* aBuffer, unsigned int aSamplesToRead,
                                            unsigned int aBufferSize) {
        if (!m_parent || !m_parent->mData || m_parent->mSampleCount == 0) {
            return 0;
        }

        const bool looping = (mFlags & LOOPING) != 0;
        const unsigned int sampleCount = m_parent->mSampleCount;
        const unsigned int channels = mChannels;
        unsigned int written = 0;

        while (written < aSamplesToRead) {
            if (m_sourceOffset >= sampleCount) {
                if (!looping) {
                    break;
                }
                m_sourceOffset = 0;
            }

            const unsigned int remaining = sampleCount - m_sourceOffset;
            const unsigned int toCopy = std::min(remaining, aSamplesToRead - written);
            for (unsigned int ch = 0; ch < channels; ++ch) {
                const float* src = m_parent->mData + (ch * sampleCount) + m_sourceOffset;
                std::memcpy(aBuffer + ch * aBufferSize + written, src, toCopy * sizeof(float));
            }
            m_sourceOffset += toCopy;
            written += toCopy;
        }

        return written;
    }

    unsigned int SyncWavInstance::renderStretched(float* aBuffer, unsigned int aSamplesToRead,
                                                  unsigned int aBufferSize, float tempo) {
        ensureConfigured();
        if (!m_processor.isConfigured()) {
            return renderDry(aBuffer, aSamplesToRead, aBufferSize);
        }

        const bool looping = (mFlags & LOOPING) != 0;
        const unsigned int channels = mChannels;
        unsigned int written = 0;

        if (!m_primed) {
            const int primeFrames =
                std::min(static_cast<int>(kScratchFrames), std::max(1, m_processor.inputLatency()));
            readSourceFrames(m_inputPointers.data(), primeFrames, looping);
            m_processor.prime(m_inputPointers.data(), primeFrames, tempo);
            m_primed = true;
        }

        while (written < aSamplesToRead) {
            const unsigned int chunk = std::min(kMaxChunk, aSamplesToRead - written);
            int inputFrames =
                AudioStreamProcessor::inputFramesFor(static_cast<int>(chunk), m_inputPhase, tempo);
            inputFrames = std::min(inputFrames, static_cast<int>(kScratchFrames));

            const int got = readSourceFrames(m_inputPointers.data(), inputFrames, looping);
            if (got <= 0 && !looping) {
                break;
            }

            for (unsigned int ch = 0; ch < channels; ++ch) {
                m_outputPointers[ch] = aBuffer + ch * aBufferSize + written;
            }

            m_processor.process(m_inputPointers.data(), inputFrames, m_outputPointers.data(),
                                static_cast<int>(chunk));
            written += chunk;
        }

        return written;
    }

    unsigned int SyncWavInstance::getAudio(float* aBuffer, unsigned int aSamplesToRead,
                                           unsigned int aBufferSize) {
        if (!m_parent || !m_parent->mData || aSamplesToRead == 0) {
            return 0;
        }

        const float tempo = AudioStreamProcessor::getGlobalTempo();
        const bool stretching = std::abs(tempo - 1.0f) > AudioStreamProcessor::kBypassEpsilon;

        if (stretching != m_wasStretching) {
            m_processor.reset();
            m_primed = false;
            m_inputPhase = 0.0;
            m_wasStretching = stretching;
        }

        if (!stretching) {
            return renderDry(aBuffer, aSamplesToRead, aBufferSize);
        }
        return renderStretched(aBuffer, aSamplesToRead, aBufferSize, tempo);
    }

    SoLoud::result SyncWavInstance::rewind() {
        m_sourceOffset = 0;
        m_inputPhase = 0.0;
        m_primed = false;
        mStreamPosition = 0.0;
        m_processor.reset();
        return SoLoud::SO_NO_ERROR;
    }

    SoLoud::result SyncWavInstance::seek(SoLoud::time aSeconds, float* /*aScratch*/,
                                         unsigned int /*aScratchSize*/) {
        if (!m_parent || m_parent->mSampleCount == 0 || mBaseSamplerate <= 0.0f) {
            return SoLoud::INVALID_PARAMETER;
        }

        const float tempo = AudioStreamProcessor::getGlobalTempo();
        const bool stretching = std::abs(tempo - 1.0f) > AudioStreamProcessor::kBypassEpsilon;
        const double sourceSeconds = stretching ? (aSeconds * static_cast<double>(tempo)) : aSeconds;
        double sourceFrames = sourceSeconds * static_cast<double>(mBaseSamplerate);
        if (sourceFrames < 0.0) {
            sourceFrames = 0.0;
        }

        const unsigned int sampleCount = m_parent->mSampleCount;
        auto pos = static_cast<unsigned int>(sourceFrames);
        if ((mFlags & LOOPING) && sampleCount > 0) {
            pos %= sampleCount;
        } else if (pos > sampleCount) {
            pos = sampleCount;
        }

        m_sourceOffset = pos;
        m_inputPhase = 0.0;
        m_primed = false;
        mStreamPosition = aSeconds;
        m_processor.reset();
        return SoLoud::SO_NO_ERROR;
    }

    bool SyncWavInstance::hasEnded() {
        if (!m_parent) {
            return true;
        }
        if (mFlags & LOOPING) {
            return false;
        }
        return m_sourceOffset >= m_parent->mSampleCount;
    }

    SoLoud::AudioSourceInstance* SyncWav::createInstance() { return new SyncWavInstance(this); }

} // namespace Dynamix
