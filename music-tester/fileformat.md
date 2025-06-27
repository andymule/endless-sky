# Dynamix Music Tester File Format

## Overview

The Dynamix Music Tester uses a folder-based structure where each project contains a master bus and multiple songs. Songs are identified by their folder names, not by any JSON field.

## Project Structure

```
project-folder/
├── _master.json          # Master bus configuration
├── song1/                # Song folder (song name = "song1")
│   ├── _song.json        # Song events and track states
│   ├── track1.ogg        # Audio files
│   └── track2.ogg
└── song2/                # Another song folder (song name = "song2")
    ├── _song.json
    └── track3.ogg
```

## Song JSON Format

Songs are stored in `_song.json` files within song folders. The song name is the folder name, not a JSON field.

```json
{
  "events": [
    {
      "name": "Event Name",
      "fadeTime": 1.0,
      "state": {
        "masterTempo": 1.0,
        "granularTempo": 1.0,
        "tracks": [
          {
            "file": "track1.ogg",
            "volume": 1.0,
            "effects": {
              "reverb": {
                "parameters": {
                  "0": 0.5,
                  "1": 0.3
                }
              }
            }
          }
        ]
      }
    }
  ]
}
```

### Song JSON Fields

- **events** (array): List of song events
  - **name** (string): Event name
  - **fadeTime** (float): Fade-in duration in seconds
  - **state** (object): Complete song state
    - **masterTempo** (float): Song tempo multiplier
    - **granularTempo** (float): Granular tempo control
    - **tracks** (array): Track states
      - **file** (string): Track filename (must exist in folder)
      - **volume** (float): Track volume (0.0-1.0)
      - **effects** (object): Effect configurations

## Master Bus JSON Format

The master bus is stored in `_master.json` at the project root.

```json
{
  "name": "Master Bus",
  "events": [
    {
      "name": "Master Event",
      "fadeTime": 1.0,
      "state": {
        "masterTempo": 1.0,
        "granularTempo": 1.0,
        "bus": {
          "volume": 1.0,
          "effects": {
            "reverb": {
              "parameters": {
                "0": 0.3
              }
            }
          }
        }
      }
    }
  ]
}
```

## Important Notes

1. **Song names are folder names**: Songs are identified by their folder names, not by any JSON field.
2. **Folder-driven workflow**: The GUI shows tracks based on actual .ogg files in the folder, not from JSON.
3. **Auto-sync**: When loading songs, tracks missing from the folder are removed from events, and new tracks are added with default settings.
4. **No song name field**: The `name` field in song JSON is deprecated and ignored.

# Event-Driven Song Format Specification

## Overview

This document specifies an event-driven song format for the music-tester system. Songs are folders containing loose OGG audio track files and a JSON metadata file. All tracks loop automatically and are always playing - volume control is used to fade tracks in/out. The system supports state-persistent events triggered externally by game engines, with complete automation of all audio effects and parameters.

**Key Architecture:**
- Songs contain tracks with per-song tempo control
- Master bus handles global effects and master-level tempo control  
- No bus per-song - bus exists only at master level
- All audio formats must be OGG Vorbis
- **All tracks loop continuously and are always active** - use volume=0 to "disable" tracks

## Current Architecture Integration

The existing codebase provides these components to leverage:
- **AudioController**: MVC controller for business logic integration
- **AudioState**: State management with change notifications - extend for complete snapshots
- **AudioSystem**: SoLoud-based audio with comprehensive filter support (all automatable)
- **TesterView**: ImGui UI - add Events window per song

## File Structure

```
sound_staging/
├── _master.json           # Master bus events (REQUIRED - global effects)
├── song-name-1/
│   ├── _song.json         # Song definition
│   ├── drums.ogg          # Audio tracks (song-specific)
│   ├── bass.ogg
│   └── melody.ogg
├── song-name-2/
│   ├── _song.json
│   ├── track1.ogg
│   └── track2.ogg
└── song-name-3/
    ├── _song.json
    └── track1.ogg
```

**Important Notes:**
- **`_master.json` is REQUIRED** in the root directory
- **Songs are loaded as holistic entities** - each song folder contains its own tracks
- **No global tracks folder** - all tracks belong to specific songs
- **Track filename conflicts between songs are allowed** - each song is independent
- **All `.ogg` files in song folders are loaded** when that song is loaded

## JSON Format Specification

### Song JSON (`_song.json`)

```json
{
  "name": "Song Name", //meta data, unused by system. use foldername as UID
  "events": [
    {
      "name": "Intro",
      "fadeTime": 2.0,
      "state": {
        "masterTempo": 1.0,
        "granularTempo": 1.0,
        "tracks": [
          {
            "file": "drums.ogg",
            "volume": 0.8,
            "effects": {
              "BassBoost": {
                "parameters": {
                  "0": 1.0,
                  "1": 3.0
                }
              },
              "Echo": {
                "parameters": {
                  "0": 0.5,
                  "1": 0.3,
                  "2": 0.5,
                  "3": 0.0
                }
              }
            }
          },
          {
            "file": "bass.ogg", 
            "volume": 0.0,
            "effects": {}
          }
        ]
      }
    },
    {
      "name": "Verse",
      "fadeTime": 1.5,
      "state": {
        // Complete state snapshot for this event
        // Only changed parameters need lerping
      }
    }
  ]
}
```

**Important Notes:**
- **No `active` field**: Tracks are always active and looping
- **Volume=0**: Used to "disable" tracks (fade to silence)
- **Effect parameters**: Stored by ID as strings (e.g., `"0"` = wet level, `"1"` = first parameter)
- **Wet level logic**: Only effects with wet level > 0 are stored in snapshots
- **Effect names preserved**: Effect names like "BassBoost", "Echo" are stored in JSON
- **Parameter names not preserved**: Only parameter IDs (0, 1, 2, etc.) are stored, not human-readable names

### Master Bus JSON (`_master.json`)

```json
{
  "name": "Master Bus",
  "events": [
    {
      "name": "Normal",
      "fadeTime": 0.0,
      "state": {
        "masterTempo": 1.0,
        "granularTempo": 1.0,
        "bus": {
          "volume": 1.0,
          "effects": {
            "DCRemoval": {
              "parameters": {
                "0": 1.0
              }
            }
          }
        }
      }
    },
    {
      "name": "Underwater",
      "fadeTime": 3.0,
      "state": {
        "masterTempo": 0.7,
        "granularTempo": 0.8,
        "bus": {
          "volume": 0.6,
          "effects": {
            "DCRemoval": {
              "parameters": {
                "0": 1.0
              }
            },
            "LoFi": {
              "parameters": {
                "0": 0.8,
                "1": 8000,
                "2": 3
              }
            }
          }
        }
      }
    }
  ]
}
```

## Core Data Structures

Extend existing AudioState with complete snapshot capability:

```cpp
// Complete state snapshot for events
struct StateSnapshot {
    float masterTempo = 1.0f;        // Per-song tempo control
    float granularTempo = 1.0f;      // Per-song granular tempo control
    std::vector<TrackState> tracks;
    // No bus - bus only exists at master level
};

// Enhanced track state with complete effect automation
struct TrackState {
    std::string file;           // Track filename
    float volume = 1.0f;        // 0.0 = "disabled" (silent), 1.0 = full volume
    // All tracks loop automatically - no looping field needed
    // All tracks are always active - no active field needed
    std::map<std::string, EffectState> effects;
};

// Complete effect state with all parameters
struct EffectState {
    std::map<std::string, float> parameters;  // Parameter ID -> value mapping
    // No enabled field - wet level (parameter "0") determines if effect is active
};

// Master bus state with effects and tempo (only exists at master level)
struct MasterBusState {
    float masterTempo = 1.0f;        // Master-level tempo control
    float granularTempo = 1.0f;      // Master-level granular tempo control
    float volume = 1.0f;
    std::map<std::string, EffectState> effects;
};

// Song event definition
struct SongEvent {
    std::string name;
    float fadeTime;           // Fade-in duration in seconds
    StateSnapshot state;      // Complete song state (tracks + tempo)
};

// Master bus event definition  
struct MasterEvent {
    std::string name;
    float fadeTime;           // Fade-in duration in seconds
    MasterBusState state;     // Complete master bus state (effects + tempo)
};

// Song container
struct Song {
    std::string name;
    std::vector<SongEvent> events;
    std::filesystem::path folderPath;
};

// Master bus container (separate from songs)
struct MasterBus {
    std::string name;
    std::vector<MasterEvent> events;  // Events containing MasterBusState and tempo
};
```

## Tempo Control Architecture

The system supports dual-level tempo control:

**Per-Song Tempo** (in song events):
- `masterTempo`: Affects playback speed of all tracks in the song
- `granularTempo`: Pitch-preserving tempo adjustment for tracks in the song
- Applied before master bus processing

**Master-Level Tempo** (in master bus events):
- `masterTempo`: Global tempo multiplier applied to all audio
- `granularTempo`: Global pitch-preserving tempo adjustment
- Applied after all song processing

**Combined Effect**: Final tempo = songTempo × masterTempo (same for granular)

## Wet Level Behavior

The system uses sophisticated wet level tracking for smooth effect transitions:

### Effect State Capture
- **Only effects with wet level > 0 are stored** in state snapshots
- **Wet level is always parameter ID "0"** for all effects
- **All parameters are captured** when wet level > 0, including the wet level itself
- **Effects with wet level = 0 are not stored** in snapshots (saves space and complexity)

### Transition Behavior
- **Current wet > 0, Target wet > 0**: Smooth lerp of all parameters including wet
- **Current wet = 0, Target wet > 0**: Start from 0 wet, lerp up to target
- **Current wet > 0, Target wet = 0**: Lerp wet down to 0, effectively disable effect
- **Current wet = 0, Target wet = 0**: No change needed

### Parameter Storage
- **Parameters stored by ID as strings** (e.g., `"0"`, `"1"`, `"2"`) for JSON compatibility
- **Parameter ID "0" is always the wet level** for all effects
- **Other parameter IDs vary by effect type** (see Available Effects section)

## Available Effects and Parameters

Based on existing AudioSystem, all effects are automatable:

```cpp
// Filter types with their parameters (ID -> description)
"BassBoost":           { "0": "wet", "1": "boost [0.0, 11.0]" }
"BiquadResonant":      { "0": "wet", "1": "frequency [0.0, 8000.0]", "2": "resonance [1.0, 20.0]", "3": "type [0, 3]" }
"DCRemoval":           { "0": "wet" }
"Echo":                { "0": "wet", "1": "delay [0.0, 1.0]", "2": "decay [0.0, 1.0]", "3": "filter [0.0, 1.0]" }
"Flanger":             { "0": "wet", "1": "delay [0.0005, 0.01]", "2": "freq [0.1, 10.0]" }
"Freeverb":            { "0": "wet", "1": "wet [0.0, 1.0]", "2": "roomsize [0.0, 1.0]", "3": "damp [0.0, 1.0]", "4": "width [0.0, 1.0]" }
"LoFi":                { "0": "wet", "1": "samplerate [1000, 8000]", "2": "bitdepth [1, 8]" }
"Robotize":            { "0": "wet", "1": "freq [1.0, 30.0]", "2": "wave [0, 5]" }
"WaveShaper":          { "0": "wet", "1": "amount [-1.0, 1.0]" }
```

## Implementation Requirements

### 1. Song Loading System

```cpp
class SongManager {
public:
    // Load songs from directory structure
    void loadSongsFromDirectory(const std::string& directory);
    
    // Load individual song (for song switching)
    bool loadSong(const std::filesystem::path& songFolder);
    
    // Load master bus configuration (REQUIRED)
    bool loadMasterBus(const std::filesystem::path& masterJsonPath);
    
    // File discovery - loads ALL OGG files in song folder
    std::vector<std::string> discoverTracks(const std::filesystem::path& folder);
    
    // Create new master directory with empty _master.json
    bool createNewMasterDirectory(const std::string& directoryName);
    
    // Create new song folder with empty _song.json
    bool createNewSongFolder(const std::string& songName);
    
    // Validation: log errors but don't crash
    bool validateSongJson(const nlohmann::json& json);
};
```

### 2. Event System

```cpp
class EventSystem {
public:
    // Trigger event by name (called from game engine)
    void triggerSongEvent(const std::string& songName, const std::string& eventName);
    void triggerMasterEvent(const std::string& eventName);
    
    // State transition with lerping
    void startSongTransition(const StateSnapshot& target, float fadeTime);
    void startMasterTransition(const MasterBusState& target, float fadeTime);
    void updateTransitions(float deltaTime);
    
    // Song switching (resets tracks & effects, preserves master bus)
    void switchToSong(const std::string& songName);
    
    // State diffing for UI
    StateSnapshot createSongStateDiff(const StateSnapshot& current, const StateSnapshot& target);
    MasterBusState createMasterStateDiff(const MasterBusState& current, const MasterBusState& target);
};
```

### 3. Events UI Window

```cpp
class EventsWindow {
public:
    // Render events list per song and master
    void renderSongEvents(Song& song);
    void renderMasterEvents(MasterBus& masterBus);
    
    // Song browser (new feature)
    void renderSongBrowser();
    void renderActiveSongIndicator();
    
    // Event creation workflow
    void showCreateSongEventDialog();
    void showCreateMasterEventDialog();
    StateSnapshot captureCurrentSongState();
    MasterBusState captureCurrentMasterState();
    void commitNewSongEvent(const std::string& name, float fadeTime);
    void commitNewMasterEvent(const std::string& name, float fadeTime);
    
    // Event management
    void renderDraggableSongEventList(std::vector<SongEvent>& events);
    void renderDraggableMasterEventList(std::vector<MasterEvent>& events);
    void reorderSongEvents(std::vector<SongEvent>& events, int oldIndex, int newIndex);
    void reorderMasterEvents(std::vector<MasterEvent>& events, int oldIndex, int newIndex);
    
    // CRUD operations
    void deleteSongEvent(const std::string& songName, const std::string& eventName);
    void deleteMasterEvent(const std::string& eventName);
    void saveSongEvent(const std::string& songName, const std::string& eventName);
    void saveMasterEvent(const std::string& eventName);
};
```

### 4. File Menu System

```cpp
class FileMenu {
public:
    // File menu operations
    void showFileMenu();
    void createNewMaster();
    void createNewSong();
    void loadDirectory();
    
    // Template creation
    void createEmptyMasterJson(const std::filesystem::path& path);
    void createEmptySongJson(const std::filesystem::path& path);
    
    // Directory management
    bool validateDirectoryStructure(const std::filesystem::path& path);
    std::vector<std::filesystem::path> discoverSongFolders(const std::filesystem::path& root);
};
```

### 5. State Persistence

- **All events modify state permanently** - no reset to original
- **Event order in JSON = display order** - no additional metadata needed
- **Complete state snapshots** - simple diffing and lerping
- **External triggering only** - no timeline/timestamp system
- **Song switching resets tracks & effects** but preserves master bus and tempo
- **Master bus state persists** across song switches

### 6. Error Handling

```cpp
// File validation - fail gracefully
if (!loadSong(path)) {
    logError("Failed to load song: " + path.string());
    return false; // Don't crash, continue with other songs
}

// Track file missing
if (!std::filesystem::exists(trackPath)) {
    logError("Track file not found: " + trackPath.string());
    // Skip track but continue loading song
}

// JSON parsing errors
try {
    file >> json;
} catch (const std::exception& e) {
    logError("JSON parsing error: " + std::string(e.what()));
    return false; // Skip this file, continue with others
}

// Parameter validation
if (paramValue < minValue || paramValue > maxValue) {
    logError("Parameter out of range: " + paramName + " = " + std::to_string(paramValue));
    // Use default value and continue
}

// Transition failures
if (!startTransition(targetState)) {
    logError("Failed to start transition to: " + targetState.name);
    // Keep current state, don't crash
}
```

**Error Recovery Strategy:**
- **Graceful degradation**: Continue operation even if some files fail to load
- **Default values**: Use sensible defaults for invalid parameters
- **Logging**: Comprehensive error logging for debugging
- **User feedback**: Clear error messages in UI when appropriate
- **Retry mechanisms**: Automatic retry for transient file system errors

## Integration Points

### Extend Existing Classes

1. **AudioController**: Add event triggering methods and song switching
2. **AudioState**: Extend with complete snapshot capabilities  
3. **TesterView**: Add File menu and Events window with song browser
4. **AudioSystem**: Ensure all effects expose their parameters for automation

### External API

Game engines will call:
```cpp
// Trigger song events (affects tracks + per-song tempo)
musicTester.triggerSongEvent("battle-theme", "intense");
musicTester.triggerSongEvent("ambient-forest", "night-cycle");

// Trigger master bus events (affects global effects + master tempo)
musicTester.triggerMasterEvent("underwater");
musicTester.triggerMasterEvent("normal");

// Switch to different song (new feature)
musicTester.switchToSong("new-song-name");
```

## File Loading Priority

1. **Validate `_master.json` exists** (REQUIRED) in root directory
2. **Scan subdirectories for `_song.json` files** to discover available songs
3. **Load master bus configuration** from `_master.json`
4. **Load currently active song** (tracks + events)
5. **Auto-discover ALL `.ogg` files** in active song folder
6. **Validate and load**, logging errors for failures
7. **Build UI** with Events window and song browser

This specification provides complete state automation, external triggering, song switching, and seamless integration with the existing AudioController/AudioState/AudioSystem architecture. 