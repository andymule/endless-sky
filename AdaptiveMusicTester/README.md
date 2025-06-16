# Adaptive Music Tester

A standalone GUI application for testing and developing adaptive music systems using SDL2 and Dear ImGui.

## Overview

This application provides a visual interface for experimenting with adaptive music concepts without needing to integrate directly into a game. It allows you to:

- Create multiple music layers with different intensity levels
- Add tracks to each layer
- Change intensity levels in real-time to test transitions
- Visualize the currently playing audio

## Requirements

- CMake 3.10+
- SDL2
- SDL2_mixer
- A C++17 compatible compiler

## Building

### Install Dependencies

#### macOS

```bash
brew install sdl2 sdl2_mixer cmake
```

#### Linux (Ubuntu/Debian)

```bash
sudo apt install libsdl2-dev libsdl2-mixer-dev cmake
```

#### Windows (using vcpkg)

```bash
vcpkg install sdl2 sdl2-mixer
```

### Clone Dear ImGui

```bash
git clone https://github.com/ocornut/imgui.git AdaptiveMusicTester/imgui
```

### Build the Project

```bash
cd AdaptiveMusicTester
mkdir build
cd build
cmake ..
make
```

## Usage

1. Run the application:
   ```bash
   ./adaptive_music_tester
   ```

2. The interface consists of several panels:
   - **Layers**: Manage music layers and add tracks
   - **Controls**: Play/stop music and change intensity levels
   - **Visualization**: View a visual representation of the music
   - **Debug**: View system state information

3. To add a music track:
   - Click on a layer in the Layers panel
   - Click "Add Track" and select a music file
   - Note: For this demo, you'll need to add your own audio files to the `resources` directory

4. To test intensity changes:
   - Use the Intensity Level dropdown in the Controls panel
   - Select different intensity levels to hear how the music adapts

## Extending the System

The core `AdaptiveMusic` class can be used independently of the GUI. To integrate it into your game:

1. Copy the `AdaptiveMusic.h` and `AdaptiveMusic.cpp` files to your project
2. Initialize the system in your game setup
3. Call the `Update()` method in your game loop
4. Use `SetIntensityLevel()` to change the music based on gameplay

## License

This project is available under the MIT License. See the LICENSE file for details. 