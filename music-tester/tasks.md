# Event-Driven Song Format Implementation

## 🎯 **Overview**

Implement the event-driven song format specification on top of the existing AudioController/AudioState/AudioSystem architecture. Songs are folders with OGG tracks + JSON metadata, triggerable via external events.

## 🏗️ **Current Architecture Strengths**

**Already Have:**
- **AudioController**: Perfect MVC controller layer
- **AudioState**: State management with change notifications  
- **AudioSystem**: Complete SoLoud integration with all filters
- **TesterView**: ImGui UI framework
- **Dual Tempo Control**: Master tempo + granular tempo [per memory][[memory:7578384773746369645]]

**Need to Add:**
- Song/Event data structures
- JSON loading system
- Event triggering API
- Events UI window

## 📋 **Implementation Tasks**

### **Task 1: Core Data Structures**

**File**: `src/AudioState.h` - Extend existing state

```cpp
// Add to AudioState.h
struct EffectState {
    bool enabled = false;
    std::map<std::string, float> parameters;
};

struct TrackState {
    std::string file;
    float volume = 1.0f;
    bool active = true;
    std::map<std::string, EffectState> effects;
    // Keep existing fields for backward compatibility
};

struct StateSnapshot {
    float masterTempo = 1.0f;
    float granularTempo = 1.0f;
    std::vector<TrackState> tracks;
};

struct SongEvent {
    std::string name;
    float fadeTime;
    StateSnapshot state;
};

struct Song {
    std::string name;
    std::vector<SongEvent> events;
    std::filesystem::path folderPath;
};

struct MasterBusState {
    float masterTempo = 1.0f;
    float granularTempo = 1.0f;
    float volume = 1.0f;
    std::map<std::string, EffectState> effects;
};

struct MasterEvent {
    std::string name;
    float fadeTime;
    MasterBusState state;
};

struct MasterBus {
    std::vector<MasterEvent> events;
};
```

### **Task 2: Song Loading System**

**File**: `src/SongManager.h` (new) + `src/SongManager.cpp` (new)

```cpp
class SongManager {
public:
    // Core loading
    bool loadSong(const std::filesystem::path& songFolder);
    bool loadMasterBus(const std::filesystem::path& masterJsonPath);
    
    // Auto-discovery
    std::vector<std::string> discoverTracks(const std::filesystem::path& folder);
    
    // Access
    const std::vector<Song>& getSongs() const { return m_songs; }
    const MasterBus& getMasterBus() const { return m_masterBus; }
    
private:
    std::vector<Song> m_songs;
    MasterBus m_masterBus;
    
    // JSON parsing helpers
    bool parseStateSnapshot(const nlohmann::json& json, StateSnapshot& state);
    bool parseMasterBusState(const nlohmann::json& json, MasterBusState& state);
};
```

**Implementation:**
- Use existing `AudioController::isSupportedFile()` logic
- Only load `.ogg` files as specified
- JSON parsing with nlohmann::json (already used?)
- Graceful error handling - log and continue

### **Task 3: Event System**

**File**: `src/EventSystem.h` (new) + `src/EventSystem.cpp` (new)

```cpp
class EventSystem {
public:
    EventSystem(AudioController* controller) : m_controller(controller) {}
    
    // External API (called by game engines)
    void triggerSongEvent(const std::string& songName, const std::string& eventName);
    void triggerMasterEvent(const std::string& eventName);
    
    // State transitions with lerping
    void update(float deltaTime);
    
private:
    AudioController* m_controller;
    
    // Transition state
    bool m_inTransition = false;
    float m_transitionTime = 0.0f;
    float m_targetTime = 0.0f;
    StateSnapshot m_startState;
    StateSnapshot m_targetState;
    
    void startTransition(const StateSnapshot& target, float fadeTime);
    void lerpStates(float t);
};
```

**Implementation:**
- Linear interpolation for volume, tempo, effect parameters
- Use existing `AudioController` methods for applying changes
- No complex buffering - leverage existing architecture

### **Task 4: Extend AudioController**

**File**: `src/AudioController.h` + `src/AudioController.cpp`

```cpp
// Add to AudioController class
class AudioController {
    // ... existing methods ...
    
    // Song management
    void loadSongsFromDirectory(const std::string& directory);
    void setCurrentSong(const std::string& songName);
    
    // Event triggering (external API)
    void triggerSongEvent(const std::string& songName, const std::string& eventName);
    void triggerMasterEvent(const std::string& eventName);
    
    // Effect automation
    void setTrackEffectEnabled(size_t trackIndex, const std::string& effectName, bool enabled);
    void setTrackEffectParameter(size_t trackIndex, const std::string& effectName, 
                                const std::string& paramName, float value);
                                
private:
    SongManager m_songManager;
    EventSystem m_eventSystem;
    std::string m_currentSongName;
};
```

**Implementation:**
- Extend existing directory loading to look for `_song.json` files
- Route event calls to EventSystem
- Map effect parameter names to AudioSystem parameter IDs

### **Task 5: Events UI Window**

**File**: `src/TesterView.cpp` - Add new window

```cpp
// Add to TesterView class
private:
    void RenderEventsWindow();
    void RenderSongEvents(const Song& song);
    void RenderMasterEvents();
    
    // Event creation
    void ShowCreateEventDialog();
    StateSnapshot CaptureCurrentState();
    
    // UI state
    bool m_showEventsWindow = true;
    bool m_showCreateEventDialog = false;
    char m_newEventName[256] = "";
    float m_newEventFadeTime = 1.0f;
```

**Implementation:**
- New ImGui window alongside main window
- List of songs with expandable event lists
- Buttons to trigger events
- "Capture Current State" workflow for creating events
- Master bus events section

### **Task 6: Integration & External API**

**File**: `src/main.cpp` - Expose external API

```cpp
// C-style API for game engine integration
extern "C" {
    void musicTester_triggerSongEvent(const char* songName, const char* eventName);
    void musicTester_triggerMasterEvent(const char* eventName);
}

// Implementation routes to singleton AudioController instance
```

## 🔧 **Implementation Strategy**

### **Phase 1: Core Foundation** 
1. Extend AudioState with new data structures
2. Create SongManager with basic JSON loading
3. Test loading songs from `sound_staging/` folder structure

### **Phase 2: Event System** 
1. Implement EventSystem with state transitions
2. Extend AudioController with song/event methods
3. Test event triggering and state lerping

### **Phase 3: UI Integration** 
1. Add Events window to TesterView
2. Implement event creation workflow
3. Connect UI to event triggering

### **Phase 4: External API**
1. Add C-style API functions
2. Test external triggering
3. Documentation and examples

## 📁 **Expected File Structure**

```
sound_staging/
├── _master.json           # Master bus events
├── song-1/
│   ├── _song.json        # Song metadata + events  
│   ├── drums.ogg
│   └── bass.ogg
└── song-2/
    ├── _song.json
    └── melody.ogg
```

## ✅ **Success Criteria**

1. **Load songs** from folder structure with JSON metadata
2. **Trigger events** externally and see smooth state transitions
3. **UI workflow** for creating/managing events
4. **Backward compatibility** with existing single-directory loading
5. **External API** working for game engine integration

## 🎯 **Key Design Principles**

- **Leverage existing architecture** - minimal changes to working code
- **Graceful error handling** - bad JSON shouldn't crash the app  
- **No over-engineering** - simple state snapshots and linear interpolation
- **External triggering only** - no timeline/auto-playback complexity

This plan builds directly on your solid foundation and delivers the event-driven format with minimal complexity. 