# Technical Debt Analysis & Refactoring Plan

## Current State Assessment

The Music Tester project has evolved into a complex audio system with several architectural issues:

### Major Issues Identified:
1. **State Duplication**: `AudioState` and `AudioSystem` maintain separate track state
2. **Massive Classes**: `AudioSystem` (299 lines) and `EventSystem` (688 lines) are too large
3. **Complex Synchronization**: Audio sync logic is scattered and overly complex
4. **Tight Coupling**: UI, audio, and state management are tightly coupled
5. **Inconsistent Error Handling**: Mixed error handling patterns throughout

## Tightly Scoped Refactoring Plan

### Phase 1: Critical State Management (High Impact, Low Risk)

- [x] **Task 1.1**: Add comprehensive documentation ✅
- [x] **Task 1.2**: Extract common file browser component ✅  
- [x] **Task 1.3**: Improve error handling consistency ✅

- [ ] **Task 1.4**: Eliminate AudioState/AudioSystem duplication
  - [ ] Move track volume storage to `AudioSystem::TrackInfo`
  - [ ] Remove volume tracking from `AudioState::TrackState`
  - [ ] Update `AudioController` to use single source of truth
  - [ ] **Impact**: Eliminates sync issues, reduces complexity
  - [ ] **Risk**: Low - isolated change to state management

- [ ] **Task 1.5**: Simplify track loading workflow
  - [ ] Remove `loadMusicFromDirectory()` calls from add/remove track methods
  - [ ] Implement incremental track loading in `AudioController`
  - [ ] **Impact**: Fixes volume control issues, improves performance
  - [ ] **Risk**: Low - isolated to track management

### Phase 2: Audio System Decomposition (Medium Impact, Medium Risk)

- [ ] **Task 2.1**: Extract TrackManager from AudioSystem
  - [ ] Create `TrackManager` class with track CRUD operations
  - [ ] Move `TrackInfo`, track loading, and track state management
  - [ ] Keep `AudioSystem` focused on audio engine coordination
  - [ ] **Impact**: Reduces AudioSystem complexity by ~40%
  - [ ] **Risk**: Medium - requires careful interface design

- [ ] **Task 2.2**: Extract FilterManager from AudioSystem
  - [ ] Move all filter-related methods to existing `FilterManager`
  - [ ] Create clean filter parameter interface
  - [ ] Remove duplicate filter state tracking
  - [ ] **Impact**: Eliminates filter duplication, improves maintainability
  - [ ] **Risk**: Medium - requires careful state migration

- [ ] **Task 2.3**: Simplify audio synchronization
  - [ ] Reduce `SyncState` complexity (remove unused fields)
  - [ ] Simplify `updateSync()` method to essential checks only
  - [ ] Remove complex drift correction logic
  - [ ] **Impact**: Reduces complexity, improves performance
  - [ ] **Risk**: Low - can be done incrementally

### Phase 3: Event System Simplification (Medium Impact, Medium Risk)

- [ ] **Task 3.1**: Break down EventSystem::lerpStates()
  - [ ] Extract `StateInterpolator` class for state transitions
  - [ ] Create `EffectLerper` class for effect parameter interpolation
  - [ ] Reduce method from 200+ lines to <50 lines
  - [ ] **Impact**: Improves readability and testability
  - [ ] **Risk**: Medium - requires careful state preservation

- [ ] **Task 3.2**: Simplify event state capture
  - [ ] Consolidate state capture logic in one place
  - [ ] Remove duplicate state capture between `AudioController` and `EventSystem`
  - [ ] Create unified `StateCapture` utility
  - [ ] **Impact**: Eliminates state capture bugs, improves consistency
  - [ ] **Risk**: Low - isolated to state capture logic

### Phase 4: UI/Controller Separation (Low Impact, Low Risk)

- [ ] **Task 4.1**: Remove UI dependencies from AudioController
  - [ ] Move UI-specific logic to `TesterView`
  - [ ] Create clean controller interface for UI
  - [ ] Remove direct UI state management from controller
  - [ ] **Impact**: Improves separation of concerns
  - [ ] **Risk**: Low - can be done incrementally

- [ ] **Task 4.2**: Standardize naming conventions
  - [ ] Fix inconsistent method naming (e.g., `setTrackVolume` vs `setBusVolume`)
  - [ ] Standardize parameter naming across classes
  - [ ] Add automated formatting (clang-format)
  - [ ] **Impact**: Improves code readability and maintainability
  - [ ] **Risk**: Low - mechanical changes

## Success Criteria

### Phase 1 Success:
- [ ] Volume controls work reliably after track operations
- [ ] No duplicate state between AudioState and AudioSystem
- [ ] Track loading is incremental and fast

### Phase 2 Success:
- [ ] AudioSystem class reduced to <200 lines
- [ ] Filter management is centralized and consistent
- [ ] Audio sync is simple and reliable

### Phase 3 Success:
- [ ] EventSystem::lerpStates() is <50 lines
- [ ] State capture is unified and bug-free
- [ ] Event transitions are smooth and predictable

### Phase 4 Success:
- [ ] Controller has no UI dependencies
- [ ] Consistent naming throughout codebase
- [ ] Code is automatically formatted

## Implementation Strategy

### Principles:
1. **Incremental Changes**: Each task should be completable in 1-2 hours
2. **Backward Compatibility**: Changes should not break existing functionality
3. **Test-Driven**: Add tests for critical paths before refactoring
4. **Documentation**: Update documentation as part of each change

### Risk Mitigation:
1. **Isolated Changes**: Each task affects only one component
2. **Rollback Plan**: Keep git commits small and focused
3. **Validation**: Test each change thoroughly before proceeding
4. **Incremental Testing**: Add tests incrementally with each change

## Timeline Estimate

- **Phase 1**: 1-2 days (critical fixes)
- **Phase 2**: 3-4 days (audio system cleanup)
- **Phase 3**: 2-3 days (event system simplification)
- **Phase 4**: 1-2 days (polish and cleanup)

**Total**: 7-11 days for complete refactoring

## Notes

- Focus on **impact vs effort** - prioritize high-impact, low-effort changes
- **Avoid premature optimization** - don't refactor working code unless it's causing issues
- **Maintain functionality** - all existing features must continue to work
- **Document as you go** - update this document as tasks are completed 