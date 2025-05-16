cmake_minimum_required(VERSION 3.19)
project(music-tester VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

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
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
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
else()
    # Look for it in the system
    pkg_check_modules(MINIZIP minizip)
    if(NOT MINIZIP_FOUND)
        message(FATAL_ERROR "minizip not found. Please install it or provide unofficial-minizip_DIR.")
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

# Create sound_staging directory if it doesn't exist
add_custom_command(TARGET music-tester POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:music-tester>/sound_staging"
)

# Add compile definition to indicate OAML is disabled
target_compile_definitions(music-tester PRIVATE OAML_DISABLED=1) 