#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * External API for Music Tester - Event-Driven Music System
 *
 * This header can be included by game engines to integrate with the Music Tester
 * event system for dynamic music control.
 */

/**
 * Trigger a song-specific event
 * @param songName Name of the song (folder name)
 * @param eventName Name of the event within that song
 */
void musicTester_triggerSongEvent(const char* songName, const char* eventName);

/**
 * Trigger a master bus event (affects all music globally)
 * @param eventName Name of the master event
 */
void musicTester_triggerMasterEvent(const char* eventName);

/**
 * Load songs from a directory
 * @param directory Path to directory containing song folders
 */
void musicTester_loadSongsFromDirectory(const char* directory);

#ifdef __cplusplus
}
#endif