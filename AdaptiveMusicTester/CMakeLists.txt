cmake_minimum_required(VERSION 3.10)
project(AdaptiveMusicTester VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
set(SDL2_DIR "/opt/homebrew/opt/sdl2")
set(SDL2_MIXER_DIR "/opt/homebrew/opt/sdl2_mixer")

# Set SDL2 paths manually for Homebrew
set(SDL2_INCLUDE_DIRS "/opt/homebrew/include/SDL2")
set(SDL2_LIBRARIES "/opt/homebrew/lib/libSDL2.a;/opt/homebrew/lib/libSDL2main.a")

include_directories(${SDL2_INCLUDE_DIRS})

# SoLoud setup
set(SOLOUD_DIR "${CMAKE_SOURCE_DIR}/soloud")
set(SOLOUD_SOURCES
    ${SOLOUD_DIR}/src/core/soloud.cpp
    ${SOLOUD_DIR}/src/core/soloud_audiosource.cpp
    ${SOLOUD_DIR}/src/core/soloud_bus.cpp
    ${SOLOUD_DIR}/src/core/soloud_core_3d.cpp
    ${SOLOUD_DIR}/src/core/soloud_core_basicops.cpp
    ${SOLOUD_DIR}/src/core/soloud_core_faderops.cpp
    ${SOLOUD_DIR}/src/core/soloud_core_filterops.cpp
    ${SOLOUD_DIR}/src/core/soloud_core_getters.cpp
    ${SOLOUD_DIR}/src/core/soloud_core_setters.cpp
    ${SOLOUD_DIR}/src/core/soloud_core_voicegroup.cpp
    ${SOLOUD_DIR}/src/core/soloud_core_voiceops.cpp
    ${SOLOUD_DIR}/src/core/soloud_fader.cpp
    ${SOLOUD_DIR}/src/core/soloud_fft.cpp
    ${SOLOUD_DIR}/src/core/soloud_fft_lut.cpp
    ${SOLOUD_DIR}/src/core/soloud_file.cpp
    ${SOLOUD_DIR}/src/core/soloud_filter.cpp
    ${SOLOUD_DIR}/src/core/soloud_misc.cpp
    ${SOLOUD_DIR}/src/core/soloud_queue.cpp
    ${SOLOUD_DIR}/src/core/soloud_thread.cpp
    ${SOLOUD_DIR}/src/audiosource/wav/soloud_wav.cpp
    ${SOLOUD_DIR}/src/audiosource/wav/soloud_wavstream.cpp
    ${SOLOUD_DIR}/src/audiosource/wav/stb_vorbis.c
    ${SOLOUD_DIR}/src/audiosource/wav/dr_impl.cpp
    ${SOLOUD_DIR}/src/backend/sdl2_static/soloud_sdl2_static.cpp
)

set(SOLOUD_INCLUDE_DIRS
    ${SOLOUD_DIR}/include
    ${SOLOUD_DIR}/src/audiosource/wav
)
include_directories(${SOLOUD_INCLUDE_DIRS})

# Add define to use SDL2 backend for SoLoud
add_definitions(-DWITH_SDL2_STATIC)

# Add Dear ImGui source files
set(IMGUI_DIR "imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)

# Include directories
include_directories(
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    src
)

# Generate config.h from config.h.in
configure_file(${CMAKE_SOURCE_DIR}/src/config.h.in ${CMAKE_BINARY_DIR}/config.h)
include_directories(${CMAKE_BINARY_DIR})

# Add source files
set(SOURCES
    src/main.cpp
    src/AdaptiveMusic.cpp
    ${IMGUI_SOURCES}
    ${SOLOUD_SOURCES}
)

# Define the executable
add_executable(adaptive_music_tester ${SOURCES})

# Link libraries
target_link_libraries(adaptive_music_tester
    ${SDL2_LIBRARIES}
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
    "-framework CoreAudio"
    "-framework AudioToolbox"
    "-framework ForceFeedback"
    "-framework Metal"
    "-framework GameController"
    "-framework CoreHaptics"
    "-framework Carbon"
)

# Copy resources to build directory
file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR}) 