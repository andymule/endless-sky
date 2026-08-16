# Dynamix Test Suite CMakeLists.txt
# Real audio verification testing with Catch2

cmake_minimum_required(VERSION 3.24)

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.15.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Catch2)

# Core library without main.cpp / GUI views. SoLoud is compiled from the parent
# source list (there is no standalone soloud CMake target). ImGui core is needed
# because SongManager/ConsoleLog include imgui headers.
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
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

add_library(dynamix_core STATIC ${DYNAMIX_CORE_SOURCES} ${SOLOUD_SOURCES})

target_include_directories(dynamix_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${signalsmith-stretch_SOURCE_DIR}/include
    ${signalsmith-linear_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${soloud_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

target_compile_definitions(dynamix_core PUBLIC
    SOLOUD_STATIC
    WITH_SDL2_STATIC
)

target_link_libraries(dynamix_core PUBLIC
    nlohmann_json::nlohmann_json
    ${SDL2_LIBRARIES}
)

if(SDL2_FOUND AND SDL2_LINK_LIBRARIES)
    target_link_directories(dynamix_core PUBLIC ${SDL2_LIBRARY_DIRS})
    target_link_libraries(dynamix_core PUBLIC ${SDL2_LINK_LIBRARIES})
endif()

if(APPLE)
    target_link_libraries(dynamix_core PUBLIC ${OPENGL_FRAMEWORK})
    if(ACCELERATE_FRAMEWORK)
        target_link_libraries(dynamix_core PUBLIC ${ACCELERATE_FRAMEWORK})
    endif()
else()
    target_link_libraries(dynamix_core PUBLIC OpenGL::GL)
endif()

set(TEST_HARNESS_SOURCES
    AudioTestHarness.cpp
    SignalAnalyzer.cpp
    TestHelpers.cpp
)

set(UNIT_TEST_SOURCES
    unit/test_json_validator.cpp
    unit/test_filter_manager.cpp
    unit/test_song_manager.cpp
    unit/test_event_system.cpp
    unit/test_audio_stream_processor.cpp
)

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

set(INTEGRATION_TEST_SOURCES
    integration/test_song_workflow.cpp
    integration/test_effect_persistence.cpp
    integration/test_event_transitions.cpp
)

set(ROUNDTRIP_TEST_SOURCES
    roundtrip/test_json_roundtrip.cpp
    roundtrip/test_state_persistence.cpp
)

add_executable(dynamix_tests
    ${TEST_HARNESS_SOURCES}
    ${UNIT_TEST_SOURCES}
    ${AUDIO_TEST_SOURCES}
    ${INTEGRATION_TEST_SOURCES}
    ${ROUNDTRIP_TEST_SOURCES}
)

# Put the test binary next to dynamix.exe so SDL2/runtime DLLs resolve.
set_target_properties(dynamix_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_include_directories(dynamix_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${soloud_SOURCE_DIR}/include
)

target_link_libraries(dynamix_tests PRIVATE
    dynamix_core
    Catch2::Catch2WithMain
)

set_target_properties(dynamix_core dynamix_tests PROPERTIES UNITY_BUILD OFF)

add_custom_command(TARGET dynamix_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/fixtures
        $<TARGET_FILE_DIR:dynamix_tests>/fixtures
    COMMENT "Copying test fixtures to build directory"
)

enable_testing()
add_test(NAME dynamix_tests COMMAND dynamix_tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_custom_target(test_verbose
    COMMAND dynamix_tests -s
    DEPENDS dynamix_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests with verbose output"
)

add_custom_target(test_unit
    COMMAND dynamix_tests "[unit]"
    DEPENDS dynamix_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests"
)

add_custom_target(test_audio
    COMMAND dynamix_tests "[audio]"
    DEPENDS dynamix_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running audio verification tests"
)

add_custom_target(test_integration
    COMMAND dynamix_tests "[integration]"
    DEPENDS dynamix_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running integration tests"
)

add_custom_target(test_roundtrip
    COMMAND dynamix_tests "[roundtrip]"
    DEPENDS dynamix_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running round-trip tests"
)

add_custom_target(test_performance
    COMMAND dynamix_tests "[performance]"
    DEPENDS dynamix_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running performance tests"
)
