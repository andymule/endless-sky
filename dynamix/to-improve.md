# Engineering Improvements - High Priority

This document tracks high-priority engineering improvements observed during codebase work. Only significant architectural, performance, or maintainability issues are included.

## 🏗️ **Architecture Issues**

### 1. **Mixed MVC Architecture** - HIGH PRIORITY
**Issue**: View components (SongView, BusView) are tightly coupled with business logic
**Evidence**: 
- `SongView::ShowOggFileDialog()` needs callbacks to `MainView`
- Dialog state scattered across multiple components
- UI components directly call controller methods

**Impact**: Hard to test, maintain, and extend UI components
**Suggested Fix**: Implement proper MVC with event/observer pattern

### 2. **Inconsistent Error Handling** - HIGH PRIORITY
**Issue**: Mixed error handling patterns throughout codebase
**Evidence**:
- Some methods return `bool` for success/failure
- Others throw exceptions
- Some log errors, others don't
- Inconsistent error messages and user feedback

**Impact**: Difficult debugging, poor user experience
**Suggested Fix**: Implement unified error handling system (already partially exists in ErrorHandling.h)

## 🔄 **Code Duplication Issues**

### 3. **Duplicate File Operations** - MEDIUM PRIORITY
**Issue**: Multiple implementations of similar file operations
**Evidence**:
- `CopyOggFileToSong()` existed in both MainView and SongView (fixed)
- File path resolution logic scattered across components
- Directory creation patterns repeated

**Impact**: Maintenance burden, inconsistent behavior
**Status**: Partially addressed, more consolidation needed

### 4. **State Management Duplication** - HIGH PRIORITY
**Issue**: Audio state tracked in multiple places
**Evidence** (from debt.md):
- `AudioState` and `AudioSystem` maintain separate track state
- Volume control issues due to state synchronization problems
- "Single source of truth" pattern not consistently applied

**Impact**: Sync bugs, data inconsistency
**Status**: Mentioned in debt.md as completed, verify implementation

## 🎯 **API Design Issues**

### 5. **External API Incompleteness** - MEDIUM PRIORITY
**Issue**: External C API missing key functionality
**Evidence**:
- `dynamix_loadAndPlaySong()` was declared but not implemented (fixed)
- No API for song creation or project management
- Limited error reporting from C API

**Impact**: Limited game engine integration capabilities
**Suggested Fix**: Expand C API with comprehensive functionality

### 6. **Inconsistent Naming Conventions** - MEDIUM PRIORITY
**Issue**: Mixed naming patterns across codebase
**Evidence**:
- `setTrackVolume` vs `setBusVolume` (inconsistent parameter order)
- Some methods use camelCase, others use snake_case in different contexts
- File naming inconsistencies (_song.json vs songName.json - fixed)

**Impact**: Developer confusion, harder codebase navigation
**Status**: Mentioned in debt.md, needs systematic cleanup

## 🚀 **Performance Concerns**

### 7. **Inefficient Directory Scanning** - LOW PRIORITY
**Issue**: Full directory rescans on every project switch
**Evidence**:
- `loadMusicFromDirectory()` rescans entire directory structure
- No caching of project/song metadata
- File system operations in UI thread

**Impact**: UI lag when switching projects with many songs
**Suggested Fix**: Implement metadata caching and background scanning

## 🧪 **Testing Infrastructure** - HIGH PRIORITY

### 8. **No Unit Testing Framework** - HIGH PRIORITY
**Issue**: No automated testing infrastructure
**Evidence**:
- No test files in codebase
- Manual testing only
- Refactoring requires extensive manual verification

**Impact**: High risk of regressions, slow development
**Suggested Fix**: Add CMake test targets with catch2 or similar framework

### 9. **No Integration Testing** - MEDIUM PRIORITY
**Issue**: No testing of external API or full workflows
**Evidence**:
- C API functions not tested
- Event triggering workflows not automated
- File operations not tested

**Impact**: API reliability concerns for game engine integration

## 🔧 **Configuration Management**

### 10. **Hardcoded Paths and Constants** - MEDIUM PRIORITY
**Issue**: Magic numbers and hardcoded paths throughout codebase
**Evidence**:
- Default directory paths hardcoded in multiple places
- UI dimensions and timeouts as magic numbers
- File extension checks hardcoded (".ogg")

**Impact**: Hard to customize, port, or configure
**Suggested Fix**: Centralized configuration system

## 📊 **Monitoring and Observability**

### 11. **Limited Logging Strategy** - LOW PRIORITY
**Issue**: Inconsistent logging levels and component identification
**Evidence**:
- Some components use LOG_ERROR_COMP, others don't
- No performance logging or metrics
- Debug information scattered

**Impact**: Difficult production debugging
**Status**: Logger system exists but not consistently used

---

## 🎯 **Implementation Priority**

### **Immediate (Next Sprint)**
1. **Mixed MVC Architecture** - Refactor dialog system
2. **No Unit Testing Framework** - Add basic test infrastructure
3. **State Management Duplication** - Verify and complete single source of truth

### **Short Term (Next Month)**
4. **Inconsistent Error Handling** - Implement unified error system
5. **External API Incompleteness** - Expand C API coverage
6. **Inconsistent Naming Conventions** - Systematic cleanup

### **Long Term (Future)**
7. **Performance Concerns** - Optimize directory scanning
8. **Configuration Management** - Centralized config system
9. **Integration Testing** - Full workflow automation

---

*Last Updated: Current session*
*Scope: Observations from recent codebase work*
