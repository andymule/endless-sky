cmake_minimum_required(VERSION 3.19)

# Set up sources for music-tester
set(MUSIC_TESTER_SOURCES
    src/main.cpp
)

# Add the executable target
add_executable(music-tester ${MUSIC_TESTER_SOURCES})

# Find or fetch ImGui
include(FetchContent)

# Fetch ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.6
)
FetchContent_MakeAvailable(imgui)

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

target_sources(music-tester PRIVATE ${IMGUI_SOURCES})

# Get Homebrew prefix for library paths
execute_process(
    COMMAND brew --prefix
    OUTPUT_VARIABLE BREW_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Find required libraries with pkg-config for Vorbis and Ogg
find_package(PkgConfig REQUIRED)
pkg_check_modules(VORBIS REQUIRED vorbis)
pkg_check_modules(VORBISFILE REQUIRED vorbisfile)
pkg_check_modules(OGG REQUIRED ogg)

# For OAML, use direct find_library instead of pkg-config
if(DEFINED OAML_LIBRARY AND DEFINED OAML_INCLUDE_DIR)
    # Use the provided paths
    message(STATUS "Using provided OAML library: ${OAML_LIBRARY}")
    message(STATUS "Using provided OAML include: ${OAML_INCLUDE_DIR}")
else()
    # Try to find it manually
    find_library(OAML_LIBRARY 
        NAMES oaml
        PATHS "/usr/local/lib"
        REQUIRED
    )
    find_path(OAML_INCLUDE_DIR 
        NAMES oaml.h
        PATHS "/usr/local/include"
        REQUIRED
    )
    message(STATUS "Found OAML library: ${OAML_LIBRARY}")
    message(STATUS "Found OAML include: ${OAML_INCLUDE_DIR}")
endif()

# Specify full paths to libraries - this resolves link issues
find_library(OGG_LIBRARY 
    NAMES ogg
    PATHS "${BREW_PREFIX}/lib" "${BREW_PREFIX}/Cellar/libogg/1.3.5/lib"
    REQUIRED
)

find_library(VORBIS_LIBRARY 
    NAMES vorbis
    PATHS "${BREW_PREFIX}/lib" "${BREW_PREFIX}/Cellar/libvorbis/1.3.7/lib"
    REQUIRED
)

find_library(VORBISFILE_LIBRARY 
    NAMES vorbisfile
    PATHS "${BREW_PREFIX}/lib" "${BREW_PREFIX}/Cellar/libvorbis/1.3.7/lib"
    REQUIRED
)

message(STATUS "Found OGG_LIBRARY: ${OGG_LIBRARY}")
message(STATUS "Found VORBIS_LIBRARY: ${VORBIS_LIBRARY}")
message(STATUS "Found VORBISFILE_LIBRARY: ${VORBISFILE_LIBRARY}")

# Include directories
target_include_directories(music-tester PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${OAML_INCLUDE_DIR}
    ${VORBIS_INCLUDE_DIRS}
    ${VORBISFILE_INCLUDE_DIRS}
    ${OGG_INCLUDE_DIRS}
)

# Handle minizip - simplified approach
if(ES_USE_VCPKG)
    # Use the find_package command directly
    # The path will be set by -Dunofficial-minizip_DIR in the build script
    find_package(unofficial-minizip CONFIG REQUIRED)
    set(MINIZIP_LIBRARIES "unofficial::minizip::minizip")
    message(STATUS "Found minizip from vcpkg")
else()
    # For non-vcpkg builds, try pkg-config
    if(CMAKE_VERSION VERSION_LESS 3.31)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(MINIZIP REQUIRED minizip)
    else()
        cmake_pkg_config(EXTRACT minizip REQUIRED)
        set(MINIZIP_LIBRARIES "minizip")
    endif()
endif()

# Handle OpenGL properly based on platform
if(APPLE)
    # On macOS, explicitly use the native framework to avoid X11 GL libraries
    find_library(OPENGL_FRAMEWORK OpenGL)
    message(STATUS "Using macOS OpenGL framework: ${OPENGL_FRAMEWORK}")
else()
    find_package(OpenGL REQUIRED)
endif()

# Link with SDL2, OpenGL, OAML and other required libraries
target_link_libraries(music-tester PRIVATE
    SDL2::SDL2
    PNG::PNG
    JPEG::JPEG
    OpenAL::OpenAL
    ${OAML_LIBRARY}
    ${VORBIS_LIBRARY}
    ${VORBISFILE_LIBRARY}
    ${OGG_LIBRARY}
    ${MINIZIP_LIBRARIES}
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

# Copy the music-tester.defs file if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/music-tester.defs")
    add_custom_command(TARGET music-tester POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/music-tester.defs"
        "$<TARGET_FILE_DIR:music-tester>/music-tester.defs"
    )
endif()

# Create sound_staging directory if it doesn't exist
add_custom_command(TARGET music-tester POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:music-tester>/sound_staging"
)

# Set C++ standard (matches main project)
set_target_properties(music-tester PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
) 