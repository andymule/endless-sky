#include "AudioTestHarness.h"

#include "FilterManager.h"
#include "AudioStreamProcessor.h"
#include "SyncWav.h"
#include "soloud.h"
#include "soloud_bus.h"
#include "soloud_filter.h"
#include "soloud_wav.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <random>

namespace DynamixTest {

namespace {
// SoLoud's FILTERS_PER_STREAM: filter slots available on a voice.
constexpr int kMaxFilterSlots = 8;
} // namespace

// ============================================================================
// AudioTestHarness Implementation
// ============================================================================

AudioTestHarness::AudioTestHarness(int sampleRate, int channels, int bufferSize)
    : m_sampleRate(sampleRate), m_channels(channels), m_bufferSize(bufferSize) {
    m_soloud = std::make_unique<SoLoud::Soloud>();
    m_bus = std::make_unique<SoLoud::Bus>();
    m_filterManager = std::make_unique<Dynamix::FilterManager>();
    Dynamix::AudioStreamProcessor::setGlobalTempo(1.0f);
}

AudioTestHarness::~AudioTestHarness() {
    Dynamix::AudioStreamProcessor::setGlobalTempo(1.0f);
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
    m_soloud->setVolume(m_busHandle, m_busVolume);

    m_initialized = true;
    return true;
}

int AudioTestHarness::loadTrack(const std::filesystem::path& path) {
    if (!m_initialized) {
        return -1;
    }

    auto wav = std::unique_ptr<SoLoud::Wav>(new Dynamix::SyncWav());
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

    auto wav = std::unique_ptr<SoLoud::Wav>(new Dynamix::SyncWav());

    // SoLoud's Wav keeps sample data planar (all of channel 0, then all of
    // channel 1, ...) and loadRawWave16 counts total samples rather than
    // frames, so deinterleave into 16-bit planar data here.
    const size_t channels = static_cast<size_t>(m_channels);
    const size_t frames = std::min(numFrames, samples.size() / channels);
    if (frames == 0) {
        return -1;
    }

    std::vector<short> planarSamples(frames * channels);
    for (size_t frame = 0; frame < frames; ++frame) {
        for (size_t ch = 0; ch < channels; ++ch) {
            const float s = std::clamp(samples[frame * channels + ch], -1.0f, 1.0f);
            planarSamples[ch * frames + frame] = static_cast<short>(s * 32767.0f);
        }
    }

    auto result = wav->loadRawWave16(
        planarSamples.data(), static_cast<unsigned int>(planarSamples.size()),
        static_cast<float>(m_sampleRate), static_cast<unsigned int>(m_channels));

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

    // mix() processes audio through the entire pipeline: all tracks, all
    // effects, the bus, everything. It mixes via an internal scratch buffer
    // sized from the buffer size given to init(), so never ask for more frames
    // than that in a single call.
    const size_t chunkFrames = static_cast<size_t>(m_bufferSize);
    for (size_t frame = 0; frame < numSamples; frame += chunkFrames) {
        const size_t frames = std::min(chunkFrames, numSamples - frame);
        m_soloud->mix(output.data() + frame * static_cast<size_t>(m_channels),
                      static_cast<unsigned int>(frames));
    }

    return output;
}

std::vector<float> AudioTestHarness::processSeconds(float seconds) {
    size_t numSamples = static_cast<size_t>(seconds * static_cast<float>(m_sampleRate));
    return processAndCapture(numSamples);
}

SoLoud::Soloud& AudioTestHarness::getSoLoud() { return *m_soloud; }

size_t AudioTestHarness::getTrackCount() const { return m_tracks.size(); }

AudioTestHarness::FilterInstance* AudioTestHarness::ensureFilter(FilterSet& set,
                                                                 const std::string& filterName) {
    auto it = set.filters.find(filterName);
    if (it != set.filters.end()) {
        return &it->second;
    }

    const int slot = static_cast<int>(set.filters.size());
    if (slot >= kMaxFilterSlots) {
        return nullptr;
    }

    auto filter = m_filterManager->createFilter(filterName);
    if (!filter) {
        return nullptr;
    }

    FilterInstance instance;
    instance.filter = std::move(filter);
    instance.slot = slot;
    return &set.filters.emplace(filterName, std::move(instance)).first->second;
}

std::vector<float>
AudioTestHarness::buildParameterValues(const std::string& filterName,
                                       const std::map<int, float>& values) const {
    // FilterManager applies parameters as a dense vector in parameter-id order,
    // so fill in defaults for whatever the caller has not set.
    std::vector<float> dense;
    for (int paramId = 0;; ++paramId) {
        const std::string paramName = m_filterManager->getParameterName(filterName, paramId);
        if (paramName.empty()) {
            break;
        }

        const auto it = values.find(paramId);
        dense.push_back(it != values.end()
                            ? it->second
                            : m_filterManager->getParameterDefault(filterName, paramName));
    }
    return dense;
}

void AudioTestHarness::pushParametersToVoice(const FilterSet& set,
                                             unsigned int voiceHandle) const {
    if (voiceHandle == 0) {
        return;
    }

    for (const auto& [filterName, instance] : set.filters) {
        for (const auto& [paramId, value] : instance.parameters) {
            m_soloud->setFilterParameter(voiceHandle, static_cast<unsigned int>(instance.slot),
                                         static_cast<unsigned int>(paramId), value);
        }
    }
}

void AudioTestHarness::setFilterParameter(size_t trackIndex, const std::string& filterName,
                                          int paramId, float value) {
    if (!m_initialized || trackIndex >= m_tracks.size()) {
        return;
    }

    FilterInstance* instance = ensureFilter(m_trackFilters[trackIndex], filterName);
    if (!instance) {
        return;
    }

    instance->parameters[paramId] = value;

    // Each filter instance is seeded from the filter object when a voice starts,
    // so the object has to carry the whole parameter set, not just this one.
    const auto values = buildParameterValues(filterName, instance->parameters);
    m_filterManager->applyAllParameters(instance->filter.get(), filterName, values);

    if (!instance->attached) {
        m_tracks[trackIndex]->setFilter(static_cast<unsigned int>(instance->slot),
                                       instance->filter.get());
        instance->attached = true;

        // A voice only creates filter instances when it starts, so an already
        // playing track has to be restarted to pick the new filter up.
        if (m_trackHandles[trackIndex] != 0) {
            m_soloud->stop(m_trackHandles[trackIndex]);
            m_trackHandles[trackIndex] = m_bus->play(*m_tracks[trackIndex]);
            pushParametersToVoice(m_trackFilters[trackIndex], m_trackHandles[trackIndex]);
            return;
        }
    }

    if (m_trackHandles[trackIndex] != 0) {
        m_soloud->setFilterParameter(m_trackHandles[trackIndex],
                                     static_cast<unsigned int>(instance->slot),
                                     static_cast<unsigned int>(paramId), value);
    }
}

float AudioTestHarness::getFilterParameter(size_t trackIndex, const std::string& filterName,
                                           int paramId) const {
    if (trackIndex >= m_trackFilters.size()) {
        return 0.0f;
    }

    auto filterIt = m_trackFilters[trackIndex].filters.find(filterName);
    if (filterIt == m_trackFilters[trackIndex].filters.end()) {
        return 0.0f;
    }

    const auto& parameters = filterIt->second.parameters;
    auto paramIt = parameters.find(paramId);
    if (paramIt == parameters.end()) {
        return 0.0f;
    }

    return paramIt->second;
}

void AudioTestHarness::setBusFilterParameter(const std::string& filterName, int paramId,
                                             float value) {
    if (!m_initialized) {
        return;
    }

    FilterInstance* instance = ensureFilter(m_busFilters, filterName);
    if (!instance) {
        return;
    }

    instance->parameters[paramId] = value;

    const auto values = buildParameterValues(filterName, instance->parameters);
    m_filterManager->applyAllParameters(instance->filter.get(), filterName, values);

    if (!instance->attached) {
        m_bus->setFilter(static_cast<unsigned int>(instance->slot), instance->filter.get());
        instance->attached = true;

        // Restarting the bus voice so it picks up the filter also drops the
        // track voices playing through it, so bring those back afterwards.
        stopAllTracks();
        m_soloud->stop(m_busHandle);
        m_busHandle = m_soloud->play(*m_bus);
        m_soloud->setVolume(m_busHandle, m_busVolume);
        pushParametersToVoice(m_busFilters, m_busHandle);

        playAllTracks();
        for (size_t i = 0; i < m_trackFilters.size(); ++i) {
            pushParametersToVoice(m_trackFilters[i], m_trackHandles[i]);
        }
        return;
    }

    m_soloud->setFilterParameter(m_busHandle, static_cast<unsigned int>(instance->slot),
                                 static_cast<unsigned int>(paramId), value);
}

float AudioTestHarness::getBusFilterParameter(const std::string& filterName, int paramId) const {
    auto filterIt = m_busFilters.filters.find(filterName);
    if (filterIt == m_busFilters.filters.end()) {
        return 0.0f;
    }

    const auto& parameters = filterIt->second.parameters;
    auto paramIt = parameters.find(paramId);
    if (paramIt == parameters.end()) {
        return 0.0f;
    }

    return paramIt->second;
}

void AudioTestHarness::setGranularTempo(float tempo) {
    Dynamix::AudioStreamProcessor::setGlobalTempo(tempo);
}

float AudioTestHarness::getGranularTempo() const {
    return Dynamix::AudioStreamProcessor::getGlobalTempo();
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

    m_busVolume = volume;
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
    std::filesystem::path songDir = m_path / songName;
    std::filesystem::create_directories(songDir);

    std::filesystem::path jsonPath = songDir / "_song.json";
    std::ofstream file(jsonPath);
    file << jsonContent;
    file.close();

    // JsonValidator rejects missing audio files. Unit tests that only check JSON
    // loading get empty placeholders; audio tests overwrite them with real OGGs.
    try {
        const auto json = nlohmann::json::parse(jsonContent);
        if (!json.contains("events") || !json["events"].is_array()) {
            return;
        }
        for (const auto& event : json["events"]) {
            if (!event.contains("state") || !event["state"].contains("tracks") ||
                !event["state"]["tracks"].is_array()) {
                continue;
            }
            for (const auto& track : event["state"]["tracks"]) {
                if (!track.contains("file") || !track["file"].is_string()) {
                    continue;
                }
                const auto filePath = songDir / track["file"].get<std::string>();
                if (!std::filesystem::exists(filePath)) {
                    std::ofstream dummy(filePath);
                }
            }
        }
    } catch (...) {
        // Invalid JSON is a valid test input; skip placeholder files.
    }
}

void TempTestDirectory::writeMasterJson(const std::string& jsonContent) {
    std::filesystem::path jsonPath = m_path / "_master.json";
    std::ofstream file(jsonPath);
    file << jsonContent;
}

} // namespace DynamixTest
