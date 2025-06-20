# Event-Driven Song Format Specification

## Overview

This document specifies an event-driven song format for the music-tester system. Songs are folders containing loose OGG audio track files and a JSON metadata file. All tracks loop automatically. The system supports state-persistent events triggered externally by game engines, with complete automation of all audio effects and parameters.

**Key Architecture:**
- Songs contain tracks with per-song tempo control
- Master bus handles global effects and master-level tempo control  
- No bus per-song - bus exists only at master level
- All audio formats must be OGG Vorbis

## Current Architecture Integration

The existing codebase provides these components to leverage:
- **AudioController**: MVC controller for business logic integration
- **AudioState**: State management with change notifications - extend for complete snapshots
- **AudioSystem**: SoLoud-based audio with comprehensive filter support (all automatable)
- **TesterView**: ImGui UI - add Events window per song

## File Structure

```
songs/
├── song-name-1/
│   ├── _song.json         # Song metadata + events (underscore prefix for top sorting)
│   ├── drums.ogg          # Loose track files
│   ├── bass.ogg
│   └── melody.ogg
├── song-name-2/
│   ├── _song.json
│   ├── track1.ogg
│   └── track2.ogg
└── _master.json           # Master bus events (global effects, no tracks)
``` (we only support ogg)

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
            "active": true,
            "effects": {
              "BassBoost": {
                "enabled": true,
                "parameters": {
                  "boost": 3.0
                }
              },
              "Echo": {
                "enabled": false,
                "parameters": {
                  "delay": 0.3,
                  "decay": 0.5,
                  "filter": 0.0
                }
              }
            }
          },
          {
            "file": "bass.ogg", 
            "volume": 1.0,
            "active": false,
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
              "enabled": true,
              "parameters": {}
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
              "enabled": true,
              "parameters": {}
            },
            "LoFi": {
              "enabled": true,
              "parameters": {
                "samplerate": 8000,
                "bitdepth": 3
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
    float volume = 1.0f;
    bool active = true;
    // All tracks loop automatically - no looping field needed
    std::map<std::string, EffectState> effects;
};

// Complete effect state with all parameters
struct EffectState {
    bool enabled = false;
    std::map<std::string, float> parameters;
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

## Available Effects and Parameters

Based on existing AudioSystem, all effects are automatable:

```cpp
// Filter types with their parameters
"BassBoost":           { "boost": [0.0, 11.0] }
"BiquadResonant":      { "frequency": [0.0, 8000.0], "resonance": [1.0, 20.0], "type": [0, 3] }
"DCRemoval":           { /* no parameters */ }
"Echo":                { "delay": [0.0, 1.0], "decay": [0.0, 1.0], "filter": [0.0, 1.0] }
"Flanger":             { "delay": [0.0005, 0.01], "freq": [0.1, 10.0] }
"Freeverb":            { "wet": [0.0, 1.0], "roomsize": [0.0, 1.0], "damp": [0.0, 1.0], "width": [0.0, 1.0] }
"LoFi":                { "samplerate": [1000, 8000], "bitdepth": [1, 8] }
"Robotize":            { "freq": [1.0, 30.0], "wave": [0, 5] }
"WaveShaper":          { "amount": [-1.0, 1.0] }
```

## Implementation Requirements

### 1. Song Loading System

```cpp
class SongManager {
public:
    // Load song from folder - discover tracks automatically
    bool loadSong(const std::filesystem::path& songFolder);
    
    // Load master bus configuration
    bool loadMasterBus(const std::filesystem::path& masterJsonPath);
    
    // File discovery
    std::vector<std::string> discoverTracks(const std::filesystem::path& folder);
    
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
    void startSongTransition(const StateSnapshot& targetState, float fadeTime);
    void startMasterTransition(const MasterBusState& targetState, float fadeTime);
    void updateTransitions(float deltaTime);
    
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
    
    // Event creation workflow
    void showCreateSongEventDialog();
    void showCreateMasterEventDialog();
    StateSnapshot captureCurrentSongState();
    MasterBusState captureCurrentMasterState();
    void commitNewSongEvent(const std::string& name, float fadeTime);
    void commitNewMasterEvent(const std::string& name, float fadeTime);
    
    // List management
    void renderDraggableSongEventList(std::vector<SongEvent>& events);
    void renderDraggableMasterEventList(std::vector<MasterEvent>& events);
    void reorderSongEvents(std::vector<SongEvent>& events, int oldIndex, int newIndex);
    void reorderMasterEvents(std::vector<MasterEvent>& events, int oldIndex, int newIndex);
};
```

### 4. State Persistence

- **All events modify state permanently** - no reset to original
- **Event order in JSON = display order** - no additional metadata needed
- **Complete state snapshots** - simple diffing and lerping
- **External triggering only** - no timeline/timestamp system

### 5. Error Handling

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
```

## Integration Points

### Extend Existing Classes

1. **AudioController**: Add event triggering methods
2. **AudioState**: Extend with complete snapshot capabilities  
3. **TesterView**: Add Events window for each loaded song
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
```

## File Loading Priority

1. Scan for `_master.json` first (global effects)
2. Scan folders for `_song.json` files  
3. Auto-discover loose track files in each song folder
4. Validate and load, logging errors for failures
5. Build UI with Events window per loaded song

This specification provides complete state automation, external triggering, and seamless integration with the existing AudioController/AudioState/AudioSystem architecture. 