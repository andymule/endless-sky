#pragma once

/**
 * AudioTestHarness - Real audio capture infrastructure for testing
 *
 * This harness initializes SoLoud with the NULLDRIVER backend, allowing
 * full audio processing without speaker output. The mix() function is
 * used to capture processed audio samples for analysis.
 *
 * Key capabilities:
 * - Headless audio processing (no speakers needed)
 * - Full effect chain processing
 * - Audio capture after all effects applied
 * - Deterministic results with test signals
 */

#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Forward declarations
namespace SoLoud {
class Soloud;
class Wav;
class Bus;
class Filter;
} // namespace SoLoud

namespace Dynamix {
class AudioSystem;
class FilterManager;
} // namespace Dynamix

namespace DynamixTest {

/**
 * Audio capture harness for testing real audio processing
 */
class AudioTestHarness {
public:
    /**
     * Create a test harness with specified audio parameters
     * @param sampleRate Audio sample rate (default 44100)
     * @param channels Number of audio channels (default 2 for stereo)
     * @param bufferSize SoLoud buffer size (default 2048)
     */
    AudioTestHarness(int sampleRate = 44100, int channels = 2, int bufferSize = 2048);
    ~AudioTestHarness();

    // Non-copyable
    AudioTestHarness(const AudioTestHarness&) = delete;
    AudioTestHarness& operator=(const AudioTestHarness&) = delete;

    /**
     * Initialize the audio system with NULLDRIVER backend
     * @return true if initialization successful
     */
    bool initialize();

    /**
     * Check if the harness is initialized
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * Get the sample rate
     */
    int getSampleRate() const { return m_sampleRate; }

    /**
     * Get the number of channels
     */
    int getChannels() const { return m_channels; }

    /**
     * Load a test track from file
     * @param path Path to OGG file
     * @return Track index, or -1 on failure
     */
    int loadTrack(const std::filesystem::path& path);

    /**
     * Load a track from raw audio data (for generated test signals)
     * @param samples Interleaved audio samples
     * @param numFrames Number of audio frames (samples per channel)
     * @return Track index, or -1 on failure
     */
    int loadTrackFromMemory(const std::vector<float>& samples, size_t numFrames);

    /**
     * Play all loaded tracks
     */
    void playAllTracks();

    /**
     * Stop all tracks
     */
    void stopAllTracks();

    /**
     * Process N samples and capture the output
     * This is THE key function - audio goes through all tracks, effects, bus
     * @param numSamples Number of samples to process
     * @return Captured audio buffer (interleaved stereo)
     */
    std::vector<float> processAndCapture(size_t numSamples);

    /**
     * Convenience: process for N seconds
     * @param seconds Duration to process
     * @return Captured audio buffer
     */
    std::vector<float> processSeconds(float seconds);

    /**
     * Get the raw SoLoud engine for advanced operations
     */
    SoLoud::Soloud& getSoLoud();

    /**
     * Get total number of loaded tracks
     */
    size_t getTrackCount() const;

    // ========== Filter Control ==========

    /**
     * Set a filter parameter on a track
     *
     * The filter is created and attached to the track on first use. Because a
     * SoLoud voice only picks up filters when it starts, attaching a filter to
     * an already playing track restarts that track from the beginning.
     *
     * @param trackIndex Track index
     * @param filterName Filter name (e.g., "echo", "freeverb")
     * @param paramId Parameter ID (0 = wet level for most filters)
     * @param value Parameter value
     */
    void setFilterParameter(size_t trackIndex, const std::string& filterName, int paramId,
                            float value);

    /**
     * Get a filter parameter from a track
     */
    float getFilterParameter(size_t trackIndex, const std::string& filterName, int paramId) const;

    /**
     * Set a filter parameter on the master bus
     *
     * Attaching the first bus filter restarts the bus voice, which also
     * restarts the tracks playing through it, so playback positions reset.
     */
    void setBusFilterParameter(const std::string& filterName, int paramId, float value);

    /**
     * Get a filter parameter from the master bus
     */
    float getBusFilterParameter(const std::string& filterName, int paramId) const;

    // ========== Tempo Control ==========

    /**
     * Set the granular tempo (pitch-preserving)
     * @param tempo Tempo multiplier (0.5 to 2.0)
     */
    void setGranularTempo(float tempo);

    /**
     * Get the current granular tempo
     */
    float getGranularTempo() const;

    /**
     * Set the master tempo (tape speed, affects pitch)
     * @param tempo Tempo multiplier (0.1 to 4.0)
     */
    void setMasterTempo(float tempo);

    // ========== Volume Control ==========

    /**
     * Set track volume
     */
    void setTrackVolume(size_t trackIndex, float volume);

    /**
     * Get track volume
     */
    float getTrackVolume(size_t trackIndex) const;

    /**
     * Set master bus volume
     */
    void setBusVolume(float volume);

    // ========== Sync Information ==========

    /**
     * Get current global playback time
     */
    double getGlobalTime() const;

    /**
     * Get track position
     */
    double getTrackPosition(size_t trackIndex) const;

    // ========== Utility ==========

    /**
     * Clear all tracks
     */
    void clearAllTracks();

    /**
     * Get the FilterManager for parameter queries
     */
    const Dynamix::FilterManager& getFilterManager() const;

private:
    std::unique_ptr<SoLoud::Soloud> m_soloud;
    std::unique_ptr<SoLoud::Bus> m_bus;
    std::vector<std::unique_ptr<SoLoud::Wav>> m_tracks;
    std::vector<unsigned int> m_trackHandles;
    unsigned int m_busHandle = 0;

    std::unique_ptr<Dynamix::FilterManager> m_filterManager;

    int m_sampleRate;
    int m_channels;
    int m_bufferSize;
    bool m_initialized = false;
    float m_busVolume = 1.0f;

    // A live SoLoud filter plus the parameter values set on it. The slot is the
    // filter slot it occupies on its audio source.
    struct FilterInstance {
        std::unique_ptr<SoLoud::Filter> filter;
        int slot = -1;
        bool attached = false;
        std::map<int, float> parameters;
    };
    struct FilterSet {
        std::map<std::string, FilterInstance> filters;
    };
    std::vector<FilterSet> m_trackFilters;
    FilterSet m_busFilters;

    FilterInstance* ensureFilter(FilterSet& set, const std::string& filterName);
    std::vector<float> buildParameterValues(const std::string& filterName,
                                            const std::map<int, float>& values) const;
    void pushParametersToVoice(const FilterSet& set, unsigned int voiceHandle) const;
};

/**
 * RAII helper for temporary test directories
 */
class TempTestDirectory {
public:
    TempTestDirectory();
    ~TempTestDirectory();

    // Non-copyable
    TempTestDirectory(const TempTestDirectory&) = delete;
    TempTestDirectory& operator=(const TempTestDirectory&) = delete;

    /**
     * Get the path to the temporary directory
     */
    const std::filesystem::path& path() const { return m_path; }

    /**
     * Create a song folder within the temp directory
     */
    void createSongFolder(const std::string& name);

    /**
     * Copy a test track file to a song folder
     * @param sourceTrack Source OGG file path
     * @param songName Song folder name
     * @param destName Destination filename (or empty to use source name)
     */
    void copyTrackToSong(const std::filesystem::path& sourceTrack, const std::string& songName,
                         const std::string& destName = "");

    /**
     * Write a _song.json file
     */
    void writeSongJson(const std::string& songName, const std::string& jsonContent);

    /**
     * Write a _master.json file at the root
     */
    void writeMasterJson(const std::string& jsonContent);

private:
    std::filesystem::path m_path;
};

} // namespace DynamixTest
