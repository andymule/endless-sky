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

**Key Features**:
- Dual tape speed architecture for granular tempo control
- Automatic track synchronization
- Effect automation for both tracks and master bus
- Path resolution relative to executable directory
- State change notifications for UI updates

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
- **Master Bus Support**: Global events affecting all music
- **File Format Support**: OGG audio files
- **Error Handling**: Robust error reporting and recovery

**File Structure**:
```
sound_staging/
├── _master.json          # Master bus events
├── song1/
│   ├── _song.json        # Song definition
│   ├── track1.ogg        # Audio tracks
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
- Provide directory input and loading
- Display tempo controls and synchronization

**Key Features**:
- **Multi-Window Support**: Main window + controls window + events window
- **Keyboard Shortcuts**: Spacebar for play/pause, number keys for tracks
- **Real-time Updates**: Live parameter control
- **Event Management**: Create, delete, and trigger events
- **Hold-to-Delete**: Long-press for event deletion
- **Filter Controls**: Collapsible effect parameter controls

**UI Components**:
- Directory input and loading
- Global playback controls
- Track volume and effect controls
- Bus volume and master effects
- Event creation and management
- Tempo controls

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

### 4. Lock-Free Audio Processing
- **Circular Buffers**: Single-producer, single-consumer design
- **Atomic Operations**: Thread safety without locks
- **Separate Processing Thread**: Real-time audio processing
- **Minimal Latency**: Optimized for live audio applications

## Parameter Loading and Wet Level System

### Current Implementation Issues

The current parameter loading system has several critical issues that need to be addressed:

#### 1. **Incomplete State Capture**
- `captureCurrentSongState()` and `captureCurrentMasterState()` do not capture effect states
- Effects are left empty during state capture, leading to incomplete snapshots
- No wet level tracking for filter enable/disable logic

#### 2. **Inefficient State Application**
- `applyStateSnapshot()` and `applyMasterBusState()` use "complete effect reset" approach
- All effects are disabled first, then re-enabled based on snapshot
- This causes unnecessary audio artifacts and doesn't leverage wet level lerping

#### 3. **Missing Wet Level Logic**
- No tracking of current wet levels for smooth transitions
- No lerping from current wet levels to target wet levels
- No conditional parameter storage based on wet level > 0

### Required Fixes

#### 1. **Enhanced State Capture**
```cpp
// In EventSystem::captureCurrentSongState()
for (size_t i = 0; i < audioState.getTrackCount(); ++i) {
    const auto& track = audioState.getTrack(i);
    const auto& trackFilters = m_controller->getAudioSystem().getFilters(i);
    
    TrackStateExtended extendedTrack;
    extendedTrack.file = std::filesystem::path(track.filepath).filename().string();
    extendedTrack.volume = track.volume;
    
    // Capture only effects with wet > 0
    for (const auto& [filterName, filterInstance] : trackFilters) {
        if (filterInstance.enabled) {
            EffectState effectState;
            // Store all parameters including wet level
            for (const auto& [paramId, param] : filterInstance.parameters) {
                effectState.parameters[std::to_string(paramId)] = param.value;
            }
            extendedTrack.effects[filterName] = effectState;
        }
    }
    state.tracks.push_back(extendedTrack);
}
```

#### 2. **Smart State Application**
```cpp
// In EventSystem::applyStateSnapshot()
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
            
            // Find wet parameter value
            auto wetIt = effectState.parameters.find("0"); // Wet is usually param ID 0
            float targetWet = (wetIt != effectState.parameters.end()) ? wetIt->second : 0.0f;
            
            if (targetWet > 0.0f) {
                // Target has effect enabled - lerp up to target wet level
                if (!currentlyEnabled) {
                    // Start from 0 wet and lerp up
                    m_controller->setTrackFilterParameter(trackIndex, effectName, 0, 0.0f);
                }
                // Apply all parameters (wet will be lerped by transition system)
                for (const auto& [paramIdStr, paramValue] : effectState.parameters) {
                    int paramId = std::stoi(paramIdStr);
                    m_controller->setTrackFilterParameter(trackIndex, effectName, paramId, paramValue);
                }
            } else if (currentlyEnabled) {
                // Target has effect disabled but currently enabled - lerp down to 0
                m_controller->setTrackFilterParameter(trackIndex, effectName, 0, 0.0f);
            }
        }
    }
}
```

#### 3. **FilterManager Integration**
```cpp
// Use FilterManager for parameter validation and mapping
bool isValidWetParameter(const std::string& filterName, float wetValue) {
    return m_filterManager.isValidParameter(filterName, "wet", wetValue);
}

int getWetParameterId(const std::string& filterName) {
    return m_filterManager.getParameterId(filterName, "wet");
}

std::string getWetParameterName(const std::string& filterName) {
    return m_filterManager.getParameterName(filterName, 0); // Wet is usually ID 0
}
```

### Wet Level Tracking Logic

#### 1. **Parameter Storage Rules**
- **Store all parameters** if wet level > 0
- **Don't store any parameters** if wet level = 0
- **Always store wet level** regardless of its value

#### 2. **Transition Logic**
- **Current wet > 0, Target wet > 0**: Lerp all parameters including wet
- **Current wet = 0, Target wet > 0**: Start from 0 wet, lerp up to target
- **Current wet > 0, Target wet = 0**: Lerp wet down to 0, disable effect
- **Current wet = 0, Target wet = 0**: No change needed

#### 3. **Lerping Strategy**
```cpp
void lerpEffectState(const EffectState& start, const EffectState& end, EffectState& result, float t) {
    // Always lerp wet parameter first
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

### Implementation Requirements

#### 1. **AudioController Enhancements**
- Add methods to capture current effect states from AudioSystem
- Implement wet level tracking for both tracks and master bus
- Provide current state access for EventSystem

#### 2. **EventSystem Updates**
- Complete `captureCurrentSongState()` and `captureCurrentMasterState()`
- Implement smart state application with wet level logic
- Add proper lerping for effect parameters

#### 3. **FilterManager Integration**
- Use FilterManager for parameter validation
- Leverage parameter name/ID mapping
- Ensure consistent parameter handling

#### 4. **AudioSystem Cooperation**
- Provide access to current filter states
- Support wet level queries
- Enable parameter validation through FilterManager

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