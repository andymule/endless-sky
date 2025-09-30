# Dynamix External API Usage Guide

This guide shows how to integrate Dynamix event-driven music system into your game engine.

## Quick Start

```c
#include "music_tester_api.h"

int main() {
    // 1. Initialize Dynamix
    DynamixErrorCode result = dynamix_initialize(NULL);
    if (result != DYNAMIX_SUCCESS) {
        printf("Failed to initialize: %s\n", dynamix_getLastError());
        return -1;
    }
    
    // 2. Load music from directory
    result = dynamix_loadSongsFromDirectory("/path/to/music");
    if (result != DYNAMIX_SUCCESS) {
        printf("Failed to load music: %s\n", dynamix_getLastError());
        dynamix_shutdown();
        return -1;
    }
    
    // 3. Trigger events in your game loop
    if (player_enters_combat) {
        result = dynamix_triggerEvent("combat");
        if (result != DYNAMIX_SUCCESS) {
            printf("Warning: %s\n", dynamix_getLastError());
        }
    }
    
    if (player_in_peaceful_area) {
        result = dynamix_triggerEvent("peaceful");
        if (result != DYNAMIX_SUCCESS) {
            printf("Warning: %s\n", dynamix_getLastError());
        }
    }
    
    // 4. Shutdown when done
    dynamix_shutdown();
    return 0;
}
```

## Error Handling

**Always check return values!** All functions return error codes:

```c
// ❌ BAD - Silent failures
dynamix_triggerEvent("boss_fight");

// ✅ GOOD - Proper error handling  
DynamixErrorCode result = dynamix_triggerEvent("boss_fight");
if (result != DYNAMIX_SUCCESS) {
    printf("Music error: %s\n", dynamix_getLastError());
    // Handle error appropriately for your game
}
```

## Error Codes

| Code | Description | Common Causes |
|------|-------------|---------------|
| `DYNAMIX_SUCCESS` | Operation successful | - |
| `DYNAMIX_ERROR_NULL_PARAM` | NULL parameter passed | Passing NULL to required parameters |
| `DYNAMIX_ERROR_EMPTY_PARAM` | Empty string parameter | Passing "" to string parameters |
| `DYNAMIX_ERROR_NOT_INITIALIZED` | Dynamix not initialized | Forgetting to call `dynamix_initialize()` |
| `DYNAMIX_ERROR_SONG_NOT_FOUND` | Song doesn't exist | Typo in song name |
| `DYNAMIX_ERROR_EVENT_NOT_FOUND` | Event doesn't exist | Event not defined in JSON |
| `DYNAMIX_ERROR_INVALID_DIRECTORY` | Directory invalid | Path doesn't exist or not a directory |
| `DYNAMIX_ERROR_PARAMETER_TOO_LONG` | Parameter exceeds 1024 chars | Very long file paths |
| `DYNAMIX_ERROR_INTERNAL` | Internal system error | System/audio driver issues |

## API Functions

### Initialization

```c
// Initialize Dynamix (call first)
DynamixErrorCode dynamix_initialize(const char* executableDirectory);

// Check if initialized
int dynamix_isInitialized(void);  // Returns 1 if ready, 0 if not

// Shutdown (call when done)
void dynamix_shutdown(void);
```

### Loading Music

```c
// Load all songs and events from a directory
DynamixErrorCode dynamix_loadSongsFromDirectory(const char* directory);

// Switch to a specific song
DynamixErrorCode dynamix_loadAndPlaySong(const char* songName);
```

### Triggering Events

```c
// Trigger any event (recommended - searches songs and master)
DynamixErrorCode dynamix_triggerEvent(const char* eventName);

// Trigger specific song event  
DynamixErrorCode dynamix_triggerSongEvent(const char* songName, const char* eventName);

// Trigger master bus event (affects all music)
DynamixErrorCode dynamix_triggerMasterEvent(const char* eventName);
```

### Error Management

```c
// Get last error message
const char* dynamix_getLastError(void);

// Get last error code
DynamixErrorCode dynamix_getLastErrorCode(void);

// Clear error state
void dynamix_clearError(void);
```

## Integration Patterns

### Game State-Driven Music

```c
void update_music_for_game_state(GameState state) {
    switch (state) {
        case GAME_STATE_MENU:
            dynamix_triggerEvent("menu_music");
            break;
        case GAME_STATE_EXPLORING:
            dynamix_triggerEvent("exploration");
            break;
        case GAME_STATE_COMBAT:
            dynamix_triggerEvent("combat_intense");
            break;
        case GAME_STATE_BOSS_FIGHT:
            dynamix_triggerEvent("boss_battle");
            break;
    }
}
```

### Gradual Music Transitions

```c
// Dynamix handles smooth transitions automatically
void enter_dungeon() {
    // Fade from exploration to dungeon ambience
    dynamix_triggerEvent("dungeon_entrance");
}

void approach_boss() {
    // Gradually increase intensity
    dynamix_triggerEvent("boss_approach");
}

void start_boss_fight() {
    // Full intensity boss music
    dynamix_triggerEvent("boss_battle");
}
```

### Error Recovery Strategies

```c
void robust_music_trigger(const char* event_name) {
    DynamixErrorCode result = dynamix_triggerEvent(event_name);
    
    switch (result) {
        case DYNAMIX_SUCCESS:
            // All good!
            break;
            
        case DYNAMIX_ERROR_EVENT_NOT_FOUND:
            // Log warning but continue - music is not critical
            log_warning("Music event not found: %s", event_name);
            break;
            
        case DYNAMIX_ERROR_NOT_INITIALIZED:
            // Try to recover
            log_error("Music not initialized, attempting recovery...");
            if (dynamix_initialize(NULL) == DYNAMIX_SUCCESS) {
                dynamix_loadSongsFromDirectory(get_music_directory());
                dynamix_triggerEvent(event_name); // Retry
            }
            break;
            
        default:
            // Log error but don't crash the game
            log_error("Music system error: %s", dynamix_getLastError());
            break;
    }
}
```

## Directory Structure

Your music directory should be organized like this:

```
music_directory/
├── _master.json          # Global effects and events
├── combat_theme/         # Song folder
│   ├── _song.json       # Song events definition
│   ├── drums.ogg        # Audio tracks
│   ├── bass.ogg
│   └── melody.ogg
├── peaceful_theme/
│   ├── _song.json
│   ├── ambient.ogg
│   └── birds.ogg
└── boss_battle/
    ├── _song.json
    ├── heavy_drums.ogg
    ├── intense_bass.ogg
    └── boss_melody.ogg
```

## Performance Notes

- ✅ **Event triggering is fast** - safe to call every frame if needed
- ✅ **Automatic fade transitions** - no need to manage crossfades manually
- ✅ **Thread-safe** - can be called from any thread
- ⚠️ **Loading is expensive** - load music at level start, not during gameplay
- ⚠️ **Parameter validation overhead** - strings are validated on every call

## Troubleshooting

**"Parameter 'eventName' is NULL"**
- Check that you're passing valid string pointers
- Use `if (event_name != NULL)` checks in your code

**"Dynamix not initialized"**
- Call `dynamix_initialize()` before any other functions
- Check that initialization succeeded before proceeding

**"Event not found"** 
- Verify event names match exactly (case-sensitive)
- Check that your `_song.json` and `_master.json` files are valid
- Use the console log feature in Dynamix to debug JSON issues

**"Directory does not exist"**
- Use absolute paths when possible
- Verify directory exists and is readable
- Check permissions on the music directory

## Integration Checklist

- [ ] Include `music_tester_api.h` in your project
- [ ] Link with Dynamix library/executable
- [ ] Call `dynamix_initialize()` at startup
- [ ] Load music with `dynamix_loadSongsFromDirectory()`
- [ ] Add `dynamix_triggerEvent()` calls to game state changes
- [ ] Check return values and handle errors appropriately
- [ ] Call `dynamix_shutdown()` at exit
- [ ] Test with invalid parameters to verify error handling

This API provides robust, production-ready music integration with comprehensive error handling and validation.