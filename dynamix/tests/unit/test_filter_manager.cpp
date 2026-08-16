#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "FilterManager.h"
#include "TestHelpers.h"

#include <algorithm>

using namespace Dynamix;
using namespace DynamixTest;

TEST_CASE("FilterManager registers all filters", "[unit][filter]") {
    FilterManager manager;

    SECTION("All 7 standard filters are available") {
        auto filters = FilterManager::getAvailableFilters();
        REQUIRE(filters.size() >= 7);

        std::vector<std::string> expectedFilters = {"echo", "freeverb", "lofi", "flanger",
                                                    "waveshaper", "robotize", "biquad"};

        for (const auto& expected : expectedFilters) {
            bool found =
                std::find(filters.begin(), filters.end(), expected) != filters.end();
            INFO("Filter: " << expected);
            REQUIRE(found);
        }
    }
}

TEST_CASE("FilterManager parameter validation", "[unit][filter]") {
    FilterManager manager;

    SECTION("Echo filter has correct parameters") {
        REQUIRE(manager.getParameterId("echo", "wet") == 0);
        REQUIRE(manager.getParameterId("echo", "delay") == 1);
        REQUIRE(manager.getParameterId("echo", "decay") == 2);
    }

    SECTION("Freeverb filter has correct parameters") {
        REQUIRE(manager.getParameterId("freeverb", "wet") == 0);
        REQUIRE(manager.getParameterId("freeverb", "freeze") == 1);
        REQUIRE(manager.getParameterId("freeverb", "roomSize") == 2);
        REQUIRE(manager.getParameterId("freeverb", "damp") == 3);
        REQUIRE(manager.getParameterId("freeverb", "width") == 4);
    }

    SECTION("LoFi filter has correct parameters") {
        REQUIRE(manager.getParameterId("lofi", "wet") == 0);
        REQUIRE(manager.getParameterId("lofi", "sampleRate") == 1);
        REQUIRE(manager.getParameterId("lofi", "bitDepth") == 2);
    }

    SECTION("Invalid parameter names return -1") {
        REQUIRE(manager.getParameterId("echo", "invalid") == -1);
        REQUIRE(manager.getParameterId("invalid_filter", "wet") == -1);
    }
}

TEST_CASE("FilterManager parameter ranges", "[unit][filter]") {
    FilterManager manager;

    SECTION("Wet parameter range is 0-1 for all filters") {
        for (const auto& filterName : getAllFilterNames()) {
            float min = manager.getParameterMin(filterName, "wet");
            float max = manager.getParameterMax(filterName, "wet");
            INFO("Filter: " << filterName);
            REQUIRE(min == 0.0f);
            REQUIRE(max == 1.0f);
        }
    }

    SECTION("Echo delay range is valid") {
        float min = manager.getParameterMin("echo", "delay");
        float max = manager.getParameterMax("echo", "delay");
        REQUIRE(min >= 0.0f);
        REQUIRE(max <= 2.0f);
        REQUIRE(max > min);
    }

    SECTION("LoFi bitDepth range is valid") {
        float min = manager.getParameterMin("lofi", "bitDepth");
        float max = manager.getParameterMax("lofi", "bitDepth");
        REQUIRE(min >= 1.0f);
        REQUIRE(max <= 16.0f);
    }

    SECTION("Biquad frequency range is valid") {
        float min = manager.getParameterMin("biquad", "freq");
        float max = manager.getParameterMax("biquad", "freq");
        REQUIRE(min >= 20.0f);
        REQUIRE(max <= 20000.0f);
    }
}

TEST_CASE("FilterManager rejects invalid parameter values", "[unit][filter]") {
    FilterManager manager;

    SECTION("Valid parameters pass validation") {
        REQUIRE(manager.isValidParameter("echo", "wet", 0.5f));
        REQUIRE(manager.isValidParameter("echo", "delay", 0.3f));
        REQUIRE(manager.isValidParameter("lofi", "bitDepth", 8.0f));
    }

    SECTION("Out of range parameters fail validation") {
        REQUIRE_FALSE(manager.isValidParameter("echo", "wet", -0.5f));
        REQUIRE_FALSE(manager.isValidParameter("echo", "wet", 1.5f));
        REQUIRE_FALSE(manager.isValidParameter("lofi", "bitDepth", 0.0f));
        REQUIRE_FALSE(manager.isValidParameter("lofi", "bitDepth", 20.0f));
    }

    SECTION("Invalid filter/parameter names fail validation") {
        REQUIRE_FALSE(manager.isValidParameter("invalid", "wet", 0.5f));
        REQUIRE_FALSE(manager.isValidParameter("echo", "invalid", 0.5f));
    }
}

TEST_CASE("FilterManager signal chain order", "[unit][filter]") {
    SECTION("Signal chain order is defined") {
        auto order = FilterManager::getFiltersInSignalChainOrder();
        REQUIRE_FALSE(order.empty());
    }

    SECTION("EQ comes before modulation effects") {
        auto order = FilterManager::getFiltersInSignalChainOrder();

        auto biquadIt = std::find(order.begin(), order.end(), "biquad");
        auto flangerIt = std::find(order.begin(), order.end(), "flanger");

        if (biquadIt != order.end() && flangerIt != order.end()) {
            REQUIRE(biquadIt < flangerIt);
        }
    }

    SECTION("Modulation comes before time-based effects") {
        auto order = FilterManager::getFiltersInSignalChainOrder();

        auto flangerIt = std::find(order.begin(), order.end(), "flanger");
        auto echoIt = std::find(order.begin(), order.end(), "echo");

        if (flangerIt != order.end() && echoIt != order.end()) {
            REQUIRE(flangerIt < echoIt);
        }
    }

    SECTION("Reverb comes last") {
        auto order = FilterManager::getFiltersInSignalChainOrder();

        auto freeverbIt = std::find(order.begin(), order.end(), "freeverb");
        auto echoIt = std::find(order.begin(), order.end(), "echo");

        if (freeverbIt != order.end() && echoIt != order.end()) {
            REQUIRE(echoIt < freeverbIt);
        }
    }
}

TEST_CASE("FilterManager parameter name mapping", "[unit][filter]") {
    FilterManager manager;

    SECTION("Parameter ID to name mapping works") {
        REQUIRE(manager.getParameterName("echo", 0) == "wet");
        REQUIRE(manager.getParameterName("echo", 1) == "delay");
        REQUIRE(manager.getParameterName("echo", 2) == "decay");
    }

    SECTION("Invalid parameter ID returns empty string") {
        REQUIRE(manager.getParameterName("echo", 99).empty());
        REQUIRE(manager.getParameterName("invalid", 0).empty());
    }

    SECTION("Bidirectional mapping is consistent") {
        for (const auto& filterName : getAllFilterNames()) {
            for (int paramId : getFilterParameterIds(filterName)) {
                std::string name = manager.getParameterName(filterName, paramId);
                if (!name.empty()) {
                    int retrievedId = manager.getParameterId(filterName, name);
                    INFO("Filter: " << filterName << ", Param: " << name);
                    REQUIRE(retrievedId == paramId);
                }
            }
        }
    }
}

TEST_CASE("FilterManager default values", "[unit][filter]") {
    FilterManager manager;

    SECTION("Wet defaults to 1 (fully wet when a filter is added)") {
        for (const auto& filterName : getAllFilterNames()) {
            float defaultWet = manager.getParameterDefault(filterName, "wet");
            INFO("Filter: " << filterName);
            REQUIRE(defaultWet == 1.0f);
        }
    }

    SECTION("Default values are within valid range") {
        for (const auto& filterName : getAllFilterNames()) {
            for (int paramId : getFilterParameterIds(filterName)) {
                std::string paramName = manager.getParameterName(filterName, paramId);
                if (!paramName.empty()) {
                    float defaultVal = manager.getParameterDefault(filterName, paramName);
                    float minVal = manager.getParameterMin(filterName, paramName);
                    float maxVal = manager.getParameterMax(filterName, paramName);
                    INFO("Filter: " << filterName << ", Param: " << paramName);
                    REQUIRE(defaultVal >= minVal);
                    REQUIRE(defaultVal <= maxVal);
                }
            }
        }
    }
}
