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
            // Continue lerping
            float t = m_currentTime / m_targetTime;
            lerpStates(t);
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

        // Capture current state
        m_startState = captureCurrentSongState();
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

        // Capture current state
        m_startMasterState = captureCurrentMasterState();
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

            // Interpolate tracks
            size_t maxTracks = std::max(m_startState.tracks.size(), m_targetState.tracks.size());
            lerpedState.tracks.resize(maxTracks);

            for (size_t i = 0; i < maxTracks; ++i) {
                TrackStateExtended& lerpedTrack = lerpedState.tracks[i];

                if (i < m_startState.tracks.size() && i < m_targetState.tracks.size()) {
                    // Both states have this track
                    const TrackStateExtended& startTrack = m_startState.tracks[i];
                    const TrackStateExtended& targetTrack = m_targetState.tracks[i];

                    lerpedTrack.file = targetTrack.file; // Use target file
                    lerpedTrack.volume = lerp(startTrack.volume, targetTrack.volume, t);
                    lerpedTrack.active = targetTrack.active; // Use target active state

                    // Interpolate effects
                    for (const auto& [effectName, targetEffect] : targetTrack.effects) {
                        auto startIt = startTrack.effects.find(effectName);
                        if (startIt != startTrack.effects.end()) {
                            // Effect exists in both states
                            lerpEffectState(startIt->second, targetEffect,
                                            lerpedTrack.effects[effectName], t);
                        } else {
                            // Effect only in target state
                            lerpedTrack.effects[effectName] = targetEffect;
                        }
                    }
                } else if (i < m_targetState.tracks.size()) {
                    // Track only in target state
                    lerpedTrack = m_targetState.tracks[i];
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

        // Apply track states
        for (size_t i = 0; i < state.tracks.size(); ++i) {
            const auto& track = state.tracks[i];

            // Find track index by filename (we'll need to add this to AudioController)
            int trackIndex = m_controller->findTrackByFilename(track.file);
            if (trackIndex >= 0) {
                m_controller->setTrackVolume(trackIndex, track.volume);
                m_controller->setTrackActive(trackIndex, track.active);

                // Apply effects
                for (const auto& [effectName, effectState] : track.effects) {
                    m_controller->setTrackEffectEnabled(trackIndex, effectName,
                                                        effectState.enabled);

                    for (const auto& [paramName, paramValue] : effectState.parameters) {
                        m_controller->setTrackEffectParameter(trackIndex, effectName, paramName,
                                                              paramValue);
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

        // Apply effects
        for (const auto& [effectName, effectState] : state.effects) {
            m_controller->setBusEffectEnabled(effectName, effectState.enabled);

            for (const auto& [paramName, paramValue] : effectState.parameters) {
                m_controller->setBusEffectParameter(effectName, paramName, paramValue);
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
            extendedTrack.active = track.active;

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

    void EventSystem::lerpEffectState(const EffectState& start, const EffectState& end,
                                      EffectState& result, float t) {
        result.enabled = end.enabled; // Use target enabled state

        // Interpolate parameters
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