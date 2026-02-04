#include <catch2/catch_test_macros.hpp>

#include "AudioState.h"
#include "TestHelpers.h"

#include <nlohmann/json.hpp>

using namespace Dynamix;
using namespace DynamixTest;

TEST_CASE("JSON round-trip preserves state data", "[roundtrip][json]") {
    SECTION("Basic state survives round-trip") {
        nlohmann::json original = nlohmann::json::parse(R"({
            "events": [{
                "name": "Test",
                "fadeTime": 2.5,
                "state": {
                    "masterTempo": 1.3,
                    "granularTempo": 0.9,
                    "tracks": [{
                        "file": "track1.ogg",
                        "volume": 0.75,
                        "effects": {}
                    }]
                }
            }]
        })");

        // Serialize to string and back
        std::string jsonStr = original.dump(2);
        nlohmann::json restored = nlohmann::json::parse(jsonStr);

        REQUIRE(jsonEquivalent(original, restored));
    }

    SECTION("Effect parameters survive round-trip") {
        nlohmann::json original = nlohmann::json::parse(R"({
            "events": [{
                "name": "WithEffects",
                "fadeTime": 1.0,
                "state": {
                    "masterTempo": 1.0,
                    "granularTempo": 1.0,
                    "tracks": [{
                        "file": "track1.ogg",
                        "volume": 1.0,
                        "effects": {
                            "echo": {
                                "parameters": {
                                    "0": 0.65,
                                    "1": 0.35,
                                    "2": 0.45
                                }
                            },
                            "freeverb": {
                                "parameters": {
                                    "0": 0.5,
                                    "2": 0.8,
                                    "3": 0.6
                                }
                            }
                        }
                    }]
                }
            }]
        })");

        std::string jsonStr = original.dump(2);
        nlohmann::json restored = nlohmann::json::parse(jsonStr);

        REQUIRE(jsonEquivalent(original, restored));

        // Verify specific values
        auto& effects = restored["events"][0]["state"]["tracks"][0]["effects"];
        REQUIRE(effects.count("echo") == 1);
        REQUIRE(effects.count("freeverb") == 1);
        REQUIRE(approxEqual(effects["echo"]["parameters"]["0"].get<float>(), 0.65f));
        REQUIRE(approxEqual(effects["freeverb"]["parameters"]["2"].get<float>(), 0.8f));
    }

    SECTION("Multiple events survive round-trip") {
        nlohmann::json original = nlohmann::json::parse(R"({
            "events": [
                {
                    "name": "Event1",
                    "fadeTime": 1.0,
                    "state": {
                        "masterTempo": 1.0,
                        "granularTempo": 1.0,
                        "tracks": []
                    }
                },
                {
                    "name": "Event2",
                    "fadeTime": 2.0,
                    "state": {
                        "masterTempo": 1.5,
                        "granularTempo": 0.8,
                        "tracks": []
                    }
                },
                {
                    "name": "Event3",
                    "fadeTime": 3.0,
                    "state": {
                        "masterTempo": 0.5,
                        "granularTempo": 1.2,
                        "tracks": []
                    }
                }
            ]
        })");

        std::string jsonStr = original.dump(2);
        nlohmann::json restored = nlohmann::json::parse(jsonStr);

        REQUIRE(restored["events"].size() == 3);
        REQUIRE(restored["events"][0]["name"] == "Event1");
        REQUIRE(restored["events"][1]["name"] == "Event2");
        REQUIRE(restored["events"][2]["name"] == "Event3");
    }

    SECTION("Master JSON survives round-trip") {
        nlohmann::json original = nlohmann::json::parse(R"({
            "name": "Master Bus",
            "events": [{
                "name": "Normal",
                "fadeTime": 0.0,
                "state": {
                    "masterTempo": 1.0,
                    "granularTempo": 1.0,
                    "bus": {
                        "volume": 1.0,
                        "effects": {
                            "freeverb": {
                                "parameters": {
                                    "0": 0.3,
                                    "2": 0.7
                                }
                            }
                        }
                    }
                }
            }]
        })");

        std::string jsonStr = original.dump(2);
        nlohmann::json restored = nlohmann::json::parse(jsonStr);

        REQUIRE(jsonEquivalent(original, restored));
    }
}

TEST_CASE("Float precision preserved in JSON", "[roundtrip][json]") {
    SECTION("Standard precision floats preserved") {
        std::vector<float> testValues = {0.0f, 1.0f, 0.5f, 0.333333f, 0.666666f, 0.123456f};

        for (float original : testValues) {
            nlohmann::json j = original;
            std::string str = j.dump();
            nlohmann::json restored = nlohmann::json::parse(str);
            float result = restored.get<float>();

            INFO("Testing value: " << original);
            REQUIRE(approxEqual(original, result, 0.0001f));
        }
    }

    SECTION("Edge case floats preserved") {
        std::vector<float> testValues = {0.001f, 0.999f, 1.999f, 0.0001f};

        for (float original : testValues) {
            nlohmann::json j = original;
            std::string str = j.dump();
            nlohmann::json restored = nlohmann::json::parse(str);
            float result = restored.get<float>();

            INFO("Testing value: " << original);
            REQUIRE(approxEqual(original, result, 0.0001f));
        }
    }
}

TEST_CASE("Complex nested structures survive round-trip", "[roundtrip][json]") {
    SECTION("All 7 effects with parameters") {
        nlohmann::json original;
        original["events"] = nlohmann::json::array();

        nlohmann::json event;
        event["name"] = "AllEffects";
        event["fadeTime"] = 1.0f;
        event["state"]["masterTempo"] = 1.0f;
        event["state"]["granularTempo"] = 1.0f;

        nlohmann::json track;
        track["file"] = "track.ogg";
        track["volume"] = 1.0f;
        track["effects"] = nlohmann::json::object();

        // Add all effects
        track["effects"]["echo"]["parameters"] = {{"0", 0.5f}, {"1", 0.3f}, {"2", 0.4f}};
        track["effects"]["freeverb"]["parameters"] = {{"0", 0.4f}, {"2", 0.7f}, {"3", 0.5f}};
        track["effects"]["lofi"]["parameters"] = {{"0", 0.3f}, {"1", 8000.0f}, {"2", 8.0f}};
        track["effects"]["flanger"]["parameters"] = {{"0", 0.5f}, {"1", 0.005f}, {"2", 2.0f}};
        track["effects"]["waveshaper"]["parameters"] = {{"0", 0.4f}, {"1", 0.3f}};
        track["effects"]["robotize"]["parameters"] = {{"0", 0.3f}, {"1", 5.0f}, {"2", 0}};
        track["effects"]["biquad"]["parameters"] = {{"0", 0.6f}, {"1", 0}, {"2", 1000.0f}, {"3", 2.0f}};

        event["state"]["tracks"] = nlohmann::json::array({track});
        original["events"].push_back(event);

        std::string jsonStr = original.dump(2);
        nlohmann::json restored = nlohmann::json::parse(jsonStr);

        REQUIRE(jsonEquivalent(original, restored));

        // Verify each effect
        auto& effects = restored["events"][0]["state"]["tracks"][0]["effects"];
        REQUIRE(effects.size() == 7);
        REQUIRE(effects.count("echo") == 1);
        REQUIRE(effects.count("freeverb") == 1);
        REQUIRE(effects.count("lofi") == 1);
        REQUIRE(effects.count("flanger") == 1);
        REQUIRE(effects.count("waveshaper") == 1);
        REQUIRE(effects.count("robotize") == 1);
        REQUIRE(effects.count("biquad") == 1);
    }
}
