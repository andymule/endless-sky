#include "TestHelpers.h"

#include "AudioState.h"

#include <chrono>
#include <fstream>

namespace DynamixTest {

// ============================================================================
// JSON Helpers
// ============================================================================

std::string createMinimalSongJson(const std::vector<std::string>& trackFiles) {
    nlohmann::json json;
    json["events"] = nlohmann::json::array();

    nlohmann::json defaultEvent;
    defaultEvent["name"] = "Default";
    defaultEvent["fadeTime"] = 1.0f;
    defaultEvent["state"]["masterTempo"] = 1.0f;
    defaultEvent["state"]["granularTempo"] = 1.0f;
    defaultEvent["state"]["tracks"] = nlohmann::json::array();

    for (const auto& file : trackFiles) {
        nlohmann::json track;
        track["file"] = file;
        track["volume"] = 1.0f;
        track["effects"] = nlohmann::json::object();
        defaultEvent["state"]["tracks"].push_back(track);
    }

    json["events"].push_back(defaultEvent);
    return json.dump(2);
}

std::string createSongJsonWithEffects(
    const std::vector<std::string>& trackFiles,
    const std::vector<std::map<std::string, std::map<std::string, float>>>& effectConfigs) {
    nlohmann::json json;
    json["events"] = nlohmann::json::array();

    nlohmann::json defaultEvent;
    defaultEvent["name"] = "WithEffects";
    defaultEvent["fadeTime"] = 1.0f;
    defaultEvent["state"]["masterTempo"] = 1.0f;
    defaultEvent["state"]["granularTempo"] = 1.0f;
    defaultEvent["state"]["tracks"] = nlohmann::json::array();

    for (size_t i = 0; i < trackFiles.size(); ++i) {
        nlohmann::json track;
        track["file"] = trackFiles[i];
        track["volume"] = 1.0f;
        track["effects"] = nlohmann::json::object();

        if (i < effectConfigs.size()) {
            for (const auto& [effectName, params] : effectConfigs[i]) {
                nlohmann::json effect;
                effect["parameters"] = nlohmann::json::object();
                for (const auto& [paramId, value] : params) {
                    effect["parameters"][paramId] = value;
                }
                track["effects"][effectName] = effect;
            }
        }

        defaultEvent["state"]["tracks"].push_back(track);
    }

    json["events"].push_back(defaultEvent);
    return json.dump(2);
}

std::string createMinimalMasterJson() {
    nlohmann::json json;
    json["name"] = "Master Bus";
    json["events"] = nlohmann::json::array();

    nlohmann::json defaultEvent;
    defaultEvent["name"] = "Normal";
    defaultEvent["fadeTime"] = 0.0f;
    defaultEvent["state"]["masterTempo"] = 1.0f;
    defaultEvent["state"]["granularTempo"] = 1.0f;
    defaultEvent["state"]["bus"]["volume"] = 1.0f;
    defaultEvent["state"]["bus"]["effects"] = nlohmann::json::object();

    json["events"].push_back(defaultEvent);
    return json.dump(2);
}

std::string createMasterJsonWithEffects(
    const std::map<std::string, std::map<std::string, float>>& effectConfigs) {
    nlohmann::json json;
    json["name"] = "Master Bus";
    json["events"] = nlohmann::json::array();

    nlohmann::json defaultEvent;
    defaultEvent["name"] = "WithEffects";
    defaultEvent["fadeTime"] = 1.0f;
    defaultEvent["state"]["masterTempo"] = 1.0f;
    defaultEvent["state"]["granularTempo"] = 1.0f;
    defaultEvent["state"]["bus"]["volume"] = 1.0f;
    defaultEvent["state"]["bus"]["effects"] = nlohmann::json::object();

    for (const auto& [effectName, params] : effectConfigs) {
        nlohmann::json effect;
        effect["parameters"] = nlohmann::json::object();
        for (const auto& [paramId, value] : params) {
            effect["parameters"][paramId] = value;
        }
        defaultEvent["state"]["bus"]["effects"][effectName] = effect;
    }

    json["events"].push_back(defaultEvent);
    return json.dump(2);
}

bool jsonEquivalent(const nlohmann::json& a, const nlohmann::json& b, float tolerance) {
    if (a.type() != b.type()) {
        return false;
    }

    if (a.is_number_float()) {
        return approxEqual(a.get<float>(), b.get<float>(), tolerance);
    }

    if (a.is_object()) {
        if (a.size() != b.size()) {
            return false;
        }
        for (auto it = a.begin(); it != a.end(); ++it) {
            if (!b.contains(it.key())) {
                return false;
            }
            if (!jsonEquivalent(it.value(), b[it.key()], tolerance)) {
                return false;
            }
        }
        return true;
    }

    if (a.is_array()) {
        if (a.size() != b.size()) {
            return false;
        }
        for (size_t i = 0; i < a.size(); ++i) {
            if (!jsonEquivalent(a[i], b[i], tolerance)) {
                return false;
            }
        }
        return true;
    }

    return a == b;
}

nlohmann::json loadJsonFromFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return nlohmann::json();
    }

    try {
        nlohmann::json json;
        file >> json;
        return json;
    } catch (...) {
        return nlohmann::json();
    }
}

bool saveJsonToFile(const std::filesystem::path& path, const nlohmann::json& json) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    file << json.dump(2);
    return true;
}

// ============================================================================
// State Comparison
// ============================================================================

bool statesEqual(const Dynamix::StateSnapshot& a, const Dynamix::StateSnapshot& b,
                 float tolerance) {
    if (!approxEqual(a.masterTempo, b.masterTempo, tolerance)) {
        return false;
    }
    if (!approxEqual(a.granularTempo, b.granularTempo, tolerance)) {
        return false;
    }
    if (a.tracks.size() != b.tracks.size()) {
        return false;
    }

    for (size_t i = 0; i < a.tracks.size(); ++i) {
        if (!trackStatesEqual(a.tracks[i], b.tracks[i], tolerance)) {
            return false;
        }
    }

    return true;
}

bool effectsEqual(const Dynamix::EffectState& a, const Dynamix::EffectState& b, float tolerance) {
    if (a.parameters.size() != b.parameters.size()) {
        return false;
    }

    for (const auto& [paramId, value] : a.parameters) {
        auto it = b.parameters.find(paramId);
        if (it == b.parameters.end()) {
            return false;
        }
        if (!approxEqual(value, it->second, tolerance)) {
            return false;
        }
    }

    return true;
}

bool masterStatesEqual(const Dynamix::MasterBusState& a, const Dynamix::MasterBusState& b,
                       float tolerance) {
    if (!approxEqual(a.masterTempo, b.masterTempo, tolerance)) {
        return false;
    }
    if (!approxEqual(a.granularTempo, b.granularTempo, tolerance)) {
        return false;
    }
    if (!approxEqual(a.volume, b.volume, tolerance)) {
        return false;
    }

    if (a.effects.size() != b.effects.size()) {
        return false;
    }

    for (const auto& [effectName, effectState] : a.effects) {
        auto it = b.effects.find(effectName);
        if (it == b.effects.end()) {
            return false;
        }
        if (!effectsEqual(effectState, it->second, tolerance)) {
            return false;
        }
    }

    return true;
}

bool trackStatesEqual(const Dynamix::TrackStateExtended& a, const Dynamix::TrackStateExtended& b,
                      float tolerance) {
    if (a.file != b.file) {
        return false;
    }
    if (!approxEqual(a.volume, b.volume, tolerance)) {
        return false;
    }

    if (a.effects.size() != b.effects.size()) {
        return false;
    }

    for (const auto& [effectName, effectState] : a.effects) {
        auto it = b.effects.find(effectName);
        if (it == b.effects.end()) {
            return false;
        }
        if (!effectsEqual(effectState, it->second, tolerance)) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// Test Data Paths
// ============================================================================

std::filesystem::path getFixturesPath() {
    // In build directory, fixtures are copied to the executable location
    return std::filesystem::path("fixtures");
}

std::filesystem::path getTestTrackPath(const std::string& filename) {
    return getFixturesPath() / "test_tracks" / filename;
}

std::filesystem::path getGoldenJsonPath(const std::string& filename) {
    return getFixturesPath() / "golden" / filename;
}

std::filesystem::path getInvalidJsonPath(const std::string& filename) {
    return getFixturesPath() / "invalid" / filename;
}

// ============================================================================
// Filter Test Helpers
// ============================================================================

std::vector<std::string> getAllFilterNames() {
    return {"echo", "freeverb", "lofi", "flanger", "waveshaper", "robotize", "biquad"};
}

std::vector<int> getFilterParameterIds(const std::string& filterName) {
    if (filterName == "echo") {
        return {0, 1, 2}; // wet, delay, decay
    } else if (filterName == "freeverb") {
        return {0, 1, 2, 3, 4}; // wet, freeze, roomSize, damp, width
    } else if (filterName == "lofi") {
        return {0, 1, 2}; // wet, sampleRate, bitDepth
    } else if (filterName == "flanger") {
        return {0, 1, 2}; // wet, delay, freq
    } else if (filterName == "waveshaper") {
        return {0, 1}; // wet, amount
    } else if (filterName == "robotize") {
        return {0, 1, 2}; // wet, freq, waveform
    } else if (filterName == "biquad") {
        return {0, 1, 2, 3}; // wet, type, freq, resonance
    }
    return {};
}

std::vector<FilterTestParams> getFilterTestConfigs() {
    return {
        {"echo", {{0, 1.0f}, {1, 0.3f}, {2, 0.5f}}, "Echo with 300ms delay"},
        {"freeverb", {{0, 0.8f}, {2, 0.8f}, {3, 0.5f}}, "Large reverb room"},
        {"lofi", {{0, 1.0f}, {1, 4000.0f}, {2, 4.0f}}, "Aggressive bitcrushing"},
        {"flanger", {{0, 0.8f}, {1, 0.005f}, {2, 2.0f}}, "Moderate flanger"},
        {"waveshaper", {{0, 0.8f}, {1, 0.5f}}, "Medium distortion"},
        {"robotize", {{0, 0.8f}, {1, 5.0f}, {2, 0}}, "Robot voice effect"},
        {"biquad", {{0, 1.0f}, {1, 0}, {2, 500.0f}, {3, 2.0f}}, "Low-pass filter at 500Hz"},
    };
}

// ============================================================================
// Timing Helpers
// ============================================================================

TestTimer::TestTimer() : m_start(std::chrono::high_resolution_clock::now()) {}

double TestTimer::elapsedMs() const {
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(now - m_start).count();
}

double TestTimer::elapsedSeconds() const { return elapsedMs() / 1000.0; }

void TestTimer::reset() { m_start = std::chrono::high_resolution_clock::now(); }

} // namespace DynamixTest
