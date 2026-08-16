#pragma once

#include "FilterManager.h"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Dynamix {

    /**
     * Comprehensive JSON validation for Dynamix audio files
     * 
     * Provides robust validation of _song.json and _master.json files with:
     * - Schema structure validation
     * - Parameter range validation
     * - Audio effect parameter validation
     * - File reference validation
     * - User-friendly error reporting
     */
    class JsonValidator {
    public:
        struct ValidationResult {
            bool isValid = false;
            std::vector<std::string> errors;
            std::vector<std::string> warnings;
            
            void addError(const std::string& error) {
                errors.push_back(error);
                isValid = false;
            }
            
            void addWarning(const std::string& warning) {
                warnings.push_back(warning);
            }
            
            std::string getErrorSummary() const {
                if (errors.empty() && warnings.empty()) {
                    return "No issues";
                }
                std::string summary;
                if (!errors.empty()) {
                    summary += "Validation errors (" + std::to_string(errors.size()) + "):\n";
                    for (size_t i = 0; i < errors.size(); ++i) {
                        summary += "  " + std::to_string(i + 1) + ". " + errors[i] + "\n";
                    }
                }
                if (!warnings.empty()) {
                    summary += "Validation warnings (" + std::to_string(warnings.size()) + "):\n";
                    for (size_t i = 0; i < warnings.size(); ++i) {
                        summary += "  " + std::to_string(i + 1) + ". " + warnings[i] + "\n";
                    }
                }
                return summary;
            }
        };

        // Accepted ranges, shared with the loaders so that what validates and
        // what loads agree.
        struct ValidationConstants {
            static constexpr float MIN_FADE_TIME = 0.0f;
            static constexpr float MAX_FADE_TIME = 60.0f;
            static constexpr float MIN_TEMPO = 0.1f;
            static constexpr float MAX_TEMPO = 4.0f;
            static constexpr float MIN_VOLUME = 0.0f;
            static constexpr float MAX_VOLUME = 2.0f;
            static const std::vector<std::string> SUPPORTED_AUDIO_EXTENSIONS;
        };

        JsonValidator();
        ~JsonValidator() = default;

        // Main validation entry points
        ValidationResult validateSongJson(const nlohmann::json& json, 
                                        const std::filesystem::path& songFolder = "");
        ValidationResult validateMasterJson(const nlohmann::json& json);

        // Schema validation
        ValidationResult validateSongSchema(const nlohmann::json& json);
        ValidationResult validateMasterSchema(const nlohmann::json& json);

        // Event validation
        ValidationResult validateSongEvent(const nlohmann::json& eventJson, 
                                         const std::filesystem::path& songFolder = "");
        ValidationResult validateMasterEvent(const nlohmann::json& eventJson);

        // State validation
        ValidationResult validateStateSnapshot(const nlohmann::json& stateJson,
                                             const std::filesystem::path& songFolder = "");
        ValidationResult validateMasterBusState(const nlohmann::json& stateJson);

        // Track validation
        ValidationResult validateTrack(const nlohmann::json& trackJson,
                                     const std::filesystem::path& songFolder = "");

        // Effect validation
        ValidationResult validateEffect(const std::string& effectName,
                                      const nlohmann::json& effectJson);

        // Utility validation
        bool isValidFileName(const std::string& filename) const;
        bool isValidAudioFile(const std::string& filename) const;
        bool fileExistsInFolder(const std::string& filename, 
                               const std::filesystem::path& folder) const;
        
        // Common validation patterns
        void validateCommonEventFields(const nlohmann::json& eventJson, 
                                     ValidationResult& result, const std::string& context);

    private:
        FilterManager m_filterManager;

        // Range validation helpers
        bool isValidFadeTime(float fadeTime) const;
        bool isValidTempo(float tempo) const;
        bool isValidVolume(float volume) const;
        
        // JSON type checking helpers
        bool requireField(const nlohmann::json& json, const std::string& field,
                         ValidationResult& result, const std::string& context = "") const;
        bool requireString(const nlohmann::json& json, const std::string& field,
                          ValidationResult& result, const std::string& context = "") const;
        bool requireNumber(const nlohmann::json& json, const std::string& field,
                          ValidationResult& result, const std::string& context = "") const;
        bool requireArray(const nlohmann::json& json, const std::string& field,
                         ValidationResult& result, const std::string& context = "") const;
        bool requireObject(const nlohmann::json& json, const std::string& field,
                          ValidationResult& result, const std::string& context = "") const;

    };

} // namespace Dynamix