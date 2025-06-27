# Music Tester Architecture Documentation

## Overview

The Music Tester is a sophisticated real-time audio processing and event-driven music system designed for game engine integration. It provides granular tempo control, advanced filtering, and dynamic music transitions through a clean MVC (Model-View-Controller) architecture.

## System Architecture

### High-Level Design

The system follows a layered architecture with clear separation of concerns:

1. **Application Layer** (`main.cpp`) - Entry point and SDL/OpenGL setup
2. **Controller Layer** (`AudioController`) - Business logic and coordination
3. **Model Layer** - Audio processing, state management, and event system
4. **View Layer** (`TesterView`) - Dear ImGui-based user interface
5. **Audio Engine Layer** - SoLoud integration with custom processing

### Core Components

- **AudioController**: Central coordinator implementing MVC pattern
- **AudioSystem**: Low-level audio processing with SoLoud integration
- **AudioStreamProcessor**: Real-time granular time stretching
- **EventSystem**: Dynamic music transitions and automation
- **SongManager**: Song/event loading and management
- **FilterManager**: Unified audio effect parameter management

## File-by-File Documentation

### main.cpp
**Purpose**: Application entry point and SDL/OpenGL initialization

**Key Responsibilities**:
- Initialize SDL2 with OpenGL 3.2 Core Profile
- Set up Dear ImGui for the user interface
- Create and wire MVC components (AudioController, TesterView)
- Provide external C API functions for game engine integration
- Main application loop with delta time calculation
- Handle window events and cleanup

**External API Functions**:
- `musicTester_triggerSongEvent()` - Trigger song-specific events
- `musicTester_triggerMasterEvent()` - Trigger global master events
- `musicTester_triggerEvent()` - Universal event trigger
- `musicTester_loadSongsFromDirectory()` - Load music from directory

**Dependencies**: SDL2, OpenGL, Dear ImGui, AudioController, TesterView

### AudioController.h/cpp
**Purpose**: Central controller implementing MVC pattern and business logic

**Key Responsibilities**:
- Coordinate between UI (View) and audio processing (Model)
- Manage music directory loading and file discovery
- Handle playback control (play/stop/toggle)
- Manage track volumes, looping, and effects
- Control master and granular tempo settings
- Provide event triggering interface
- Maintain synchronization between audio system and UI state
- Handle song switching and master bus persistence

**Key Features**:
- Dual tape speed architecture for granular tempo control
- Automatic track synchronization
- Effect automation for both tracks and master bus
- Path resolution relative to executable directory
- State change notifications for UI updates
- **All tracks loop continuously and are always active** - volume control for fade in/out
- **Song switching**: Load songs as holistic entities
- **Master bus persistence**: Master bus state persists across song switches
- **File menu operations**: Create new master/song directories

**Dependencies**: AudioState, AudioSystem, SongManager, EventSystem

### AudioState.h
**Purpose**: Data structures for complete audio state management

**Key Data Structures**:
- `TrackState`: Basic track information (name, filepath, volume, looping)
- `EffectState`: Complete effect parameter storage
- `TrackStateExtended`: Enhanced track state with effects
- `StateSnapshot`: Complete song state (tracks + tempo)
- `SongEvent`: Event definition with state and fade time
- `Song`: Container for songs with events
- `MasterBusState`: Master bus state with effects and tempo
- `MasterEvent`: Master event definition
- `MasterBus`: Master bus container

**Key Features**:
- Complete state serialization for events
- Effect parameter storage with validation
- Tempo control at both song and master levels
- State change notification system
- **No active field for tracks** - tracks are always active, use volume=0 to disable

**Dependencies**: Standard library containers, filesystem

### AudioSystem.h/cpp
**Purpose**: Low-level audio processing with SoLoud integration

**Key Responsibilities**:
- Initialize and manage SoLoud audio engine
- Load and play OGG audio files
- Maintain track synchronization across multiple audio streams
- Apply audio filters and effects
- Implement dual tape speed architecture for granular tempo
- Manage audio bus routing and mixing
- Handle real-time audio processing with minimal latency

**Key Features**:
- **Dual Tape Speed Architecture**: 
  - Internal Tape Speed = userTapeSpeed * granularTempo
  - Granular Pitch Compensation = 1.0 / granularTempo
  - Eliminates complex buffering while maintaining perfect sample ratios
- **GranularInterceptFilter**: Custom SoLoud filter for granular processing
- **SyncWav/SyncWavInstance**: Custom WAV classes with accurate seeking
- **Track synchronization**: Automatic drift detection and correction
- **Filter management**: Dynamic filter application with parameter validation
- **All tracks loop continuously** - no individual loop control needed

**Supported Filters**:
- biquad, echo, lofi, flanger, dcremoval, bassboost, waveshaper, robotize, freeverb

**Dependencies**: SoLoud, AudioStreamProcessor, FilterManager

### AudioStreamProcessor.h/cpp
**Purpose**: Real-time granular time stretching with pitch preservation

**Key Responsibilities**:
- Process audio streams through Signalsmith Stretch
- Maintain 1:1 input/output sample ratios for SoLoud compatibility
- Handle pitch compensation for dual tape speed architecture
- Provide lock-free audio buffering with circular buffers
- Run processing in separate thread for real-time performance
- Manage audio latency and buffer management

**Key Features**:
- **Signalsmith Stretch Integration**: High-quality time stretching
- **Circular Buffer System**: Lock-free audio streaming
- **Pitch Compensation**: Preserves pitch while changing tempo
- **Thread-Safe Processing**: Separate processing thread
- **Configurable Quality**: Adjustable block sizes and overlap ratios
- **Formant Preservation**: Maintains vocal characteristics

**Dependencies**: Signalsmith Stretch, CircularBuffer, threading

### SongManager.h/cpp
**Purpose**: Song and event loading, management, and serialization

**Key Responsibilities**:
- Load songs from directory structure
- Parse JSON song and event definitions
- Discover audio tracks within song folders
- Manage master bus events
- Validate JSON structure and parameters
- Save song and event configurations
- Provide song lookup and event management

**Key Features**:
- **Auto-discovery**: Automatically finds tracks and song folders
- **JSON Parsing**: Uses nlohmann/json for configuration
- **Validation**: Comprehensive JSON structure validation
- **Master Bus Support**: Global events affecting all music (REQUIRED)
- **File Format Support**: OGG audio files only
- **Error Handling**: Robust error reporting and recovery
- **Parameter Storage**: Effect parameters stored by ID as strings
- **Effect Names**: Effect names preserved in JSON for readability
- **Parameter IDs**: Parameter names not preserved - only IDs stored for efficiency
- **Song Switching**: Load songs as holistic entities, switch between them
- **Master Persistence**: Master bus state persists across song switches

**File Structure**:
```
sound_staging/
├── _master.json          # Master bus events (REQUIRED)
├── song1/
│   ├── _song.json        # Song definition
│   ├── track1.ogg        # Audio tracks (song-specific)
│   └── track2.ogg
└── song2/
    ├── _song.json
    └── track1.ogg
```

**Dependencies**: nlohmann/json, filesystem, AudioState

### EventSystem.h/cpp
**Purpose**: Dynamic music transitions and state automation

**Key Responsibilities**:
- Trigger song and master events
- Perform smooth state transitions with lerping
- Handle transition cancellation and interruption
- Apply state snapshots to audio system
- Manage transition timing and easing curves
- Capture current audio state for transitions

**Key Features**:
- **Smooth Transitions**: EASE_IN_OUT easing curves
- **State Lerping**: Interpolate between audio states
- **Transition Cancellation**: Elegant handling of interrupted transitions
- **Cross-Transition Support**: Master ↔ Song transitions
- **Effect Automation**: Smooth parameter transitions
- **Real-time Updates**: Frame-based transition processing
- **Wet Level Logic**: Only captures effects with wet level > 0

**Transition Types**:
- **Song Events**: Affect individual song tracks and tempo
- **Master Events**: Affect global master bus and tempo

**Dependencies**: AudioController, SongManager, AudioState

### FilterManager.h/cpp
**Purpose**: Unified audio effect parameter management

**Key Responsibilities**:
- Define filter parameters and validation rules
- Create and manage SoLoud filter instances
- Provide parameter name/ID mapping
- Validate parameter values and ranges
- Set filter parameters by name or ID
- Maintain filter registry and definitions

**Key Features**:
- **Unified Interface**: Consistent parameter access across all filters
- **Parameter Validation**: Range checking and type validation
- **Name/ID Mapping**: Human-readable parameter names
- **Factory Pattern**: Dynamic filter creation
- **Extensible Design**: Easy to add new filter types
- **Wet Level Support**: Parameter ID 0 is always wet level

**Supported Filters**:
- biquad, echo, lofi, flanger, dcremoval, bassboost, waveshaper, robotize, freeverb

**Dependencies**: SoLoud filters

### TesterView.h/cpp
**Purpose**: Dear ImGui-based user interface

**Key Responsibilities**:
- Render main application window
- Display track controls and volume sliders
- Show filter/effect controls
- Manage event creation and deletion
- Handle keyboard shortcuts
- Provide file menu and directory management
- Display tempo controls and synchronization

**Key Features**:
- **Multi-Window Support**: Main window + controls window + events window
- **File Menu**: New Master, New Song, and Load operations
- **Keyboard Shortcuts**: Spacebar for play/pause, number keys for tracks
- **Real-time Updates**: Live parameter control
- **Event Management**: Create, delete, and trigger events
- **Hold-to-Delete**: Long-press for event deletion
- **Filter Controls**: Collapsible effect parameter controls
- **Events Window**: Complete CRUD operations for events
- **Song Browser**: Switch between available songs
- **Active Song Indicator**: Shows currently loaded song

**UI Components**:
- File menu (New Master, New Song, Load)
- Global playback controls
- Track volume and effect controls
- Bus volume and master effects
- Event creation and management
- Tempo controls
- **Events Window Features**:
  - Create new song and master events
  - Delete events with confirmation
  - Save/overwrite existing events
  - Event triggering with visual feedback
  - Event list management and organization
  - Song browser at bottom for switching songs
  - Active song indicator

**Dependencies**: Dear ImGui, SDL2, AudioController

### CircularBuffer.h
**Purpose**: Lock-free ring buffer for real-time audio processing

**Key Responsibilities**:
- Provide single-producer, single-consumer lock-free buffer
- Handle audio sample streaming with minimal latency
- Manage buffer wrapping and overflow protection
- Ensure thread safety without locks
- Optimize for real-time audio applications

**Key Features**:
- **Lock-Free Design**: Atomic operations for thread safety
- **Power-of-2 Capacity**: Optimized for bitwise operations
- **Overflow Protection**: Automatic space checking
- **Wrapping Support**: Seamless buffer wrapping
- **Memory Ordering**: Proper memory barriers for consistency

**Dependencies**: Standard library, atomic operations

### Logger.h/cpp
**Purpose**: Centralized logging system

**Key Responsibilities**:
- Provide configurable log levels
- Output formatted log messages
- Support component-specific logging
- Handle error vs. info output streams
- Provide convenience macros for easy usage

**Key Features**:
- **Log Levels**: ERROR, WARN, INFO, DEBUG, TRACE
- **Component Logging**: Tagged log messages
- **Convenience Macros**: Easy-to-use logging macros
- **Stream Separation**: Errors to stderr, others to stdout
- **Singleton Pattern**: Global logger instance

**Dependencies**: Standard library I/O

### music_tester_api.h
**Purpose**: External C API for game engine integration

**Key Responsibilities**:
- Provide C-compatible function declarations
- Enable integration with game engines
- Abstract internal C++ implementation
- Define clear API contract

**API Functions**:
- `musicTester_triggerSongEvent()` - Trigger song events
- `musicTester_triggerMasterEvent()` - Trigger master events  
- `musicTester_triggerEvent()` - Universal event trigger
- `musicTester_loadSongsFromDirectory()` - Load music

**Dependencies**: None (header-only)

## Key Architectural Patterns

### 1. MVC (Model-View-Controller)
- **Model**: AudioState, AudioSystem, SongManager
- **View**: TesterView (Dear ImGui interface)
- **Controller**: AudioController (business logic coordination)

### 2. Dual Tape Speed Architecture
For granular tempo effects with SoLoud filters (requiring 1:1 input/output ratios):
- **Internal Tape Speed** = userTapeSpeed * granularTempo
- **Granular Pitch Compensation** = 1.0 / granularTempo
- Eliminates complex buffering by maintaining perfect sample ratios
- Signalsmith Stretch handles pitch compensation
- SoLoud handles time changes through tape speed

### 3. Event-Driven State Transitions
- **State Snapshots**: Complete audio state serialization
- **Smooth Transitions**: Lerping between states with easing curves
- **Transition Cancellation**: Elegant handling of interrupted transitions
- **Cross-Transition Support**: Master ↔ Song transitions
- **Wet Level Logic**: Sophisticated effect state management

### 4. Lock-Free Audio Processing
- **Circular Buffers**: Single-producer, single-consumer design
- **Atomic Operations**: Thread safety without locks
- **Separate Processing Thread**: Real-time audio processing
- **Minimal Latency**: Optimized for live audio applications

## Parameter Loading and Wet Level System

### Implementation Status: COMPLETED ✅

The parameter loading and wet level tracking system has been fully implemented with robust state management and smooth transitions.

### Key Features Implemented

#### 1. **Enhanced State Capture with Wet Level Tracking**
- `captureCurrentSongState()` and `captureCurrentMasterState()` now capture complete effect states
- Only effects with wet level > 0 are stored in snapshots
- All parameters including wet level are captured for enabled effects
- Consistent parameter storage using string IDs for JSON compatibility

#### 2. **Smart State Application with Lerping**
- `applyStateSnapshot()` and `applyMasterBusState()` use intelligent effect management
- Effects are only enabled/disabled based on wet level changes
- Smooth transitions between wet levels using lerping
- No unnecessary effect resets that cause audio artifacts

#### 3. **Robust Lerping System**
- `lerpStates()` and `lerpEffectState()` with comprehensive safety checks
- Defensive programming to prevent segfaults from missing tracks/effects
- Wet level lerping with conditional parameter interpolation
- Graceful handling of mismatched parameter sets

### Implementation Details

#### State Capture Logic
```cpp
// Enhanced captureCurrentSongState() - captures only enabled effects
for (size_t i = 0; i < audioState.getTrackCount(); ++i) {
    const auto& track = audioState.getTrack(i);
    const auto& trackFilters = m_controller->getAudioSystem().getFilters(i);
    
    TrackStateExtended extendedTrack;
    extendedTrack.file = std::filesystem::path(track.filepath).filename().string();
    extendedTrack.volume = track.volume;
    
    // Only capture effects with wet > 0
    for (const auto& [filterName, filterInstance] : trackFilters) {
        if (filterInstance.enabled) {
            EffectState effectState;
            // Store all parameters by ID for consistency
            for (const auto& [paramId, param] : filterInstance.parameters) {
                effectState.parameters[std::to_string(paramId)] = param.value;
            }
            extendedTrack.effects[filterName] = effectState;
        }
    }
    state.tracks.push_back(extendedTrack);
}
```

#### Smart State Application
```cpp
// Intelligent applyStateSnapshot() - handles wet level transitions
for (size_t i = 0; i < state.tracks.size(); ++i) {
    const auto& track = state.tracks[i];
    int trackIndex = m_controller->findTrackByFilename(track.file);
    if (trackIndex >= 0) {
        m_controller->setTrackVolume(trackIndex, track.volume);
        
        // Get current audio system state for comparison
        const auto& currentFilters = m_controller->getAudioSystem().getFilters(trackIndex);
        
        // Apply effects with wet level logic
        for (const auto& [effectName, effectState] : track.effects) {
            auto currentIt = currentFilters.find(effectName);
            bool currentlyEnabled = (currentIt != currentFilters.end() && currentIt->second.enabled);
            
            // Find wet parameter value (usually ID 0)
            auto wetIt = effectState.parameters.find("0");
            float targetWet = (wetIt != effectState.parameters.end()) ? wetIt->second : 0.0f;
            
            if (targetWet > 0.0f) {
                // Target has effect enabled - apply all parameters
                for (const auto& [paramIdStr, paramValue] : effectState.parameters) {
                    int paramId = std::stoi(paramIdStr);
                    m_controller->setTrackFilterParameter(trackIndex, effectName, paramId, paramValue);
                }
            } else if (currentlyEnabled) {
                // Target has effect disabled but currently enabled - disable
                m_controller->setTrackFilterParameter(trackIndex, effectName, 0, 0.0f);
            }
        }
    }
}
```

#### Robust Lerping with Safety Checks
```cpp
// Safe lerpStates() with comprehensive guards
void EventSystem::lerpStates(float t) {
    if (!m_controller) return;
    
    // Lerp master tempo settings
    float currentMasterTempo = lerp(m_startState.masterTempo, m_targetState.masterTempo, t);
    float currentGranularTempo = lerp(m_startState.granularTempo, m_targetState.granularTempo, t);
    m_controller->setMasterTempo(currentMasterTempo);
    m_controller->setGranularTempo(currentGranularTempo);
    
    // Lerp track states with safety checks
    size_t maxTracks = std::max(m_startState.tracks.size(), m_targetState.tracks.size());
    for (size_t i = 0; i < maxTracks; ++i) {
        // Defensive checks for track existence
        if (i >= m_startState.tracks.size() || i >= m_targetState.tracks.size()) continue;
        
        const auto& startTrack = m_startState.tracks[i];
        const auto& targetTrack = m_targetState.tracks[i];
        
        // Find track by filename with validation
        int trackIndex = m_controller->findTrackByFilename(targetTrack.file);
        if (trackIndex < 0) continue;
        
        // Lerp volume
        float currentVolume = lerp(startTrack.volume, targetTrack.volume, t);
        m_controller->setTrackVolume(trackIndex, currentVolume);
        
        // Lerp effects with wet level logic
        lerpTrackEffects(trackIndex, startTrack, targetTrack, t);
    }
}
```

#### Wet Level Lerping Logic
```cpp
// lerpEffectState() with wet level conditional logic
void EventSystem::lerpEffectState(const EffectState& start, const EffectState& end, 
                                 EffectState& result, float t) {
    // Always lerp wet parameter first (ID 0)
    auto startWetIt = start.parameters.find("0");
    auto endWetIt = end.parameters.find("0");
    
    if (startWetIt != start.parameters.end() && endWetIt != end.parameters.end()) {
        float startWet = startWetIt->second;
        float endWet = endWetIt->second;
        result.parameters["0"] = lerp(startWet, endWet, t);
        
        // Only lerp other parameters if either wet level > 0
        if (startWet > 0.0f || endWet > 0.0f) {
            for (const auto& [paramName, endValue] : end.parameters) {
                if (paramName != "0") { // Skip wet parameter (already handled)
                    auto startIt = start.parameters.find(paramName);
                    if (startIt != start.parameters.end()) {
                        result.parameters[paramName] = lerp(startIt->second, endValue, t);
                    } else {
                        result.parameters[paramName] = endValue;
                    }
                }
            }
        }
    }
}
```

### Wet Level Tracking Rules

#### 1. **Parameter Storage Logic**
- **Store all parameters** if wet level > 0 (effect is active)
- **Don't store any parameters** if wet level = 0 (effect is inactive)
- **Always capture wet level** regardless of its value for transition logic

#### 2. **Transition Behavior**
- **Current wet > 0, Target wet > 0**: Smooth lerp of all parameters including wet
- **Current wet = 0, Target wet > 0**: Start from 0 wet, lerp up to target
- **Current wet > 0, Target wet = 0**: Lerp wet down to 0, disable effect
- **Current wet = 0, Target wet = 0**: No change needed

#### 3. **Safety Features**
- Comprehensive null pointer checks in all lerping functions
- Defensive programming to prevent segfaults from missing tracks/effects
- Graceful handling of mismatched parameter sets
- Validation of track indices and effect names before access

### Integration Points

#### AudioController Integration
- Enhanced event creation to capture only enabled effects
- Consistent parameter storage using string IDs
- Proper wet level tracking for both tracks and master bus

#### FilterManager Integration
- Leverages existing parameter validation and mapping
- Uses consistent parameter ID system
- Maintains compatibility with SoLoud filter interface

#### JSON Serialization
- SongManager already supports the parameter format used
- String-based parameter IDs for JSON compatibility
- Complete state serialization for events and snapshots

## Error Handling and Recovery

### Implementation Status: COMPLETED ✅

The system implements comprehensive error handling with graceful degradation and recovery mechanisms.

### Error Handling Strategy

#### 1. **File Loading Errors**
```cpp
// Graceful file loading with error recovery
if (!std::filesystem::exists(trackPath)) {
    logError("Track file not found: " + trackPath.string());
    // Skip track but continue loading song
    continue;
}

// JSON parsing with exception handling
try {
    file >> json;
} catch (const std::exception& e) {
    logError("JSON parsing error: " + std::string(e.what()));
    return false; // Skip this file, continue with others
}
```

#### 2. **Parameter Validation**
```cpp
// Parameter range checking with defaults
if (paramValue < minValue || paramValue > maxValue) {
    logError("Parameter out of range: " + paramName + " = " + std::to_string(paramValue));
    // Use default value and continue
    paramValue = defaultValue;
}
```

#### 3. **Transition Failures**
```cpp
// Safe transition handling
if (!startTransition(targetState)) {
    logError("Failed to start transition to: " + targetState.name);
    // Keep current state, don't crash
    return false;
}
```

#### 4. **Memory and Resource Errors**
```cpp
// Resource allocation with fallbacks
if (!allocateAudioBuffer(size)) {
    logError("Failed to allocate audio buffer, using fallback size");
    size = fallbackSize;
    allocateAudioBuffer(size);
}
```

### Recovery Mechanisms

#### 1. **Graceful Degradation**
- Continue operation even if some files fail to load
- Use default values for invalid parameters
- Skip problematic effects while keeping others functional

#### 2. **Retry Logic**
- Automatic retry for transient file system errors
- Exponential backoff for network resources
- User-initiated retry for failed operations

#### 3. **State Recovery**
- Maintain previous state on transition failures
- Rollback partial changes on error
- Preserve user settings across sessions

#### 4. **User Feedback**
- Clear error messages in UI
- Progress indicators for long operations
- Recovery suggestions for common issues

### Error Categories

#### 1. **File System Errors**
- Missing files or directories
- Permission denied
- Disk space issues
- Corrupted files

#### 2. **JSON Parsing Errors**
- Malformed JSON syntax
- Missing required fields
- Invalid data types
- Schema validation failures

#### 3. **Audio System Errors**
- Device initialization failures
- Buffer allocation errors
- Filter parameter validation
- Transition conflicts

#### 4. **UI and User Input Errors**
- Invalid user input
- Resource exhaustion
- Thread safety violations
- Memory allocation failures

## Dependencies

### External Libraries
- **SoLoud**: Audio engine and filters
- **Signalsmith Stretch**: High-quality time stretching
- **Dear ImGui**: User interface
- **SDL2**: Window management and input
- **OpenGL**: Graphics rendering
- **nlohmann/json**: JSON parsing and serialization

### System Requirements
- **C++17**: Modern C++ features
- **Threading**: Multi-threaded audio processing
- **Filesystem**: Directory scanning and file management
- **Atomic Operations**: Lock-free data structures

## Performance Characteristics

- **Audio Latency**: < 50ms typical
- **Granular Processing**: Real-time with configurable quality
- **Memory Usage**: Efficient circular buffer design
- **CPU Usage**: Optimized for real-time audio
- **Thread Safety**: Lock-free audio processing

## Integration Points

### Game Engine Integration
- **C API**: Simple function calls for event triggering
- **Event System**: Dynamic music transitions
- **File Loading**: Automatic track and event discovery
- **Real-time Control**: Live parameter adjustment

### Audio Pipeline
- **Input**: OGG audio files
- **Processing**: SoLoud + Signalsmith Stretch
- **Effects**: Real-time filter application
- **Output**: Synchronized multi-track playback 