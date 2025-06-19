# Granular Time Stretching Implementation Tasks

## Context & Current State

This document outlines the implementation of proper pitch-preserving granular time stretching for the Music Tester application. The application currently has:

- **Working Playback Speed Control**: Uses SoLoud's `setRelativePlaySpeed()` for tape-like speed changes (affects both tempo and pitch)
- **Broken Granular Tempo Control**: Attempts to use Signalsmith Stretch within SoLoud's filter system, resulting in stuttering and no actual tempo change
- **Existing Architecture**: AudioSystem → SoLoud → Audio Output, with various filters and track management

### Current File Structure
```
src/
├── AudioSystem.h/.cpp          # Main audio management
├── AudioController.h/.cpp      # Controller layer between UI and AudioSystem  
├── TesterView.h/.cpp          # UI layer with ImGui controls
├── GranularTempoFilter.h/.cpp # Current broken filter implementation
└── MasterTempoProcessor.h/.cpp # Signalsmith Stretch wrapper (broken)
```

### Dependencies Already Available
- **Signalsmith Stretch**: Fetched via CMake FetchContent from GitHub
- **SoLoud**: Audio engine with filter support
- **ImGui**: UI framework for controls

## Problem Analysis

Currently, our "Granular Tempo" control is fundamentally broken because we're trying to implement time stretching within SoLoud's filter system, which has critical limitations:

1. **SoLoud Filter Constraint**: Filters MUST output the same number of samples they receive
2. **Time Stretching Reality**: True tempo change requires different input/output sample ratios
3. **Current Implementation**: We're calling `seek()` every frame and using equal input/output samples, which defeats the purpose of time stretching

## Understanding Signalsmith Stretch

From analyzing the official example (`cmd/main.cpp`), Signalsmith Stretch works as follows:

### Correct Usage Pattern
```cpp
// Setup phase (once)
stretch.presetDefault(channels, sampleRate);

// Seeking phase (when changing tempo/position)
stretch.seek(inputBuffer, inputLatency, 1.0/timeStretchFactor);

// Processing phase (continuous)
stretch.process(inputBuffer, inputSamples, outputBuffer, outputSamples);
```

### Key Insights
- `seek()` is called ONCE when changing tempo or seeking position, not every frame
- `timeStretchFactor = 2.0` means double length (half speed)
- `playbackRate = 1/timeStretchFactor` 
- Input and output sample counts are different based on stretch ratio
- Requires proper buffering and latency management

## Proposed Solutions

### Option 1: Pre-Processing Architecture (Recommended)

**Concept**: Process audio files through Signalsmith Stretch BEFORE feeding to SoLoud

**Implementation**:
1. **AudioPreProcessor Class**: 
   - Wraps Signalsmith Stretch for offline/real-time processing
   - Maintains ring buffers for continuous processing
   - Handles tempo changes by adjusting processing parameters

2. **Integration Points**:
   - Intercept audio between file loading and SoLoud playback
   - Create processed audio streams that SoLoud consumes
   - Use separate thread for time-stretching processing

3. **Architecture**:
   ```
   AudioFile → AudioPreProcessor → ProcessedBuffer → SoLoud → Output
                     ↑
               Tempo Control
   ```

**Advantages**:
- Clean separation from SoLoud
- Proper time stretching with variable sample ratios
- Can handle seek operations correctly
- Allows for advanced features (pitch + tempo independence)

**Challenges**:
- Requires significant buffering
- Added latency for real-time processing
- Complex synchronization between processor and SoLoud

### Option 2: Custom Audio Source

**Concept**: Create a custom SoLoud AudioSource that internally handles time stretching

**Implementation**:
1. **TimeStretchAudioSource Class**: 
   - Extends SoLoud::AudioSource
   - Contains original audio data + Signalsmith Stretch instance
   - Generates samples on-demand with time stretching

2. **Instance Management**:
   - Each playing track gets its own TimeStretchAudioSourceInstance
   - Instances maintain their own stretch state and buffers
   - Handle tempo changes through parameter updates

**Advantages**:
- Integrates cleanly with SoLoud's architecture
- Per-track tempo control possible
- No global buffer management needed

**Challenges**:
- Complex AudioSource implementation
- Need to handle SoLoud's streaming expectations
- Tempo changes affect all instances globally (unless per-track)

### Option 3: Post-Processing Hook

**Concept**: Hook into SoLoud's final output stage before hardware

**Implementation**:
1. **Master Output Processor**:
   - Intercept audio after SoLoud's final mix
   - Apply time stretching to the master output
   - Maintain large ring buffers for continuous processing

2. **Buffer Management**:
   - Large input buffer (several seconds)
   - Continuously process chunks through Signalsmith Stretch
   - Handle underruns and tempo changes gracefully

**Advantages**:
- Affects all audio uniformly
- Simpler integration than per-track processing
- Can leverage SoLoud's existing mixing

**Challenges**:
- High memory usage for buffers
- Significant latency (several seconds)
- Complex state management for tempo changes

## Detailed Implementation Plan (Option 1 - Recommended)

### Phase 1: Core Infrastructure

#### 1.1 AudioStreamProcessor Class
```cpp
class AudioStreamProcessor {
    // Ring buffer management
    CircularBuffer<float> inputBuffer;
    CircularBuffer<float> outputBuffer;
    
    // Signalsmith Stretch instance
    signalsmith::stretch::SignalsmithStretch<float> stretcher;
    
    // Processing thread
    std::thread processingThread;
    std::atomic<bool> shouldStop;
    
    // Tempo control
    std::atomic<float> targetTempo;
    float currentTempo;
    
public:
    void setTempo(float tempo);
    void feedInput(const float* samples, size_t count);
    size_t readOutput(float* samples, size_t count);
    void seek(double timeSeconds);
};
```

#### 1.2 Buffer Management
- **Input Buffer**: 5-10 seconds of audio for smooth processing
- **Output Buffer**: 2-3 seconds to handle tempo changes
- **Thread Safety**: Lock-free ring buffers for real-time performance
- **Underrun Handling**: Graceful degradation when buffers empty

#### 1.3 Processing Thread
```cpp
void AudioStreamProcessor::processingLoop() {
    while (!shouldStop) {
        // Check for tempo changes
        if (abs(targetTempo - currentTempo) > 0.001f) {
            handleTempoChange();
        }
        
        // Process a chunk if we have enough input and output space
        if (inputBuffer.available() >= chunkSize && 
            outputBuffer.space() >= getOutputChunkSize()) {
            processChunk();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}
```

### Phase 2: SoLoud Integration

#### 2.1 ProcessedAudioSource
```cpp
class ProcessedAudioSource : public SoLoud::AudioSource {
    std::shared_ptr<AudioStreamProcessor> processor;
    SoLoud::Wav originalWav;
    
public:
    SoLoud::result load(const char* filename);
    void setProcessor(std::shared_ptr<AudioStreamProcessor> proc);
    
    // Override AudioSource methods to read from processor
    virtual AudioSourceInstance* createInstance() override;
};
```

#### 2.2 Modified AudioSystem
- Replace direct SoLoud::Wav usage with ProcessedAudioSource
- Route tempo commands to AudioStreamProcessor
- Maintain processor instances per track or globally

### Phase 3: Advanced Features

#### 3.1 Seek Support
- Coordinate seeking between SoLoud and Signalsmith Stretch
- Handle buffer flushing and refilling
- Maintain temporal alignment

#### 3.2 Quality Settings
- Expose Signalsmith Stretch presets (default, cheaper, custom)
- Allow block size and interval configuration
- Trade-off between quality and latency

#### 3.3 Multiple Tracks
- Decide between global vs per-track processing
- Handle CPU usage with multiple processors
- Implement priority systems for processing resources

## Technical Challenges & Solutions

### 1. Latency Management
**Problem**: Time stretching introduces significant latency
**Solution**: 
- Implement look-ahead buffering
- Pre-fill buffers on track load
- Use smaller block sizes for lower latency
- Display actual latency to user

### 2. Tempo Change Responsiveness
**Problem**: Large buffers mean slow tempo change response
**Solution**:
- Implement buffer flushing on tempo change
- Use smaller processing chunks
- Crossfade between old and new tempo sections

### 3. Memory Usage
**Problem**: Large buffers consume significant RAM
**Solution**:
- Implement adaptive buffer sizing
- Use compressed audio storage where possible
- Memory-map large files instead of loading entirely

### 4. CPU Usage
**Problem**: Real-time time stretching is computationally expensive
**Solution**:
- Use cheaper Signalsmith presets for less critical applications
- Implement dynamic quality adjustment based on CPU load
- Consider GPU acceleration for complex processing

### 5. Synchronization
**Problem**: Keeping processed audio in sync with SoLoud playback
**Solution**:
- Implement precise timing mechanisms
- Use SoLoud's time tracking for coordination
- Handle clock drift between systems

## Implementation Milestones

### Milestone 1: Basic Processor (Week 1)
- [x] CircularBuffer implementation
- [x] AudioStreamProcessor basic structure
- [x] Simple processing thread

### Milestone 2: Signalsmith Integration (Week 2)
- [x] Integrate Signalsmith Stretch
- [x] Implement tempo change handling
- [x] Latency measurement and reporting

### Milestone 3: SoLoud Integration (Week 3)
- [x] ProcessedAudioSource implementation
- [x] AudioSystem modifications
- [x] Basic playback with time stretching
- [x] UI integration for tempo control

### Milestone 4: Robustness (Week 4)
- [ ] Seek support
- [ ] Error handling and recovery
- [ ] Performance optimization
- [ ] Memory usage optimization

### Milestone 5: Polish (Week 5)
- [ ] Advanced quality settings
- [ ] Multiple track support

## Testing Strategy

### Unit Tests
- Buffer operations (write, read, wrap-around)
- Tempo change calculations
- Latency measurements

### Integration Tests
- SoLoud playback with processing
- Seek operations accuracy
- Memory leak detection

### Performance Tests
- CPU usage under various loads
- Real-time processing capabilities
- Stress testing with multiple tracks

### Quality Tests
- Audio quality comparisons
- Artifact detection
- Tempo accuracy verification

## Risk Mitigation

### High Risk: Real-time Performance
**Mitigation**: Implement fallback to bypass processing if CPU overloaded

### Medium Risk: Memory Usage
**Mitigation**: Configurable buffer sizes, adaptive management

### Medium Risk: Audio Quality
**Mitigation**: Multiple quality presets, user-configurable settings

### Low Risk: Complexity
**Mitigation**: Phased implementation, comprehensive testing

## Reference Materials

### Signalsmith Stretch Documentation
- **Main Header**: `build/_deps/signalsmith-stretch-src/signalsmith-stretch.h`
- **Official Example**: `build/_deps/signalsmith-stretch-src/cmd/main.cpp`
- **GitHub Repository**: https://github.com/Signalsmith-Audio/signalsmith-stretch

### Key API Methods to Study
```cpp
// Configuration
stretch.presetDefault(channels, sampleRate);
stretch.presetCheaper(channels, sampleRate); // Lower CPU usage

// Tempo/pitch control  
stretch.seek(inputWrapper, inputSamples, playbackRate);
stretch.process(inputWrapper, inputSamples, outputWrapper, outputSamples);

// Quality settings
stretch.setTransposeFactor(multiplier, tonalityLimit);
stretch.setFormantFactor(multiplier, compensatePitch);
```

### Current Working Components to Reference
- `AudioSystem::setGlobalPlaybackRate()` - Shows how to apply rate changes globally
- `TesterView::RenderControlsWindow()` - UI pattern for tempo controls
- `AudioController` tempo methods - Controller layer patterns

## Common Pitfalls to Avoid

1. **Don't use SoLoud filters for time stretching** - They can't change sample counts
2. **Don't call `seek()` every frame** - Only call when tempo actually changes  
3. **Don't ignore latency** - Time stretching introduces significant delay that must be managed
4. **Don't assume synchronous processing** - Use threading and buffering appropriately
5. **Don't neglect memory management** - Large buffers can quickly consume RAM

## Troubleshooting Guide

### If audio stutters or glitches:
- Check buffer sizes (may be too small)
- Verify processing thread isn't starving
- Ensure tempo changes are smoothed/crossfaded

### If tempo changes are slow to respond:
- Reduce buffer sizes (trade-off with stability)
- Implement buffer flushing on tempo change
- Use smaller processing chunks

### If CPU usage is too high:
- Switch to `presetCheaper()` quality setting
- Increase processing chunk sizes
- Implement dynamic quality adjustment

### If memory usage grows:
- Check for buffer leaks in ring buffer implementation
- Implement adaptive buffer sizing
- Consider memory-mapped file access for large audio files

This implementation will provide true pitch-preserving granular time stretching that works properly outside of SoLoud's filter constraints, giving users the real-time tempo control they expect. 

## Files to Remove/Modify

When implementing the new solution, the following files should be handled:

### Remove (Current Broken Implementation)
- `src/GranularTempoFilter.h/.cpp` - Current SoLoud filter approach is fundamentally flawed
- `src/MasterTempoProcessor.h/.cpp` - Current wrapper doesn't work in filter context

### Keep & Modify
- `src/AudioSystem.h/.cpp` - Add new granular processing integration points
- `src/AudioController.h/.cpp` - Update granular tempo control methods
- `src/TesterView.h/.cpp` - Update UI labels and tooltips to clarify the difference between playback speed and granular tempo

### Keep As-Is
- Playback speed control (SoLoud rate control) - This works correctly for tape-like speed changes

## Expected User Experience

The final implementation should provide:

### Two Independent Controls
1. **Playback Speed Slider** (already working)
   - Range: 0.1x to 4.0x
   - Behavior: Changes both tempo and pitch (like tape speed)
   - Tooltip: "Tape-style speed control (affects pitch)"
   
2. **Granular Tempo Slider** (to be implemented)
   - Range: 0.5x to 2.0x (more conservative for quality)
   - Behavior: Changes tempo while preserving pitch
   - Tooltip: "Pitch-preserving tempo stretching"
   - Latency indicator showing processing delay

### Combined Behavior
- Both controls can be used simultaneously
- Audio processing chain: Original → Playback Speed → Granular Tempo → Output
- Real-time responsiveness for both controls
- Graceful degradation if CPU overloaded 