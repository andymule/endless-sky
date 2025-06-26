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
 * Trigger any event (searches both songs and master events automatically)
 * This is the recommended function for game engines - just pass the event name
 * @param eventName Name of the event to trigger
 */
void musicTester_triggerEvent(const char* eventName);

/**
 * Load music from a directory (both tracks and events automatically)
 * This will load both individual track files and song/event definitions
 * @param directory Path to directory containing tracks and song folders
 */
void musicTester_loadSongsFromDirectory(const char* directory);

#ifdef __cplusplus
}
#endif