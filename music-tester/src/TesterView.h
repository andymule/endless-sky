#pragma once

#include "AudioController.h"
#include "imgui.h"
#include <SDL2/SDL.h>
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
    void SetController(AudioTester::AudioController* controller) { m_controller = controller; }

    // Window management for multi-window support
    enum class ActiveWindow { MAIN = 0, SECONDARY = 1 };
    void SetActiveWindow(ActiveWindow window) { m_activeWindow = window; }
    ActiveWindow GetActiveWindow() const { return m_activeWindow; }

    // Window focus tracking
    void UpdateActiveWindow();

private:
    void cleanup();
    void RenderMainWindow();
    void RenderDirectoryInput();
    void RenderGlobalControls();
    void RenderTrackControls();
    void RenderBusControls();
    void drawFilterControls(size_t trackIndex);

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
    bool m_secondaryWindowWasFocused = false; // Future: for secondary window support

    // Controller reference (managed externally)
    AudioTester::AudioController* m_controller = nullptr;

    // SDL/OpenGL
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;
};