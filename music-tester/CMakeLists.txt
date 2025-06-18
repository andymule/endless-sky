add_definitions(-DWITH_SDL2_STATIC)

cmake_minimum_required(VERSION 3.19)
project(music-tester VERSION 0.1.0)

# Generate compile_commands.json for better IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

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

# Add ImGui and SoLoud specific definitions
add_definitions(
    -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS
    -DIMGUI_DISABLE_OBSOLETE_KEYIO
    -DIMGUI_IMPL_OPENGL_LOADER_GL3W
    -DIMGUI_IMPL_OPENGL_USE_DESKTOP_GL
    -DSOLOUD_STATIC
)

# Set up sources for music-tester
set(MUSIC_TESTER_SOURCES
    src/main.cpp
    src/AudioSystem.cpp
    src/AudioController.cpp
    src/TesterView.cpp
    src/MasterTempoProcessor.cpp
)

# Set up headers for music-tester
set(MUSIC_TESTER_HEADERS
    src/AudioSystem.h
    src/AudioState.h
    src/AudioController.h
    src/TesterView.h
    src/ErrorHandling.h
    src/MasterTempoProcessor.h
)

# Add the executable target
add_executable(music-tester ${MUSIC_TESTER_SOURCES})

# Find or fetch ImGui
include(FetchContent)

# Fetch ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.1
)
FetchContent_MakeAvailable(imgui)

# Fetch SoLoud
FetchContent_Declare(
    soloud
    GIT_REPOSITORY https://github.com/jarikomppa/soloud.git
    GIT_TAG master
)
FetchContent_MakeAvailable(soloud)

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

# Add SoLoud sources (core, backend/sdl2, audiosource/wav, audiosource/wavstream)
file(GLOB SOLOUD_CORE_SOURCES ${soloud_SOURCE_DIR}/src/core/*.cpp)
set(SOLOUD_SOURCES
    ${SOLOUD_CORE_SOURCES}
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
    ${soloud_SOURCE_DIR}/src/filter/soloud_lofifilter.cpp
    ${soloud_SOURCE_DIR}/src/filter/soloud_flangerfilter.cpp
    ${soloud_SOURCE_DIR}/src/filter/soloud_dcremovalfilter.cpp
    ${soloud_SOURCE_DIR}/src/filter/soloud_fftfilter.cpp
    ${soloud_SOURCE_DIR}/src/filter/soloud_bassboostfilter.cpp
    ${soloud_SOURCE_DIR}/src/filter/soloud_waveshaperfilter.cpp
    ${soloud_SOURCE_DIR}/src/filter/soloud_robotizefilter.cpp
    ${soloud_SOURCE_DIR}/src/filter/soloud_freeverbfilter.cpp
)

target_sources(music-tester PRIVATE ${IMGUI_SOURCES} ${SOLOUD_SOURCES})

# Get Homebrew prefix for library paths
execute_process(
    COMMAND brew --prefix
    OUTPUT_VARIABLE BREW_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Find SDL2
find_package(SDL2 REQUIRED)
if(NOT SDL2_FOUND)
    message(FATAL_ERROR "SDL2 not found.")
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

# Include directories
target_include_directories(music-tester PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third-party/signalsmith-stretch
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${soloud_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${JPEG_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIR}
)

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
    message(STATUS "Using macOS OpenGL framework: ${OPENGL_FRAMEWORK}")
else()
    find_package(OpenGL REQUIRED)
endif()

# Link with SDL2, OpenGL, and other required libraries
target_link_libraries(music-tester PRIVATE
    ${MINIZIP_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${PNG_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${OPENAL_LIBRARY}
)

# Link with OpenGL library based on platform
if(APPLE)
    target_link_libraries(music-tester PRIVATE ${OPENGL_FRAMEWORK})
    # Apple deprecated OpenGL in MacOS 10.14, but we don't care
    target_compile_definitions(music-tester PRIVATE GL_SILENCE_DEPRECATION)
else()
    target_link_libraries(music-tester PRIVATE OpenGL::GL)
    if(NOT APPLE)
        target_link_libraries(music-tester PRIVATE GLEW::glew)
    endif()
endif()

# Create sound_staging directory and copy contents
add_custom_command(TARGET music-tester POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Source directory: ${CMAKE_SOURCE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "Target directory: $<TARGET_FILE_DIR:music-tester>"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:music-tester>/sound_staging"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/sound_staging" "$<TARGET_FILE_DIR:music-tester>/sound_staging"
    COMMAND ${CMAKE_COMMAND} -E echo "Contents of target sound_staging:"
    COMMAND ls "$<TARGET_FILE_DIR:music-tester>/sound_staging"
    COMMENT "Copying sound_staging directory to build directory"
)

# The OAML-related definition has been removed 