#include "EventSystem.h"
#include "AudioController.h"
#include "Logger.h"
#include "SongManager.h"
#include <algorithm>
#include <iostream>
#include <map>
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

        const Song* song = songManager->findSongByFolder(songName);
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

        // Always capture the current LIVE audio system state
        // This ensures we always lerp from the actual current position, not from interpolated
        // states
        m_startState = captureCurrentSongState();

        // Debug: Show what was captured in the start state
        LOG_INFO_COMP("EventSystem", "Captured start state:");
        for (const auto& track : m_startState.tracks) {
            std::string trackInfo =
                "  Track: " + track.file + " (vol: " + std::to_string(track.volume) + ")";
            if (!track.effects.empty()) {
                trackInfo += " Effects: ";
                for (const auto& [effectName, effect] : track.effects) {
                    auto wetIt = effect.parameters.find("0");
                    float wet = (wetIt != effect.parameters.end()) ? wetIt->second : 0.0f;
                    trackInfo += effectName + "(" + std::to_string(wet) + ") ";
                }
            }
            LOG_INFO_COMP("EventSystem", trackInfo);
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

        // Always capture the current LIVE audio system state
        // This ensures we always lerp from the actual current position, not from interpolated
        // states
        m_startMasterState = captureCurrentMasterState();

        m_targetMasterState = target;

        // Setup transition
        m_inTransition = true;
        m_transitionType = TransitionType::MASTER;
        m_transitionTime = fadeTime;
        m_targetTime = fadeTime;
        m_currentTime = 0.0f;
    }

    /**
     * Performs smooth state interpolation between start and target states.
     *
     * This is the core method for dynamic music transitions, handling:
     * - Tempo changes with musical interval easing
     * - Volume interpolation with logarithmic easing for perceived linearity
     * - Effect parameter transitions with wet level logic
     * - Track fade-in/fade-out scenarios
     * - Master bus state transitions
     *
     * The method processes both song events (affecting individual tracks) and
     * master events (affecting global bus state).
     *
     * @param t Interpolation factor (0.0 = start state, 1.0 = target state)
     */
    void EventSystem::lerpStates(float t) {
        if (m_transitionType == TransitionType::SONG) {
            // SONG TRANSITION: Interpolate between two complete song states
            StateSnapshot lerpedState;

            // Apply musical interval easing to tempo changes to preserve harmonic relationships
            // This prevents jarring tempo shifts that could break musical flow
            float easedT = tempoEase(t);
            lerpedState.masterTempo =
                lerp(m_startState.masterTempo, m_targetState.masterTempo, easedT);
            lerpedState.granularTempo =
                lerp(m_startState.granularTempo, m_targetState.granularTempo, easedT);

            // Create efficient lookup maps for track matching by filename
            // This avoids O(n²) nested loops when processing tracks
            std::map<std::string, const TrackStateExtended*> startTracks;
            std::map<std::string, const TrackStateExtended*> targetTracks;

            for (const auto& track : m_startState.tracks) {
                startTracks[track.file] = &track;
            }
            for (const auto& track : m_targetState.tracks) {
                targetTracks[track.file] = &track;
            }

            // Build set of all unique track filenames from both states
            // This ensures we process tracks that exist in either start or target
            std::set<std::string> allTrackFiles;
            for (const auto& track : m_startState.tracks) {
                allTrackFiles.insert(track.file);
            }
            for (const auto& track : m_targetState.tracks) {
                allTrackFiles.insert(track.file);
            }

            // Process each unique track - this handles all possible scenarios:
            // 1. Track exists in both states (normal transition)
            // 2. Track only in start state (fade out)
            // 3. Track only in target state (fade in)
            for (const auto& trackFile : allTrackFiles) {
                const TrackStateExtended* startTrack =
                    startTracks.count(trackFile) ? startTracks[trackFile] : nullptr;
                const TrackStateExtended* targetTrack =
                    targetTracks.count(trackFile) ? targetTracks[trackFile] : nullptr;

                // Create the interpolated track state
                TrackStateExtended lerpedTrack;
                lerpedTrack.file = trackFile;

                // VOLUME INTERPOLATION
                // Handle cases where track might not exist in one state
                float startVol = startTrack ? startTrack->volume : 1.0f;
                float targetVol = targetTrack ? targetTrack->volume : 1.0f;

                // Use logarithmic easing for volume changes to achieve perceived linearity
                // Human hearing is logarithmic, so linear interpolation sounds non-linear
                float easedT = volumeEase(t);
                lerpedTrack.volume = lerp(startVol, targetVol, easedT);

                // DEBUG LOGGING: Only log at transition boundaries to avoid spam
                // This helps with debugging while keeping performance high
                if (t < 0.001f || t > 0.999f) {
                    if (startVol > 0.0f || targetVol > 0.0f) {
                        LOG_INFO_COMP("EventSystem",
                                      "Lerping volume - Start vol: " + std::to_string(startVol) +
                                          ", End vol: " + std::to_string(targetVol) +
                                          ", t: " + std::to_string(t));
                    }

                    // Show which track is being processed
                    LOG_INFO_COMP("EventSystem", "Processing track: " + trackFile);

                    // Show track matching info for debugging
                    if (targetTrack && startTrack) {
                        LOG_INFO_COMP("EventSystem", "  Track matched: " + targetTrack->file +
                                                         " (target) with " + startTrack->file +
                                                         " (start)");
                    } else if (targetTrack) {
                        LOG_INFO_COMP("EventSystem", "  Track target only: " + targetTrack->file);
                    } else if (startTrack) {
                        LOG_INFO_COMP("EventSystem", "  Track start only: " + startTrack->file);
                    }
                }

                // EFFECTS PROCESSING - Handle all possible effect transition scenarios
                if (targetTrack) {
                    // Track exists in target event - process its effects
                    std::set<std::string> allEffects;

                    // Include all effects from current state (to handle fade-outs)
                    // This ensures effects that should fade out are processed
                    if (startTrack) {
                        for (const auto& [ename, _] : startTrack->effects) {
                            allEffects.insert(ename);
                        }
                    }
                    // Include all effects from target state (to handle fade-ins)
                    // This ensures new effects that should fade in are processed
                    for (const auto& [ename, _] : targetTrack->effects) {
                        allEffects.insert(ename);
                    }

                    // Debug logging for effect processing
                    if (t < 0.001f || t > 0.999f) {
                        if (!allEffects.empty()) {
                            std::string effectList = "Track effects: ";
                            for (const auto& effect : allEffects) {
                                effectList += effect + " ";
                            }
                            LOG_INFO_COMP("EventSystem", effectList);
                        }
                    }

                    // Process each effect with sophisticated transition logic
                    for (const auto& effectName : allEffects) {
                        const EffectState* startEff =
                            (startTrack && startTrack->effects.count(effectName))
                                ? &startTrack->effects.at(effectName)
                                : nullptr;
                        const EffectState* targetEff = targetTrack->effects.count(effectName)
                                                           ? &targetTrack->effects.at(effectName)
                                                           : nullptr;

                        if (startEff && targetEff) {
                            // SCENARIO 1: Both states have this effect - smooth transition between
                            // them
                            lerpEffectState(*startEff, *targetEff, lerpedTrack.effects[effectName],
                                            t);
                        } else if (targetEff) {
                            // SCENARIO 2: Only target has this effect - fade in from zero
                            EffectState zeroStart;
                            // Create a proper zero state with all parameters at 0
                            // This ensures clean fade-in without artifacts
                            for (const auto& [paramName, targetValue] : targetEff->parameters) {
                                zeroStart.parameters[paramName] = 0.0f;
                            }
                            lerpEffectState(zeroStart, *targetEff, lerpedTrack.effects[effectName],
                                            t);
                        } else if (startEff) {
                            // SCENARIO 3: Only start has this effect - fade out to zero
                            EffectState zeroTarget = *startEff;
                            zeroTarget.parameters["0"] = 0.0f; // Set wet to 0 to disable effect

                            // Debug logging for fade-out effects
                            if (t < 0.001f || t > 0.999f) {
                                LOG_INFO_COMP("EventSystem",
                                              "Fading out effect: " + effectName + " from wet: " +
                                                  std::to_string(startEff->parameters.count("0")
                                                                     ? startEff->parameters.at("0")
                                                                     : 0.0f) +
                                                  " to 0.0");
                            }

                            lerpEffectState(*startEff, zeroTarget, lerpedTrack.effects[effectName],
                                            t);
                        }
                    }
                } else if (startTrack) {
                    // SCENARIO 4: Track is NOT in target event but exists in current state
                    // This means the track should completely fade out (all effects disabled)
                    for (const auto& [effectName, startEffect] : startTrack->effects) {
                        EffectState zeroTarget = startEffect;
                        zeroTarget.parameters["0"] = 0.0f; // Set wet to 0 to disable effect

                        // Debug logging for complete track fade-out
                        if (t < 0.001f || t > 0.999f) {
                            LOG_INFO_COMP("EventSystem",
                                          "Fading out effect: " + effectName + " from wet: " +
                                              std::to_string(startEffect.parameters.count("0")
                                                                 ? startEffect.parameters.at("0")
                                                                 : 0.0f) +
                                              " to 0.0 (track not in target)");
                        }

                        lerpEffectState(startEffect, zeroTarget, lerpedTrack.effects[effectName],
                                        t);
                    }
                }

                // Add the fully processed track to the result state
                lerpedState.tracks.push_back(lerpedTrack);
            }

            // Apply the interpolated state to the audio system
            applyStateSnapshot(lerpedState);

        } else if (m_transitionType == TransitionType::MASTER) {
            // MASTER TRANSITION: Interpolate between two master bus states
            // This affects global settings that persist across song switches
            MasterBusState lerpedState;

            // Apply musical interval easing to tempo changes (same as song transitions)
            float easedT = tempoEase(t);
            lerpedState.masterTempo =
                lerp(m_startMasterState.masterTempo, m_targetMasterState.masterTempo, easedT);
            lerpedState.granularTempo =
                lerp(m_startMasterState.granularTempo, m_targetMasterState.granularTempo, easedT);
            lerpedState.volume = lerp(m_startMasterState.volume, m_targetMasterState.volume, t);

            // MASTER EFFECTS PROCESSING
            // Process ALL effects from current state, plus any new effects in target
            // This ensures complete effect state management at the master level
            std::set<std::string> allEffects;

            // Include all effects from current state (to handle fade-outs)
            for (const auto& [ename, _] : m_startMasterState.effects) allEffects.insert(ename);
            // Include all effects from target state (to handle fade-ins)
            for (const auto& [ename, _] : m_targetMasterState.effects) allEffects.insert(ename);

            // Process each master effect with the same logic as track effects
            for (const auto& effectName : allEffects) {
                const EffectState* startEff = m_startMasterState.effects.count(effectName)
                                                  ? &m_startMasterState.effects.at(effectName)
                                                  : nullptr;
                const EffectState* targetEff = m_targetMasterState.effects.count(effectName)
                                                   ? &m_targetMasterState.effects.at(effectName)
                                                   : nullptr;

                if (startEff && targetEff) {
                    // Both states have this effect - lerp between them
                    lerpEffectState(*startEff, *targetEff, lerpedState.effects[effectName], t);
                } else if (targetEff) {
                    // Only target has this effect - fade in from zero
                    EffectState zeroStart;
                    // Create a proper zero state with all parameters at 0
                    for (const auto& [paramName, targetValue] : targetEff->parameters) {
                        zeroStart.parameters[paramName] = 0.0f;
                    }
                    lerpEffectState(zeroStart, *targetEff, lerpedState.effects[effectName], t);
                } else if (startEff) {
                    // Only start has this effect - fade out to zero
                    EffectState zeroTarget = *startEff;
                    zeroTarget.parameters["0"] = 0.0f; // Set wet to 0
                    lerpEffectState(*startEff, zeroTarget, lerpedState.effects[effectName], t);
                }
            }

            // Apply the interpolated master state to the audio system
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

        // Capture tracks with complete effect states from CURRENT audio system
        const auto& audioState = m_controller->getState();
        const auto& audioSystem = m_controller->getAudioSystem();

        for (size_t i = 0; i < audioState.getTrackCount(); ++i) {
            const auto& track = audioState.getTrack(i);
            const auto& trackFilters = audioSystem.getFilters(i);

            TrackStateExtended extendedTrack;
            extendedTrack.file = std::filesystem::path(track.filepath).filename().string();
            extendedTrack.volume = track.volume;

            // Capture only effects with wet > 0 from current audio system
            for (const auto& [filterName, filterInstance] : trackFilters) {
                // Check if this effect has wet > 0 (is actually active)
                auto wetParam = filterInstance.parameters.find(0); // Wet is usually param ID 0
                if (wetParam != filterInstance.parameters.end() && wetParam->second.value > 0.0f) {
                    EffectState effectState;
                    // Store all parameters including the ACTUAL current wet level
                    for (const auto& [paramId, param] : filterInstance.parameters) {
                        effectState.parameters[std::to_string(paramId)] = param.value;
                    }
                    // Only include effects that are actually active (wet > 0)
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

        // Capture ALL master effects from current audio system with their ACTUAL wet levels
        const auto& busFilters = m_controller->getAudioSystem().getBusFilters();

        // Capture only effects with wet > 0 from current audio system
        for (const auto& [filterName, filterInstance] : busFilters) {
            // Check if this effect has wet > 0 (is actually active)
            auto wetParam = filterInstance.parameters.find(0); // Wet is usually param ID 0
            if (wetParam != filterInstance.parameters.end() && wetParam->second.value > 0.0f) {
                EffectState effectState;
                // Store all parameters including the ACTUAL current wet level
                for (const auto& [paramId, param] : filterInstance.parameters) {
                    effectState.parameters[std::to_string(paramId)] = param.value;
                }
                // Only include effects that are actually active (wet > 0)
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

    float EventSystem::volumeEase(float t) {
        // Logarithmic easing for perceived linear volume changes
        // Human hearing perceives volume logarithmically, so we use square root
        // to make the transition feel more natural
        return std::sqrt(t);
    }

    float EventSystem::tempoEase(float t) {
        // Musical interval easing for tempo changes
        // Uses exponential curve to maintain musical relationships
        // This helps preserve harmonic relationships during tempo transitions
        return std::pow(2.0f, t - 1.0f);
    }

    float EventSystem::wetEase(float t) {
        // Sigmoid easing for smooth effect crossfades
        // Provides smooth transitions for wet/dry mixing
        // Avoids artifacts that can occur with linear wet level changes
        return 1.0f / (1.0f + std::exp(-10.0f * (t - 0.5f)));
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

        // Debug logging only at start (t=0) and end (t=1) of transition
        if (t < 0.001f || t > 0.999f) {
            if (startWet > 0.0f || endWet > 0.0f) {
                LOG_INFO_COMP("EventSystem",
                              "Lerping effect - Start wet: " + std::to_string(startWet) +
                                  ", End wet: " + std::to_string(endWet) +
                                  ", t: " + std::to_string(t));
            }
        }

        if (hasStartWet && hasEndWet) {
            // Use sigmoid easing for smooth wet level transitions
            float easedT = wetEase(t);
            result.parameters["0"] = lerp(startWet, endWet, easedT);
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
                    result.parameters[paramName] = lerp(s, e, easedT);
                }
            }
        } else if (hasEndWet) {
            // No start wet, lerp from 0
            float easedT = wetEase(t);
            result.parameters["0"] = lerp(0.0f, endWet, easedT);
            for (const auto& [paramName, endValue] : end.parameters) {
                if (paramName == "0")
                    continue;
                result.parameters[paramName] = endValue;
            }
        } else if (hasStartWet) {
            // No end wet, lerp to 0
            float easedT = wetEase(t);
            result.parameters["0"] = lerp(startWet, 0.0f, easedT);
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