# MainView Refactor Plan

## Goal
Break down `MainView` into smaller, focused view components for better maintainability, clarity, and reusability. Refactor one view at a time to minimize risk and keep changes manageable.

---

## Step-by-Step Plan

### 1. Identify Logical Subviews
- **SongView**: Handles song list, track controls, and related dialogs. ✅ **COMPLETED**
- **BusView**: Handles bus controls and FX. ✅ **COMPLETED**
- **SpeedView**: Handles tempo, tape speed, and granular controls.
- **EventView**: Handles event management and event dialogs.
- **MenuBarView**: Handles the menu bar.
- **FileDialogs**: Handles file/ogg dialogs (optional, can be grouped or split).

### 2. Create a `Views/` Directory
- Place all new view components in `src/Views/`
- Use `Dynamix::Views` namespace for all view components
- Update CMakeLists.txt to include new files

### 3. Extract SongView ✅ **COMPLETED**
**Status**: Successfully extracted and working

**What was moved**:
- Track list rendering and controls
- Track volume sliders with overlay text
- Track effects/filter controls (collapsible)
- Track deletion functionality
- New song dialog
- OGG file dialog
- Track file copying functionality

**Files created**:
- `src/Views/SongView.h`
- `src/Views/SongView.cpp`

**Integration**:
- MainView now owns a `std::unique_ptr<SongView>`
- SongView receives AudioController pointer in constructor
- MainView calls `m_songView->Render()` in place of old track controls
- Dialog triggers updated to use SongView methods

**Benefits achieved**:
- Reduced MainView.cpp from ~1959 lines to ~1560 lines (~400 lines moved)
- Track-related functionality is now isolated and reusable
- Clearer separation of concerns
- Easier to test and maintain track functionality

### 4. Extract BusView ✅ **COMPLETED**
**Status**: Successfully extracted and working

**What was moved**:
- Bus volume controls
- Bus filter/effects controls (collapsible)
- Bus FX parameter adjustment with type-specific controls
- Bus filter enable/disable functionality
- Special handling for biquad filter types
- Wet parameter highlighting and tooltips

**Files created**:
- `src/Views/BusView.h`
- `src/Views/BusView.cpp`

**Integration**:
- MainView now owns a `std::unique_ptr<BusView>`
- BusView receives AudioController pointer in constructor
- MainView calls `m_busView->Render()` in place of old bus controls
- Both SongView and BusView initialized in SetController()

**Benefits achieved**:
- Further reduced MainView.cpp by ~200 lines
- Bus-related functionality is now isolated and reusable
- Consistent pattern with SongView extraction
- Clean separation between track and bus controls

### 5. Extract SpeedView
**Target**: Extract tempo and granular controls

**What to move**:
- Tempo controls
- Tape speed controls
- Granular processing controls

### 6. Extract EventView
**Target**: Extract event management UI

**What to move**:
- Event list rendering
- Event creation dialogs
- Event triggering UI

### 7. Extract MenuBarView (Optional)
**Target**: Extract menu bar functionality

**What to move**:
- Menu bar rendering
- Theme selector
- Project/song dropdowns

### 8. Final Integration
- Ensure all views work together seamlessly
- Update any remaining cross-view dependencies
- Add comprehensive testing

---

## Example: SongView Extraction (Completed)

### Before
```cpp
// In MainView.cpp - 400+ lines of track-related code
void MainView::RenderTrackControls() { /* ... */ }
void MainView::drawFilterControls(size_t trackIndex) { /* ... */ }
void MainView::RenderNewSongDialog() { /* ... */ }
void MainView::RenderOggFileDialog() { /* ... */ }
```

### After
```cpp
// In MainView.cpp - clean and focused
void MainView::RenderMainWindow() {
    // ... other UI ...
    m_songView->Render();  // Delegated to SongView
    // ... other UI ...
}

// In SongView.cpp - dedicated track functionality
void SongView::Render() {
    RenderTrackControls();
    if (m_showNewSongDialog) RenderNewSongDialog();
    if (m_showOggFileDialog) RenderOggFileDialog();
}
```

---

## Example: BusView Extraction (Completed)

### Before
```cpp
// In MainView.cpp - 200+ lines of bus-related code
void MainView::RenderBusControls() { /* ... */ }
```

### After
```cpp
// In MainView.cpp - clean and focused
void MainView::RenderMainWindow() {
    // ... other UI ...
    m_songView->Render();  // Delegated to SongView
    ImGui::Separator();
    m_busView->Render();   // Delegated to BusView
    // ... other UI ...
}

// In BusView.cpp - dedicated bus functionality
void BusView::Render() {
    RenderBusControls();
}
```

---

## Benefits of This Approach

1. **Maintainability**: Each view has a single responsibility
2. **Reusability**: Views can be used in different contexts
3. **Testability**: Individual views can be tested in isolation
4. **Collaboration**: Multiple developers can work on different views
5. **Performance**: Only changed views need recompilation
6. **Clarity**: Code organization matches UI organization

---

## Next Steps

1. ✅ **SongView extraction completed**
2. ✅ **BusView extraction completed**
3. **Extract SpeedView** (recommended next step)
4. Continue with remaining views
5. Add comprehensive testing
6. Update documentation 