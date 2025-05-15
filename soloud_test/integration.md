# SoLoud Integration with Endless Sky

This document outlines how to integrate the SoLoud audio engine with the main Endless Sky project.

## Background

The Endless Sky project currently uses OpenAL directly for audio playback. SoLoud provides an abstraction layer over OpenAL and other audio backends, which can simplify audio management while maintaining compatibility with the existing OpenAL functionality.

## Integration Steps

### 1. Add SoLoud as a Submodule

```bash
# From the root of the Endless Sky project
git submodule add https://github.com/jarikomppa/soloud.git extern/soloud
```

### 2. Update CMakeLists.txt

Add SoLoud to the main CMakeLists.txt. This can be done by adding the following section after finding the OpenAL package:

```cmake
# Build SoLoud library with OpenAL backend
set(SOLOUD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/soloud)
set(SOLOUD_SRCS
    # Core files
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
    
    # OpenAL backend
    ${SOLOUD_DIR}/src/backend/openal/soloud_openal.cpp
    ${SOLOUD_DIR}/src/backend/openal/soloud_openal_dll.c
    
    # Audio sources
    ${SOLOUD_DIR}/src/audiosource/wav/soloud_wav.cpp
    ${SOLOUD_DIR}/src/audiosource/wav/stb_vorbis.c
    ${SOLOUD_DIR}/src/audiosource/wav/dr_impl.cpp
)

add_library(soloud STATIC ${SOLOUD_SRCS})
target_include_directories(soloud 
    PUBLIC ${SOLOUD_DIR}/include
    PRIVATE ${SOLOUD_DIR}/src
)
target_compile_definitions(soloud PRIVATE 
    WITH_OPENAL=1
    WITH_OPENAL_STATICLINK=1
)
target_link_libraries(soloud PRIVATE OpenAL::OpenAL)

# Add SoLoud to ExternalLibraries
target_link_libraries(ExternalLibraries INTERFACE soloud)
```

### 3. Create an Audio Manager Class

Create a new audio manager class that wraps the SoLoud functionality:

```cpp
// source/Audio.h
#pragma once

#include "soloud.h"
#include "soloud_wav.h"

#include <map>
#include <string>

class Audio {
public:
    Audio();
    ~Audio();
    
    // Initialize the audio system
    bool Init();
    
    // Load a sound file
    bool LoadSound(const std::string& identifier, const std::string& path);
    
    // Play a loaded sound
    int PlaySound(const std::string& identifier, float volume = 1.0f);
    
    // Stop all sounds
    void StopAll();
    
    // Stop a specific sound
    void Stop(int handle);
    
    // Clean up resources
    void Deinit();
    
private:
    SoLoud::Soloud soloud;
    std::map<std::string, SoLoud::Wav> sounds;
};
```

```cpp
// source/Audio.cpp
#include "Audio.h"
#include <iostream>

Audio::Audio()
{
    // Constructor
}

Audio::~Audio()
{
    Deinit();
}

bool Audio::Init()
{
    // Try to initialize with OpenAL backend first (same as Endless Sky's existing audio)
    SoLoud::result result = soloud.init(SoLoud::Soloud::CLIP_ROUNDOFF, SoLoud::Soloud::OPENAL);
    
    if (result != SoLoud::SO_NO_ERROR)
    {
        std::cerr << "OpenAL backend initialization failed, trying automatic selection..." << std::endl;
        
        // Fall back to automatic backend selection
        result = soloud.init();
        
        if (result != SoLoud::SO_NO_ERROR)
        {
            std::cerr << "Audio initialization failed!" << std::endl;
            return false;
        }
    }
    
    return true;
}

bool Audio::LoadSound(const std::string& identifier, const std::string& path)
{
    // Check if sound is already loaded
    if (sounds.find(identifier) != sounds.end())
        return true;
    
    // Load the sound file
    SoLoud::Wav sound;
    SoLoud::result result = sound.load(path.c_str());
    
    if (result != SoLoud::SO_NO_ERROR)
    {
        std::cerr << "Failed to load sound: " << path << std::endl;
        return false;
    }
    
    // Store the loaded sound
    sounds[identifier] = std::move(sound);
    return true;
}

int Audio::PlaySound(const std::string& identifier, float volume)
{
    // Check if sound is loaded
    auto it = sounds.find(identifier);
    if (it == sounds.end())
        return -1;
    
    // Play the sound
    return soloud.play(it->second, volume);
}

void Audio::StopAll()
{
    soloud.stopAll();
}

void Audio::Stop(int handle)
{
    soloud.stop(handle);
}

void Audio::Deinit()
{
    soloud.deinit();
}
```

### 4. Integrate with Existing Audio Code

Replace or extend the current audio handling code in Endless Sky with the new Audio manager class. This would typically involve:

1. Instantiating the Audio class
2. Initializing it in the Engine's initialization
3. Loading sounds through the Audio manager
4. Playing sounds through the Audio manager
5. Properly shutting down the Audio system on exit

### 5. Testing

Create tests for the audio integration to ensure all sounds play correctly with the new SoLoud system. The test should verify:

1. Initialization succeeds
2. Sound loading works
3. Sound playback functions correctly 
4. Multiple simultaneous sounds work
5. Volume control works
6. The system cleans up properly

## Benefits of Integration

1. **Simplified API**: SoLoud provides an easier-to-use API than raw OpenAL
2. **Cross-platform consistency**: Better handles differences between platforms
3. **Multiple backends**: Can fall back to other backends if OpenAL fails
4. **Advanced features**: Provides built-in support for effects, 3D audio, etc.
5. **Maintained library**: Active development and bug fixes

## Potential Challenges

1. **Performance impact**: Adding an abstraction layer may have minimal performance cost
2. **Migration effort**: Existing code needs to be adapted to use the new system
3. **Testing requirements**: All audio functionality needs retesting

## Implementation Timeline

1. Add SoLoud as a dependency (1 day)
2. Create Audio wrapper class (2 days)
3. Migrate existing functionality (3-5 days)
4. Testing and debugging (2-3 days)
5. Documentation updates (1 day)

## Conclusion

Integrating SoLoud with Endless Sky would provide a more robust audio system while maintaining compatibility with the current OpenAL-based implementation. This approach allows for gradual migration and fallback options if any issues arise. 