#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Dynamix {
    class AudioController;
}

namespace Dynamix {
    namespace Views {

        // First step of MainView refactor: SongView handles song/track list and controls
        class SongView {
        public:
            SongView(Dynamix::AudioController* controller);
            void Render();

            // Dialog control methods
            void ShowNewSongDialog() { m_showNewSongDialog = true; }
            void ShowOggFileDialog() { m_showOggFileDialog = true; }

        private:
            void RenderTrackControls();
            void drawFilterControls(size_t trackIndex);
            bool RenderDeleteButton(const std::string& eventId, const char* eventName);
            bool RenderSaveButton(const std::string& eventId, const char* eventName);
            void RenderNewSongDialog();
            void RenderOggFileDialog();
            bool CopyOggFileToSong(const std::string& sourcePath, const std::string& songName);

            Dynamix::AudioController* m_controller;

            // Dialog state
            bool m_showNewSongDialog = false;
            bool m_showOggFileDialog = false;

            // Dialog input state
            char m_newSongName[256] = "";
        };

    } // namespace Views
} // namespace Dynamix