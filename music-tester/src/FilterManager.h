#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations
namespace SoLoud {
    class Filter;
}

namespace AudioTester {

    class FilterManager {
    public:
        // Unified parameter mapping (replaces both AudioController and AudioSystem systems)
        int getParameterId(const std::string& filterName, const std::string& paramName) const;
        std::string getParameterName(const std::string& filterName, int paramId) const;

        // Parameter validation and setting
        bool setFilterParameter(SoLoud::Filter* filter, const std::string& filterName,
                                const std::string& paramName, float value);
        bool setFilterParameter(SoLoud::Filter* filter, const std::string& filterName, int paramId,
                                float value);
        bool applyAllParameters(SoLoud::Filter* filter, const std::string& filterName,
                                const std::vector<float>& paramValues);
        float getFilterParameter(const SoLoud::Filter* filter, const std::string& filterName,
                                 const std::string& paramName) const;
        float getFilterParameter(const SoLoud::Filter* filter, const std::string& filterName,
                                 int paramId) const;

        // Filter lifecycle
        std::unique_ptr<SoLoud::Filter> createFilter(const std::string& filterName);
        void destroyFilter(std::unique_ptr<SoLoud::Filter>& filter);

        // Validation
        bool isValidFilterName(const std::string& filterName) const;
        bool isValidParameter(const std::string& filterName, const std::string& paramName,
                              float value) const;
        bool isValidParameter(const std::string& filterName, int paramId, float value) const;

        // Parameter info
        float getParameterMin(const std::string& filterName, const std::string& paramName) const;
        float getParameterMax(const std::string& filterName, const std::string& paramName) const;
        float getParameterDefault(const std::string& filterName,
                                  const std::string& paramName) const;

        // Filter registry
        static const std::vector<std::string>& getAvailableFilters();

    private:
        struct ParameterDefinition {
            int id;
            std::string name;
            float min, max, defaultValue;
            std::function<bool(float)> validator;
        };

        struct FilterDefinition {
            std::string name;
            std::vector<ParameterDefinition> parameters;
            std::function<std::unique_ptr<SoLoud::Filter>()> factory;
            std::function<void(SoLoud::Filter*, const std::vector<float>&)> parameterSetter;
        };

        static const std::unordered_map<std::string, FilterDefinition> FILTER_DEFINITIONS;

        // Helper methods
        const FilterDefinition* getFilterDefinition(const std::string& filterName) const;
        const ParameterDefinition* getParameterDefinition(const std::string& filterName,
                                                          const std::string& paramName) const;
        const ParameterDefinition* getParameterDefinition(const std::string& filterName,
                                                          int paramId) const;
    };

} // namespace AudioTester