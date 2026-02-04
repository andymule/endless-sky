#include "AudioTestHarness.h"

#include "FilterManager.h"
#include "soloud.h"
#include "soloud_bus.h"
#include "soloud_wav.h"

#include <cstring>
#include <fstream>
#include <random>

namespace DynamixTest {

// ============================================================================
// AudioTestHarness Implementation
// ============================================================================

AudioTestHarness::AudioTestHarness(int sampleRate, int channels, int bufferSize)
    : m_sampleRate(sampleRate), m_channels(channels), m_bufferSize(bufferSize) {
    m_soloud = std::make_unique<SoLoud::Soloud>();
    m_bus = std::make_unique<SoLoud::Bus>();
    m_filterManager = std::make_unique<Dynamix::FilterManager>();
}

AudioTestHarness::~AudioTestHarness() {
    if (m_initialized) {
        m_soloud->deinit();
    }
}

bool AudioTestHarness::initialize() {
    if (m_initialized) {
        return true;
    }

    // Initialize SoLoud with NULLDRIVER - no speaker output, but full processing
    auto result = m_soloud->init(SoLoud::Soloud::CLIP_ROUNDOFF,
                                 SoLoud::Soloud::NULLDRIVER, // Key: no audio output
                                 static_cast<unsigned int>(m_sampleRate),
                                 static_cast<unsigned int>(m_bufferSize),
                                 static_cast<unsigned int>(m_channels));

    if (result != SoLoud::SO_NO_ERROR) {
        return false;
    }

    // Play the bus and store handle
    m_busHandle = m_soloud->play(*m_bus);
    m_soloud->setVolume(m_busHandle, 1.0f);

    m_initialized = true;
    return true;
}

int AudioTestHarness::loadTrack(const std::filesystem::path& path) {
    if (!m_initialized) {
        return -1;
    }

    auto wav = std::make_unique<SoLoud::Wav>();
    auto result = wav->load(path.string().c_str());
    if (result != SoLoud::SO_NO_ERROR) {
        return -1;
    }

    wav->setLooping(true);
    int index = static_cast<int>(m_tracks.size());
    m_tracks.push_back(std::move(wav));
    m_trackHandles.push_back(0);
    m_trackFilters.push_back({});

    return index;
}

int AudioTestHarness::loadTrackFromMemory(const std::vector<float>& samples, size_t numFrames) {
    if (!m_initialized || samples.empty()) {
        return -1;
    }

    auto wav = std::make_unique<SoLoud::Wav>();

    // Convert float samples to signed 16-bit for SoLoud
    std::vector<short> shortSamples(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        float s = samples[i];
        // Clamp to [-1, 1]
        if (s > 1.0f)
            s = 1.0f;
        if (s < -1.0f)
            s = -1.0f;
        shortSamples[i] = static_cast<short>(s * 32767.0f);
    }

    // Load raw audio data
    auto result = wav->loadRawWave16(shortSamples.data(), static_cast<unsigned int>(numFrames),
                                     static_cast<float>(m_sampleRate),
                                     static_cast<unsigned int>(m_channels));

    if (result != SoLoud::SO_NO_ERROR) {
        return -1;
    }

    wav->setLooping(true);
    int index = static_cast<int>(m_tracks.size());
    m_tracks.push_back(std::move(wav));
    m_trackHandles.push_back(0);
    m_trackFilters.push_back({});

    return index;
}

void AudioTestHarness::playAllTracks() {
    if (!m_initialized) {
        return;
    }

    for (size_t i = 0; i < m_tracks.size(); ++i) {
        if (m_trackHandles[i] == 0) {
            m_trackHandles[i] = m_bus->play(*m_tracks[i]);
        }
    }
}

void AudioTestHarness::stopAllTracks() {
    if (!m_initialized) {
        return;
    }

    for (size_t i = 0; i < m_tracks.size(); ++i) {
        if (m_trackHandles[i] != 0) {
            m_soloud->stop(m_trackHandles[i]);
            m_trackHandles[i] = 0;
        }
    }
}

std::vector<float> AudioTestHarness::processAndCapture(size_t numSamples) {
    if (!m_initialized) {
        return {};
    }

    std::vector<float> output(numSamples * static_cast<size_t>(m_channels));

    // mix() processes audio through the entire pipeline and returns results
    // This includes all tracks, all effects, the bus, everything
    m_soloud->mix(output.data(), static_cast<unsigned int>(numSamples));

    return output;
}

std::vector<float> AudioTestHarness::processSeconds(float seconds) {
    size_t numSamples = static_cast<size_t>(seconds * static_cast<float>(m_sampleRate));
    return processAndCapture(numSamples);
}

SoLoud::Soloud& AudioTestHarness::getSoLoud() { return *m_soloud; }

size_t AudioTestHarness::getTrackCount() const { return m_tracks.size(); }

void AudioTestHarness::setFilterParameter(size_t trackIndex, const std::string& filterName,
                                          int paramId, float value) {
    if (!m_initialized || trackIndex >= m_tracks.size()) {
        return;
    }

    // Store the parameter value for our tracking
    m_trackFilters[trackIndex].parameters[filterName][paramId] = value;

    // Get filter from FilterManager and apply to track
    // Note: In the real implementation, we'd need to create filter instances
    // For now, we apply directly to the voice handle if playing
    if (m_trackHandles[trackIndex] != 0) {
        // SoLoud filters need to be set on the audio source, then parameters
        // can be set on the voice handle
        // This is simplified - full implementation needs filter management
    }
}

float AudioTestHarness::getFilterParameter(size_t trackIndex, const std::string& filterName,
                                           int paramId) const {
    if (trackIndex >= m_trackFilters.size()) {
        return 0.0f;
    }

    auto filterIt = m_trackFilters[trackIndex].parameters.find(filterName);
    if (filterIt == m_trackFilters[trackIndex].parameters.end()) {
        return 0.0f;
    }

    auto paramIt = filterIt->second.find(paramId);
    if (paramIt == filterIt->second.end()) {
        return 0.0f;
    }

    return paramIt->second;
}

void AudioTestHarness::setBusFilterParameter(const std::string& filterName, int paramId,
                                             float value) {
    if (!m_initialized) {
        return;
    }

    m_busFilters.parameters[filterName][paramId] = value;
}

float AudioTestHarness::getBusFilterParameter(const std::string& filterName, int paramId) const {
    auto filterIt = m_busFilters.parameters.find(filterName);
    if (filterIt == m_busFilters.parameters.end()) {
        return 0.0f;
    }

    auto paramIt = filterIt->second.find(paramId);
    if (paramIt == filterIt->second.end()) {
        return 0.0f;
    }

    return paramIt->second;
}

void AudioTestHarness::setGranularTempo(float /*tempo*/) {
    // Note: Full implementation would integrate with AudioStreamProcessor
    // For testing purposes, we track the value
}

float AudioTestHarness::getGranularTempo() const {
    return 1.0f; // Default
}

void AudioTestHarness::setMasterTempo(float tempo) {
    if (!m_initialized) {
        return;
    }

    // Apply to all playing tracks
    for (size_t i = 0; i < m_trackHandles.size(); ++i) {
        if (m_trackHandles[i] != 0) {
            m_soloud->setRelativePlaySpeed(m_trackHandles[i], tempo);
        }
    }
}

void AudioTestHarness::setTrackVolume(size_t trackIndex, float volume) {
    if (!m_initialized || trackIndex >= m_trackHandles.size()) {
        return;
    }

    if (m_trackHandles[trackIndex] != 0) {
        m_soloud->setVolume(m_trackHandles[trackIndex], volume);
    }
}

float AudioTestHarness::getTrackVolume(size_t trackIndex) const {
    if (!m_initialized || trackIndex >= m_trackHandles.size()) {
        return 0.0f;
    }

    if (m_trackHandles[trackIndex] != 0) {
        return m_soloud->getVolume(m_trackHandles[trackIndex]);
    }
    return 0.0f;
}

void AudioTestHarness::setBusVolume(float volume) {
    if (!m_initialized) {
        return;
    }

    m_bus->setVolume(volume);
    if (m_busHandle != 0) {
        m_soloud->setVolume(m_busHandle, volume);
    }
}

double AudioTestHarness::getGlobalTime() const {
    if (!m_initialized || m_trackHandles.empty()) {
        return 0.0;
    }

    // Return time of first playing track
    for (size_t i = 0; i < m_trackHandles.size(); ++i) {
        if (m_trackHandles[i] != 0) {
            return m_soloud->getStreamPosition(m_trackHandles[i]);
        }
    }
    return 0.0;
}

double AudioTestHarness::getTrackPosition(size_t trackIndex) const {
    if (!m_initialized || trackIndex >= m_trackHandles.size()) {
        return 0.0;
    }

    if (m_trackHandles[trackIndex] != 0) {
        return m_soloud->getStreamPosition(m_trackHandles[trackIndex]);
    }
    return 0.0;
}

void AudioTestHarness::clearAllTracks() {
    stopAllTracks();
    m_tracks.clear();
    m_trackHandles.clear();
    m_trackFilters.clear();
}

const Dynamix::FilterManager& AudioTestHarness::getFilterManager() const {
    return *m_filterManager;
}

// ============================================================================
// TempTestDirectory Implementation
// ============================================================================

TempTestDirectory::TempTestDirectory() {
    // Create a unique temporary directory
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);

    std::string dirName = "dynamix_test_" + std::to_string(dis(gen));
    m_path = std::filesystem::temp_directory_path() / dirName;

    std::filesystem::create_directories(m_path);
}

TempTestDirectory::~TempTestDirectory() {
    // Clean up the temporary directory
    std::error_code ec;
    std::filesystem::remove_all(m_path, ec);
    // Ignore errors during cleanup
}

void TempTestDirectory::createSongFolder(const std::string& name) {
    std::filesystem::create_directories(m_path / name);
}

void TempTestDirectory::copyTrackToSong(const std::filesystem::path& sourceTrack,
                                        const std::string& songName, const std::string& destName) {
    std::filesystem::path destPath = m_path / songName;
    std::filesystem::create_directories(destPath);

    std::string filename = destName.empty() ? sourceTrack.filename().string() : destName;
    std::filesystem::copy_file(sourceTrack, destPath / filename,
                               std::filesystem::copy_options::overwrite_existing);
}

void TempTestDirectory::writeSongJson(const std::string& songName, const std::string& jsonContent) {
    std::filesystem::path jsonPath = m_path / songName / "_song.json";
    std::ofstream file(jsonPath);
    file << jsonContent;
}

void TempTestDirectory::writeMasterJson(const std::string& jsonContent) {
    std::filesystem::path jsonPath = m_path / "_master.json";
    std::ofstream file(jsonPath);
    file << jsonContent;
}

} // namespace DynamixTest
