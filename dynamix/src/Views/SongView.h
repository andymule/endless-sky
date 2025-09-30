#pragma once

#include <filesystem>
#include <functional>
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
            void ShowOggFileDialog(); // Forward to MainView's FileBrowser
            
            // Callback for OGG file dialog
            void SetOggFileDialogCallback(std::function<void()> callback) { m_oggFileDialogCallback = callback; }

        private:
            void RenderTrackControls();
            void drawFilterControls(size_t trackIndex);
            bool RenderDeleteButton(const std::string& eventId, const char* eventName);
            bool RenderSaveButton(const std::string& eventId, const char* eventName);
            void RenderNewSongDialog();

            Dynamix::AudioController* m_controller;

            // Dialog state
            bool m_showNewSongDialog = false;

            // Dialog input state
            char m_newSongName[256] = "";
            
            // Callback for OGG file dialog
            std::function<void()> m_oggFileDialogCallback;
        };

    } // namespace Views
} // namespace Dynamix