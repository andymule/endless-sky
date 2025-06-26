#include "EventSystem.h"
#include "AudioController.h"
#include "Logger.h"
#include "SongManager.h"
#include <algorithm>
#include <iostream>
#include <set>

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
            StateSnapshot lerpedState;
            lerpedState.masterTempo = lerp(m_startState.masterTempo, m_targetState.masterTempo, t);
            lerpedState.granularTempo =
                lerp(m_startState.granularTempo, m_targetState.granularTempo, t);

            size_t maxTracks = std::max(m_startState.tracks.size(), m_targetState.tracks.size());
            lerpedState.tracks.resize(maxTracks);

            for (size_t i = 0; i < maxTracks; ++i) {
                // Defensive: check bounds for both start and target
                const TrackStateExtended* targetTrack =
                    (i < m_targetState.tracks.size()) ? &m_targetState.tracks[i] : nullptr;
                const TrackStateExtended* startTrack =
                    (i < m_startState.tracks.size()) ? &m_startState.tracks[i] : nullptr;
                TrackStateExtended& lerpedTrack = lerpedState.tracks[i];

                if (targetTrack)
                    lerpedTrack.file = targetTrack->file;
                else if (startTrack)
                    lerpedTrack.file = startTrack->file;
                else
                    lerpedTrack.file = "";

                // Volume
                float startVol = startTrack ? startTrack->volume : 1.0f;
                float targetVol = targetTrack ? targetTrack->volume : 1.0f;
                lerpedTrack.volume = lerp(startVol, targetVol, t);

                // Effects: union of all effect names in start and target
                std::set<std::string> allEffects;
                if (startTrack)
                    for (const auto& [ename, _] : startTrack->effects) allEffects.insert(ename);
                if (targetTrack)
                    for (const auto& [ename, _] : targetTrack->effects) allEffects.insert(ename);

                for (const auto& effectName : allEffects) {
                    const EffectState* startEff =
                        (startTrack && startTrack->effects.count(effectName))
                            ? &startTrack->effects.at(effectName)
                            : nullptr;
                    const EffectState* targetEff =
                        (targetTrack && targetTrack->effects.count(effectName))
                            ? &targetTrack->effects.at(effectName)
                            : nullptr;

                    if (startEff && targetEff) {
                        lerpEffectState(*startEff, *targetEff, lerpedTrack.effects[effectName], t);
                    } else if (targetEff) {
                        // No start, lerp from zero wet
                        EffectState zeroStart = *targetEff;
                        zeroStart.parameters["0"] = 0.0f;
                        lerpEffectState(zeroStart, *targetEff, lerpedTrack.effects[effectName], t);
                    } else if (startEff) {
                        // No target, lerp to zero wet
                        EffectState zeroTarget = *startEff;
                        zeroTarget.parameters["0"] = 0.0f;
                        lerpEffectState(*startEff, zeroTarget, lerpedTrack.effects[effectName], t);
                    }
                }
            }
            applyStateSnapshot(lerpedState);
        } else if (m_transitionType == TransitionType::MASTER) {
            MasterBusState lerpedState;
            lerpedState.masterTempo =
                lerp(m_startMasterState.masterTempo, m_targetMasterState.masterTempo, t);
            lerpedState.granularTempo =
                lerp(m_startMasterState.granularTempo, m_targetMasterState.granularTempo, t);
            lerpedState.volume = lerp(m_startMasterState.volume, m_targetMasterState.volume, t);

            // Effects: union of all effect names in start and target
            std::set<std::string> allEffects;
            for (const auto& [ename, _] : m_startMasterState.effects) allEffects.insert(ename);
            for (const auto& [ename, _] : m_targetMasterState.effects) allEffects.insert(ename);

            for (const auto& effectName : allEffects) {
                const EffectState* startEff = m_startMasterState.effects.count(effectName)
                                                  ? &m_startMasterState.effects.at(effectName)
                                                  : nullptr;
                const EffectState* targetEff = m_targetMasterState.effects.count(effectName)
                                                   ? &m_targetMasterState.effects.at(effectName)
                                                   : nullptr;

                if (startEff && targetEff) {
                    lerpEffectState(*startEff, *targetEff, lerpedState.effects[effectName], t);
                } else if (targetEff) {
                    EffectState zeroStart = *targetEff;
                    zeroStart.parameters["0"] = 0.0f;
                    lerpEffectState(zeroStart, *targetEff, lerpedState.effects[effectName], t);
                } else if (startEff) {
                    EffectState zeroTarget = *startEff;
                    zeroTarget.parameters["0"] = 0.0f;
                    lerpEffectState(*startEff, zeroTarget, lerpedState.effects[effectName], t);
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

        // Apply track states with smart wet level logic
        for (size_t i = 0; i < state.tracks.size(); ++i) {
            const auto& track = state.tracks[i];

            // Find track index by filename
            int trackIndex = m_controller->findTrackByFilename(track.file);
            if (trackIndex >= 0) {
                m_controller->setTrackVolume(trackIndex, track.volume);

                // Get current audio system state for comparison
                const auto& currentFilters = m_controller->getAudioSystem().getFilters(trackIndex);

                // Apply effects with wet level logic
                for (const auto& [effectName, effectState] : track.effects) {
                    auto currentIt = currentFilters.find(effectName);
                    bool currentlyEnabled =
                        (currentIt != currentFilters.end() && currentIt->second.enabled);

                    // Find wet parameter value (wet is usually param ID 0)
                    auto wetIt = effectState.parameters.find("0");
                    float targetWet =
                        (wetIt != effectState.parameters.end()) ? wetIt->second : 0.0f;

                    if (targetWet > 0.0f) {
                        // Target has effect enabled - apply all parameters
                        // The transition system will handle lerping the wet level
                        for (const auto& [paramIdStr, paramValue] : effectState.parameters) {
                            try {
                                int paramId = std::stoi(paramIdStr);
                                m_controller->setTrackFilterParameter(trackIndex, effectName,
                                                                      paramId, paramValue);
                            } catch (const std::exception& e) {
                                // Skip invalid parameter IDs
                                continue;
                            }
                        }
                    } else if (currentlyEnabled) {
                        // Target has effect disabled but currently enabled - lerp down to 0
                        m_controller->setTrackFilterParameter(trackIndex, effectName, 0, 0.0f);
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

        // Get current audio system state for comparison
        const auto& currentFilters = m_controller->getAudioSystem().getBusFilters();

        // Apply effects with wet level logic
        for (const auto& [effectName, effectState] : state.effects) {
            auto currentIt = currentFilters.find(effectName);
            bool currentlyEnabled =
                (currentIt != currentFilters.end() && currentIt->second.enabled);

            // Find wet parameter value (wet is usually param ID 0)
            auto wetIt = effectState.parameters.find("0");
            float targetWet = (wetIt != effectState.parameters.end()) ? wetIt->second : 0.0f;

            if (targetWet > 0.0f) {
                // Target has effect enabled - apply all parameters
                // The transition system will handle lerping the wet level
                for (const auto& [paramIdStr, paramValue] : effectState.parameters) {
                    try {
                        int paramId = std::stoi(paramIdStr);
                        m_controller->setBusFilterParameter(effectName, paramId, paramValue);
                    } catch (const std::exception& e) {
                        // Skip invalid parameter IDs
                        continue;
                    }
                }
            } else if (currentlyEnabled) {
                // Target has effect disabled but currently enabled - lerp down to 0
                m_controller->setBusFilterParameter(effectName, 0, 0.0f);
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

        // Capture tracks with complete effect states
        const auto& audioState = m_controller->getState();
        const auto& audioSystem = m_controller->getAudioSystem();

        for (size_t i = 0; i < audioState.getTrackCount(); ++i) {
            const auto& track = audioState.getTrack(i);
            const auto& trackFilters = audioSystem.getFilters(i);

            TrackStateExtended extendedTrack;
            extendedTrack.file = std::filesystem::path(track.filepath).filename().string();
            extendedTrack.volume = track.volume;

            // Capture only effects with wet > 0 (enabled effects)
            for (const auto& [filterName, filterInstance] : trackFilters) {
                if (filterInstance.enabled) {
                    EffectState effectState;
                    // Store all parameters including wet level
                    for (const auto& [paramId, param] : filterInstance.parameters) {
                        effectState.parameters[std::to_string(paramId)] = param.value;
                    }
                    extendedTrack.effects[filterName] = effectState;
                }
            }

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

        // Capture master effects with complete effect states
        const auto& busFilters = m_controller->getAudioSystem().getBusFilters();

        // Capture only effects with wet > 0 (enabled effects)
        for (const auto& [filterName, filterInstance] : busFilters) {
            if (filterInstance.enabled) {
                EffectState effectState;
                // Store all parameters including wet level
                for (const auto& [paramId, param] : filterInstance.parameters) {
                    effectState.parameters[std::to_string(paramId)] = param.value;
                }
                state.effects[filterName] = effectState;
            }
        }

        return state;
    }

    float EventSystem::lerp(float a, float b, float t) { return a + (b - a) * t; }

    float EventSystem::easeInOut(float t) {
        // EASE_IN_OUT curve: slow start, fast middle, slow end
        return t < 0.5f ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
    }

    void EventSystem::lerpEffectState(const EffectState& start, const EffectState& end,
                                      EffectState& result, float t) {
        // Always lerp wet parameter first (wet is usually param ID 0)
        float startWet = 0.0f, endWet = 0.0f;
        bool hasStartWet = false, hasEndWet = false;
        if (auto it = start.parameters.find("0"); it != start.parameters.end()) {
            startWet = it->second;
            hasStartWet = true;
        }
        if (auto it = end.parameters.find("0"); it != end.parameters.end()) {
            endWet = it->second;
            hasEndWet = true;
        }
        if (hasStartWet && hasEndWet) {
            result.parameters["0"] = lerp(startWet, endWet, t);
            // Only lerp other parameters if either wet level > 0
            if (startWet > 0.0f || endWet > 0.0f) {
                std::set<std::string> allParams;
                for (const auto& [p, _] : start.parameters) allParams.insert(p);
                for (const auto& [p, _] : end.parameters) allParams.insert(p);
                for (const auto& paramName : allParams) {
                    if (paramName == "0")
                        continue;
                    float s =
                        start.parameters.count(paramName) ? start.parameters.at(paramName) : 0.0f;
                    float e = end.parameters.count(paramName) ? end.parameters.at(paramName) : 0.0f;
                    result.parameters[paramName] = lerp(s, e, t);
                }
            }
        } else if (hasEndWet) {
            // No start wet, lerp from 0
            result.parameters["0"] = lerp(0.0f, endWet, t);
            for (const auto& [paramName, endValue] : end.parameters) {
                if (paramName == "0")
                    continue;
                result.parameters[paramName] = endValue;
            }
        } else if (hasStartWet) {
            // No end wet, lerp to 0
            result.parameters["0"] = lerp(startWet, 0.0f, t);
            for (const auto& [paramName, startValue] : start.parameters) {
                if (paramName == "0")
                    continue;
                result.parameters[paramName] = startValue;
            }
        } else {
            // No wet in either, do nothing
        }
    }

} // namespace AudioTester