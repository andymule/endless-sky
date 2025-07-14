cmake_minimum_required(VERSION 3.16)
project(soloud_nosound_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use FetchContent to get SoLoud
include(FetchContent)
FetchContent_Declare(
    soloud
    GIT_REPOSITORY https://github.com/jarikomppa/soloud.git
    GIT_TAG master
)
FetchContent_MakeAvailable(soloud)

# Add executable
add_executable(test_soloud_nosound test_soloud_nosound.cpp)

# Include directories
target_include_directories(test_soloud_nosound PRIVATE
    ${soloud_SOURCE_DIR}/include
)

# Add SoLoud sources (core + nosound backend)
file(GLOB SOLOUD_CORE_SOURCES ${soloud_SOURCE_DIR}/src/core/*.cpp)
set(SOLOUD_SOURCES
    ${SOLOUD_CORE_SOURCES}
    ${soloud_SOURCE_DIR}/src/backend/nosound/soloud_nosound.cpp
)

target_sources(test_soloud_nosound PRIVATE ${SOLOUD_SOURCES})

# Add SoLoud definitions
target_compile_definitions(test_soloud_nosound PRIVATE
    -DSOLOUD_STATIC
    -DWITH_NOSOUND
)

# Fix linker issues with MSYS2/MinGW64
if(WIN32)
    target_link_libraries(test_soloud_nosound PRIVATE
        -static-libgcc
        -static-libstdc++
    )
endif() 