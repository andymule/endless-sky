cmake_minimum_required(VERSION 3.24)
project(dynamix_emscripten_spike VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DYNAMIX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(PERSISTENT_DEPS_DIR "${DYNAMIX_ROOT}/.deps")

option(USE_PREDOWNLOADED_DEPS "Use pre-downloaded dependencies from .deps/" ON)

set(DYNAMIX_SOLOUD_TAG "e82fd32c1f62183922f08c14c814a02b58db1873")
set(DYNAMIX_STRETCH_TAG "57b93f4e9206a089a45387eaa39bdc9f310d3308")
set(DYNAMIX_LINEAR_TAG "0.6.0")

function(download_dependency_if_needed dep_name repo_url commit_hash)
    set(dep_dir "${PERSISTENT_DEPS_DIR}/${dep_name}")
    set(dep_marker "${dep_dir}/.downloaded")

    if(NOT EXISTS "${dep_marker}")
        message(STATUS "Downloading ${dep_name} for emscripten spike...")
        file(MAKE_DIRECTORY "${dep_dir}")
        execute_process(
            COMMAND git clone --filter=blob:none "${repo_url}" .
            WORKING_DIRECTORY "${dep_dir}"
            RESULT_VARIABLE clone_result
        )
        if(clone_result EQUAL 0)
            execute_process(
                COMMAND git checkout --quiet "${commit_hash}"
                WORKING_DIRECTORY "${dep_dir}"
                RESULT_VARIABLE clone_result
            )
        endif()
        if(NOT clone_result EQUAL 0)
            message(FATAL_ERROR "Failed to download ${dep_name}")
        endif()
        file(WRITE "${dep_marker}" "Downloaded for emscripten spike")
    endif()
endfunction()

if(USE_PREDOWNLOADED_DEPS)
    download_dependency_if_needed("soloud" "https://github.com/jarikomppa/soloud.git" "${DYNAMIX_SOLOUD_TAG}")
    download_dependency_if_needed("signalsmith-stretch" "https://github.com/Signalsmith-Audio/signalsmith-stretch.git" "${DYNAMIX_STRETCH_TAG}")
    download_dependency_if_needed("signalsmith-linear" "https://github.com/Signalsmith-Audio/linear.git" "${DYNAMIX_LINEAR_TAG}")
endif()

set(soloud_SOURCE_DIR "${PERSISTENT_DEPS_DIR}/soloud")
set(signalsmith-stretch_SOURCE_DIR "${PERSISTENT_DEPS_DIR}/signalsmith-stretch")
set(signalsmith-linear_SOURCE_DIR "${PERSISTENT_DEPS_DIR}/signalsmith-linear")

set(SPIKE_SOURCES
    spike_main.cpp
    ${DYNAMIX_ROOT}/src/AudioStreamProcessor.cpp
    ${DYNAMIX_ROOT}/src/SyncWav.cpp
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
    ${soloud_SOURCE_DIR}/src/backend/null/soloud_null.cpp
    ${soloud_SOURCE_DIR}/src/audiosource/wav/dr_impl.cpp
    ${soloud_SOURCE_DIR}/src/audiosource/wav/stb_vorbis.c
)

add_executable(spike ${SPIKE_SOURCES})

target_include_directories(spike PRIVATE
    ${DYNAMIX_ROOT}/src
    ${signalsmith-stretch_SOURCE_DIR}/include
    ${signalsmith-linear_SOURCE_DIR}/include
    ${soloud_SOURCE_DIR}/include
)

target_compile_definitions(spike PRIVATE
    SOLOUD_STATIC
    WITH_NULL
)

target_compile_options(spike PRIVATE
    -Wno-unused-parameter
    -Wno-unused-variable
)

set_target_properties(spike PROPERTIES
    OUTPUT_NAME "spike"
    SUFFIX ".js"
)

target_link_options(spike PRIVATE
    -sWASM=1
    -sENVIRONMENT=node,web
    -sALLOW_MEMORY_GROWTH=1
    -sINITIAL_MEMORY=67108864
    -sSTACK_SIZE=1048576
    -sASSERTIONS=1
    -sEXIT_RUNTIME=1
)
