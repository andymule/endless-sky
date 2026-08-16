#pragma once

/**
 * SignalAnalyzer - Audio signal analysis utilities for testing
 *
 * Provides tools to measure and verify audio signals:
 * - Basic measurements (RMS, peak, dBFS)
 * - Frequency analysis (dominant frequency, spectrum)
 * - Effect verification (echo detection, reverb tail, distortion)
 * - Comparative analysis (correlation, difference detection)
 * - Performance measurements (throughput, latency)
 */

#include <cstddef>
#include <vector>

namespace DynamixTest {

class AudioTestHarness; // Forward declaration

/**
 * Audio signal analysis utilities
 */
class SignalAnalyzer {
public:
    // ========== Basic Measurements ==========

    /**
     * Pull one channel out of an interleaved buffer
     *
     * Frequency analysis treats its input as a single stream, so an interleaved
     * buffer has to be split first or the channels read as one detuned signal.
     *
     * @param interleaved Interleaved audio samples
     * @param channels Number of channels in the buffer
     * @param channel Channel to extract
     * @return The samples of that channel
     */
    static std::vector<float> extractChannel(const std::vector<float>& interleaved, int channels,
                                             int channel = 0);

    /**
     * Calculate RMS (Root Mean Square) level of audio samples
     * @param samples Audio samples
     * @return RMS value (0.0 to ~1.0 for normalized audio)
     */
    static float calculateRMS(const std::vector<float>& samples);

    /**
     * Calculate peak (maximum absolute) level
     * @param samples Audio samples
     * @return Peak value (0.0 to ~1.0 for normalized audio)
     */
    static float calculatePeak(const std::vector<float>& samples);

    /**
     * Calculate level in dBFS (decibels relative to full scale)
     * @param samples Audio samples
     * @return dBFS value (0 = full scale, -inf = silence)
     */
    static float calculateDBFS(const std::vector<float>& samples);

    /**
     * Calculate the DC offset of the signal
     * @param samples Audio samples
     * @return DC offset value
     */
    static float calculateDCOffset(const std::vector<float>& samples);

    // ========== Frequency Analysis ==========

    /**
     * Find the dominant frequency in the signal
     * Uses autocorrelation for simple frequency detection
     * @param samples Audio samples (mono or first channel used)
     * @param sampleRate Sample rate in Hz
     * @return Dominant frequency in Hz
     */
    static float findDominantFrequency(const std::vector<float>& samples, int sampleRate);

    /**
     * Calculate a simple magnitude spectrum using DFT
     * @param samples Audio samples
     * @param numBins Number of frequency bins
     * @return Magnitude spectrum (numBins values, normalized)
     */
    static std::vector<float> calculateSpectrum(const std::vector<float>& samples,
                                                size_t numBins = 256);

    /**
     * Check if signal has significant energy at a specific frequency
     * @param spectrum Pre-calculated spectrum
     * @param targetFreq Target frequency in Hz
     * @param sampleRate Sample rate in Hz
     * @param threshold Minimum normalized magnitude
     * @return true if frequency peak found
     */
    static bool hasFrequencyPeak(const std::vector<float>& spectrum, float targetFreq,
                                 int sampleRate, float threshold = 0.1f);

    /**
     * Measure high frequency content relative to low frequency
     * @param samples Audio samples
     * @param sampleRate Sample rate
     * @param cutoffHz Frequency to split low/high
     * @return Ratio of high to low frequency energy
     */
    static float measureHighFrequencyRatio(const std::vector<float>& samples, int sampleRate,
                                           float cutoffHz = 2000.0f);

    // ========== Effect-Specific Verification ==========

    /**
     * Detect if echo/delay effect is present
     * Looks for repeated peaks at expected delay interval
     * @param dry Dry (unprocessed) signal
     * @param wet Wet (processed) signal
     * @param delayMs Expected delay in milliseconds
     * @param sampleRate Sample rate in Hz
     * @param tolerance Timing tolerance in ms
     * @return true if echo detected at expected delay
     */
    static bool hasEcho(const std::vector<float>& dry, const std::vector<float>& wet,
                        float delayMs, int sampleRate, float tolerance = 10.0f);

    /**
     * Detect reverb tail in signal
     * Looks for sustained energy after impulse
     * @param samples Audio samples (response to impulse)
     * @param tailThresholdDb Threshold below peak to consider as tail (e.g., -60 for RT60)
     * @param sampleRate Sample rate
     * @return true if reverb tail detected
     */
    static bool hasReverbTail(const std::vector<float>& samples, float tailThresholdDb,
                              int sampleRate);

    /**
     * Measure reverb decay time (RT60 approximation)
     * @param samples Audio samples (response to impulse)
     * @param sampleRate Sample rate
     * @return Decay time in seconds (time to decay 60dB)
     */
    static float measureReverbTime(const std::vector<float>& samples, int sampleRate);

    /**
     * Detect bit crushing artifacts
     * Looks for quantization steps in the signal
     * @param samples Audio samples
     * @param expectedBitDepth Expected bit depth (1-16)
     * @return true if quantization consistent with bit depth
     */
    static bool isBitCrushed(const std::vector<float>& samples, int expectedBitDepth);

    /**
     * Detect flanging/modulation effects
     * Looks for periodic amplitude/phase modulation
     * @param samples Audio samples
     * @param expectedRate Expected modulation rate in Hz
     * @param sampleRate Sample rate
     * @return true if modulation detected
     */
    static bool hasFlanger(const std::vector<float>& samples, float expectedRate, int sampleRate);

    /**
     * Detect distortion/waveshaping
     * Compares harmonic content before/after
     * @param dry Dry signal
     * @param wet Wet signal
     * @return true if distortion detected
     */
    static bool hasDistortion(const std::vector<float>& dry, const std::vector<float>& wet);

    /**
     * Measure Total Harmonic Distortion (THD)
     * @param samples Audio samples of sine wave
     * @param fundamentalFreq Fundamental frequency
     * @param sampleRate Sample rate
     * @return THD as percentage
     */
    static float measureTHD(const std::vector<float>& samples, float fundamentalFreq,
                            int sampleRate);

    /**
     * Detect frequency shift (pitch change)
     * @param dry Dry signal
     * @param wet Wet signal
     * @param sampleRate Sample rate
     * @return true if frequency shifted
     */
    static bool hasFrequencyShift(const std::vector<float>& dry, const std::vector<float>& wet,
                                  int sampleRate);

    /**
     * Detect low-pass filter effect
     * @param dry Dry signal
     * @param wet Wet signal
     * @param cutoffHz Expected cutoff frequency
     * @param sampleRate Sample rate
     * @return true if filter cutoff detected
     */
    static bool hasFilterCutoff(const std::vector<float>& dry, const std::vector<float>& wet,
                                float cutoffHz, int sampleRate);

    /**
     * Detect high-frequency rolloff in signal
     * @param samples Audio samples
     * @param sampleRate Sample rate
     * @return true if high frequencies attenuated
     */
    static bool hasHighFrequencyRolloff(const std::vector<float>& samples, int sampleRate);

    // ========== Comparative Analysis ==========

    /**
     * Calculate normalized cross-correlation between two signals
     * @param a First signal
     * @param b Second signal
     * @return Correlation coefficient (-1 to 1)
     */
    static float correlate(const std::vector<float>& a, const std::vector<float>& b);

    /**
     * Check if two signals are significantly different
     * @param a First signal
     * @param b Second signal
     * @param threshold Minimum RMS difference to consider different
     * @return true if signals are different
     */
    static bool signalsDifferent(const std::vector<float>& a, const std::vector<float>& b,
                                 float threshold = 0.01f);

    /**
     * Calculate RMS difference between two signals
     * @param a First signal
     * @param b Second signal
     * @return RMS of the difference signal
     */
    static float calculateDifference(const std::vector<float>& a, const std::vector<float>& b);

    /**
     * Count peaks in signal above threshold
     * @param samples Audio samples
     * @param threshold Minimum peak level
     * @return Number of peaks found
     */
    static size_t countPeaks(const std::vector<float>& samples, float threshold);

    /**
     * Find positions of peaks in signal
     * @param samples Audio samples
     * @param threshold Minimum peak level
     * @return Vector of sample indices where peaks occur
     */
    static std::vector<size_t> findPeakPositions(const std::vector<float>& samples,
                                                 float threshold);

    // ========== Latency Measurement ==========

    /**
     * Measure latency between input and output signals
     * Uses cross-correlation to find delay
     * @param input Input signal
     * @param output Output signal
     * @param sampleRate Sample rate
     * @return Latency in milliseconds
     */
    static float measureLatencyMs(const std::vector<float>& input,
                                  const std::vector<float>& output, int sampleRate);

    // ========== Throughput Analysis ==========

    /**
     * Statistics from throughput measurement
     */
    struct ThroughputStats {
        float avgProcessTimeMs = 0.0f;
        float maxProcessTimeMs = 0.0f;
        float minProcessTimeMs = 0.0f;
        size_t bufferUnderruns = 0;
        size_t totalBuffersProcessed = 0;
        float cpuUsagePercent = 0.0f;
    };

    /**
     * Measure audio processing throughput
     * @param harness Audio test harness to measure
     * @param durationSeconds Duration to measure
     * @param bufferSize Size of each processing buffer
     * @return Throughput statistics
     */
    static ThroughputStats measureThroughput(AudioTestHarness& harness, float durationSeconds,
                                             size_t bufferSize = 512);

    // ========== Test Signal Generation ==========

    /**
     * Generate a sine wave test signal
     * @param frequency Frequency in Hz
     * @param durationSeconds Duration
     * @param sampleRate Sample rate
     * @param amplitude Peak amplitude (0-1)
     * @param channels Number of channels
     * @return Interleaved audio samples
     */
    static std::vector<float> generateSineWave(float frequency, float durationSeconds,
                                               int sampleRate, float amplitude = 0.8f,
                                               int channels = 2);

    /**
     * Generate white noise test signal
     * @param durationSeconds Duration
     * @param sampleRate Sample rate
     * @param amplitude Peak amplitude (0-1)
     * @param channels Number of channels
     * @return Interleaved audio samples
     */
    static std::vector<float> generateWhiteNoise(float durationSeconds, int sampleRate,
                                                 float amplitude = 0.5f, int channels = 2);

    /**
     * Generate impulse test signal (single spike)
     * @param durationSeconds Total duration
     * @param sampleRate Sample rate
     * @param impulsePosition Position of impulse (0-1, relative to duration)
     * @param amplitude Impulse amplitude
     * @param channels Number of channels
     * @return Interleaved audio samples
     */
    static std::vector<float> generateImpulse(float durationSeconds, int sampleRate,
                                              float impulsePosition = 0.1f, float amplitude = 1.0f,
                                              int channels = 2);

    /**
     * Generate a chirp (frequency sweep) test signal
     * @param startFreq Start frequency in Hz
     * @param endFreq End frequency in Hz
     * @param durationSeconds Duration
     * @param sampleRate Sample rate
     * @param amplitude Peak amplitude
     * @param channels Number of channels
     * @return Interleaved audio samples
     */
    static std::vector<float> generateChirp(float startFreq, float endFreq, float durationSeconds,
                                            int sampleRate, float amplitude = 0.8f,
                                            int channels = 2);
};

} // namespace DynamixTest
