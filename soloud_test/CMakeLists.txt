cmake_minimum_required(VERSION 3.10)
project(SoloudTest)

# Set C++ standard - using C++11 for SoLoud compatibility
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add Homebrew directory to pkg-config path on macOS
if(APPLE)
    # Add Homebrew's pkg-config path if it exists
    if(EXISTS "/opt/homebrew/opt/sdl2")
        set(ENV{PKG_CONFIG_PATH} "/opt/homebrew/opt/sdl2/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
        message(STATUS "Added Homebrew SDL2 to PKG_CONFIG_PATH")
    endif()
endif()

# More robust SDL2 detection
if(APPLE)
    # Try multiple methods to find SDL2 on macOS
    # First try pkg-config (Homebrew installs pkg-config files)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(SDL2 QUIET sdl2)
        if(SDL2_FOUND)
            message(STATUS "Found SDL2 via pkg-config")
        endif()
    endif()
    
    # If pkg-config didn't work, try CMake config
    if(NOT SDL2_FOUND)
        find_package(SDL2 CONFIG QUIET)
        if(SDL2_FOUND)
            message(STATUS "Found SDL2 via CMake config")
        endif()
    endif()
    
    # If still not found, try direct path to Homebrew
    if(NOT SDL2_FOUND)
        if(EXISTS "/opt/homebrew/opt/sdl2")
            set(SDL2_INCLUDE_DIRS "/opt/homebrew/include/SDL2")
            set(SDL2_LIBRARIES "/opt/homebrew/lib/libSDL2.dylib")
            set(SDL2_FOUND TRUE)
            message(STATUS "Using Homebrew SDL2 installation directly")
        endif()
    endif()
    
    # Finally, try framework as last resort
    if(NOT SDL2_FOUND)
        set(SDL2_FRAMEWORK_PATH "/Library/Frameworks/SDL2.framework")
        if(EXISTS ${SDL2_FRAMEWORK_PATH})
            set(SDL2_INCLUDE_DIRS ${SDL2_FRAMEWORK_PATH}/Headers)
            set(SDL2_LIBRARIES "-framework SDL2")
            set(SDL2_FOUND TRUE)
            message(STATUS "Using SDL2 framework")
        endif()
    endif()
elseif(WIN32)
    find_package(SDL2 CONFIG REQUIRED)
else()
    # Linux and other platforms
    find_package(SDL2 REQUIRED)
endif()

# Check if SDL2 was found
if(NOT SDL2_FOUND)
    message(FATAL_ERROR "SDL2 not found. Please install SDL2 development libraries.")
endif()

# Print SDL2 info for debugging
message(STATUS "SDL2 Include Dirs: ${SDL2_INCLUDE_DIRS}")
message(STATUS "SDL2 Libraries: ${SDL2_LIBRARIES}")

# Build SoLoud library
set(SOLOUD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../extern/soloud)

# Display SoLoud directory for debugging
message(STATUS "SoLoud directory: ${SOLOUD_DIR}")

# Make sure the directory exists
if(NOT EXISTS ${SOLOUD_DIR})
    message(FATAL_ERROR "SoLoud directory not found at ${SOLOUD_DIR}")
endif()

# Define sources for SoLoud using the SDL2 static backend like Endless Sky would use
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
    
    # Using SDL2 static backend to match Endless Sky's setup
    ${SOLOUD_DIR}/src/backend/sdl2_static/soloud_sdl2_static.cpp
    
    # Add fallback for robustness
    ${SOLOUD_DIR}/src/backend/nosound/soloud_nosound.cpp
    
    # Wav audio source
    ${SOLOUD_DIR}/src/audiosource/wav/soloud_wav.cpp
    ${SOLOUD_DIR}/src/audiosource/wav/stb_vorbis.c
    ${SOLOUD_DIR}/src/audiosource/wav/dr_impl.cpp
)

# Create SoLoud library
add_library(soloud STATIC ${SOLOUD_SRCS})
target_include_directories(soloud 
    PUBLIC ${SOLOUD_DIR}/include
    PRIVATE ${SOLOUD_DIR}/src
)

# Enable SDL2 static backend - consistent with Endless Sky's dependencies
target_compile_definitions(soloud PRIVATE 
    WITH_SDL2_STATIC=1
    WITH_NOSOUND=1
)

# Create test executable
add_executable(soloud_test main.cpp)
target_include_directories(soloud_test PRIVATE ${SOLOUD_DIR}/include)

# Link with SDL2 library - more robust approach for macOS
if(APPLE AND EXISTS "/opt/homebrew/opt/sdl2")
    # Direct path to Homebrew SDL2 for macOS
    set(SDL2_INCLUDE_DIRS "/opt/homebrew/include/SDL2")
    set(SDL2_LIBRARIES "/opt/homebrew/lib/libSDL2.dylib")
    message(STATUS "Using direct Homebrew SDL2 paths for macOS")
    
    # Configure soloud library
    target_include_directories(soloud PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(soloud PRIVATE ${SDL2_LIBRARIES})
    
    # Configure executable
    target_include_directories(soloud_test PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(soloud_test PRIVATE soloud ${SDL2_LIBRARIES})
    
    # Set rpath to find dylib at runtime
    set_target_properties(soloud_test PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "/opt/homebrew/lib"
    )
elseif(TARGET SDL2::SDL2)
    # Modern CMake targets available
    target_link_libraries(soloud PRIVATE SDL2::SDL2)
    message(STATUS "Linking with SDL2::SDL2 target")
    
    # For the executable
    target_link_libraries(soloud_test PRIVATE soloud SDL2::SDL2)
    
    # Also add SDL2main if it exists (needed on some platforms)
    if(TARGET SDL2::SDL2main)
        target_link_libraries(soloud_test PRIVATE SDL2::SDL2main)
    endif()
else()
    # Traditional approach
    target_include_directories(soloud PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(soloud PRIVATE ${SDL2_LIBRARIES})
    message(STATUS "Linking with SDL2 variables: ${SDL2_LIBRARIES}")
    
    # For the executable
    target_include_directories(soloud_test PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(soloud_test PRIVATE soloud ${SDL2_LIBRARIES})
endif()

# Copy sound file to build directory
add_custom_command(
    TARGET soloud_test
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/alarm.wav
    $<TARGET_FILE_DIR:soloud_test>/alarm.wav
) 