# TODO - Items Needing Discussion

## 2. Master Bus Directory Structure
**Current Implementation**: Uses parent directory of first song for `_master.json`
**Proposed Approach**: 
- User selects root directory (e.g., `sound_staging/`)
- System looks for `_master.json` in root directory
- System scans subdirectories for `_song.json` files
- All `.ogg` files in root and subdirectories loaded as available tracks
- Events can reference any loaded track by filename

**Questions to Discuss**:
- Should `_master.json` be required or optional? Required
- How to handle tracks that exist in multiple song folders? we only load songs as hollistic entities, it doesnt matter at all if the same track is in more than one song
- Should there be a "global tracks" folder at root level? no
- How to organize the UI to show available songs vs loaded tracks? clarified below

### UI and Workflow Improvements
**Current State**: Single "Load" button in main window
**Proposed Changes**:
1. **Add File Menu** with "New Master", "New Song", and "Load" buttons (remove current load button)
2. **New Master**: Creates new directory with empty but valid `_master.json`
3. **New Song**: Creates new subfolder with empty but valid `_song.json`
4. **Event Browser Enhancement**: Add song browsing at bottom to change active song
   - Loading a song resets all tracks & effects
   - Does NOT reset master bus or playback speed
   - Shows which song is currently active in song window

## 3. Track Loading Strategy
**Current Issue**: Only loads tracks referenced in song events
**Proposed Fix**: Load ALL `.ogg` files in directory structure
**Implementation Needed**: Modify `AudioController::loadMusicFromDirectory()` to discover and load all OGG files regardless of event references

## 4. Event Browser UI Enhancement
**Current State**: Basic event creation/deletion
**Proposed Features**:
- Show available songs (folders with `_song.json`) in a separate panel
- Allow loading songs into active memory
- Show all available tracks vs currently loaded tracks
- Better organization of master events vs song events

## 5. Error Recovery Strategy
**Current State**: Basic error logging
**Proposed Enhancements**:
- Graceful degradation when files are missing
- Retry mechanisms for file loading
- User-friendly error messages in UI
- Recovery options when JSON is corrupted

## 6. Performance Considerations
**Areas to Discuss**:
- Memory usage with many loaded tracks
- Audio streaming optimization
- Transition performance with complex effect chains
- UI responsiveness with large event lists 