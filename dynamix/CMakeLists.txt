cmake_minimum_required(VERSION 3.24)
project(dynamix VERSION 0.1.0)

# Build optimization options
option(ENABLE_UNITY_BUILD "Enable unity builds for faster compilation" ON)
option(ENABLE_PCH "Enable precompiled headers" OFF)
option(ENABLE_CCACHE "Enable ccache for faster rebuilds" ON)
option(ENABLE_LTO "Enable Link Time Optimization" ON)

# Option for bundling SDL2 (creates self-contained binary with bundled dependencies)
# Only enable SDL2 bundling by default on macOS
if(APPLE)
    option(BUNDLE_SDL2 "Bundle SDL2 dylib with the application for distribution" ON)
else()
    option(BUNDLE_SDL2 "Bundle SDL2 dylib with the application for distribution" OFF)
endif()

# Generate compile_commands.json for better IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

# Cache Homebrew prefix to avoid repeated execution
if(APPLE AND NOT DEFINED BREW_PREFIX)
    execute_process(
        COMMAND brew --prefix
        OUTPUT_VARIABLE BREW_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(BREW_PREFIX ${BREW_PREFIX} CACHE STRING "Homebrew prefix path")
endif()

# Enable ccache if available and requested
if(ENABLE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
        message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
    else()
        message(STATUS "ccache not found, continuing without it")
    endif()
endif()

# Optimize compiler flags based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Release optimizations
    if(MSVC)
        add_compile_options(/O2 /DNDEBUG /GL /Gy)
        add_link_options(/LTCG)
    else()
        add_compile_options(-O3 -DNDEBUG -march=native)
        if(ENABLE_LTO)
            add_compile_options(-flto)
            add_link_options(-flto)
        endif()
    endif()
else()
    # Debug optimizations - still optimize for speed but keep debug info
    if(MSVC)
        add_compile_options(/Od /Zi)
    else()
        add_compile_options(-O1 -g)
    endif()
endif()

# Add warning flags
if(MSVC)
    add_compile_options(/W4)
    # Suppress specific warnings for third-party code
    add_compile_options(/wd4100) # Unused parameter
    add_compile_options(/wd4505) # Unreferenced local function
else()
    add_compile_options(-Wall -Wextra)
    # Suppress specific warnings for third-party code
    add_compile_options(-Wno-unused-parameter)
    add_compile_options(-Wno-unused-variable)
endif()

# Enable unity builds if requested
if(ENABLE_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 5)  # Group 5 files together
    message(STATUS "Unity builds enabled with batch size 5")
endif()

# SDL2 bundling configuration
if(BUNDLE_SDL2)
    message(STATUS "Building with SDL2 bundling for self-contained distribution")
    
    # Find SDL2 dylib to bundle using cached Homebrew prefix
    find_library(SDL2_DYLIB
        NAMES libSDL2-2.0.0.dylib SDL2-2.0.0.dylib
        PATHS ${BREW_PREFIX}/lib
        NO_DEFAULT_PATH
    )
    
    if(SDL2_DYLIB)
        message(STATUS "Found SDL2 dylib to bundle: ${SDL2_DYLIB}")
        set(SDL2_LIBRARIES ${SDL2_DYLIB})
    else()
        message(WARNING "SDL2 dylib not found, falling back to system linking")
        set(BUNDLE_SDL2 OFF)
    endif()
else()
    message(STATUS "Using system SDL2 linking")
endif()

# Add ImGui and SoLoud specific definitions
add_definitions(
    -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS
    -DIMGUI_DISABLE_OBSOLETE_KEYIO
    -DIMGUI_IMPL_OPENGL_LOADER_GL3W
    -DIMGUI_IMPL_OPENGL_USE_DESKTOP_GL
    -DIMGUI_DEFINE_MATH_OPERATORS
    -DSOLOUD_STATIC
    -DWITH_SDL2_STATIC
)

# Set up sources for dynamix
set(DYNAMIX_SOURCES
    src/main.cpp
    # src/winmain_forward.cpp  # Temporarily disabled to test infinite loop
    src/AudioSystem.cpp
    src/AudioController.cpp
    src/MainView.cpp
    src/AudioStreamProcessor.cpp
    src/SongManager.cpp
    src/EventSystem.cpp
    src/Logger.cpp
    src/FilterManager.cpp
    src/ErrorHandling.cpp
    src/FileBrowser.cpp
    src/TrackManager.cpp
    src/SyncWav.cpp
    src/ThemeManager.cpp
    src/Views/SongView.cpp
    src/Views/BusView.cpp
    src/Views/SpeedView.cpp
)

# Set up headers for dynamix
set(DYNAMIX_HEADERS
    src/AudioSystem.h
    src/AudioState.h
    src/AudioController.h
    src/MainView.h
    src/ErrorHandling.h
    src/CircularBuffer.h
    src/AudioStreamProcessor.h
    src/SongManager.h
    src/EventSystem.h
    src/Logger.h
    src/FilterManager.h
    src/FileBrowser.h
    src/TrackManager.h
    src/SyncWav.h
    src/ThemeManager.h
    src/Views/SongView.h
    src/Views/BusView.h
)

# Copy assets to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Add the executable target
add_executable(dynamix ${DYNAMIX_SOURCES})

# Find or fetch ImGui
include(FetchContent)

# Fetch ImGui with caching
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

# Fetch SoLoud with caching
FetchContent_Declare(
    soloud
    GIT_REPOSITORY https://github.com/jarikomppa/soloud.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(soloud)

# Fetch Signalsmith Stretch with caching
FetchContent_Declare(
    signalsmith-stretch
    GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-stretch.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(signalsmith-stretch)

# Fetch Signalsmith Linear with caching
FetchContent_Declare(
    signalsmith-linear
    GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-linear.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(signalsmith-linear)

# Fetch nlohmann/json with caching
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# Add ImGui sources
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Create precompiled header if enabled - simplified version
if(ENABLE_PCH)
    # Create a minimal PCH header with only essential includes
    set(PCH_HEADER "${CMAKE_CURRENT_BINARY_DIR}/pch.h")
    set(PCH_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/pch.cpp")
    
    # Generate minimal PCH header
    file(WRITE ${PCH_HEADER}
"#pragma once
// Minimal precompiled header for essential includes only
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <mutex>
#include <map>
#include <set>
#include <functional>
#include <utility>
#include <array>
#include <tuple>
#include <optional>
#include <stdexcept>
#include <limits>
#include <cmath>
#include <cstring>
#include <cassert>
#include <cstdint>
")
    
    # Generate PCH source
    file(WRITE ${PCH_SOURCE} "#include \"pch.h\"\n")
    
    # Add PCH to sources
    list(PREPEND DYNAMIX_SOURCES ${PCH_SOURCE})
    
    # Enable PCH for the target
    target_precompile_headers(dynamix PRIVATE ${PCH_HEADER})
    
    message(STATUS "Precompiled headers enabled (minimal)")
endif()

# Add SoLoud sources - explicit list instead of globbing
set(SOLOUD_SOURCES
    ${soloud_SOURCE_DIR}/src/core/soloud.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_audiosource.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_bus.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_core_3d.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_core_basicops.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_core_faderops.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_core_filterops.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_core_getters.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_core_setters.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_core_voicegroup.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_core_voiceops.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_fft.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_fft_lut.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_file.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_fader.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_filter.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_misc.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_queue.cpp
    ${soloud_SOURCE_DIR}/src/core/soloud_thread.cpp
    ${soloud_SOURCE_DIR}/src/audiosource/wav/soloud_wav.cpp
    ${soloud_SOURCE_DIR}/src/audiosource/wav/soloud_wavstream.cpp
    ${soloud_SOURCE_DIR}/src/backend/sdl2_static/soloud_sdl2_static.cpp
    ${soloud_SOURCE_DIR}/src/audiosource/wav/dr_flac.h
    ${soloud_SOURCE_DIR}/src/audiosource/wav/dr_mp3.h
    ${soloud_SOURCE_DIR}/src/audiosource/wav/dr_wav.h
    ${soloud_SOURCE_DIR}/src/audiosource/wav/stb_vorbis.c
    ${soloud_SOURCE_DIR}/src/audiosource/wav/dr_impl.cpp
    ${soloud_SOURCE_DIR}/src/filter/soloud_biquadresonantfilter.cpp
    ${soloud_SOURCE_DIR}/src/filter/soloud_echofilter.cpp
    ${soloud_SOURCE_DIR}/src/filter/soloud_flangerfilter.cpp
    ${soloud_SOURCE_DIR}/src/filter/soloud_freeverbfilter.cpp
    ${soloud_SOURCE_DIR}/src/filter/soloud_lofifilter.cpp
    ${soloud_SOURCE_DIR}/src/filter/soloud_robotizefilter.cpp
    ${soloud_SOURCE_DIR}/src/filter/soloud_waveshaperfilter.cpp
    ${soloud_SOURCE_DIR}/src/filter/soloud_fftfilter.cpp
)

target_sources(dynamix PRIVATE ${IMGUI_SOURCES} ${SOLOUD_SOURCES})

# Find SDL2 - prefer pkg-config to avoid framework issues
if(BUNDLE_SDL2 AND SDL2_DYLIB)
    # For bundled builds, we already found SDL2 above
    set(SDL2_FOUND TRUE)
    set(SDL2_INCLUDE_DIRS ${BREW_PREFIX}/include/SDL2)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)

    # If pkg-config fails, fall back to traditional find_package
    if(NOT SDL2_FOUND)
        find_package(SDL2 REQUIRED)
        if(NOT SDL2_FOUND)
            message(FATAL_ERROR "SDL2 not found.")
        endif()
    endif()
endif()

# Find PNG
find_package(PNG REQUIRED)
if(NOT PNG_FOUND)
    message(FATAL_ERROR "PNG not found.")
endif()

# Find JPEG
find_package(JPEG REQUIRED)
if(NOT JPEG_FOUND)
    message(FATAL_ERROR "JPEG not found.")
endif()

# Find OpenAL
find_package(OpenAL REQUIRED)
if(NOT OPENAL_FOUND)
    message(FATAL_ERROR "OpenAL not found.")
endif()

# Find GLEW (only on non-Apple platforms)
if(NOT APPLE)
    find_package(GLEW REQUIRED)
    if(NOT GLEW_FOUND)
        message(FATAL_ERROR "GLEW not found.")
    endif()
endif()

# Include directories
target_include_directories(dynamix PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
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

# Add SDL2, PNG, JPEG include directories
if(BUNDLE_SDL2 AND SDL2_DYLIB)
    # For bundled builds, add Homebrew include paths
    target_include_directories(dynamix PRIVATE
        ${BREW_PREFIX}/include
        ${BREW_PREFIX}/include/SDL2
    )
else()
    # For system builds, use pkg-config paths
    target_include_directories(dynamix PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${PNG_INCLUDE_DIRS}
        ${JPEG_INCLUDE_DIRS}
    )
endif()

# Handle minizip using vcpkg
if(DEFINED unofficial-minizip_DIR)
    # Use the provided path
    find_package(unofficial-minizip CONFIG REQUIRED)
    set(MINIZIP_LIBRARIES "unofficial::minizip::minizip")
    message(STATUS "Found minizip from vcpkg")
endif()

# Handle OpenGL properly based on platform
if(APPLE)
    # On macOS, explicitly use the native framework to avoid X11 GL libraries
    find_library(OPENGL_FRAMEWORK OpenGL)
    
    # Find Accelerate framework for Signalsmith optimization
    find_library(ACCELERATE_FRAMEWORK Accelerate)
    if(ACCELERATE_FRAMEWORK)
        message(STATUS "Found Accelerate framework: ${ACCELERATE_FRAMEWORK}")
        # Enable Signalsmith Accelerate optimization
        target_compile_definitions(dynamix PRIVATE SIGNALSMITH_USE_ACCELERATE)
    endif()
    
    message(STATUS "Using macOS OpenGL framework: ${OPENGL_FRAMEWORK}")
else()
    find_package(OpenGL REQUIRED)
endif()

# Link with SDL2, OpenGL, and other required libraries
target_link_libraries(dynamix PRIVATE
    ${MINIZIP_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${PNG_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${OPENAL_LIBRARY}
    nlohmann_json::nlohmann_json
)

# Add SDL2 library directories and link flags from pkg-config
if(SDL2_FOUND AND SDL2_LINK_LIBRARIES)
    target_link_directories(dynamix PRIVATE ${SDL2_LIBRARY_DIRS})
    target_link_libraries(dynamix PRIVATE ${SDL2_LINK_LIBRARIES})
endif()

# Link with OpenGL library based on platform
if(APPLE)
    target_link_libraries(dynamix PRIVATE ${OPENGL_FRAMEWORK})
    
    # Link with Accelerate framework if available for Signalsmith optimization
    if(ACCELERATE_FRAMEWORK)
        target_link_libraries(dynamix PRIVATE ${ACCELERATE_FRAMEWORK})
        message(STATUS "Linked with Accelerate framework for optimized DSP")
    endif()
    
    # Apple deprecated OpenGL in MacOS 10.14, but we don't care
    target_compile_definitions(dynamix PRIVATE GL_SILENCE_DEPRECATION)
else()
    target_link_libraries(dynamix PRIVATE OpenGL::GL)
    # Temporarily comment out GLEW for testing
    # if(NOT APPLE)
    #     target_link_libraries(dynamix PRIVATE GLEW::glew_s)
    # endif()
endif()

# Bundle SDL2 dylib if requested
if(BUNDLE_SDL2 AND SDL2_DYLIB)
    add_custom_command(TARGET dynamix POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${SDL2_DYLIB}" "$<TARGET_FILE_DIR:dynamix>/"
        COMMAND ${CMAKE_COMMAND} -E echo "Bundled SDL2 dylib: $<TARGET_FILE_DIR:dynamix>/libSDL2-2.0.0.dylib"
        COMMENT "Bundling SDL2 dylib with application"
    )
    
    # Set up @rpath to find the bundled SDL2
    set_target_properties(dynamix PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "@executable_path"
    )
    
    # Update the binary to use @rpath for SDL2
    add_custom_command(TARGET dynamix POST_BUILD
        COMMAND install_name_tool -change "${SDL2_DYLIB}" "@rpath/libSDL2-2.0.0.dylib" "$<TARGET_FILE:dynamix>"
        COMMENT "Updating SDL2 library path to use @rpath"
    )
endif()

# The OAML-related definition has been removed 

add_custom_target(static_build ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/static_build"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:dynamix>" "${CMAKE_SOURCE_DIR}/static_build/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_SOURCE_DIR}/static_build/assets"
    COMMENT "Creating static_build folder with only the required files"
    DEPENDS dynamix
) 

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "Full clean: removed build directory and CMake-generated files."
    COMMENT "Cleaning all build files and CMake cache."
) 