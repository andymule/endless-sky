#pragma once

#include "AudioController.h"
#include "AudioState.h"
#include "imgui.h"
#include <SDL2/SDL.h>
#include <filesystem>
#include <string>

// Pure View class that only handles UI rendering
class TesterView {
public:
    TesterView();
    ~TesterView();

    bool Initialize(SDL_Window* window, SDL_GLContext glContext);
    void Render();
    void ProcessEvents(const SDL_Event& event);
    bool IsRunning() const { return m_isRunning; }
    void SetRunning(bool running) { m_isRunning = running; }

    // Set the controller (dependency injection)
    void SetController(AudioTester::AudioController* controller) {
        m_controller = controller;

        // Update the directory input with the current directory from controller
        if (m_controller) {
            const std::string& currentDir = m_controller->getCurrentDirectory();
            if (!currentDir.empty()) {
                // Extract just the directory name for display
                std::filesystem::path path(currentDir);
                std::string dirName = path.filename().string();
                if (dirName.empty()) {
                    dirName = path.string(); // Use full path if no filename
                }
                strncpy(m_dirInput, dirName.c_str(), DIR_INPUT_SIZE);
                m_dirInput[DIR_INPUT_SIZE - 1] = '\0';
            }
        }
    }

    // Window management for multi-window support
    enum class ActiveWindow { MAIN = 0, SECONDARY = 1 };
    void SetActiveWindow(ActiveWindow window) { m_activeWindow = window; }
    ActiveWindow GetActiveWindow() const { return m_activeWindow; }

    // Window focus tracking
    void UpdateActiveWindow();

private:
    void cleanup();
    void RenderMainWindow();
    void RenderControlsWindow();
    void RenderEventsWindow();
    void RenderDirectoryInput();
    void RenderGlobalControls();
    void RenderTrackControls();
    void RenderBusControls();
    void drawFilterControls(size_t trackIndex);

    // Events UI helpers
    void RenderSongEvents();
    void RenderMasterEvents();
    void ShowCreateEventDialog();
    AudioTester::StateSnapshot CaptureCurrentState();

    // Modular hold-to-action button system
    enum class HoldActionType { DELETE, SAVE };
    struct HoldActionState {
        std::string actionId = ""; // Unique identifier for the action
        float holdTime = 0.0f;
        bool isHolding = false;
        bool hasTriggered = false; // Prevent multiple actions per button press
        HoldActionType actionType = HoldActionType::DELETE;
        static constexpr float HOLD_DURATION = 1.0f; // 1 second
    };

    // Generic hold-to-action button renderer
    bool RenderHoldActionButton(const std::string& actionId, const char* buttonText,
                                const char* tooltipText, HoldActionType actionType,
                                const ImVec4& textColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f),
                                const ImVec4& progressColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f),
                                const ImVec4& bgColor = ImVec4(0.9f, 0.2f, 0.2f, 0.3f));

    // Specific button renderers (use the generic one)
    bool RenderDeleteButton(const std::string& eventId, const char* eventName);
    bool RenderSaveButton(const std::string& eventId, const char* eventName);

    // Input handling helpers
    void handleNumberKeyPress(int keyNumber);

    // Constants
    static constexpr size_t DIR_INPUT_SIZE = 256;

    // Pure UI State
    bool m_isRunning = true;
    std::string m_musicDir;
    char m_dirInput[DIR_INPUT_SIZE] = "";
    ActiveWindow m_activeWindow = ActiveWindow::MAIN;
    bool m_mainWindowWasFocused = false;
    bool m_controlsWindowWasFocused = false;
    // bool m_secondaryWindowWasFocused = false; // Future: for secondary window support

    // Events UI state
    bool m_showEventsWindow = true;
    bool m_showCreateEventDialog = false;
    bool m_showErrorPopup = false;
    char m_errorMessage[512] = "";
    char m_newEventName[256] = "";
    float m_newEventFadeTime = 1.0f;

    // Event creation state
    enum class EventCreationType { MASTER, SONG };
    EventCreationType m_eventCreationType = EventCreationType::MASTER;
    std::string m_targetSongName = ""; // For song events

    // Modular hold-to-action state (replaces old delete state)
    HoldActionState m_holdActionState;

    // Controller reference (managed externally)
    AudioTester::AudioController* m_controller = nullptr;

    // SDL/OpenGL
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;

    // Fonts
    ImFont* m_mainFont = nullptr;
    ImFont* m_iconFont = nullptr;

    // Local UI state for tempo control to avoid ImGui slider issues
    float m_masterTempoUI = 1.0f;
};