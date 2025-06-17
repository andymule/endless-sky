#include "AudioSystem.h"
#include "ErrorHandling.h"
#include <filesystem>
#include <iostream>

namespace AudioTester {

    const std::vector<std::string> AudioSystem::AVAILABLE_FILTERS = {
        "BassBoost", "BiquadResonant", "DCRemoval", "Echo",      "Flanger",
        "Freeverb",  "Lofi",           "Robotize",  "WaveShaper"};

    AudioSystem::AudioSystem() = default;

    AudioSystem::~AudioSystem() { cleanup(); }

    bool AudioSystem::initialize() {
        try {
            m_engine = std::make_unique<SoloudEngine>();
            if (!m_engine->initialize()) {
                return false;
            }

            m_masterBus = std::make_unique<AudioBus>(m_engine->get());
            m_isInitialized = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize audio system: " << e.what() << std::endl;
            return false;
        }
    }

    void AudioSystem::cleanup() {
        if (!m_isInitialized)
            return;

        if (m_busHandle != 0) {
            m_engine->get().stop(m_busHandle);
            m_busHandle = 0;
        }

        m_masterBus.reset();
        m_engine.reset();
        m_isInitialized = false;
    }

    void AudioSystem::loadAudioFile(const std::string& path) {
        if (!m_isInitialized)
            return;

        auto track = std::make_unique<SoLoud::Wav>();
        if (track->load(path.c_str()) != SoLoud::SO_NO_ERROR) {
            throw std::runtime_error("Failed to load audio file: " + path);
        }
        m_tracks.push_back(std::move(track));
    }

    void AudioSystem::playTrack(size_t index) {
        if (!m_isInitialized || index >= m_tracks.size())
            return;

        if (m_busHandle == 0) {
            m_busHandle = m_engine->get().play(m_masterBus->get());
            m_engine->get().setVolume(m_busHandle, m_busVolume);
        }

        m_engine->get().play(*m_tracks[index]);
    }

    void AudioSystem::stopTrack(size_t index) {
        if (!m_isInitialized || index >= m_tracks.size())
            return;

        m_engine->get().stopAudioSource(*m_tracks[index]);
    }

    void AudioSystem::setTrackVolume(size_t index, float volume) {
        if (!m_isInitialized || index >= m_tracks.size())
            return;

        m_tracks[index]->setVolume(volume);
    }

    void AudioSystem::setTrackLooping(size_t index, bool looping) {
        if (!m_isInitialized || index >= m_tracks.size())
            return;

        m_tracks[index]->setLooping(looping);
    }

    void AudioSystem::setBusVolume(float volume) {
        if (!m_isInitialized)
            return;

        m_busVolume = volume;
        if (m_busHandle != 0) {
            m_engine->get().setVolume(m_busHandle, volume);
        }
    }

    float AudioSystem::getBusVolume() const { return m_busVolume; }

    void AudioSystem::addFilterToTrack(size_t trackIndex, const std::string& filterName) {
        if (!m_isInitialized || trackIndex >= m_tracks.size())
            return;

        FilterInstance instance;
        instance.initialize(filterName);
        m_trackFilters.push_back(std::move(instance));
    }

    void AudioSystem::removeFilterFromTrack(size_t trackIndex, const std::string& filterName) {
        if (!m_isInitialized || trackIndex >= m_tracks.size())
            return;

        auto it = std::find_if(
            m_trackFilters.begin(), m_trackFilters.end(), [&](const FilterInstance& instance) {
                return instance.filter && instance.filter->getParamName(0) == filterName;
            });

        if (it != m_trackFilters.end()) {
            m_trackFilters.erase(it);
        }
    }

    void AudioSystem::setFilterParameter(size_t trackIndex, const std::string& filterName,
                                         int paramId, float value) {
        if (!m_isInitialized || trackIndex >= m_tracks.size())
            return;

        for (auto& instance : m_trackFilters) {
            if (instance.filter && instance.filter->getParamName(0) == filterName) {
                instance.parameters[paramId].value = value;
                instance.parameters[paramId].changed = true;
                instance.needsUpdate = true;
                break;
            }
        }
    }

    bool AudioSystem::isSupportedFileExtension(const std::string& ext) {
        static const std::vector<std::string> supportedExtensions = {".wav", ".ogg", ".mp3",
                                                                     ".flac"};
        return std::find(supportedExtensions.begin(), supportedExtensions.end(), ext) !=
               supportedExtensions.end();
    }

} // namespace AudioTester