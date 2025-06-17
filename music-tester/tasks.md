### CODING PROTOCOL ###
" Coding Instructions
- Write the absolute minimum code required
- No sweeping changes
- No unrelated edits - focus on just the task you're on
- Make code precise, modular
- Don't break existing functionality
- If I need to do anything, tell me clearly "


# Music Tester Architecture Improvements

## Phase 1: Core Architecture Improvements

### 3. State Management
- [x] Create `AudioState` class to centralize state
- [x] Implement state change notifications
- [x] Remove state duplication between `TesterView` and `AudioSystem`
- [x] Add state persistence capabilities

## Phase 2: UI and Configuration

### 4. Configuration Management
- [ ] Create `Config` class for centralized configuration
- [ ] Implement configuration file loading/saving
- [ ] Move hardcoded values to configuration
- [ ] Add configuration validation

### 5. UI Architecture
- [ ] Separate UI rendering from business logic
- [ ] Implement proper MVC/MVVM architecture
- [ ] Create UI state management system
- [ ] Add UI component tests


## Phase 4: Build System and Project Structure

### 8. Build System Improvements
- [ ] Reorganize CMake structure
- [ ] Add proper dependency management
- [ ] Add build configuration options
- [ ] Improve build performance

### 9. Project Structure
- [ ] Reorganize code into logical modules:
  ```
  src/
    ├── audio/           # Core audio engine
    ├── ui/             # UI components
    ├── utils/          # Utilities
    ├── config/         # Configuration
    └── tests/          # Test files
  ```
- [ ] Update include paths
- [ ] Add module documentation
- [ ] Create module interfaces

## Phase 5: Performance and Optimization

### 10. Performance Improvements
- [ ] Profile audio processing pipeline
- [ ] Optimize filter processing
- [ ] Implement audio streaming for large files
- [ ] Add performance monitoring

## Notes
- Each task should be completed with corresponding tests
- Documentation should be updated as features are implemented
- Consider backward compatibility when making changes
- Prioritize tasks based on current needs and dependencies

## Getting Started
1. Start with Phase 1 tasks as they form the foundation
2. Complete each task with tests before moving to the next
3. Review and refactor as needed
4. Update documentation as you go 