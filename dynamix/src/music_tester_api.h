#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * External API for Dynamix - Event-Driven Music System
 *
 * This header can be included by game engines to integrate with the Dynamix
 * event system for dynamic music control.
 *
 * All functions return error codes for robust integration.
 * Check return values and call dynamix_getLastError() for details.
 */

/**
 * Error codes returned by API functions
 */
typedef enum {
    DYNAMIX_SUCCESS = 0,           // Operation successful
    DYNAMIX_ERROR_NULL_PARAM = -1, // NULL parameter passed
    DYNAMIX_ERROR_EMPTY_PARAM = -2, // Empty string parameter
    DYNAMIX_ERROR_NOT_INITIALIZED = -3, // Dynamix not initialized
    DYNAMIX_ERROR_SONG_NOT_FOUND = -4, // Song doesn't exist
    DYNAMIX_ERROR_EVENT_NOT_FOUND = -5, // Event doesn't exist
    DYNAMIX_ERROR_INVALID_DIRECTORY = -6, // Directory doesn't exist or is invalid
    DYNAMIX_ERROR_PARAMETER_TOO_LONG = -7, // Parameter exceeds maximum length
    DYNAMIX_ERROR_INTERNAL = -8 // Internal system error
} DynamixErrorCode;

/**
 * Get human-readable error message for the last operation
 * @return Error message string, or NULL if no error
 */
const char* dynamix_getLastError(void);

/**
 * Get the error code from the last operation
 * @return DynamixErrorCode value
 */
DynamixErrorCode dynamix_getLastErrorCode(void);

/**
 * Clear the last error (reset to DYNAMIX_SUCCESS)
 */
void dynamix_clearError(void);

/**
 * Trigger a song-specific event
 * @param songName Name of the song (folder name) - must not be NULL or empty
 * @param eventName Name of the event within that song - must not be NULL or empty
 * @return DYNAMIX_SUCCESS on success, error code on failure
 */
DynamixErrorCode dynamix_triggerSongEvent(const char* songName, const char* eventName);

/**
 * Trigger a master bus event (affects all music globally)
 * @param eventName Name of the master event - must not be NULL or empty
 * @return DYNAMIX_SUCCESS on success, error code on failure
 */
DynamixErrorCode dynamix_triggerMasterEvent(const char* eventName);

/**
 * Trigger any event (searches both songs and master events automatically)
 * This is the recommended function for game engines - just pass the event name
 * @param eventName Name of the event to trigger - must not be NULL or empty
 * @return DYNAMIX_SUCCESS on success, error code on failure
 */
DynamixErrorCode dynamix_triggerEvent(const char* eventName);

/**
 * Load music from a directory (both tracks and events automatically)
 * This will load both individual track files and song/event definitions
 * @param directory Path to directory containing tracks and song folders - must not be NULL or empty
 * @return DYNAMIX_SUCCESS on success, error code on failure
 */
DynamixErrorCode dynamix_loadSongsFromDirectory(const char* directory);

/**
 * Load and switch to a specific song by name
 * @param songName Name of the song folder to load and play - must not be NULL or empty
 * @return DYNAMIX_SUCCESS on success, error code on failure
 */
DynamixErrorCode dynamix_loadAndPlaySong(const char* songName);

/**
 * Initialize Dynamix system (call this first)
 * @param executableDirectory Optional directory where Dynamix executable is located
 * @return DYNAMIX_SUCCESS on success, error code on failure
 */
DynamixErrorCode dynamix_initialize(const char* executableDirectory);

/**
 * Shutdown Dynamix system (call when done)
 */
void dynamix_shutdown(void);

/**
 * Check if Dynamix is initialized and ready
 * @return 1 if initialized, 0 if not
 */
int dynamix_isInitialized(void);

#ifdef __cplusplus
}
#endif