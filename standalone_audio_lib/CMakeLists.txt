cmake_minimum_required(VERSION 3.14)
project(AudioLib)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try to find SDL2 using different methods based on OS
if(APPLE)
    # On macOS, try to find SDL2 via Homebrew
    execute_process(
        COMMAND brew --prefix sdl2
        OUTPUT_VARIABLE SDL2_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    if(SDL2_PREFIX)
        message(STATUS "Using direct Homebrew SDL2 paths for macOS")
        include_directories(${SDL2_PREFIX}/include ${SDL2_PREFIX}/include/SDL2)
        link_directories(${SDL2_PREFIX}/lib)
        set(SDL2_LIBRARIES SDL2)
    else()
        # Try pkg-config as fallback
        set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig")
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SDL2 REQUIRED sdl2)
        include_directories(${SDL2_INCLUDE_DIRS})
        link_directories(${SDL2_LIBRARY_DIRS})
    endif()
else()
    # For other platforms, try the FindSDL2 module
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
endif()

# Set SoLoud directory
set(SOLOUD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../extern/soloud)
message(STATUS "SoLoud directory: ${SOLOUD_DIR}")

# Define SoLoud sources
set(SOLOUD_SRCS
    # Core
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
    
    # Using SDL2 static backend
    ${SOLOUD_DIR}/src/backend/sdl2_static/soloud_sdl2_static.cpp
    
    # Add fallback for robustness
    ${SOLOUD_DIR}/src/backend/nosound/soloud_nosound.cpp
    
    # WavStream for music
    ${SOLOUD_DIR}/src/audiosource/wav/soloud_wav.cpp
    ${SOLOUD_DIR}/src/audiosource/wav/soloud_wavstream.cpp
    ${SOLOUD_DIR}/src/audiosource/wav/stb_vorbis.c
    ${SOLOUD_DIR}/src/audiosource/wav/dr_impl.cpp
)

# Create SoLoud library
add_library(soloud STATIC ${SOLOUD_SRCS})
target_include_directories(soloud PUBLIC ${SOLOUD_DIR}/include)
target_include_directories(soloud PRIVATE ${SOLOUD_DIR}/src)

# Enable SDL2 static backend for SoLoud
target_compile_definitions(soloud PRIVATE 
    WITH_SDL2_STATIC=1 
    WITH_NOSOUND=1
)

# Link SoLoud with SDL2
target_link_libraries(soloud PUBLIC ${SDL2_LIBRARIES})

# Create AudioLib library
add_library(audiolib STATIC
    AudioLib.cpp
)

# Include directories for AudioLib
target_include_directories(audiolib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SOLOUD_DIR}/include
)

# Link AudioLib with SoLoud
target_link_libraries(audiolib PUBLIC soloud)

# Create a test executable
add_executable(audiolib_test main.cpp)
target_link_libraries(audiolib_test PRIVATE audiolib)

# Copy the audio file to the build directory
add_custom_command(TARGET audiolib_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/../sounds/music/main_menu.mp3
    ${CMAKE_CURRENT_BINARY_DIR}/main_menu.mp3
) 