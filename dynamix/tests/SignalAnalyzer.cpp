#include "SignalAnalyzer.h"
#include "AudioTestHarness.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <numeric>
#include <random>

namespace DynamixTest {

namespace {
// calculateSpectrum evaluates bin k at k/numBins cycles per sample, so the bins
// span the whole sample rate and everything above numBins/2 is a mirror image.
size_t frequencyToBin(float frequency, int sampleRate, size_t numBins) {
    const float binWidth = static_cast<float>(sampleRate) / static_cast<float>(numBins);
    return static_cast<size_t>(frequency / binWidth + 0.5f);
}
} // namespace

// ============================================================================
// Basic Measurements
// ============================================================================

std::vector<float> SignalAnalyzer::extractChannel(const std::vector<float>& interleaved,
                                                  int channels, int channel) {
    if (channels <= 0 || channel < 0 || channel >= channels) {
        return {};
    }

    const size_t stride = static_cast<size_t>(channels);
    std::vector<float> mono;
    mono.reserve(interleaved.size() / stride);
    for (size_t i = static_cast<size_t>(channel); i < interleaved.size(); i += stride) {
        mono.push_back(interleaved[i]);
    }
    return mono;
}

float SignalAnalyzer::calculateRMS(const std::vector<float>& samples) {
    if (samples.empty()) {
        return 0.0f;
    }

    double sumSquares = 0.0;
    for (float s : samples) {
        sumSquares += static_cast<double>(s) * static_cast<double>(s);
    }

    return static_cast<float>(std::sqrt(sumSquares / static_cast<double>(samples.size())));
}

float SignalAnalyzer::calculatePeak(const std::vector<float>& samples) {
    if (samples.empty()) {
        return 0.0f;
    }

    float peak = 0.0f;
    for (float s : samples) {
        float abs = std::abs(s);
        if (abs > peak) {
            peak = abs;
        }
    }

    return peak;
}

float SignalAnalyzer::calculateDBFS(const std::vector<float>& samples) {
    float rms = calculateRMS(samples);
    if (rms <= 0.0f) {
        return -120.0f; // Practical minimum
    }
    return 20.0f * std::log10(rms);
}

float SignalAnalyzer::calculateDCOffset(const std::vector<float>& samples) {
    if (samples.empty()) {
        return 0.0f;
    }

    double sum = 0.0;
    for (float s : samples) {
        sum += static_cast<double>(s);
    }

    return static_cast<float>(sum / static_cast<double>(samples.size()));
}

// ============================================================================
// Frequency Analysis
// ============================================================================

float SignalAnalyzer::findDominantFrequency(const std::vector<float>& samples, int sampleRate) {
    if (samples.size() < 100) {
        return 0.0f;
    }

    // Use autocorrelation for frequency detection
    // This is simpler and more robust than FFT for single frequency detection
    size_t maxLag = std::min(samples.size() / 2, static_cast<size_t>(sampleRate / 20)); // Min 20Hz
    size_t minLag = static_cast<size_t>(sampleRate / 20000);                            // Max 20kHz

    std::vector<double> autocorr(maxLag - minLag);

    for (size_t lag = minLag; lag < maxLag; ++lag) {
        double sum = 0.0;
        size_t count = samples.size() - lag;
        for (size_t i = 0; i < count; ++i) {
            sum += static_cast<double>(samples[i]) * static_cast<double>(samples[i + lag]);
        }
        autocorr[lag - minLag] = sum / static_cast<double>(count);
    }

    // Autocorrelation peaks at every multiple of the period, so take the first
    // peak after the initial zero crossing. Taking the global maximum instead
    // picks an arbitrary multiple of the period and reports a subharmonic.
    size_t peakIndex = 0;
    bool foundZeroCrossing = false;

    for (size_t i = 1; i + 1 < autocorr.size(); ++i) {
        if (!foundZeroCrossing) {
            if (autocorr[i] < 0.0) {
                foundZeroCrossing = true;
            }
            continue;
        }
        if (autocorr[i] > autocorr[i - 1] && autocorr[i] >= autocorr[i + 1]) {
            peakIndex = i;
            break;
        }
    }

    if (peakIndex == 0) {
        return 0.0f;
    }

    // Parabolic interpolation around the peak, so the resolution isn't limited
    // to whole-sample lags (at 44.1kHz that is ~4Hz of error near 440Hz).
    const double prev = autocorr[peakIndex - 1];
    const double curr = autocorr[peakIndex];
    const double next = autocorr[peakIndex + 1];
    const double denom = 2.0 * (prev - 2.0 * curr + next);

    double peakLag = static_cast<double>(peakIndex + minLag);
    if (std::abs(denom) > 1e-12) {
        peakLag += (prev - next) / denom;
    }

    if (peakLag <= 0.0) {
        return 0.0f;
    }

    return static_cast<float>(static_cast<double>(sampleRate) / peakLag);
}

std::vector<float> SignalAnalyzer::calculateSpectrum(const std::vector<float>& samples,
                                                     size_t numBins) {
    if (samples.empty()) {
        return std::vector<float>(numBins, 0.0f);
    }

    std::vector<float> spectrum(numBins, 0.0f);
    size_t N = std::min(samples.size(), numBins * 4); // Use at most 4x bins samples

    // Simple DFT (not FFT, but sufficient for testing)
    for (size_t k = 0; k < numBins; ++k) {
        double real = 0.0;
        double imag = 0.0;
        double freq = static_cast<double>(k) / static_cast<double>(numBins);

        for (size_t n = 0; n < N; ++n) {
            double angle = 2.0 * M_PI * freq * static_cast<double>(n);
            real += static_cast<double>(samples[n]) * std::cos(angle);
            imag -= static_cast<double>(samples[n]) * std::sin(angle);
        }

        spectrum[k] = static_cast<float>(std::sqrt(real * real + imag * imag) / static_cast<double>(N));
    }

    // Normalize
    float maxVal = *std::max_element(spectrum.begin(), spectrum.end());
    if (maxVal > 0.0f) {
        for (float& s : spectrum) {
            s /= maxVal;
        }
    }

    return spectrum;
}

bool SignalAnalyzer::hasFrequencyPeak(const std::vector<float>& spectrum, float targetFreq,
                                      int sampleRate, float threshold) {
    if (spectrum.empty()) {
        return false;
    }

    const size_t targetBin = frequencyToBin(targetFreq, sampleRate, spectrum.size());
    if (targetBin >= spectrum.size() / 2) {
        return false;
    }

    // Check if there's a peak at or near the target bin
    size_t searchRadius = std::max(size_t(1), static_cast<size_t>(spectrum.size() / 50));
    size_t startBin = targetBin > searchRadius ? targetBin - searchRadius : 0;
    size_t endBin = std::min(targetBin + searchRadius, spectrum.size() - 1);

    for (size_t i = startBin; i <= endBin; ++i) {
        if (spectrum[i] >= threshold) {
            return true;
        }
    }

    return false;
}

float SignalAnalyzer::measureHighFrequencyRatio(const std::vector<float>& samples, int sampleRate,
                                                float cutoffHz) {
    auto spectrum = calculateSpectrum(samples, 256);
    if (spectrum.empty()) {
        return 0.0f;
    }

    const size_t nyquistBin = spectrum.size() / 2;
    const size_t cutoffBin = frequencyToBin(cutoffHz, sampleRate, spectrum.size());
    if (cutoffBin >= nyquistBin) {
        return 0.0f;
    }

    float lowEnergy = 0.0f;
    float highEnergy = 0.0f;

    for (size_t i = 0; i < cutoffBin; ++i) {
        lowEnergy += spectrum[i] * spectrum[i];
    }

    for (size_t i = cutoffBin; i < nyquistBin; ++i) {
        highEnergy += spectrum[i] * spectrum[i];
    }

    if (lowEnergy <= 0.0f) {
        return 0.0f;
    }

    return std::sqrt(highEnergy) / std::sqrt(lowEnergy);
}

// ============================================================================
// Effect-Specific Verification
// ============================================================================

bool SignalAnalyzer::hasEcho(const std::vector<float>& dry, const std::vector<float>& wet,
                             float delayMs, int sampleRate, float tolerance) {
    // Find peaks in both signals
    float dryPeak = calculatePeak(dry);
    float threshold = dryPeak * 0.3f;

    auto dryPeaks = findPeakPositions(dry, threshold);
    auto wetPeaks = findPeakPositions(wet, threshold);

    if (dryPeaks.empty()) {
        return false;
    }

    // Calculate expected delay in samples
    size_t expectedDelay = static_cast<size_t>(delayMs * static_cast<float>(sampleRate) / 1000.0f);
    size_t toleranceSamples = static_cast<size_t>(tolerance * static_cast<float>(sampleRate) / 1000.0f);

    // Check if wet signal has more peaks, and they appear at expected delay intervals
    if (wetPeaks.size() <= dryPeaks.size()) {
        return false;
    }

    // Look for peaks at expected delay from original peaks
    size_t echoesFound = 0;
    for (size_t dryPeak : dryPeaks) {
        size_t expectedEchoPos = dryPeak + expectedDelay;

        for (size_t wetPeak : wetPeaks) {
            if (wetPeak >= expectedEchoPos - toleranceSamples &&
                wetPeak <= expectedEchoPos + toleranceSamples) {
                echoesFound++;
                break;
            }
        }
    }

    return echoesFound > 0;
}

bool SignalAnalyzer::hasReverbTail(const std::vector<float>& samples, float tailThresholdDb,
                                   int sampleRate) {
    if (samples.empty()) {
        return false;
    }

    float peak = calculatePeak(samples);
    if (peak <= 0.0f) {
        return false;
    }

    // Convert threshold to linear
    float linearThreshold = peak * std::pow(10.0f, tailThresholdDb / 20.0f);

    // Find where signal drops below threshold
    size_t tailStart = 0;
    bool foundPeak = false;

    for (size_t i = 0; i < samples.size(); ++i) {
        if (!foundPeak && std::abs(samples[i]) >= peak * 0.9f) {
            foundPeak = true;
            tailStart = i;
        }
    }

    if (!foundPeak) {
        return false;
    }

    // Check if signal remains above threshold for significant time after peak
    size_t tailLength = 0;
    for (size_t i = tailStart; i < samples.size(); ++i) {
        if (std::abs(samples[i]) > linearThreshold) {
            tailLength = i - tailStart;
        }
    }

    // Reverb tail should last at least 100ms
    size_t minTailSamples = static_cast<size_t>(0.1f * static_cast<float>(sampleRate));
    return tailLength > minTailSamples;
}

float SignalAnalyzer::measureReverbTime(const std::vector<float>& samples, int sampleRate) {
    if (samples.empty()) {
        return 0.0f;
    }

    float peak = calculatePeak(samples);
    if (peak <= 0.0f) {
        return 0.0f;
    }

    // Find peak position
    size_t peakPos = 0;
    for (size_t i = 0; i < samples.size(); ++i) {
        if (std::abs(samples[i]) >= peak * 0.9f) {
            peakPos = i;
            break;
        }
    }

    // Calculate envelope decay
    float rt60Threshold = peak * 0.001f; // -60dB
    size_t rt60Pos = samples.size();

    for (size_t i = peakPos; i < samples.size(); ++i) {
        if (std::abs(samples[i]) < rt60Threshold) {
            rt60Pos = i;
            break;
        }
    }

    return static_cast<float>(rt60Pos - peakPos) / static_cast<float>(sampleRate);
}

bool SignalAnalyzer::isBitCrushed(const std::vector<float>& samples, int expectedBitDepth) {
    if (samples.empty() || expectedBitDepth < 1 || expectedBitDepth > 16) {
        return false;
    }

    // Count unique values - bit-crushed signal should have limited levels
    float stepSize = 2.0f / static_cast<float>(1 << expectedBitDepth);
    std::vector<int> quantizedValues;

    for (float s : samples) {
        int quantized = static_cast<int>(std::round(s / stepSize));
        if (std::find(quantizedValues.begin(), quantizedValues.end(), quantized) ==
            quantizedValues.end()) {
            quantizedValues.push_back(quantized);
        }
    }

    // Expected number of levels for given bit depth
    int expectedLevels = 1 << expectedBitDepth;

    // Allow some tolerance
    return quantizedValues.size() <= static_cast<size_t>(expectedLevels * 1.2);
}

bool SignalAnalyzer::hasFlanger(const std::vector<float>& samples, float expectedRate,
                                int sampleRate) {
    if (samples.empty()) {
        return false;
    }

    // Calculate envelope of signal
    size_t windowSize = static_cast<size_t>(sampleRate / 100); // 10ms windows
    std::vector<float> envelope;

    for (size_t i = 0; i + windowSize < samples.size(); i += windowSize / 2) {
        float maxVal = 0.0f;
        for (size_t j = 0; j < windowSize && i + j < samples.size(); ++j) {
            maxVal = std::max(maxVal, std::abs(samples[i + j]));
        }
        envelope.push_back(maxVal);
    }

    if (envelope.size() < 10) {
        return false;
    }

    // Check for periodic modulation in envelope
    float envelopeRate = findDominantFrequency(envelope, sampleRate / static_cast<int>(windowSize / 2));

    // Allow 50% tolerance on rate
    return envelopeRate >= expectedRate * 0.5f && envelopeRate <= expectedRate * 1.5f;
}

bool SignalAnalyzer::hasDistortion(const std::vector<float>& dry, const std::vector<float>& wet) {
    if (dry.empty() || wet.empty()) {
        return false;
    }

    // Compare harmonic content using spectrum
    auto drySpectrum = calculateSpectrum(dry, 256);
    auto wetSpectrum = calculateSpectrum(wet, 256);

    // Distortion adds harmonics - wet should have more high frequency content
    float dryHighFreq = measureHighFrequencyRatio(dry, 44100, 2000.0f);
    float wetHighFreq = measureHighFrequencyRatio(wet, 44100, 2000.0f);

    // Wet signal should have noticeably more high frequency content
    return wetHighFreq > dryHighFreq * 1.2f;
}

float SignalAnalyzer::measureTHD(const std::vector<float>& samples, float fundamentalFreq,
                                 int sampleRate) {
    constexpr size_t kNumBins = 512;
    auto spectrum = calculateSpectrum(samples, kNumBins);
    if (spectrum.empty()) {
        return 0.0f;
    }

    const size_t nyquistBin = kNumBins / 2;

    // A partial that does not land exactly on a bin leaks into its neighbours,
    // so measure each one over a small window rather than a single bin.
    auto partialPower = [&](int harmonic) {
        const size_t center =
            frequencyToBin(fundamentalFreq * static_cast<float>(harmonic), sampleRate, kNumBins);
        if (center == 0 || center >= nyquistBin) {
            return 0.0f;
        }

        float power = 0.0f;
        for (size_t bin = center - 1; bin <= std::min(center + 1, nyquistBin - 1); ++bin) {
            power += spectrum[bin] * spectrum[bin];
        }
        return power;
    };

    const float fundamentalPower = partialPower(1);
    if (fundamentalPower <= 0.0f) {
        return 0.0f;
    }

    float harmonicPower = 0.0f;
    for (int h = 2; h <= 6; ++h) {
        harmonicPower += partialPower(h);
    }

    return 100.0f * std::sqrt(harmonicPower / fundamentalPower);
}

bool SignalAnalyzer::hasFrequencyShift(const std::vector<float>& dry, const std::vector<float>& wet,
                                       int sampleRate) {
    float dryFreq = findDominantFrequency(dry, sampleRate);
    float wetFreq = findDominantFrequency(wet, sampleRate);

    if (dryFreq <= 0.0f || wetFreq <= 0.0f) {
        return false;
    }

    // Consider shifted if more than 5% change
    float ratio = wetFreq / dryFreq;
    return ratio < 0.95f || ratio > 1.05f;
}

bool SignalAnalyzer::hasFilterCutoff(const std::vector<float>& dry, const std::vector<float>& wet,
                                     float cutoffHz, int sampleRate) {
    // Compare high frequency content above cutoff
    float dryHigh = measureHighFrequencyRatio(dry, sampleRate, cutoffHz);
    float wetHigh = measureHighFrequencyRatio(wet, sampleRate, cutoffHz);

    // Filtered signal should have significantly less high frequency content
    return wetHigh < dryHigh * 0.5f;
}

bool SignalAnalyzer::hasHighFrequencyRolloff(const std::vector<float>& samples, int sampleRate) {
    float ratio = measureHighFrequencyRatio(samples, sampleRate, 4000.0f);
    return ratio < 0.3f; // High frequencies significantly attenuated
}

// ============================================================================
// Comparative Analysis
// ============================================================================

float SignalAnalyzer::correlate(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.empty() || b.empty()) {
        return 0.0f;
    }

    size_t N = std::min(a.size(), b.size());

    // Calculate means
    double meanA = 0.0, meanB = 0.0;
    for (size_t i = 0; i < N; ++i) {
        meanA += static_cast<double>(a[i]);
        meanB += static_cast<double>(b[i]);
    }
    meanA /= static_cast<double>(N);
    meanB /= static_cast<double>(N);

    // Calculate correlation
    double num = 0.0, denA = 0.0, denB = 0.0;
    for (size_t i = 0; i < N; ++i) {
        double diffA = static_cast<double>(a[i]) - meanA;
        double diffB = static_cast<double>(b[i]) - meanB;
        num += diffA * diffB;
        denA += diffA * diffA;
        denB += diffB * diffB;
    }

    double den = std::sqrt(denA * denB);
    if (den <= 0.0) {
        return 0.0f;
    }

    return static_cast<float>(num / den);
}

bool SignalAnalyzer::signalsDifferent(const std::vector<float>& a, const std::vector<float>& b,
                                      float threshold) {
    float diff = calculateDifference(a, b);
    return diff > threshold;
}

float SignalAnalyzer::calculateDifference(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.empty() || b.empty()) {
        return 0.0f;
    }

    size_t N = std::min(a.size(), b.size());
    double sumSquares = 0.0;

    for (size_t i = 0; i < N; ++i) {
        double diff = static_cast<double>(a[i]) - static_cast<double>(b[i]);
        sumSquares += diff * diff;
    }

    return static_cast<float>(std::sqrt(sumSquares / static_cast<double>(N)));
}

size_t SignalAnalyzer::countPeaks(const std::vector<float>& samples, float threshold) {
    return findPeakPositions(samples, threshold).size();
}

std::vector<size_t> SignalAnalyzer::findPeakPositions(const std::vector<float>& samples,
                                                      float threshold) {
    std::vector<size_t> peaks;
    if (samples.size() < 3) {
        return peaks;
    }

    // Rising-or-equal then falling, so that a plateau counts once instead of
    // not at all: in an interleaved buffer one impulse occupies as many equal
    // adjacent samples as there are channels.
    for (size_t i = 1; i < samples.size() - 1; ++i) {
        float val = std::abs(samples[i]);
        if (val >= threshold && val >= std::abs(samples[i - 1]) && val > std::abs(samples[i + 1])) {
            peaks.push_back(i);
        }
    }

    return peaks;
}

// ============================================================================
// Latency Measurement
// ============================================================================

float SignalAnalyzer::measureLatencyMs(const std::vector<float>& input,
                                       const std::vector<float>& output, int sampleRate) {
    if (input.empty() || output.empty()) {
        return 0.0f;
    }

    // Find correlation at different lags
    size_t maxLag = std::min(input.size(), static_cast<size_t>(sampleRate / 10)); // Max 100ms
    float maxCorr = 0.0f;
    size_t bestLag = 0;

    for (size_t lag = 0; lag < maxLag; ++lag) {
        size_t N = std::min(input.size(), output.size() - lag);
        if (N < 100) continue;

        double sum = 0.0;
        for (size_t i = 0; i < N; ++i) {
            sum += static_cast<double>(input[i]) * static_cast<double>(output[i + lag]);
        }

        float corr = static_cast<float>(std::abs(sum) / static_cast<double>(N));
        if (corr > maxCorr) {
            maxCorr = corr;
            bestLag = lag;
        }
    }

    return 1000.0f * static_cast<float>(bestLag) / static_cast<float>(sampleRate);
}

// ============================================================================
// Throughput Analysis
// ============================================================================

SignalAnalyzer::ThroughputStats SignalAnalyzer::measureThroughput(AudioTestHarness& harness,
                                                                  float durationSeconds,
                                                                  size_t bufferSize) {
    ThroughputStats stats;

    size_t totalSamples =
        static_cast<size_t>(durationSeconds * static_cast<float>(harness.getSampleRate()));
    size_t numBuffers = totalSamples / bufferSize;

    std::vector<float> processTimes;
    processTimes.reserve(numBuffers);

    float budgetMs = 1000.0f * static_cast<float>(bufferSize) / static_cast<float>(harness.getSampleRate());

    for (size_t i = 0; i < numBuffers; ++i) {
        auto start = std::chrono::high_resolution_clock::now();

        harness.processAndCapture(bufferSize);

        auto end = std::chrono::high_resolution_clock::now();
        float processTime = std::chrono::duration<float, std::milli>(end - start).count();

        processTimes.push_back(processTime);

        if (processTime > budgetMs) {
            stats.bufferUnderruns++;
        }
    }

    stats.totalBuffersProcessed = numBuffers;

    if (!processTimes.empty()) {
        float sum = std::accumulate(processTimes.begin(), processTimes.end(), 0.0f);
        stats.avgProcessTimeMs = sum / static_cast<float>(processTimes.size());
        stats.maxProcessTimeMs = *std::max_element(processTimes.begin(), processTimes.end());
        stats.minProcessTimeMs = *std::min_element(processTimes.begin(), processTimes.end());
        stats.cpuUsagePercent = 100.0f * stats.avgProcessTimeMs / budgetMs;
    }

    return stats;
}

// ============================================================================
// Test Signal Generation
// ============================================================================

std::vector<float> SignalAnalyzer::generateSineWave(float frequency, float durationSeconds,
                                                    int sampleRate, float amplitude,
                                                    int channels) {
    size_t numFrames = static_cast<size_t>(durationSeconds * static_cast<float>(sampleRate));
    std::vector<float> samples(numFrames * static_cast<size_t>(channels));

    for (size_t i = 0; i < numFrames; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        float value = amplitude * std::sin(2.0f * static_cast<float>(M_PI) * frequency * t);

        for (int c = 0; c < channels; ++c) {
            samples[i * static_cast<size_t>(channels) + static_cast<size_t>(c)] = value;
        }
    }

    return samples;
}

std::vector<float> SignalAnalyzer::generateWhiteNoise(float durationSeconds, int sampleRate,
                                                      float amplitude, int channels) {
    size_t numFrames = static_cast<size_t>(durationSeconds * static_cast<float>(sampleRate));
    std::vector<float> samples(numFrames * static_cast<size_t>(channels));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-amplitude, amplitude);

    for (size_t i = 0; i < numFrames; ++i) {
        float value = dis(gen);
        for (int c = 0; c < channels; ++c) {
            samples[i * static_cast<size_t>(channels) + static_cast<size_t>(c)] = value;
        }
    }

    return samples;
}

std::vector<float> SignalAnalyzer::generateImpulse(float durationSeconds, int sampleRate,
                                                   float impulsePosition, float amplitude,
                                                   int channels) {
    size_t numFrames = static_cast<size_t>(durationSeconds * static_cast<float>(sampleRate));
    std::vector<float> samples(numFrames * static_cast<size_t>(channels), 0.0f);

    size_t impulseFrame = static_cast<size_t>(impulsePosition * static_cast<float>(numFrames));
    if (impulseFrame < numFrames) {
        for (int c = 0; c < channels; ++c) {
            samples[impulseFrame * static_cast<size_t>(channels) + static_cast<size_t>(c)] = amplitude;
        }
    }

    return samples;
}

std::vector<float> SignalAnalyzer::generateChirp(float startFreq, float endFreq,
                                                 float durationSeconds, int sampleRate,
                                                 float amplitude, int channels) {
    size_t numFrames = static_cast<size_t>(durationSeconds * static_cast<float>(sampleRate));
    std::vector<float> samples(numFrames * static_cast<size_t>(channels));

    float freqSlope = (endFreq - startFreq) / durationSeconds;

    for (size_t i = 0; i < numFrames; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        float freq = startFreq + freqSlope * t;
        float phase = 2.0f * static_cast<float>(M_PI) * (startFreq * t + 0.5f * freqSlope * t * t);
        float value = amplitude * std::sin(phase);

        for (int c = 0; c < channels; ++c) {
            samples[i * static_cast<size_t>(channels) + static_cast<size_t>(c)] = value;
        }
    }

    return samples;
}

} // namespace DynamixTest
