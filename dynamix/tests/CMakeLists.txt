# Dynamix Test Suite CMakeLists.txt
# Real audio verification testing with Catch2

cmake_minimum_required(VERSION 3.24)

# Fetch Catch2
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Catch2)

# Create a static library from Dynamix core sources (excludes main.cpp and GUI)
set(DYNAMIX_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/AudioSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/AudioController.cpp
    ${CMAKE_SOURCE_DIR}/src/AudioStreamProcessor.cpp
    ${CMAKE_SOURCE_DIR}/src/SongManager.cpp
    ${CMAKE_SOURCE_DIR}/src/EventSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/FilterManager.cpp
    ${CMAKE_SOURCE_DIR}/src/JsonValidator.cpp
    ${CMAKE_SOURCE_DIR}/src/ErrorHandling.cpp
    ${CMAKE_SOURCE_DIR}/src/Logger.cpp
    ${CMAKE_SOURCE_DIR}/src/TrackManager.cpp
    ${CMAKE_SOURCE_DIR}/src/SyncWav.cpp
    ${CMAKE_SOURCE_DIR}/src/ConsoleLog.cpp
)

add_library(dynamix_core STATIC ${DYNAMIX_CORE_SOURCES})

# dynamix_core needs the same include directories and dependencies as the main target
target_include_directories(dynamix_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${signalsmith-stretch_SOURCE_DIR}/include
    ${signalsmith-linear_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${soloud_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${JPEG_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIR}
)

# Note: dynamix_core links against dependencies from main CMakeLists.txt
# The soloud library and other deps should be available from parent scope
target_link_libraries(dynamix_core PUBLIC
    nlohmann_json::nlohmann_json
    soloud
)

# Add the same compile definitions
target_compile_definitions(dynamix_core PUBLIC
    SOLOUD_STATIC
    WITH_SDL2_STATIC
)

# Test helpers and harness sources
set(TEST_HARNESS_SOURCES
    AudioTestHarness.cpp
    SignalAnalyzer.cpp
    TestHelpers.cpp
)

# Unit tests
set(UNIT_TEST_SOURCES
    unit/test_json_validator.cpp
    unit/test_filter_manager.cpp
    unit/test_song_manager.cpp
    unit/test_event_system.cpp
)

# Audio verification tests
set(AUDIO_TEST_SOURCES
    audio/test_effect_echo.cpp
    audio/test_effect_freeverb.cpp
    audio/test_effect_lofi.cpp
    audio/test_effect_flanger.cpp
    audio/test_effect_waveshaper.cpp
    audio/test_effect_robotize.cpp
    audio/test_effect_biquad.cpp
    audio/test_granular_tempo.cpp
    audio/test_audio_sync.cpp
    audio/test_audio_throughput.cpp
)

# Integration tests
set(INTEGRATION_TEST_SOURCES
    integration/test_song_workflow.cpp
    integration/test_effect_persistence.cpp
    integration/test_event_transitions.cpp
)

# Round-trip tests
set(ROUNDTRIP_TEST_SOURCES
    roundtrip/test_json_roundtrip.cpp
    roundtrip/test_state_persistence.cpp
)

# Create the test executable
add_executable(dynamix_tests
    ${TEST_HARNESS_SOURCES}
    ${UNIT_TEST_SOURCES}
    ${AUDIO_TEST_SOURCES}
    ${INTEGRATION_TEST_SOURCES}
    ${ROUNDTRIP_TEST_SOURCES}
)

target_include_directories(dynamix_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${soloud_SOURCE_DIR}/include
)

target_link_libraries(dynamix_tests PRIVATE
    dynamix_core
    Catch2::Catch2WithMain
)

# Copy test fixtures to build directory
add_custom_command(TARGET dynamix_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures
        $<TARGET_FILE_DIR:dynamix_tests>/fixtures
    COMMENT "Copying test fixtures to build directory"
)

# Enable CTest integration
include(CTest)
include(Catch)
catch_discover_tests(dynamix_tests)

# Add a custom target for running tests with verbose output
add_custom_target(test_verbose
    COMMAND dynamix_tests -v
    DEPENDS dynamix_tests
    COMMENT "Running tests with verbose output"
)

# Add targets for running specific test categories
add_custom_target(test_unit
    COMMAND dynamix_tests "[unit]"
    DEPENDS dynamix_tests
    COMMENT "Running unit tests"
)

add_custom_target(test_audio
    COMMAND dynamix_tests "[audio]"
    DEPENDS dynamix_tests
    COMMENT "Running audio verification tests"
)

add_custom_target(test_integration
    COMMAND dynamix_tests "[integration]"
    DEPENDS dynamix_tests
    COMMENT "Running integration tests"
)

add_custom_target(test_roundtrip
    COMMAND dynamix_tests "[roundtrip]"
    DEPENDS dynamix_tests
    COMMENT "Running round-trip tests"
)

add_custom_target(test_performance
    COMMAND dynamix_tests "[performance]"
    DEPENDS dynamix_tests
    COMMENT "Running performance tests"
)
