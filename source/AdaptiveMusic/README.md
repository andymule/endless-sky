# AdaptiveMusic Library for Endless Sky

This is a prototype implementation of an adaptive music system for Endless Sky. 

## Current Functionality

The current implementation provides a simple wrapper around Endless Sky's existing Audio system to demonstrate the integration of adaptive music in the MenuPanel. It offers:

- Basic music playback (currently uses Endless Sky's Audio system)
- Music looping
- Volume control (through Endless Sky's Audio system)
- Play/pause/stop functionality

## Usage

```cpp
// Create an instance of AdaptiveMusic
AdaptiveMusic music;

// Initialize the audio system
if (music.Initialize())
{
    // Load a music file
    std::filesystem::path musicPath = Files::Resources() / "sounds" / "music" / "main_menu.mp3";
    if (music.LoadMusic(musicPath.string()))
    {
        // Play the music with looping
        music.PlayMusicLooped(0.8f);
    }
}

// Stop the music when you're done
music.StopMusic();
```

## Integration Example

The MenuPanel class now uses AdaptiveMusic to play the main menu music. See `source/MenuPanel.cpp` for an example of how to integrate AdaptiveMusic into your panels.

## Future Plans

This library is a starting point for a more sophisticated adaptive music system. Future enhancements could include:

1. **SoLoud Integration**: Replace Endless Sky's current audio system with SoLoud for more advanced audio capabilities.
2. **Layered Music**: Support for multiple layers of music that can fade in/out based on game state.
3. **Combat Music**: Dynamic transition to combat music when entering battles.
4. **Mood-based System**: Music that changes based on player actions, location, and story events.
5. **Cross-fading**: Smooth transitions between different music tracks.
6. **Per-system Music**: Different ambient music for different star systems.
7. **Event-triggered Music**: Special music for significant events in the game.

## Implementation Notes

The current implementation is a simple wrapper around Endless Sky's existing Audio system. A full implementation would use SoLoud directly, but this would require more complex integration with the build system.

## Credits

- Endless Sky: https://endless-sky.github.io/
- SoLoud Audio Library: https://sol.gfxile.net/soloud/ 