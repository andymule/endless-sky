#pragma once

#include "AudioState.h"
#include "AudioStreamProcessor.h"
#include "FilterManager.h"
#include "Logger.h"
#include "soloud.h"
#include "soloud_bassboostfilter.h"
#include "soloud_biquadresonantfilter.h"
#include "soloud_bus.h"
#include "soloud_dcremovalfilter.h"
#include "soloud_echofilter.h"
#include "soloud_flangerfilter.h"
#include "soloud_freeverbfilter.h"
#include "soloud_lofifilter.h"
#include "soloud_robotizefilter.h"
#include "soloud_wav.h"
#include "soloud_waveshaperfilter.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace AudioTester {

    // Forward declaration for granular processor
    class AudioStreamProcessor;

    // Custom filter to intercept bus audio and route through granular processor
    class GranularInterceptFilter : public SoLoud::Filter {
    public:
        GranularInterceptFilter(AudioStreamProcessor* processor, bool* enabledFlag);
        virtual SoLoud::FilterInstance* createInstance() override;

    private:
        AudioStreamProcessor* m_processor;
        bool* m_enabledFlag;
    };

    class GranularInterceptFilterInstance : public SoLoud::FilterInstance {
    public:
        GranularInterceptFilterInstance(AudioStreamProcessor* processor, bool* enabledFlag);

        virtual void filterChannel(float* aBuffer, unsigned int aSamples, float aSamplerate,
                                   double aTime, unsigned int aChannel,
                                   unsigned int aChannels) override;

    private:
        AudioStreamProcessor* m_processor;
        bool* m_enabledFlag;
        float m_lastPitchCompensation;
    };

    // RAII wrapper for SoLoud engine
    class SoloudEngine {
    public:
        SoloudEngine() = default;
        ~SoloudEngine() = default;

        bool initialize() { return m_soloud.init() == 0; }
        void deinitialize() { m_soloud.deinit(); }
        SoLoud::Soloud& get() { return m_soloud; }
        const SoLoud::Soloud& get() const { return m_soloud; }

    private:
        SoLoud::Soloud m_soloud;
    };

    // RAII wrapper for audio bus
    class AudioBus {
    public:
        AudioBus(SoLoud::Soloud& /*soloud*/, float volume = 1.0f) : m_bus(), m_volume(volume) {
            m_bus.setVolume(volume);
        }

        ~AudioBus() = default;

        // Delete copy and move operations
        AudioBus(const AudioBus&) = delete;
        AudioBus& operator=(const AudioBus&) = delete;
        AudioBus(AudioBus&&) = delete;
        AudioBus& operator=(AudioBus&&) = delete;

        void setVolume(float volume) {
            m_volume = volume;
            m_bus.setVolume(volume);
        }

        float getVolume() const { return m_volume; }
        SoLoud::Bus& get() { return m_bus; }
        const SoLoud::Bus& get() const { return m_bus; }

    private:
        SoLoud::Bus m_bus;
        float m_volume;
    };

    enum class ParameterType { FLOAT, INT, BOOL };

    struct FilterParameter {
        float value;
        float min;
        float max;
        std::string name;
        ParameterType type = ParameterType::FLOAT;
    };

    struct FilterInstance {
        std::unique_ptr<SoLoud::Filter> filter;
        std::unordered_map<int, FilterParameter> parameters;
        bool enabled = false;
        int slot = -1;

        // Note: For most filters, parameter index 0 is the WET parameter
        // which controls the dry/wet mix (0.0 = dry only, 1.0 = wet only)
        // DCRemovalFilter is an exception and doesn't have a WET parameter
    };

    struct TrackFilters {
        std::unordered_map<std::string, FilterInstance> filters;
    };

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

    struct TrackInfo {
        std::unique_ptr<SyncWav> wav;
        double duration = 0.0;   // Track duration in seconds
        unsigned int handle = 0; // SoLoud voice handle
        bool isPlaying = false;
        bool isPaused = false;         // Track is paused (preserves position)
        double lastSyncCheck = 0.0;    // Last time we checked sync
        double expectedPosition = 0.0; // Expected playback position
        float volume = 1.0f;           // Track volume (0.0 to 1.0)
    };

    struct SyncState {
        double masterDuration = 0.0; // Duration of shortest track (master clock)
        double globalTime = 0.0;     // Global playback time
        double lastSyncCheck = 0.0;  // Last time we checked for sync issues
        bool isPlaying = false;
        size_t masterTrackIndex = 0;                       // Index of the shortest track
        static constexpr double SYNC_CHECK_INTERVAL = 0.1; // Check every 100ms
        static constexpr double DRIFT_TOLERANCE = 0.001;   // 1ms tolerance
    };

    class AudioSystem {
    public:
        AudioSystem();
        ~AudioSystem();

        bool initialize();
        void cleanup();

        // Track management
        void loadTrack(const std::string& path);
        void playTrack(size_t index);
        void stopTrack(size_t index);
        void pauseTrack(size_t index);
        void resumeTrack(size_t index);
        void setTrackVolume(size_t index, float volume);
        void setTrackLooping(size_t index, bool looping);
        void removeTrack(size_t index); // Remove track completely from audio system
        size_t getTrackCount() const { return m_tracks.size(); } // Get number of tracks
        void playAllTracks();                                    // Play all tracks with sync
        void stopAllTracks();                                    // Stop all tracks
        void pauseAllTracks();  // Pause all tracks (preserves position)
        void resumeAllTracks(); // Resume all tracks from paused position
        void clearAllTracks() {
            m_tracks.clear();
            m_trackFilters.clear();
            // Reset playback state for fresh start
            m_hasEverPlayed = false;
            m_syncState.isPlaying = false;
            m_syncState.globalTime = 0.0;
            m_syncState.lastSyncCheck = 0.0;
        }

        // Synchronization
        void updateSync(); // Call this regularly to maintain sync
        double getMasterDuration() const { return m_syncState.masterDuration; }
        double getGlobalTime() const { return m_syncState.globalTime; }
        bool isPlaying() const { return m_syncState.isPlaying; }
        bool hasPausedTracks() const; // Check if any tracks are paused

        // Bus management
        void setBusVolume(float volume);
        float getBusVolume() const;

        // Tempo/playback rate control
        void setGlobalPlaybackRate(float rate);

        // Granular tempo control (pitch-preserving)
        void setGranularTempo(float tempo);
        float getGranularTempo() const;
        float getGranularLatencyMs() const;
        bool isGranularProcessorReady() const;
        void setGranularEnabled(bool enabled);
        bool isGranularEnabled() const;

        // Dual tape speed architecture for granular tempo
        void updateDualTapeSpeed();
        float calculateInternalTapeSpeed() const;
        float calculatePitchCompensation() const;

        // Filter management
        void addFilterToTrack(size_t trackIndex, const std::string& filterName);
        void removeFilterFromTrack(size_t trackIndex, const std::string& filterName);
        void setFilterParameter(size_t trackIndex, const std::string& filterName, int paramId,
                                float value);
        void setFilterParameterByName(size_t trackIndex, const std::string& filterName,
                                      const std::string& paramName, float value);
        void setFilterEnabled(size_t trackIndex, const std::string& filterName, bool enabled);
        bool isFilterEnabled(size_t trackIndex, const std::string& filterName) const;
        float getFilterParameter(size_t trackIndex, const std::string& filterName,
                                 int paramId) const;
        float getFilterParameterByName(size_t trackIndex, const std::string& filterName,
                                       const std::string& paramName) const;
        const std::unordered_map<std::string, FilterInstance>& getFilters(size_t trackIndex) const;

        // Bus filter management
        void setBusFilterEnabled(const std::string& filterName, bool enabled);
        bool isBusFilterEnabled(const std::string& filterName) const;
        void setBusFilterParameter(const std::string& filterName, int paramId, float value);
        void setBusFilterParameterByName(const std::string& filterName,
                                         const std::string& paramName, float value);
        float getBusFilterParameter(const std::string& filterName, int paramId) const;
        float getBusFilterParameterByName(const std::string& filterName,
                                          const std::string& paramName) const;
        const std::unordered_map<std::string, FilterInstance>& getBusFilters() const;

        // Static members
        static const std::vector<std::string> AVAILABLE_FILTERS;

    private:
        void applyFiltersToTrack(size_t trackIndex);
        void updateBusFilterParams();
        void initializeFilter(FilterInstance& instance, const std::string& filterName);
        void updateFilterInstance(FilterInstance& instance, const std::string& filterName);

        // Synchronization methods
        void calculateMasterDuration();
        void checkAndCorrectSync();
        void correctTrackSync(size_t trackIndex, double targetTime);
        double getTrackCurrentTime(size_t trackIndex) const;
        bool isTrackDrifting(size_t trackIndex) const;

        // Helper methods for wet-based effect automation
        bool isWetParameter(const std::string& filterName, int paramId) const;
        int getWetParameterId(const std::string& filterName) const;
        bool shouldAutoEnableFilter(const std::string& filterName, int paramId, float value) const;

        std::unique_ptr<SoloudEngine> m_engine;
        std::unique_ptr<AudioBus> m_masterBus;
        std::unique_ptr<AudioStreamProcessor> m_granularProcessor;
        std::unique_ptr<GranularInterceptFilter> m_granularFilter;
        FilterManager m_filterManager;
        std::vector<TrackInfo> m_tracks;
        std::vector<TrackFilters> m_trackFilters;
        std::unordered_map<std::string, FilterInstance> m_busFilters;
        float m_busVolume = 1.0f;
        float m_globalPlaybackRate = 1.0f;
        bool m_isInitialized = false;
        unsigned int m_busHandle = 0;
        SyncState m_syncState;

        // Granular tempo processing
        bool m_granularEnabled = false;
        std::vector<float> m_captureBuffer;
        std::vector<float> m_outputBuffer;

        // Audio capture callback for granular processing
        void processMasterOutput(float* buffer, unsigned int samples, unsigned int channels);
        void testGranularProcessing();

        // Dual tape speed architecture
        float m_userTapeSpeed = 1.0f;     // User-controlled tape speed (0.1x - 4.0x)
        float m_granularTempo = 1.0f;     // Granular tempo multiplier (0.5x - 2.0x)
        float m_internalTapeSpeed = 1.0f; // Hidden: userTapeSpeed * granularTempo
        float m_pitchCompensation = 1.0f; // Hidden: 1.0 / granularTempo
        bool m_hasEverPlayed = false;     // Track if tracks have ever been played
    };

} // namespace AudioTester