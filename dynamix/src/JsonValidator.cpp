#include "JsonValidator.h"
#include "Logger.h"
#include <algorithm>
#include <filesystem>

namespace Dynamix {

    const std::vector<std::string> JsonValidator::ValidationConstants::SUPPORTED_AUDIO_EXTENSIONS = {
        ".ogg", ".wav", ".aif", ".aiff"
    };

    JsonValidator::JsonValidator() {
        // FilterManager is initialized by default constructor
    }

    JsonValidator::ValidationResult JsonValidator::validateSongJson(const nlohmann::json& json,
                                                                   const std::filesystem::path& songFolder) {
        ValidationResult result;
        result.isValid = true;

        // First validate schema structure
        auto schemaResult = validateSongSchema(json);
        result.errors.insert(result.errors.end(), schemaResult.errors.begin(), schemaResult.errors.end());
        result.warnings.insert(result.warnings.end(), schemaResult.warnings.begin(), schemaResult.warnings.end());
        
        if (!schemaResult.isValid) {
            result.isValid = false;
            return result; // Don't proceed with content validation if schema is invalid
        }

        // Validate each event in detail
        if (json.contains("events") && json["events"].is_array()) {
            const auto& events = json["events"];
            
            for (size_t i = 0; i < events.size(); ++i) {
                auto eventResult = validateSongEvent(events[i], songFolder);
                
                // Prefix errors with event context
                for (const auto& error : eventResult.errors) {
                    result.addError("Event " + std::to_string(i + 1) + ": " + error);
                }
                for (const auto& warning : eventResult.warnings) {
                    result.addWarning("Event " + std::to_string(i + 1) + ": " + warning);
                }
                
                if (!eventResult.isValid) {
                    result.isValid = false;
                }
            }
        }

        return result;
    }

    JsonValidator::ValidationResult JsonValidator::validateMasterJson(const nlohmann::json& json) {
        ValidationResult result;
        result.isValid = true;

        // First validate schema structure  
        auto schemaResult = validateMasterSchema(json);
        result.errors.insert(result.errors.end(), schemaResult.errors.begin(), schemaResult.errors.end());
        result.warnings.insert(result.warnings.end(), schemaResult.warnings.begin(), schemaResult.warnings.end());
        
        if (!schemaResult.isValid) {
            result.isValid = false;
            return result;
        }

        // Validate each event in detail
        if (json.contains("events") && json["events"].is_array()) {
            const auto& events = json["events"];
            
            for (size_t i = 0; i < events.size(); ++i) {
                auto eventResult = validateMasterEvent(events[i]);
                
                // Prefix errors with event context
                for (const auto& error : eventResult.errors) {
                    result.addError("Master Event " + std::to_string(i + 1) + ": " + error);
                }
                for (const auto& warning : eventResult.warnings) {
                    result.addWarning("Master Event " + std::to_string(i + 1) + ": " + warning);
                }
                
                if (!eventResult.isValid) {
                    result.isValid = false;
                }
            }
        }

        return result;
    }

    JsonValidator::ValidationResult JsonValidator::validateSongSchema(const nlohmann::json& json) {
        ValidationResult result;
        result.isValid = true;

        // Root must be object
        if (!json.is_object()) {
            result.addError("Root must be a JSON object");
            return result;
        }

        // Must have a non-empty events array
        if (!requireArray(json, "events", result, "root")) {
            return result;
        }
        if (json["events"].empty()) {
            result.addWarning("events array is empty; a Default event will be used");
        }

        // Optional name field should be string if present
        if (json.contains("name") && !json["name"].is_string()) {
            result.addWarning("'name' field should be a string if present");
        }

        return result;
    }

    JsonValidator::ValidationResult JsonValidator::validateMasterSchema(const nlohmann::json& json) {
        ValidationResult result;
        result.isValid = true;

        // Root must be object
        if (!json.is_object()) {
            result.addError("Root must be a JSON object");
            return result;
        }

        // Must have a non-empty events array
        if (!requireArray(json, "events", result, "root")) {
            return result;
        }
        if (json["events"].empty()) {
            result.addWarning("events array is empty; a Default event will be used");
        }

        // Optional name field should be string if present  
        if (json.contains("name") && !json["name"].is_string()) {
            result.addWarning("'name' field should be a string if present");
        }

        return result;
    }

    void JsonValidator::validateCommonEventFields(const nlohmann::json& eventJson, 
                                                 ValidationResult& result, const std::string& context) {
        // Event must be object
        if (!eventJson.is_object()) {
            result.addError("Event must be a JSON object");
            return;
        }

        // Require name (string)
        if (!requireString(eventJson, "name", result, context)) {
            return;
        }

        // Require fadeTime (number)
        if (!requireNumber(eventJson, "fadeTime", result, context)) {
            return;
        }

        // Validate fadeTime range
        float fadeTime = eventJson["fadeTime"].get<float>();
        if (!isValidFadeTime(fadeTime)) {
            result.addError("fadeTime must be between " + std::to_string(ValidationConstants::MIN_FADE_TIME) + " and " + std::to_string(ValidationConstants::MAX_FADE_TIME) + " seconds");
        }
    }

    JsonValidator::ValidationResult JsonValidator::validateSongEvent(const nlohmann::json& eventJson,
                                                                    const std::filesystem::path& songFolder) {
        ValidationResult result;
        result.isValid = true;

        // Validate common event fields
        validateCommonEventFields(eventJson, result, "event");
        if (!result.isValid) return result;

        // Validate state if present
        if (eventJson.contains("state")) {
            auto stateResult = validateStateSnapshot(eventJson["state"], songFolder);
            result.errors.insert(result.errors.end(), stateResult.errors.begin(), stateResult.errors.end());
            result.warnings.insert(result.warnings.end(), stateResult.warnings.begin(), stateResult.warnings.end());
            
            if (!stateResult.isValid) {
                result.isValid = false;
            }
        }

        return result;
    }

    JsonValidator::ValidationResult JsonValidator::validateMasterEvent(const nlohmann::json& eventJson) {
        ValidationResult result;
        result.isValid = true;

        // Validate common event fields
        validateCommonEventFields(eventJson, result, "master event");
        if (!result.isValid) return result;

        // Validate state if present
        if (eventJson.contains("state")) {
            auto stateResult = validateMasterBusState(eventJson["state"]);
            result.errors.insert(result.errors.end(), stateResult.errors.begin(), stateResult.errors.end());
            result.warnings.insert(result.warnings.end(), stateResult.warnings.begin(), stateResult.warnings.end());
            
            if (!stateResult.isValid) {
                result.isValid = false;
            }
        }

        return result;
    }

    JsonValidator::ValidationResult JsonValidator::validateStateSnapshot(const nlohmann::json& stateJson,
                                                                        const std::filesystem::path& songFolder) {
        ValidationResult result;
        result.isValid = true;

        // State must be object
        if (!stateJson.is_object()) {
            result.addError("State must be a JSON object");
            return result;
        }

        // Validate tempo fields if present
        if (stateJson.contains("masterTempo")) {
            if (!stateJson["masterTempo"].is_number()) {
                result.addError("masterTempo must be a number");
            } else {
                float tempo = stateJson["masterTempo"].get<float>();
                if (!isValidTempo(tempo)) {
                    result.addError("masterTempo must be between " + std::to_string(ValidationConstants::MIN_TEMPO) + " and " + std::to_string(ValidationConstants::MAX_TEMPO));
                }
            }
        }

        if (stateJson.contains("granularTempo")) {
            if (!stateJson["granularTempo"].is_number()) {
                result.addError("granularTempo must be a number");
            } else {
                float tempo = stateJson["granularTempo"].get<float>();
                if (!isValidTempo(tempo)) {
                    result.addError("granularTempo must be between " + std::to_string(ValidationConstants::MIN_TEMPO) + " and " + std::to_string(ValidationConstants::MAX_TEMPO));
                }
            }
        }

        // Validate tracks array if present
        if (stateJson.contains("tracks")) {
            if (!stateJson["tracks"].is_array()) {
                result.addError("tracks must be an array");
            } else {
                const auto& tracks = stateJson["tracks"];
                
                for (size_t i = 0; i < tracks.size(); ++i) {
                    auto trackResult = validateTrack(tracks[i], songFolder);
                    
                    // Prefix errors with track context
                    for (const auto& error : trackResult.errors) {
                        result.addError("Track " + std::to_string(i + 1) + ": " + error);
                    }
                    for (const auto& warning : trackResult.warnings) {
                        result.addWarning("Track " + std::to_string(i + 1) + ": " + warning);
                    }
                    
                    if (!trackResult.isValid) {
                        result.isValid = false;
                    }
                }
            }
        }

        return result;
    }

    JsonValidator::ValidationResult JsonValidator::validateMasterBusState(const nlohmann::json& stateJson) {
        ValidationResult result;
        result.isValid = true;

        // State must be object
        if (!stateJson.is_object()) {
            result.addError("Master bus state must be a JSON object");
            return result;
        }

        // Validate tempo fields if present
        if (stateJson.contains("masterTempo")) {
            if (!stateJson["masterTempo"].is_number()) {
                result.addError("masterTempo must be a number");
            } else {
                float tempo = stateJson["masterTempo"].get<float>();
                if (!isValidTempo(tempo)) {
                    result.addError("masterTempo must be between " + std::to_string(ValidationConstants::MIN_TEMPO) + " and " + std::to_string(ValidationConstants::MAX_TEMPO));
                }
            }
        }

        if (stateJson.contains("granularTempo")) {
            if (!stateJson["granularTempo"].is_number()) {
                result.addError("granularTempo must be a number");  
            } else {
                float tempo = stateJson["granularTempo"].get<float>();
                if (!isValidTempo(tempo)) {
                    result.addError("granularTempo must be between " + std::to_string(ValidationConstants::MIN_TEMPO) + " and " + std::to_string(ValidationConstants::MAX_TEMPO));
                }
            }
        }

        // Validate bus section if present
        if (stateJson.contains("bus")) {
            const auto& busJson = stateJson["bus"];
            
            if (!busJson.is_object()) {
                result.addError("bus must be a JSON object");
            } else {
                // Validate volume if present
                if (busJson.contains("volume")) {
                    if (!busJson["volume"].is_number()) {
                        result.addError("bus volume must be a number");
                    } else {
                        float volume = busJson["volume"].get<float>();
                        if (!isValidVolume(volume)) {
                            result.addError("bus volume must be between " + std::to_string(ValidationConstants::MIN_VOLUME) + " and " + std::to_string(ValidationConstants::MAX_VOLUME));
                        }
                    }
                }

                // Validate effects if present
                if (busJson.contains("effects")) {
                    if (!busJson["effects"].is_object()) {
                        result.addError("bus effects must be a JSON object");
                    } else {
                        for (const auto& [effectName, effectJson] : busJson["effects"].items()) {
                            auto effectResult = validateEffect(effectName, effectJson);
                            
                            // Prefix errors with effect context
                            for (const auto& error : effectResult.errors) {
                                result.addError("Bus effect '" + effectName + "': " + error);
                            }
                            for (const auto& warning : effectResult.warnings) {
                                result.addWarning("Bus effect '" + effectName + "': " + warning);
                            }
                            
                            if (!effectResult.isValid) {
                                result.isValid = false;
                            }
                        }
                    }
                }
            }
        }

        return result;
    }

    JsonValidator::ValidationResult JsonValidator::validateTrack(const nlohmann::json& trackJson,
                                                               const std::filesystem::path& songFolder) {
        ValidationResult result;
        result.isValid = true;

        // Track must be object
        if (!trackJson.is_object()) {
            result.addError("Track must be a JSON object");
            return result;
        }

        // Require file (string)
        if (!requireString(trackJson, "file", result, "track")) {
            return result;
        }

        // Validate file name
        std::string fileName = trackJson["file"].get<std::string>();
        if (!isValidFileName(fileName)) {
            result.addError("Invalid file name: " + fileName);
        }

        if (!isValidAudioFile(fileName)) {
            result.addError("File must be a supported audio format (.ogg, .wav, .aif): " + fileName);
        }

        // Check if file exists in song folder (if folder provided)
        // Check if file exists in song folder (if folder provided)
        if (!songFolder.empty() && !fileExistsInFolder(fileName, songFolder)) {
            result.addWarning("Audio file not found: " + fileName);
        }

        // Validate volume if present
        if (trackJson.contains("volume")) {
            if (!trackJson["volume"].is_number()) {
                result.addError("volume must be a number");
            } else {
                float volume = trackJson["volume"].get<float>();
                if (!isValidVolume(volume)) {
                    result.addError("volume must be between " + std::to_string(ValidationConstants::MIN_VOLUME) + " and " + std::to_string(ValidationConstants::MAX_VOLUME));
                }
            }
        }

        // Validate effects if present
        if (trackJson.contains("effects")) {
            if (!trackJson["effects"].is_object()) {
                result.addError("effects must be a JSON object");
            } else {
                for (const auto& [effectName, effectJson] : trackJson["effects"].items()) {
                    auto effectResult = validateEffect(effectName, effectJson);
                    
                    // Prefix errors with effect context
                    for (const auto& error : effectResult.errors) {
                        result.addError("Effect '" + effectName + "': " + error);
                    }
                    for (const auto& warning : effectResult.warnings) {
                        result.addWarning("Effect '" + effectName + "': " + warning);
                    }
                    
                    if (!effectResult.isValid) {
                        result.isValid = false;
                    }
                }
            }
        }

        return result;
    }

    JsonValidator::ValidationResult JsonValidator::validateEffect(const std::string& effectName,
                                                                const nlohmann::json& effectJson) {
        ValidationResult result;
        result.isValid = true;

        // Check if effect name is valid
        // Unknown effects are skipped on load rather than rejecting the whole song
        if (!m_filterManager.isValidFilterName(effectName)) {
            result.addWarning("Unknown effect type (ignored): " + effectName);
            return result;
        }

        // Effect must be object
        if (!effectJson.is_object()) {
            result.addError("Effect must be a JSON object");
            return result;
        }

        // Validate parameters if present
        if (effectJson.contains("parameters")) {
            if (!effectJson["parameters"].is_object()) {
                result.addError("parameters must be a JSON object");
            } else {
                for (const auto& [paramIdStr, paramValue] : effectJson["parameters"].items()) {
                    // Check parameter value type
                    if (!paramValue.is_number()) {
                        result.addError("Parameter '" + paramIdStr + "' must be a number");
                        continue;
                    }

                    // Validate parameter ID and value range
                    try {
                        float value = paramValue.get<float>();
                        int paramId = m_filterManager.resolveParameterId(effectName, paramIdStr);
                        if (paramId < 0) {
                            result.addWarning("Ignoring unknown parameter '" + paramIdStr +
                                             "' on effect " + effectName);
                            continue;
                        }

                        if (!m_filterManager.isValidParameter(effectName, paramId, value)) {
                            result.addWarning("Parameter '" + paramIdStr + "' value " +
                                             std::to_string(value) +
                                             " is out of range for effect " + effectName +
                                             " (will be clamped)");
                        }
                    } catch (const std::exception&) {
                        result.addError("Parameter '" + paramIdStr + "' must be a number");
                    }
                }
            }
        }

        return result;
    }

    bool JsonValidator::isValidFileName(const std::string& filename) const {
        if (filename.empty()) return false;
        if (filename.find("..") != std::string::npos) return false; // No path traversal
        if (filename.find('/') != std::string::npos) return false;  // No directory separators
        if (filename.find('\\') != std::string::npos) return false; // No Windows separators
        return true;
    }

    bool JsonValidator::isValidAudioFile(const std::string& filename) const {
        std::filesystem::path path(filename);
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        return std::find(ValidationConstants::SUPPORTED_AUDIO_EXTENSIONS.begin(), 
                        ValidationConstants::SUPPORTED_AUDIO_EXTENSIONS.end(), 
                        ext) != ValidationConstants::SUPPORTED_AUDIO_EXTENSIONS.end();
    }

    bool JsonValidator::fileExistsInFolder(const std::string& filename, 
                                          const std::filesystem::path& folder) const {
        try {
            auto filePath = folder / filename;
            return std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath);
        } catch (const std::filesystem::filesystem_error&) {
            return false;
        }
    }

    bool JsonValidator::isValidFadeTime(float fadeTime) const {
        return fadeTime >= ValidationConstants::MIN_FADE_TIME && fadeTime <= ValidationConstants::MAX_FADE_TIME;
    }

    bool JsonValidator::isValidTempo(float tempo) const {
        return tempo >= ValidationConstants::MIN_TEMPO && tempo <= ValidationConstants::MAX_TEMPO;
    }

    bool JsonValidator::isValidVolume(float volume) const {
        return volume >= ValidationConstants::MIN_VOLUME && volume <= ValidationConstants::MAX_VOLUME;
    }

    bool JsonValidator::requireField(const nlohmann::json& json, const std::string& field,
                                   ValidationResult& result, const std::string& context) const {
        if (!json.contains(field)) {
            result.addError("Missing required field '" + field + "'" + 
                           (context.empty() ? "" : " in " + context));
            return false;
        }
        return true;
    }

    bool JsonValidator::requireString(const nlohmann::json& json, const std::string& field,
                                    ValidationResult& result, const std::string& context) const {
        if (!requireField(json, field, result, context)) return false;
        
        if (!json[field].is_string()) {
            result.addError("Field '" + field + "' must be a string" +
                           (context.empty() ? "" : " in " + context));
            return false;
        }
        return true;
    }

    bool JsonValidator::requireNumber(const nlohmann::json& json, const std::string& field,
                                    ValidationResult& result, const std::string& context) const {
        if (!requireField(json, field, result, context)) return false;
        
        if (!json[field].is_number()) {
            result.addError("Field '" + field + "' must be a number" +
                           (context.empty() ? "" : " in " + context));
            return false;
        }
        return true;
    }

    bool JsonValidator::requireArray(const nlohmann::json& json, const std::string& field,
                                   ValidationResult& result, const std::string& context) const {
        if (!requireField(json, field, result, context)) return false;
        
        if (!json[field].is_array()) {
            result.addError("Field '" + field + "' must be an array" +
                           (context.empty() ? "" : " in " + context));
            return false;
        }
        return true;
    }

    bool JsonValidator::requireObject(const nlohmann::json& json, const std::string& field,
                                    ValidationResult& result, const std::string& context) const {
        if (!requireField(json, field, result, context)) return false;
        
        if (!json[field].is_object()) {
            result.addError("Field '" + field + "' must be an object" +
                           (context.empty() ? "" : " in " + context));
            return false;
        }
        return true;
    }

} // namespace Dynamix