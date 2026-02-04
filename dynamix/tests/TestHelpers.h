#pragma once

/**
 * TestHelpers - Common test utilities for Dynamix testing
 *
 * Provides:
 * - JSON helpers and comparison
 * - State comparison utilities
 * - File system helpers
 * - Test data generation
 */

#include <chrono>
#include <cmath>
#include <filesystem>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// Forward declarations from Dynamix
namespace Dynamix {
struct StateSnapshot;
struct EffectState;
struct MasterBusState;
struct TrackStateExtended;
} // namespace Dynamix

namespace DynamixTest {

// ============================================================================
// JSON Helpers
// ============================================================================

/**
 * Create a minimal valid song JSON
 * @param trackFiles List of track filenames
 * @return JSON string
 */
std::string createMinimalSongJson(const std::vector<std::string>& trackFiles);

/**
 * Create a song JSON with effects configured
 * @param trackFiles List of track filenames
 * @param effectConfigs Map of track index to effect configurations
 * @return JSON string
 */
std::string createSongJsonWithEffects(
    const std::vector<std::string>& trackFiles,
    const std::vector<std::map<std::string, std::map<std::string, float>>>& effectConfigs);

/**
 * Create a minimal valid master JSON
 * @return JSON string
 */
std::string createMinimalMasterJson();

/**
 * Create a master JSON with effects
 * @param effectConfigs Effect configurations
 * @return JSON string
 */
std::string createMasterJsonWithEffects(
    const std::map<std::string, std::map<std::string, float>>& effectConfigs);

/**
 * Check if two JSON objects are equivalent
 * Handles floating point comparison with tolerance
 * @param a First JSON
 * @param b Second JSON
 * @param tolerance Float comparison tolerance
 * @return true if equivalent
 */
bool jsonEquivalent(const nlohmann::json& a, const nlohmann::json& b, float tolerance = 0.001f);

/**
 * Parse JSON from file
 * @param path File path
 * @return Parsed JSON, or empty object on error
 */
nlohmann::json loadJsonFromFile(const std::filesystem::path& path);

/**
 * Write JSON to file
 * @param path File path
 * @param json JSON to write
 * @return true on success
 */
bool saveJsonToFile(const std::filesystem::path& path, const nlohmann::json& json);

// ============================================================================
// State Comparison
// ============================================================================

/**
 * Compare two StateSnapshots for equality
 * @param a First state
 * @param b Second state
 * @param tolerance Float comparison tolerance
 * @return true if equal within tolerance
 */
bool statesEqual(const Dynamix::StateSnapshot& a, const Dynamix::StateSnapshot& b,
                 float tolerance = 0.001f);

/**
 * Compare two EffectStates for equality
 */
bool effectsEqual(const Dynamix::EffectState& a, const Dynamix::EffectState& b,
                  float tolerance = 0.001f);

/**
 * Compare two MasterBusStates for equality
 */
bool masterStatesEqual(const Dynamix::MasterBusState& a, const Dynamix::MasterBusState& b,
                       float tolerance = 0.001f);

/**
 * Compare two TrackStateExtended for equality
 */
bool trackStatesEqual(const Dynamix::TrackStateExtended& a, const Dynamix::TrackStateExtended& b,
                      float tolerance = 0.001f);

// ============================================================================
// Test Data Paths
// ============================================================================

/**
 * Get the path to test fixtures directory
 * @return Path to fixtures directory
 */
std::filesystem::path getFixturesPath();

/**
 * Get the path to a specific test track
 * @param filename Track filename
 * @return Full path to test track
 */
std::filesystem::path getTestTrackPath(const std::string& filename);

/**
 * Get the path to a golden JSON file
 * @param filename JSON filename
 * @return Full path to golden JSON
 */
std::filesystem::path getGoldenJsonPath(const std::string& filename);

/**
 * Get the path to an invalid JSON file
 * @param filename JSON filename
 * @return Full path to invalid JSON
 */
std::filesystem::path getInvalidJsonPath(const std::string& filename);

// ============================================================================
// Test Assertions Helpers
// ============================================================================

/**
 * Check if a value is approximately equal to expected
 */
inline bool approxEqual(float a, float b, float tolerance = 0.001f) {
    return std::abs(a - b) <= tolerance;
}

/**
 * Check if a value is within a range
 */
inline bool inRange(float value, float min, float max) { return value >= min && value <= max; }

// ============================================================================
// Filter Test Helpers
// ============================================================================

/**
 * Get all available filter names
 */
std::vector<std::string> getAllFilterNames();

/**
 * Get parameter IDs for a filter
 */
std::vector<int> getFilterParameterIds(const std::string& filterName);

/**
 * Get test parameter values that should produce audible effects
 */
struct FilterTestParams {
    std::string filterName;
    std::map<int, float> params; // paramId -> value
    std::string description;
};

/**
 * Get standard test configurations for each filter
 */
std::vector<FilterTestParams> getFilterTestConfigs();

// ============================================================================
// Timing Helpers
// ============================================================================

/**
 * Simple timer for measuring test durations
 */
class TestTimer {
public:
    TestTimer();

    /**
     * Get elapsed time in milliseconds
     */
    double elapsedMs() const;

    /**
     * Get elapsed time in seconds
     */
    double elapsedSeconds() const;

    /**
     * Reset the timer
     */
    void reset();

private:
    std::chrono::high_resolution_clock::time_point m_start;
};

} // namespace DynamixTest
