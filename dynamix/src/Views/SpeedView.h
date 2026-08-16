#pragma once

#include <memory>
#include <string>
#include <vector>

namespace Dynamix {
    class AudioController;
}

namespace Dynamix {
    namespace Views {

        // Third step of MainView refactor: SpeedView handles tempo and speed controls
        class SpeedView {
        public:
            SpeedView(Dynamix::AudioController* controller);
            void Render();

        private:
            void RenderSpeedControls();

            Dynamix::AudioController* m_controller;

            // Local UI state so sliders don't fight event lerping while dragging
            float m_masterTempoUI = 1.0f;
            float m_granularTempoUI = 1.0f;
        };

    } // namespace Views
} // namespace Dynamix