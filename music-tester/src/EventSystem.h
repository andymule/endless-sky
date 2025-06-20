#pragma once

#include "AudioState.h"
#include <memory>
#include <string>

namespace AudioTester {

    // Forward declaration
    class AudioController;

    class EventSystem {
    public:
        EventSystem(AudioController* controller);
        ~EventSystem() = default;

        // External API (called by game engines)
        void triggerSongEvent(const std::string& songName, const std::string& eventName);
        void triggerMasterEvent(const std::string& eventName);

        // State transitions with lerping
        void update(float deltaTime);

        // Check if currently in transition
        bool isInTransition() const { return m_inTransition; }

        // Get current transition progress (0.0 to 1.0)
        float getTransitionProgress() const;

    private:
        AudioController* m_controller;

        // Transition state
        bool m_inTransition = false;
        float m_transitionTime = 0.0f;
        float m_targetTime = 0.0f;
        float m_currentTime = 0.0f;

        // State snapshots for lerping
        StateSnapshot m_startState;
        StateSnapshot m_targetState;
        MasterBusState m_startMasterState;
        MasterBusState m_targetMasterState;

        // Transition type
        enum class TransitionType { NONE, SONG, MASTER };
        TransitionType m_transitionType = TransitionType::NONE;

        // Helper methods
        void startSongTransition(const StateSnapshot& target, float fadeTime);
        void startMasterTransition(const MasterBusState& target, float fadeTime);
        void lerpStates(float t);
        void applyStateSnapshot(const StateSnapshot& state);
        void applyMasterBusState(const MasterBusState& state);

        // State capture
        StateSnapshot captureCurrentSongState();
        MasterBusState captureCurrentMasterState();

        // Utility
        float lerp(float a, float b, float t);
        void lerpEffectState(const EffectState& start, const EffectState& end, EffectState& result,
                             float t);
    };

} // namespace AudioTester