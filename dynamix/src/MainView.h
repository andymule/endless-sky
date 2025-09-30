#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include "imgui.h"

#include "AudioController.h"
#include "AudioState.h"
#include "FileBrowser.h"
#include "ThemeManager.h"
#include "Views/BusView.h"
#include "Views/SongView.h"
#include "Views/SpeedView.h"
#include "ConsoleLog.h"

// Pure View class that only handles UI rendering
class MainView {
public:
    MainView();
    ~MainView();

    bool Initialize(SDL_Window* window, SDL_GLContext glContext);
    void Render();
    void ProcessEvents(const SDL_Event& event);
    bool IsRunning() const { return m_isRunning; }
    void SetRunning(bool running) { m_isRunning = running; }

    // Set the controller (dependency injection)
    void SetController(Dynamix::AudioController* controller) {
        m_controller = controller;

        // Initialize views with the controller
        if (m_controller) {
            m_songView = std::make_unique<Dynamix::Views::SongView>(m_controller);
            m_busView = std::make_unique<Dynamix::Views::BusView>(m_controller);
            m_speedView = std::make_unique<Dynamix::Views::SpeedView>(m_controller);
            
            // Set up callback for OGG file dialog
            m_songView->SetOggFileDialogCallback([this]() {
                m_showOggFileDialog = true;
            });

            // Connect console log for error reporting
            m_controller->setConsoleLog(&m_consoleLog);
        }

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

    // File menu methods
    void RenderNewMasterDialog();
    void RenderSetRootFolderDialog();
    void InitializeDefaultDirectory();
    void RefreshBrowserEntries();

    // .ogg file browser methods (handled by FileBrowser component)
    bool CopyOggFileToSong(const std::string& sourcePath, const std::string& songName);

    // Project and song selection methods
    void DiscoverAvailableProjects();
    void RenderProjectSongDropdown();

    // Menu bar methods
    void RenderMenuBar();
    std::string GetWindowTitle();

    // Event tracking methods
    void SetLastTriggeredMasterEvent(const std::string& eventName);
    void SetLastTriggeredSongEvent(const std::string& songName, const std::string& eventName);
    void ClearLastTriggeredEvents();

    // Theme management methods
    void SetTheme(Dynamix::ThemeManager::Theme theme);
    Dynamix::ThemeManager::Theme GetCurrentTheme() const { return m_currentTheme; }

    // Theme persistence methods
    void LoadThemeFromConfig();
    void SaveThemeToConfig();

    // Console log access
    Dynamix::ConsoleLog& GetConsoleLog() { return m_consoleLog; }

private:
    void cleanup();
    void RenderMainWindow();
    void RenderEventsWindow();
    void RenderDirectoryInput();
    void RenderGlobalControls();

    // Events UI helpers
    void RenderSongEvents();
    void RenderMasterEvents();
    void ShowCreateEventDialog();
    Dynamix::StateSnapshot CaptureCurrentState();

    // Modular hold-to-action button system
    enum class HoldActionType { REMOVE, SAVE };
    struct HoldActionState {
        std::string actionId = ""; // Unique identifier for the action
        float holdTime = 0.0f;
        bool isHolding = false;
        bool hasTriggered = false; // Prevent multiple actions per button press
        HoldActionType actionType = HoldActionType::REMOVE;
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

    // File browser callback handlers
    void onDirectorySelected(const std::filesystem::path& path);
    void onOggFileSelected(const std::filesystem::path& path);

    // Constants
    static constexpr size_t DIR_INPUT_SIZE = 256;

    // Pure UI State
    bool m_isRunning = true;
    std::string m_musicDir;
    char m_dirInput[DIR_INPUT_SIZE] = "";
    ActiveWindow m_activeWindow = ActiveWindow::MAIN;
    bool m_mainWindowWasFocused = false;
    bool m_controlsWindowWasFocused = false;

    // Events UI state
    bool m_showEventsWindow = true;
    bool m_showCreateEventDialog = false;
    bool m_showErrorPopup = false;
    char m_errorMessage[512] = "";
    char m_newEventName[256] = "";
    float m_newEventFadeTime = 1.0f;

    // File menu state
    bool m_makeNewProjectInCurrentRoot = false;
    char m_newMasterName[256] = "";

    // File dialog state
    bool m_showSetRootDialog = false;
    bool m_showOggFileDialog = false;
    std::string m_defaultDirectory = "";

    // File browser components
    Dynamix::FileBrowser m_directoryBrowser;
    Dynamix::FileBrowser m_oggFileBrowser;

    // Project and song selection state
    std::vector<std::string> m_availableProjects;
    std::string m_currentProject = "";
    bool m_showProjectSongDropdown = false;

    // Event creation state
    enum class EventCreationType { MASTER, SONG };
    EventCreationType m_eventCreationType = EventCreationType::MASTER;
    std::string m_targetSongName = ""; // For song events

    // Last triggered event tracking (persists between song switches)
    std::string m_lastTriggeredMasterEvent = "";
    std::string m_lastTriggeredSongEvent = "";
    std::string m_lastTriggeredSongName = ""; // Which song the last song event was from

    // Modular hold-to-action state
    HoldActionState m_holdActionState;

    // Controller reference (managed externally)
    Dynamix::AudioController* m_controller = nullptr;

    // SDL/OpenGL
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;

    // Theme management state
    Dynamix::ThemeManager::Theme m_currentTheme = Dynamix::ThemeManager::Theme::RED;
    std::string m_configFilePath;

    std::unique_ptr<Dynamix::Views::SongView> m_songView;
    std::unique_ptr<Dynamix::Views::BusView> m_busView;
    std::unique_ptr<Dynamix::Views::SpeedView> m_speedView;
    
    // Console log system
    Dynamix::ConsoleLog m_consoleLog;
};