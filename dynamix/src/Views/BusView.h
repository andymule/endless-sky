#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Dynamix {
    class AudioController;
}

namespace Dynamix {
    namespace Views {

        // Second step of MainView refactor: BusView handles bus controls and FX
        class BusView {
        public:
            BusView(Dynamix::AudioController* controller);
            void Render();

        private:
            void RenderBusControls();
            void drawBusFilterControls(const std::string& filterName);

            Dynamix::AudioController* m_controller;
        };

    } // namespace Views
} // namespace Dynamix