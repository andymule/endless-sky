# Event-Driven Song Format Implementation - UPDATED STATUS

## 🎯 **Overview**

**STATUS: ~80% COMPLETE** - The core event-driven song format system is fully implemented. Only UI integration and external API remain.

Songs are folders with OGG tracks + JSON metadata, triggerable via external events. The backend is complete and working.

## 🏗️ **Current Architecture Status**

**✅ FULLY IMPLEMENTED:**
- **AudioController**: Complete MVC controller with song/event methods ✅
- **AudioState**: Complete with all data structures (EffectState, StateSnapshot, Song, etc.) ✅
- **AudioSystem**: Complete SoLoud integration with all filters ✅ 
- **SongManager**: Complete JSON loading system with validation ✅
- **EventSystem**: Complete state transitions with lerping ✅
- **Dual Tempo Control**: Master tempo + granular tempo [per memory][[memory:7578384773746369645]] ✅
- **Build System**: All dependencies (nlohmann::json, SoLoud, ImGui) ✅
- **File Format**: Working _master.json and _song.json examples ✅

**❌ NEEDS IMPLEMENTATION:**
- Events UI window for triggering and creating events
- External C API for game engine integration
- Integration of event updates in main loop
- UI integration for song loading

## 📋 **REMAINING IMPLEMENTATION TASKS**

### **Task 1: Complete Event System Integration** ⚡ **HIGH PRIORITY**

**Issue**: Events exist but won't update because main loop doesn't call them.

**File**: `src/main.cpp`

```cpp
// Add after controller.updateSync(); around line 115
controller.updateEvents(1.0f/60.0f); // Assume 60 FPS for deltaTime
```

**Implementation:**
- Calculate proper deltaTime using SDL2 timing
- Add event update call in main loop
- Test event transitions work properly

### **Task 2: Events UI Window** ⚡ **HIGH PRIORITY**

**File**: `src/TesterView.h` + `src/TesterView.cpp`

**Add to TesterView.h:**
```cpp
private:
    void RenderEventsWindow();
    void RenderSongEvents();
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

**Add to TesterView.cpp Render() method:**
```cpp
RenderEventsWindow(); // Add alongside RenderMainWindow()
```

**Implementation:**
- New ImGui window for events
- List loaded songs with expandable event lists  
- Buttons to trigger song events and master events
- "Capture Current State" workflow for creating new events
- Event creation dialog


### **Task 4: External C API** 🔧 **MEDIUM PRIORITY**

**File**: `src/main.cpp` - Add external API

```cpp
// Global controller instance for external API
static AudioTester::AudioController* g_controller = nullptr;

extern "C" {
    void musicTester_triggerSongEvent(const char* songName, const char* eventName) {
        if (g_controller) {
            g_controller->triggerSongEvent(songName, eventName);
        }
    }

	// might be on song(s) or on master
	void musicTester_triggerEvent(const char* eventName) {
        if (g_controller) {
            g_controller->triggerEvent(eventName);
        }
    }
    
    void musicTester_triggerMasterEvent(const char* eventName) {
        if (g_controller) {
            g_controller->triggerMasterEvent(eventName);
        }
    }
}
```

**Implementation:**
- Store global controller reference
- Expose C-style functions for external calling
- Add header file for external inclusion
- Test external triggering

### **Task 5: Testing & Polish** 🧪 **LOW PRIORITY**

**Implementation:**
- Test song loading from different directories
- Verify event transitions work smoothly  
- Error handling for malformed JSON files
- Performance testing with large numbers of tracks/events

## 📁 **Current Working File Structure**

```
sound_staging/
├── _master.json           # ✅ Working master bus events
├── test-song/
│   ├── _song.json        # ✅ Working song metadata + events
│   ├── drums.ogg         # ✅ Audio tracks  
│   └── bass1.ogg
└── ...
```
(no loose tracks in master folder, must be in song folder)

## ✅ **Success Criteria**

1. **✅ Load songs** - Implemented and working
2. **✅ Trigger events externally** - Complete with UI and API
3. **✅ UI workflow** - Events window with full functionality  
4. **✅ Unified architecture** - No mode switching, clean design
5. **✅ External API** - C API ready for game engine integration

## 🎯 **Implementation Status: Core Complete, Enhancements Needed**

✅ **COMPLETED TASKS:**
1. **✅ Event updates in main loop** - Events transition smoothly
2. **✅ Events UI window** - Master and song events with triggering  
3. **✅ Unified song loading** - Removed confusing mode switching
4. **✅ Basic External C API** - Core functions implemented
5. **✅ Architecture cleanup** - Clean, intuitive design

❌ **REMAINING ENHANCEMENT TASKS:**

### **Task 1: Complete External C API** ⚡ **HIGH PRIORITY** (30 min)
**Missing**: `musicTester_triggerEvent(eventName)` - searches both songs and master
**File**: `src/main.cpp` + `src/AudioController.h/.cpp`

### **Task 2: Enhanced Animation System** ⚡ **HIGH PRIORITY** (2-3 hours)
**Issues**: 
- Using linear interpolation instead of EASE_IN_OUT
- Not capturing live state for elegant cancelling
**Files**: `src/EventSystem.cpp`

### **Task 3: Stackable Master Events** 🔧 **MEDIUM PRIORITY** (2-3 hours)
**Issues**:
- Master events replace everything instead of being additive
- Need optional fields for partial updates
**Files**: `src/AudioState.h`, `src/EventSystem.cpp`, `src/SongManager.cpp`

### **Task 4: Track Name Validation** 🔧 **MEDIUM PRIORITY** (1-2 hours)
**Issues**:
- JSON references tracks by name but no validation
- Need to ensure event tracks match actual files
**Files**: `src/SongManager.cpp`

### **Task 5: Testing & Polish** 🧪 **LOW PRIORITY** (1-2 hours)
**Remaining**: Verify smooth transitions and edge cases

**Total Remaining Work: ~6-8 hours**

## 🔧 **Key Findings from Analysis**

- **Architecture is solid** - All backend systems work correctly
- **JSON format is proven** - Working examples in sound_staging/
- **Event system is complete** - Just needs UI to trigger it
- **Build system is robust** - All dependencies properly integrated
- **Memory integration works** - Dual tempo architecture implemented correctly

The core event-driven system is **fully functional** - we just need to expose it through UI and external API.

## 🎨 **Event System Design Principles**

### **Core Animation Philosophy**
- **Automatic Interpolation**: Always interpolate from current live state, never from saved snapshots
- **Elegant Cancelling**: New events smoothly transition from wherever the current animation is
- **EASE_IN_OUT Default**: All transitions use smooth easing curves (not linear) for natural feel

### **Song Events vs Master Events**

#### **Song Events = Full State Snapshots** 🎵
- Replace ALL properties of the current song
- **Static Track Set**: Tracks never change during playback (fixed per song folder)
- Track mapping: `tracks[0]` = first .ogg file, `tracks[1]` = second .ogg file, etc.
	... but designer might change which track in which slot, so we need to reference tracks by name and do some analysis to ensure valid json file and not pointing to dead tracks etc
- Events modify: volume, active state, effects of existing tracks
- Properties: `masterTempo`, `granularTempo`, all track states

```cpp
// Song event affects complete song state
struct StateSnapshot {
    float masterTempo;
    float granularTempo;
    std::vector<TrackStateExtended> tracks;  // Same size as song.trackFiles
};
```

#### **Master Events = Partial Updates (Stackable)** 🔧
- Only modify specified properties, leave others unchanged
- **Additive Effects**: Master effects stack with song effects
- Can combine multiple master events (e.g., "Underwater" + "TechAttack")
- Optional fields: only update what's specified

```cpp
// Master event affects only specified properties
struct MasterBusState {
    std::optional<float> masterTempo;    // Only if specified
    std::optional<float> granularTempo;
    std::optional<float> volume;
    std::map<std::string, EffectState> effects;  // Additive, stackable
};
```

### **Track Architecture**
- **Design Time**: Tracks can be added/removed/changed in song folders
- **Runtime**: Track list is immutable - only properties change
- **Events**: Modify existing track properties (volume, active, effects)
- **No Dynamic Tracks**: Never add/remove tracks during playback

### **Transition Behavior**
```cpp
// ✅ Correct: Always start from current state
void startTransition(target, fadeTime) {
    m_startState = captureCurrentLiveState();  // Smooth cancelling
    m_targetState = target;
    // Apply EASE_IN_OUT curve during interpolation
}

// ❌ Wrong: Starting from old saved state
void startTransition(target, fadeTime) {
    m_startState = m_savedStartState;  // Jarring jump
}
```

### **Usage Examples**

**Stacking Master Events:**
```
1. Trigger "Underwater" → LoFi effect enabled
2. Trigger "TechAttack" → WaveShaper effect enabled  
3. Result: Both LoFi + WaveShaper active (stacked)
```

**Song Event Override:**
```
1. Current: Song A playing with some master effects
2. Trigger Song Event "Combat" → Completely replaces Song A state
3. Result: New song state + existing master effects remain
```

This design enables sophisticated dynamic music with intuitive behavior and smooth transitions. 