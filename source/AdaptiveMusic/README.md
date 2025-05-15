# AdaptiveMusic Integration for Endless Sky

## Overview

AdaptiveMusic is a simple library for playing background music in Endless Sky. It uses the SoLoud audio library with SDL2 backend to provide looping audio playback.

This integration is currently a proof-of-concept to demonstrate how to:
1. Add a new audio system using SoLoud
2. Load and play WAV files
3. Integrate with Endless Sky's main game loop

## Implementation Details

The library consists of two main parts:
- `AdaptiveMusic.h` - Header file with the API
- `AdaptiveMusic.cpp` - Implementation file

AdaptiveMusic is integrated into the MainPanel and plays a test sound ("alarm.wav") when the main game panel is active.

## Building

To build Endless Sky with AdaptiveMusic support:
```bash
./build_with_adaptive_music.sh
```

This script handles:
- Downloading SoLoud (if needed)
- Setting up SDL2 dependencies
- Building Endless Sky with AdaptiveMusic

## Usage

The AdaptiveMusic class provides the following functionality:
- Initialize the audio system
- Load audio files
- Play audio in a loop
- Control playback (stop, pause, resume)
- Adjust volume

## Future Improvements

Possible enhancements include:
- Support for different music tracks based on game state (combat, exploration, etc.)
- Crossfading between tracks
- More audio formats (not just WAV)
- Configuration options for volume
- Spatial audio for in-game sounds

## Dependencies

- SoLoud audio library
- SDL2 (reusing Endless Sky's existing dependency)

## Credits

- SoLoud: https://github.com/jarikomppa/soloud
- SDL2: https://www.libsdl.org/ 