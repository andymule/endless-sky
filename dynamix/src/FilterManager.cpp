#include "FilterManager.h"
#include "Logger.h"

// SoLoud filter includes
#include "soloud_bassboostfilter.h"
#include "soloud_biquadresonantfilter.h"
#include "soloud_echofilter.h"
#include "soloud_flangerfilter.h"
#include "soloud_freeverbfilter.h"
#include "soloud_lofifilter.h"
#include "soloud_robotizefilter.h"
#include "soloud_waveshaperfilter.h"

#include <algorithm>
#include <cctype>
#include <climits>

namespace Dynamix {

    namespace {
        std::string normalizeParamName(std::string name) {
            name.erase(std::remove_if(name.begin(), name.end(),
                                      [](unsigned char c) { return c == '_' || c == '-'; }),
                       name.end());
            std::transform(name.begin(), name.end(), name.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return name;
        }
    }

    // Unified filter definitions that replace both AudioController and AudioSystem parameter
    // systems
    const std::unordered_map<std::string, FilterManager::FilterDefinition>
        FilterManager::FILTER_DEFINITIONS = {
            {"echo",
             {"echo",
              {{0, "wet", 0.0f, 1.0f, 1.0f, [](float v) { return v >= 0.0f && v <= 1.0f; }},
               {1, "delay", 0.01f, 2.0f, 0.2f, [](float v) { return v > 0.0f; }},
               {2, "decay", 0.0f, 1.0f, 0.5f, [](float v) { return v >= 0.0f && v <= 1.0f; }},
               {3, "filter", 0.0f, 1.0f, 0.0f, [](float v) { return v >= 0.0f && v <= 1.0f; }}},
              []() { return std::make_unique<SoLoud::EchoFilter>(); },
              [](SoLoud::Filter* f, const std::vector<float>& params) {
                  auto* filter = dynamic_cast<SoLoud::EchoFilter*>(f);
                  if (filter && params.size() >= 4) {
                      filter->setParams(params[1], params[2], params[3]);
                  }
              }}},
            {"freeverb",
             {"freeverb",
              {{0, "wet", 0.0f, 1.0f, 1.0f, [](float v) { return v >= 0.0f && v <= 1.0f; }},
               {1, "freeze", 0.0f, 1.0f, 0.0f, [](float v) { return v >= 0.0f && v <= 1.0f; }},
               {2, "roomSize", 0.0f, 1.0f, 0.5f, [](float v) { return v >= 0.0f && v <= 1.0f; }},
               {3, "damp", 0.0f, 1.0f, 0.5f, [](float v) { return v >= 0.0f && v <= 1.0f; }},
               {4, "width", 0.0f, 1.0f, 0.5f, [](float v) { return v >= 0.0f && v <= 1.0f; }}},
              []() { return std::make_unique<SoLoud::FreeverbFilter>(); },
              [](SoLoud::Filter* f, const std::vector<float>& params) {
                  auto* filter = dynamic_cast<SoLoud::FreeverbFilter*>(f);
                  if (filter && params.size() >= 5) {
                      filter->setParams(params[1], params[2], params[3], params[4]);
                  }
              }}},
            {"lofi",
             {"lofi",
              {{0, "wet", 0.0f, 1.0f, 1.0f, [](float v) { return v >= 0.0f && v <= 1.0f; }},
               {1, "sampleRate", 1000.0f, 20000.0f, 8000.0f, [](float v) { return v > 0.0f; }},
               {2, "bitDepth", 1.0f, 16.0f, 8.0f, [](float v) { return v > 0.0f; }}},
              []() { return std::make_unique<SoLoud::LofiFilter>(); },
              [](SoLoud::Filter* f, const std::vector<float>& params) {
                  auto* filter = dynamic_cast<SoLoud::LofiFilter*>(f);
                  if (filter && params.size() >= 3) {
                      filter->setParams(params[1], params[2]);
                  }
              }}},
            {"flanger",
             {"flanger",
              {{0, "wet", 0.0f, 1.0f, 1.0f, [](float v) { return v >= 0.0f && v <= 1.0f; }},
               {1, "delay", 0.001f, 0.1f, 0.005f, [](float v) { return v > 0.0f; }},
               {2, "freq", 0.1f, 10.0f, 1.0f, [](float v) { return v > 0.0f; }}},
              []() { return std::make_unique<SoLoud::FlangerFilter>(); },
              [](SoLoud::Filter* f, const std::vector<float>& params) {
                  auto* filter = dynamic_cast<SoLoud::FlangerFilter*>(f);
                  if (filter && params.size() >= 3) {
                      filter->setParams(params[1], params[2]);
                  }
              }}},
            {"waveshaper",
             {"waveshaper",
              {{0, "wet", 0.0f, 1.0f, 1.0f, [](float v) { return v >= 0.0f && v <= 1.0f; }},
               {1, "amount", 0.0f, 1.0f, 0.5f, [](float v) { return v >= 0.0f && v <= 1.0f; }}},
              []() { return std::make_unique<SoLoud::WaveShaperFilter>(); },
              [](SoLoud::Filter* f, const std::vector<float>& params) {
                  auto* filter = dynamic_cast<SoLoud::WaveShaperFilter*>(f);
                  if (filter && params.size() >= 2) {
                      filter->setParams(params[1]);
                  }
              }}},
            {"robotize",
             {"robotize",
              {{0, "wet", 0.0f, 1.0f, 1.0f, [](float v) { return v >= 0.0f && v <= 1.0f; }},
               {1, "freq", 0.1f, 100.0f, 1.0f, [](float v) { return v > 0.0f; }},
               {2, "waveform", 0.0f, 6.0f, 0.0f, [](float v) { return v >= 0.0f && v <= 6.0f; }}},
              []() { return std::make_unique<SoLoud::RobotizeFilter>(); },
              [](SoLoud::Filter* f, const std::vector<float>& params) {
                  auto* filter = dynamic_cast<SoLoud::RobotizeFilter*>(f);
                  if (filter && params.size() >= 3) {
                      filter->setParams(params[1], params[2]);
                  }
              }}},
            {"biquad",
             {"biquad",
              {{0, "wet", 0.0f, 1.0f, 1.0f, [](float v) { return v >= 0.0f && v <= 1.0f; }},
               {1, "type", 0.0f, 7.0f, 0.0f, [](float v) { return v >= 0.0f && v <= 7.0f; }},
               {2, "freq", 20.0f, 8000.0f, 1000.0f, [](float v) { return v > 0.0f; }},
               {3, "resonance", 0.1f, 10.0f, 2.0f, [](float v) { return v > 0.0f; }}},
              []() { return std::make_unique<SoLoud::BiquadResonantFilter>(); },
              [](SoLoud::Filter* f, const std::vector<float>& params) {
                  auto* filter = dynamic_cast<SoLoud::BiquadResonantFilter*>(f);
                  if (filter && params.size() >= 4) {
                      filter->setParams(params[1], params[2], params[3]);
                  }
              }}},
            {"bassboost",
             {"bassboost",
              {{0, "wet", 0.0f, 1.0f, 1.0f, [](float v) { return v >= 0.0f && v <= 1.0f; }},
               {1, "boost", 0.0f, 10.0f, 2.0f, [](float v) { return v >= 0.0f && v <= 10.0f; }}},
              []() { return std::make_unique<SoLoud::BassboostFilter>(); },
              [](SoLoud::Filter* f, const std::vector<float>& params) {
                  auto* filter = dynamic_cast<SoLoud::BassboostFilter*>(f);
                  if (filter && params.size() >= 2) {
                      filter->setParams(params[1]);
                  }
              }}}};

    // Static filter list for backward compatibility
    const std::vector<std::string>& FilterManager::getAvailableFilters() {
        static const std::vector<std::string> availableFilters = {
            "echo", "freeverb", "lofi", "flanger", "waveshaper", "robotize", "biquad", "bassboost"};
        return availableFilters;
    }

    // Signal chain order - filters appear in this order in the audio processing chain
    const std::vector<std::string>& FilterManager::getFiltersInSignalChainOrder() {
        static const std::vector<std::string> signalChainOrder = {
            "biquad",     // EQ first (affects frequency response)
            "bassboost",  // Low-end emphasis
            "waveshaper", // Distortion/saturation
            "lofi",       // Bit reduction/sample rate reduction
            "flanger",    // Modulation effects
            "robotize",   // Pitch modulation
            "echo",       // Time-based effects
            "freeverb"    // Reverb last (spatial effects)
        };
        return signalChainOrder;
    }

    // Helper methods
    const FilterManager::FilterDefinition*
    FilterManager::getFilterDefinition(const std::string& filterName) const {
        auto it = FILTER_DEFINITIONS.find(filterName);
        return (it != FILTER_DEFINITIONS.end()) ? &it->second : nullptr;
    }

    const FilterManager::ParameterDefinition*
    FilterManager::getParameterDefinition(const std::string& filterName,
                                          const std::string& paramName) const {
        const auto* filterDef = getFilterDefinition(filterName);
        if (!filterDef)
            return nullptr;

        for (const auto& param : filterDef->parameters) {
            if (param.name == paramName || normalizeParamName(param.name) == normalizeParamName(paramName)) {
                return &param;
            }
        }
        return nullptr;
    }

    const FilterManager::ParameterDefinition*
    FilterManager::getParameterDefinition(const std::string& filterName, int paramId) const {
        const auto* filterDef = getFilterDefinition(filterName);
        if (!filterDef)
            return nullptr;

        for (const auto& param : filterDef->parameters) {
            if (param.id == paramId) {
                return &param;
            }
        }
        return nullptr;
    }

    // Public interface implementation
    int FilterManager::getParameterId(const std::string& filterName,
                                      const std::string& paramName) const {
        const auto* paramDef = getParameterDefinition(filterName, paramName);
        return paramDef ? paramDef->id : -1;
    }

    std::string FilterManager::getParameterName(const std::string& filterName, int paramId) const {
        const auto* paramDef = getParameterDefinition(filterName, paramId);
        return paramDef ? paramDef->name : "";
    }

    int FilterManager::resolveParameterId(const std::string& filterName,
                                          const std::string& key) const {
        if (key.empty()) {
            return -1;
        }
        try {
            size_t idx = 0;
            const int id = std::stoi(key, &idx);
            if (idx == key.size() && getParameterDefinition(filterName, id) != nullptr) {
                return id;
            }
        } catch (const std::exception&) {
        }
        return getParameterId(filterName, key);
    }

    bool FilterManager::setFilterParameter(SoLoud::Filter* filter, const std::string& filterName,
                                           const std::string& paramName, float value) {
        const auto* paramDef = getParameterDefinition(filterName, paramName);
        if (!paramDef) {
            LOG_ERROR("Invalid parameter: " + filterName + "." + paramName);
            return false;
        }

        if (!paramDef->validator(value)) {
            LOG_ERROR("Invalid value for parameter " + filterName + "." + paramName + ": " +
                      std::to_string(value));
            return false;
        }

        return setFilterParameter(filter, filterName, paramDef->id, value);
    }

    bool FilterManager::setFilterParameter(SoLoud::Filter* filter, const std::string& filterName,
                                           int paramId, float value) {
        const auto* paramDef = getParameterDefinition(filterName, paramId);
        if (!paramDef) {
            LOG_ERROR("Invalid parameter ID: " + filterName + "[" + std::to_string(paramId) + "]");
            return false;
        }

        if (!paramDef->validator(value)) {
            LOG_ERROR("Invalid value for parameter " + filterName + "." + paramDef->name + ": " +
                      std::to_string(value));
            return false;
        }

        // Apply the parameter using the filter-specific parameterSetter
        const auto* filterDef = getFilterDefinition(filterName);
        if (filterDef && filterDef->parameterSetter) {
            // Build a parameter vector with the current value at the correct position
            // We need to get all current parameters and update the specific one
            std::vector<float> paramValues;

            // Initialize with default values for all parameters
            for (const auto& param : filterDef->parameters) {
                paramValues.push_back(param.defaultValue);
            }

            // Update the specific parameter with the new value
            if (paramId < static_cast<int>(paramValues.size())) {
                paramValues[paramId] = value;
            }

            // Apply all parameters using the parameterSetter
            filterDef->parameterSetter(filter, paramValues);
            return true;
        }

        return false;
    }

    bool FilterManager::applyAllParameters(SoLoud::Filter* filter, const std::string& filterName,
                                           const std::vector<float>& paramValues) {
        const auto* filterDef = getFilterDefinition(filterName);
        if (!filterDef || !filterDef->parameterSetter) {
            return false;
        }

        // Apply parameters using the filter-specific parameterSetter
        // This respects the quirks that have been worked out for each filter type
        filterDef->parameterSetter(filter, paramValues);
        return true;
    }

    float FilterManager::getFilterParameter(const SoLoud::Filter* filter,
                                            const std::string& filterName,
                                            const std::string& paramName) const {
        const auto* paramDef = getParameterDefinition(filterName, paramName);
        if (!paramDef)
            return 0.0f;
        return getFilterParameter(filter, filterName, paramDef->id);
    }

    float FilterManager::getFilterParameter(const SoLoud::Filter* filter,
                                            const std::string& filterName, int paramId) const {
        // Note: This would need to retrieve the actual stored parameter values
        // For now, return default values
        const auto* paramDef = getParameterDefinition(filterName, paramId);
        return paramDef ? paramDef->defaultValue : 0.0f;
    }

    std::unique_ptr<SoLoud::Filter> FilterManager::createFilter(const std::string& filterName) {
        const auto* filterDef = getFilterDefinition(filterName);
        if (!filterDef) {
            LOG_ERROR("Invalid filter name: " + filterName);
            return nullptr;
        }

        return filterDef->factory();
    }

    void FilterManager::destroyFilter(std::unique_ptr<SoLoud::Filter>& filter) { filter.reset(); }

    bool FilterManager::isValidFilterName(const std::string& filterName) const {
        return getFilterDefinition(filterName) != nullptr;
    }

    bool FilterManager::isValidParameter(const std::string& filterName,
                                         const std::string& paramName, float value) const {
        const auto* paramDef = getParameterDefinition(filterName, paramName);
        return paramDef && value >= paramDef->min && value <= paramDef->max &&
               paramDef->validator(value);
    }

    bool FilterManager::isValidParameter(const std::string& filterName, int paramId,
                                         float value) const {
        const auto* paramDef = getParameterDefinition(filterName, paramId);
        return paramDef && value >= paramDef->min && value <= paramDef->max &&
               paramDef->validator(value);
    }

    float FilterManager::getParameterMin(const std::string& filterName,
                                         const std::string& paramName) const {
        const auto* paramDef = getParameterDefinition(filterName, paramName);
        return paramDef ? paramDef->min : 0.0f;
    }

    float FilterManager::getParameterMax(const std::string& filterName,
                                         const std::string& paramName) const {
        const auto* paramDef = getParameterDefinition(filterName, paramName);
        return paramDef ? paramDef->max : 0.0f;
    }

    float FilterManager::getParameterDefault(const std::string& filterName,
                                             const std::string& paramName) const {
        const auto* paramDef = getParameterDefinition(filterName, paramName);
        return paramDef ? paramDef->defaultValue : 0.0f;
    }

} // namespace Dynamix