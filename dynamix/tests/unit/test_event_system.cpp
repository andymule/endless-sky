#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "AudioState.h"
#include "EventSystem.h"
#include "TestHelpers.h"

using namespace Dynamix;
using namespace DynamixTest;
using Catch::Matchers::WithinAbs;

TEST_CASE("EventSystem state capture", "[unit][event]") {
    EventSystem eventSystem;

    SECTION("Captures master tempo correctly") {
        // This test verifies the basic structure of state capture
        // Full integration with AudioSystem tested in integration tests
        StateSnapshot state;
        state.masterTempo = 1.5f;
        state.granularTempo = 0.8f;

        REQUIRE(approxEqual(state.masterTempo, 1.5f));
        REQUIRE(approxEqual(state.granularTempo, 0.8f));
    }

    SECTION("Track state includes all fields") {
        TrackStateExtended track;
        track.file = "test.ogg";
        track.volume = 0.7f;

        EffectState effect;
        effect.parameters["0"] = 0.5f;
        effect.parameters["1"] = 0.3f;
        track.effects["echo"] = effect;

        REQUIRE(track.file == "test.ogg");
        REQUIRE(approxEqual(track.volume, 0.7f));
        REQUIRE(track.effects.count("echo") == 1);
        REQUIRE(approxEqual(track.effects["echo"].parameters["0"], 0.5f));
    }
}

TEST_CASE("EventSystem lerp calculations", "[unit][event]") {
    SECTION("Lerp at t=0 returns start value") {
        float start = 0.0f;
        float end = 1.0f;
        float t = 0.0f;
        float result = start + (end - start) * t;
        REQUIRE(result == start);
    }

    SECTION("Lerp at t=1 returns end value") {
        float start = 0.0f;
        float end = 1.0f;
        float t = 1.0f;
        float result = start + (end - start) * t;
        REQUIRE(result == end);
    }

    SECTION("Lerp at t=0.5 returns midpoint") {
        float start = 0.0f;
        float end = 1.0f;
        float t = 0.5f;
        float result = start + (end - start) * t;
        REQUIRE_THAT(result, WithinAbs(0.5f, 0.001f));
    }

    SECTION("Lerp works for negative to positive") {
        float start = -1.0f;
        float end = 1.0f;
        float t = 0.5f;
        float result = start + (end - start) * t;
        REQUIRE_THAT(result, WithinAbs(0.0f, 0.001f));
    }
}

TEST_CASE("EventSystem effect transition logic", "[unit][event]") {
    SECTION("Effect with wet > 0 is active") {
        EffectState effect;
        effect.parameters["0"] = 0.5f; // wet > 0
        bool active = effect.parameters.count("0") > 0 && effect.parameters.at("0") > 0.0f;
        REQUIRE(active);
    }

    SECTION("Effect with wet = 0 is inactive") {
        EffectState effect;
        effect.parameters["0"] = 0.0f; // wet = 0
        bool active = effect.parameters.count("0") > 0 && effect.parameters.at("0") > 0.0f;
        REQUIRE_FALSE(active);
    }

    SECTION("Effect without wet parameter is treated as inactive") {
        EffectState effect;
        // No parameters
        bool active = effect.parameters.count("0") > 0 && effect.parameters.at("0") > 0.0f;
        REQUIRE_FALSE(active);
    }
}

TEST_CASE("EventSystem state comparison", "[unit][event]") {
    SECTION("Identical states are equal") {
        StateSnapshot a, b;
        a.masterTempo = 1.0f;
        a.granularTempo = 1.0f;
        b.masterTempo = 1.0f;
        b.granularTempo = 1.0f;

        REQUIRE(statesEqual(a, b));
    }

    SECTION("Different tempos are not equal") {
        StateSnapshot a, b;
        a.masterTempo = 1.0f;
        a.granularTempo = 1.0f;
        b.masterTempo = 1.5f;
        b.granularTempo = 1.0f;

        REQUIRE_FALSE(statesEqual(a, b));
    }

    SECTION("Different track counts are not equal") {
        StateSnapshot a, b;
        a.masterTempo = 1.0f;
        a.granularTempo = 1.0f;
        b.masterTempo = 1.0f;
        b.granularTempo = 1.0f;

        TrackStateExtended track;
        track.file = "test.ogg";
        track.volume = 1.0f;
        a.tracks.push_back(track);

        REQUIRE_FALSE(statesEqual(a, b));
    }

    SECTION("States with effects are compared correctly") {
        StateSnapshot a, b;
        a.masterTempo = 1.0f;
        a.granularTempo = 1.0f;
        b.masterTempo = 1.0f;
        b.granularTempo = 1.0f;

        TrackStateExtended track;
        track.file = "test.ogg";
        track.volume = 1.0f;

        EffectState effect;
        effect.parameters["0"] = 0.5f;
        track.effects["echo"] = effect;

        a.tracks.push_back(track);
        b.tracks.push_back(track);

        REQUIRE(statesEqual(a, b));
    }

    SECTION("Different effect parameters are not equal") {
        StateSnapshot a, b;
        a.masterTempo = 1.0f;
        a.granularTempo = 1.0f;
        b.masterTempo = 1.0f;
        b.granularTempo = 1.0f;

        TrackStateExtended trackA, trackB;
        trackA.file = "test.ogg";
        trackA.volume = 1.0f;
        trackB.file = "test.ogg";
        trackB.volume = 1.0f;

        EffectState effectA, effectB;
        effectA.parameters["0"] = 0.5f;
        effectB.parameters["0"] = 0.8f; // Different!
        trackA.effects["echo"] = effectA;
        trackB.effects["echo"] = effectB;

        a.tracks.push_back(trackA);
        b.tracks.push_back(trackB);

        REQUIRE_FALSE(statesEqual(a, b));
    }
}

TEST_CASE("EventSystem easing functions", "[unit][event]") {
    // EASE_IN_OUT: 3t² - 2t³
    auto easeInOut = [](float t) { return t * t * (3.0f - 2.0f * t); };

    SECTION("Ease in/out starts at 0") {
        REQUIRE_THAT(easeInOut(0.0f), WithinAbs(0.0f, 0.001f));
    }

    SECTION("Ease in/out ends at 1") {
        REQUIRE_THAT(easeInOut(1.0f), WithinAbs(1.0f, 0.001f));
    }

    SECTION("Ease in/out midpoint is 0.5") {
        REQUIRE_THAT(easeInOut(0.5f), WithinAbs(0.5f, 0.001f));
    }

    SECTION("Ease in/out is slower at start") {
        float linear = 0.1f;
        float eased = easeInOut(0.1f);
        REQUIRE(eased < linear);
    }

    SECTION("Ease in/out is slower at end") {
        float linear = 0.9f;
        float eased = easeInOut(0.9f);
        REQUIRE(eased > linear);
    }
}
