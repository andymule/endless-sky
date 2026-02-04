#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "JsonValidator.h"
#include "TestHelpers.h"

#include <nlohmann/json.hpp>

using namespace Dynamix;
using namespace DynamixTest;

TEST_CASE("JsonValidator validates song JSON schema", "[unit][json]") {
    JsonValidator validator;

    SECTION("Valid minimal song JSON passes validation") {
        nlohmann::json json = nlohmann::json::parse(createMinimalSongJson({"track1.ogg"}));
        auto result = validator.validateSongSchema(json);
        REQUIRE(result.isValid);
        REQUIRE(result.errors.empty());
    }

    SECTION("Missing events array is rejected") {
        nlohmann::json json = {{"name", "Invalid"}};
        auto result = validator.validateSongSchema(json);
        REQUIRE_FALSE(result.isValid);
        REQUIRE_FALSE(result.errors.empty());
    }

    SECTION("Empty events array is rejected") {
        nlohmann::json json = {{"events", nlohmann::json::array()}};
        auto result = validator.validateSongSchema(json);
        REQUIRE_FALSE(result.isValid);
    }

    SECTION("Event missing name is rejected") {
        nlohmann::json json = {{"events", {{{"fadeTime", 1.0}, {"state", {{"tracks", nlohmann::json::array()}}}}}}};
        auto result = validator.validateSongSchema(json);
        REQUIRE_FALSE(result.isValid);
    }

    SECTION("Event missing fadeTime is rejected") {
        nlohmann::json json = {{"events", {{{"name", "Test"}, {"state", {{"tracks", nlohmann::json::array()}}}}}}};
        auto result = validator.validateSongSchema(json);
        REQUIRE_FALSE(result.isValid);
    }
}

TEST_CASE("JsonValidator validates parameter ranges", "[unit][json]") {
    JsonValidator validator;

    SECTION("FadeTime within valid range passes") {
        nlohmann::json json = nlohmann::json::parse(R"({
            "events": [{
                "name": "Test",
                "fadeTime": 5.0,
                "state": {
                    "masterTempo": 1.0,
                    "granularTempo": 1.0,
                    "tracks": []
                }
            }]
        })");
        auto result = validator.validateSongSchema(json);
        REQUIRE(result.isValid);
    }

    SECTION("Negative fadeTime is rejected") {
        nlohmann::json json = nlohmann::json::parse(R"({
            "events": [{
                "name": "Test",
                "fadeTime": -1.0,
                "state": {
                    "masterTempo": 1.0,
                    "granularTempo": 1.0,
                    "tracks": []
                }
            }]
        })");
        auto result = validator.validateSongSchema(json);
        REQUIRE_FALSE(result.isValid);
    }

    SECTION("FadeTime above max (60s) is rejected") {
        nlohmann::json json = nlohmann::json::parse(R"({
            "events": [{
                "name": "Test",
                "fadeTime": 100.0,
                "state": {
                    "masterTempo": 1.0,
                    "granularTempo": 1.0,
                    "tracks": []
                }
            }]
        })");
        auto result = validator.validateSongSchema(json);
        REQUIRE_FALSE(result.isValid);
    }

    SECTION("Tempo below minimum (0.1) is rejected") {
        nlohmann::json json = nlohmann::json::parse(R"({
            "events": [{
                "name": "Test",
                "fadeTime": 1.0,
                "state": {
                    "masterTempo": 0.01,
                    "granularTempo": 1.0,
                    "tracks": []
                }
            }]
        })");
        auto result = validator.validateSongSchema(json);
        REQUIRE_FALSE(result.isValid);
    }

    SECTION("Tempo above maximum (4.0) is rejected") {
        nlohmann::json json = nlohmann::json::parse(R"({
            "events": [{
                "name": "Test",
                "fadeTime": 1.0,
                "state": {
                    "masterTempo": 10.0,
                    "granularTempo": 1.0,
                    "tracks": []
                }
            }]
        })");
        auto result = validator.validateSongSchema(json);
        REQUIRE_FALSE(result.isValid);
    }

    SECTION("Volume within valid range passes") {
        nlohmann::json json = nlohmann::json::parse(R"({
            "events": [{
                "name": "Test",
                "fadeTime": 1.0,
                "state": {
                    "masterTempo": 1.0,
                    "granularTempo": 1.0,
                    "tracks": [{
                        "file": "track.ogg",
                        "volume": 1.5,
                        "effects": {}
                    }]
                }
            }]
        })");
        auto result = validator.validateSongSchema(json);
        REQUIRE(result.isValid);
    }

    SECTION("Volume above maximum (2.0) is rejected") {
        nlohmann::json json = nlohmann::json::parse(R"({
            "events": [{
                "name": "Test",
                "fadeTime": 1.0,
                "state": {
                    "masterTempo": 1.0,
                    "granularTempo": 1.0,
                    "tracks": [{
                        "file": "track.ogg",
                        "volume": 5.0,
                        "effects": {}
                    }]
                }
            }]
        })");
        auto result = validator.validateSongSchema(json);
        REQUIRE_FALSE(result.isValid);
    }
}

TEST_CASE("JsonValidator validates effect parameters", "[unit][json]") {
    JsonValidator validator;

    SECTION("Valid echo parameters pass validation") {
        nlohmann::json effectJson = {
            {"parameters", {{"0", 0.5f}, {"1", 0.3f}, {"2", 0.5f}}}};
        auto result = validator.validateEffect("echo", effectJson);
        REQUIRE(result.isValid);
    }

    SECTION("Invalid effect name is rejected") {
        nlohmann::json effectJson = {{"parameters", {{"0", 0.5f}}}};
        auto result = validator.validateEffect("invalid_effect", effectJson);
        REQUIRE_FALSE(result.isValid);
    }

    SECTION("Wet parameter out of range is rejected") {
        nlohmann::json effectJson = {{"parameters", {{"0", 5.0f}}}};
        auto result = validator.validateEffect("echo", effectJson);
        REQUIRE_FALSE(result.isValid);
    }
}

TEST_CASE("JsonValidator validates master JSON schema", "[unit][json]") {
    JsonValidator validator;

    SECTION("Valid minimal master JSON passes validation") {
        nlohmann::json json = nlohmann::json::parse(createMinimalMasterJson());
        auto result = validator.validateMasterSchema(json);
        REQUIRE(result.isValid);
        REQUIRE(result.errors.empty());
    }

    SECTION("Master JSON missing events is rejected") {
        nlohmann::json json = {{"name", "Master"}};
        auto result = validator.validateMasterSchema(json);
        REQUIRE_FALSE(result.isValid);
    }
}

TEST_CASE("JsonValidator file name validation", "[unit][json]") {
    JsonValidator validator;

    SECTION("Valid audio file extensions are accepted") {
        REQUIRE(validator.isValidAudioFile("track.ogg"));
        REQUIRE(validator.isValidAudioFile("track.OGG"));
        REQUIRE(validator.isValidAudioFile("track.wav"));
        REQUIRE(validator.isValidAudioFile("track.aif"));
        REQUIRE(validator.isValidAudioFile("track.aiff"));
    }

    SECTION("Invalid audio file extensions are rejected") {
        REQUIRE_FALSE(validator.isValidAudioFile("track.mp3"));
        REQUIRE_FALSE(validator.isValidAudioFile("track.txt"));
        REQUIRE_FALSE(validator.isValidAudioFile("track"));
    }

    SECTION("Path traversal attempts are rejected") {
        REQUIRE_FALSE(validator.isValidFileName("../track.ogg"));
        REQUIRE_FALSE(validator.isValidFileName("../../secret.ogg"));
        REQUIRE_FALSE(validator.isValidFileName("/etc/passwd"));
    }
}
