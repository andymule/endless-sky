#include "EventSystem.h"
#include "AudioController.h"
#include "Logger.h"
#include "SongManager.h"
#include <algorithm>
#include <iostream>

namespace AudioTester {

    EventSystem::EventSystem(AudioController* controller) : m_controller(controller) {}

    void EventSystem::triggerSongEvent(const std::string& songName, const std::string& eventName) {
        if (!m_controller) {
            LOG_ERROR_COMP("EventSystem", "No controller available");
            return;
        }

        const SongManager* songManager = m_controller->getSongManager();
        if (!songManager) {
            LOG_ERROR_COMP("EventSystem", "No song manager available");
            return;
        }

        // Find the song
        const Song* song = songManager->findSong(songName);
        if (!song) {
            LOG_ERROR_COMP("EventSystem", "Song not found: " + songName);
            return;
        }

        // Find the event
        auto eventIt =
            std::find_if(song->events.begin(), song->events.end(),
                         [&eventName](const SongEvent& event) { return event.name == eventName; });

        if (eventIt == song->events.end()) {
            LOG_ERROR_COMP("EventSystem",
                           "Event not found: " + eventName + " in song: " + songName);
            return;
        }

        LOG_INFO_COMP("EventSystem", "Triggering song event: " + songName + " -> " + eventName);

        // Start transition
        startSongTransition(eventIt->state, eventIt->fadeTime);
    }

    void EventSystem::triggerMasterEvent(const std::string& eventName) {
        if (!m_controller) {
            LOG_ERROR_COMP("EventSystem", "No controller available");
            return;
        }

        const SongManager* songManager = m_controller->getSongManager();
        if (!songManager) {
            LOG_ERROR_COMP("EventSystem", "No song manager available");
            return;
        }

        // Find the master event
        const MasterBus& masterBus = songManager->getMasterBus();
        auto eventIt = std::find_if(
            masterBus.events.begin(), masterBus.events.end(),
            [&eventName](const MasterEvent& event) { return event.name == eventName; });

        if (eventIt == masterBus.events.end()) {
            LOG_ERROR_COMP("EventSystem", "Master event not found: " + eventName);
            return;
        }

        LOG_INFO_COMP("EventSystem", "Triggering master event: " + eventName);

        // Start transition
        startMasterTransition(eventIt->state, eventIt->fadeTime);
    }

    void EventSystem::update(float deltaTime) {
        if (!m_inTransition) {
            return;
        }

        m_currentTime += deltaTime;

        if (m_currentTime >= m_targetTime) {
            // Transition complete
            m_inTransition = false;
            m_transitionType = TransitionType::NONE;

            // Apply final state
            if (m_transitionType == TransitionType::SONG) {
                applyStateSnapshot(m_targetState);
            } else if (m_transitionType == TransitionType::MASTER) {
                applyMasterBusState(m_targetMasterState);
            }

            LOG_INFO_COMP("EventSystem", "Transition completed");
        } else {
            // Continue lerping with EASE_IN_OUT curve
            float rawT = m_currentTime / m_targetTime;
            float easedT = easeInOut(rawT);
            lerpStates(easedT);
        }
    }

    float EventSystem::getTransitionProgress() const {
        if (!m_inTransition || m_targetTime <= 0.0f) {
            return 0.0f;
        }
        return std::min(m_currentTime / m_targetTime, 1.0f);
    }

    void EventSystem::startSongTransition(const StateSnapshot& target, float fadeTime) {
        LOG_INFO_COMP("EventSystem",
                      "Starting song transition (" + std::to_string(fadeTime) + "s)");

        // For elegant cancelling: Always capture the current LIVE state
        // If we're mid-transition, this gets the interpolated state, not the original start
        if (m_inTransition && m_transitionType == TransitionType::SONG) {
            // We're already in a song transition - capture the current interpolated state
            float rawT = m_currentTime / m_targetTime;
            float easedT = easeInOut(rawT);

            // Create current interpolated state as new starting point
            StateSnapshot currentState;
            currentState.masterTempo =
                lerp(m_startState.masterTempo, m_targetState.masterTempo, easedT);
            currentState.granularTempo =
                lerp(m_startState.granularTempo, m_targetState.granularTempo, easedT);

            // Interpolate current track states
            size_t maxTracks = std::max(m_startState.tracks.size(), m_targetState.tracks.size());
            currentState.tracks.resize(maxTracks);

            for (size_t i = 0; i < maxTracks; ++i) {
                if (i < m_startState.tracks.size() && i < m_targetState.tracks.size()) {
                    const auto& startTrack = m_startState.tracks[i];
                    const auto& targetTrack = m_targetState.tracks[i];

                    TrackStateExtended& currentTrack = currentState.tracks[i];
                    currentTrack.file = targetTrack.file;
                    currentTrack.volume = lerp(startTrack.volume, targetTrack.volume, easedT);

                    // For effects, use target effects (simplified for now)
                    currentTrack.effects = targetTrack.effects;
                } else if (i < m_targetState.tracks.size()) {
                    currentState.tracks[i] = m_targetState.tracks[i];
                }
            }

            m_startState = currentState;
        } else {
            // Not in transition or different transition type - capture fresh state
            // This handles cross-transition (master->song) elegantly
            m_startState = captureCurrentSongState();
        }

        m_targetState = target;

        // Setup transition
        m_inTransition = true;
        m_transitionType = TransitionType::SONG;
        m_transitionTime = fadeTime;
        m_targetTime = fadeTime;
        m_currentTime = 0.0f;
    }

    void EventSystem::startMasterTransition(const MasterBusState& target, float fadeTime) {
        LOG_INFO_COMP("EventSystem",
                      "Starting master transition (" + std::to_string(fadeTime) + "s)");

        // For elegant cancelling: Always capture the current LIVE state
        if (m_inTransition && m_transitionType == TransitionType::MASTER) {
            // We're already in a master transition - capture the current interpolated state
            float rawT = m_currentTime / m_targetTime;
            float easedT = easeInOut(rawT);

            // Create current interpolated state as new starting point
            MasterBusState currentState;
            currentState.masterTempo =
                lerp(m_startMasterState.masterTempo, m_targetMasterState.masterTempo, easedT);
            currentState.granularTempo =
                lerp(m_startMasterState.granularTempo, m_targetMasterState.granularTempo, easedT);
            currentState.volume =
                lerp(m_startMasterState.volume, m_targetMasterState.volume, easedT);

            // For effects, use target effects (simplified for now)
            currentState.effects = m_targetMasterState.effects;

            m_startMasterState = currentState;
        } else {
            // Not in transition or different transition type - capture fresh state
            // This handles cross-transition (song->master) elegantly
            m_startMasterState = captureCurrentMasterState();
        }

        m_targetMasterState = target;

        // Setup transition
        m_inTransition = true;
        m_transitionType = TransitionType::MASTER;
        m_transitionTime = fadeTime;
        m_targetTime = fadeTime;
        m_currentTime = 0.0f;
    }

    void EventSystem::lerpStates(float t) {
        if (m_transitionType == TransitionType::SONG) {
            // Create interpolated state
            StateSnapshot lerpedState;
            lerpedState.masterTempo = lerp(m_startState.masterTempo, m_targetState.masterTempo, t);
            lerpedState.granularTempo =
                lerp(m_startState.granularTempo, m_targetState.granularTempo, t);

            // For tracks, we need to handle them by filename to ensure proper interpolation
            // Target state defines which tracks should exist
            lerpedState.tracks.resize(m_targetState.tracks.size());

            for (size_t i = 0; i < m_targetState.tracks.size(); ++i) {
                const auto& targetTrack = m_targetState.tracks[i];
                TrackStateExtended& lerpedTrack = lerpedState.tracks[i];

                // Find corresponding track in start state by filename
                const TrackStateExtended* startTrack = nullptr;
                for (const auto& startT : m_startState.tracks) {
                    if (startT.file == targetTrack.file) {
                        startTrack = &startT;
                        break;
                    }
                }

                lerpedTrack.file = targetTrack.file; // Always use target filename

                if (startTrack) {
                    // Track exists in both states - interpolate volume
                    lerpedTrack.volume = lerp(startTrack->volume, targetTrack.volume, t);
                } else {
                    // Track only exists in target - use current live volume as start
                    // This handles the case where JSON has tracks that aren't in captured state
                    int trackIndex = m_controller->findTrackByFilename(targetTrack.file);
                    float currentVolume = 1.0f; // Default fallback
                    if (trackIndex >= 0) {
                        const auto& audioState = m_controller->getState();
                        if (trackIndex < static_cast<int>(audioState.getTrackCount())) {
                            currentVolume = audioState.getTrack(trackIndex).volume;
                        }
                    }
                    lerpedTrack.volume = lerp(currentVolume, targetTrack.volume, t);
                }

                // Interpolate effects
                for (const auto& [effectName, targetEffect] : targetTrack.effects) {
                    if (startTrack) {
                        auto startIt = startTrack->effects.find(effectName);
                        if (startIt != startTrack->effects.end()) {
                            // Effect exists in both states
                            lerpEffectState(startIt->second, targetEffect,
                                            lerpedTrack.effects[effectName], t);
                        } else {
                            // Effect only in target state
                            lerpedTrack.effects[effectName] = targetEffect;
                        }
                    } else {
                        // No start track, use target effect
                        lerpedTrack.effects[effectName] = targetEffect;
                    }
                }
            }

            applyStateSnapshot(lerpedState);

        } else if (m_transitionType == TransitionType::MASTER) {
            // Create interpolated master state
            MasterBusState lerpedState;
            lerpedState.masterTempo =
                lerp(m_startMasterState.masterTempo, m_targetMasterState.masterTempo, t);
            lerpedState.granularTempo =
                lerp(m_startMasterState.granularTempo, m_targetMasterState.granularTempo, t);
            lerpedState.volume = lerp(m_startMasterState.volume, m_targetMasterState.volume, t);

            // Interpolate effects
            for (const auto& [effectName, targetEffect] : m_targetMasterState.effects) {
                auto startIt = m_startMasterState.effects.find(effectName);
                if (startIt != m_startMasterState.effects.end()) {
                    // Effect exists in both states
                    lerpEffectState(startIt->second, targetEffect, lerpedState.effects[effectName],
                                    t);
                } else {
                    // Effect only in target state
                    lerpedState.effects[effectName] = targetEffect;
                }
            }

            applyMasterBusState(lerpedState);
        }
    }

    void EventSystem::applyStateSnapshot(const StateSnapshot& state) {
        if (!m_controller)
            return;

        // Apply tempo controls
        m_controller->setMasterTempo(state.masterTempo);
        m_controller->setGranularTempo(state.granularTempo);

        // Apply track states with complete effect reset
        const auto& audioSystem = m_controller->getAudioSystem();

        for (size_t i = 0; i < state.tracks.size(); ++i) {
            const auto& track = state.tracks[i];

            // Find track index by filename
            int trackIndex = m_controller->findTrackByFilename(track.file);
            if (trackIndex >= 0) {
                m_controller->setTrackVolume(trackIndex, track.volume);

                // COMPLETE EFFECT RESET: First disable ALL effects for this track
                for (const auto& filterName : AudioTester::AudioSystem::AVAILABLE_FILTERS) {
                    m_controller->setTrackEffectEnabled(trackIndex, filterName, false);
                }

                // Then apply effects from the snapshot
                for (const auto& [effectName, effectState] : track.effects) {
                    // Apply all effect parameters using IDs instead of names
                    for (const auto& [paramIdStr, paramValue] : effectState.parameters) {
                        try {
                            // Convert string back to int ID
                            int paramId = std::stoi(paramIdStr);
                            m_controller->setTrackFilterParameter(trackIndex, effectName, paramId,
                                                                  paramValue);
                        } catch (const std::exception& e) {
                            // Skip invalid parameter IDs - just continue silently
                            continue;
                        }
                    }
                }
            }
        }
    }

    void EventSystem::applyMasterBusState(const MasterBusState& state) {
        if (!m_controller)
            return;

        // Apply tempo controls
        m_controller->setMasterTempo(state.masterTempo);
        m_controller->setGranularTempo(state.granularTempo);

        // Apply bus volume
        m_controller->setBusVolume(state.volume);

        // COMPLETE EFFECT RESET: First disable ALL bus effects
        for (const auto& filterName : AudioTester::AudioSystem::AVAILABLE_FILTERS) {
            m_controller->setBusEffectEnabled(filterName, false);
        }

        // Then apply effects from the snapshot
        for (const auto& [effectName, effectState] : state.effects) {
            // Apply all effect parameters using IDs instead of names
            for (const auto& [paramIdStr, paramValue] : effectState.parameters) {
                try {
                    // Convert string back to int ID
                    int paramId = std::stoi(paramIdStr);
                    m_controller->setBusFilterParameter(effectName, paramId, paramValue);
                } catch (const std::exception& e) {
                    // Skip invalid parameter IDs - just continue silently
                    continue;
                }
            }
        }
    }

    StateSnapshot EventSystem::captureCurrentSongState() {
        StateSnapshot state;

        if (!m_controller)
            return state;

        // Capture tempo
        state.masterTempo = m_controller->getMasterTempo();
        state.granularTempo = m_controller->getGranularTempo();

        // Capture tracks
        const auto& audioState = m_controller->getState();

        for (size_t i = 0; i < audioState.getTrackCount(); ++i) {
            const auto& track = audioState.getTrack(i);

            TrackStateExtended extendedTrack;
            extendedTrack.file = std::filesystem::path(track.filepath).filename().string();
            extendedTrack.volume = track.volume;

            // Capture effects (we'll need to add this to AudioController)
            // For now, leave effects empty as we'll implement this in the next step

            state.tracks.push_back(extendedTrack);
        }

        return state;
    }

    MasterBusState EventSystem::captureCurrentMasterState() {
        MasterBusState state;

        if (!m_controller)
            return state;

        // Capture tempo
        state.masterTempo = m_controller->getMasterTempo();
        state.granularTempo = m_controller->getGranularTempo();

        // Capture bus volume
        state.volume = m_controller->getState().busVolume;

        // Capture effects (we'll implement this in the next step)

        return state;
    }

    float EventSystem::lerp(float a, float b, float t) { return a + (b - a) * t; }

    float EventSystem::easeInOut(float t) {
        // EASE_IN_OUT curve: slow start, fast middle, slow end
        return t < 0.5f ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
    }

    void EventSystem::lerpEffectState(const EffectState& start, const EffectState& end,
                                      EffectState& result, float t) {
        // Interpolate parameters (enabled state is handled automatically by wet parameter)
        for (const auto& [paramName, endValue] : end.parameters) {
            auto startIt = start.parameters.find(paramName);
            if (startIt != start.parameters.end()) {
                // Parameter exists in both states
                result.parameters[paramName] = lerp(startIt->second, endValue, t);
            } else {
                // Parameter only in end state
                result.parameters[paramName] = endValue;
            }
        }
    }

} // namespace AudioTester