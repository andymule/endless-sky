#include "TesterView.h"
#include "Logger.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <SDL2/SDL_opengl.h>
#include <cstring>
#include <filesystem>
#include <iostream>

TesterView::TesterView() {
    // Initialize UI with default directory - will be updated when controller is set
    strncpy(m_dirInput, "sound_staging", DIR_INPUT_SIZE);
    m_dirInput[DIR_INPUT_SIZE - 1] = '\0';

    // Initialize tempo UI state
    m_masterTempoUI = 1.0f;
}

TesterView::~TesterView() { cleanup(); }

bool TesterView::Initialize(SDL_Window* window, SDL_GLContext glContext) {
    m_window = window;
    m_glContext = glContext;

    // Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    if (!ImGui_ImplSDL2_InitForOpenGL(window, glContext)) {
        LOG_ERROR_COMP("TesterView", "Failed to initialize ImGui SDL2 backend");
        return false;
    }

    const char* glsl_version = "#version 150";
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        LOG_ERROR_COMP("TesterView", "Failed to initialize ImGui OpenGL3 backend");
        return false;
    }

    return true;
}

void TesterView::ProcessEvents(const SDL_Event& event) {
    ImGui_ImplSDL2_ProcessEvent(&event);

    // Handle keyboard shortcuts
    ImGuiIO& io = ImGui::GetIO();
    if (event.type == SDL_KEYDOWN) {
        // Allow our shortcuts to work unless we're actively typing in a text input
        bool allowShortcuts = !io.WantTextInput;

        if (allowShortcuts) {
            // Handle spacebar for play/pause
            if (event.key.keysym.sym == SDLK_SPACE) {
                if (m_controller) {
                    m_controller->toggleGlobalPlayback();
                }
            }
            // Handle number keys 1-9, 0 for track toggling
            else if (event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_9) {
                int keyNumber = event.key.keysym.sym - SDLK_1 + 1; // Convert to 1-9
                handleNumberKeyPress(keyNumber);
            } else if (event.key.keysym.sym == SDLK_0) {
                handleNumberKeyPress(0);
            }
        }
    }

    if (event.type == SDL_QUIT)
        m_isRunning = false;
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
        event.window.windowID == SDL_GetWindowID(m_window))
        m_isRunning = false;
}

void TesterView::Render() {
    // Guard against missing controller
    if (!m_controller) {
        return;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    RenderMainWindow();
    RenderControlsWindow();

    // Update which window is currently active/focused
    UpdateActiveWindow();

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(m_window);
}

void TesterView::RenderMainWindow() {
    ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
    ImGui::Begin("Music Tester");

    // Track if this window is focused for keyboard input routing
    m_mainWindowWasFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    RenderDirectoryInput();
    ImGui::Separator();
    RenderGlobalControls();
    ImGui::Separator();
    RenderTrackControls();
    ImGui::Separator();
    RenderBusControls();

    ImGui::End();
}

void TesterView::RenderDirectoryInput() {
    ImGui::Text("Music Directory:");
    if (ImGui::InputText("##dir", m_dirInput, DIR_INPUT_SIZE,
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        m_controller->setMusicDirectory(m_dirInput);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        m_controller->setMusicDirectory(m_dirInput);
    }
}

void TesterView::RenderGlobalControls() {
    const auto& state = m_controller->getState();

    if (ImGui::Button(state.globalPlaying ? "Stop" : "Play")) {
        m_controller->toggleGlobalPlayback();
    }

    // Add keyboard shortcuts help
    ImGui::Separator();
    ImGui::Text("Keyboard Shortcuts:");
    ImGui::Text("  Space: Toggle Play/Pause");
    ImGui::Text("  1-9:   Toggle tracks 1-9 (active window)");
    ImGui::Text("  0:     Toggle track 10 (active window)");

    // Show which window is currently active for number keys
    const char* activeWindowName = (m_activeWindow == ActiveWindow::MAIN) ? "Main" : "Secondary";
    ImGui::Text("Active window: %s", activeWindowName);
}

void TesterView::RenderTrackControls() {
    const auto& state = m_controller->getState();

    ImGui::Text("Tracks (%d):", static_cast<int>(state.getTrackCount()));
    for (size_t i = 0; i < state.getTrackCount(); i++) {
        const auto& track = state.getTrack(i);
        ImGui::PushID(static_cast<int>(i));

        // Active checkbox
        bool active = track.active;
        if (ImGui::Checkbox("##active", &active)) {
            m_controller->setTrackActive(i, active);
        }
        ImGui::SameLine();

        // Show keyboard shortcut for track
        std::string keyText = (i < 9) ? std::to_string(i + 1) : "0";
        ImGui::Text("[%s] %s", keyText.c_str(), track.name.c_str());

        // Volume slider
        float volume = track.volume;
        if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f)) {
            m_controller->setTrackVolume(i, volume);
        }

        // Collapsible filter controls (start collapsed)
        if (ImGui::TreeNodeEx("Effects", ImGuiTreeNodeFlags_None)) {
            drawFilterControls(i);
            ImGui::TreePop();
        }

        ImGui::PopID();
        ImGui::Separator();
    }
}

void TesterView::drawFilterControls(size_t trackIndex) {
    const auto& audioSystem = m_controller->getAudioSystem();
    const auto& filters = audioSystem.getFilters(trackIndex);

    for (const auto& filterName : AudioTester::AudioSystem::AVAILABLE_FILTERS) {
        bool enabled = audioSystem.isFilterEnabled(trackIndex, filterName);
        if (ImGui::Checkbox(filterName.c_str(), &enabled)) {
            m_controller->setTrackFilterEnabled(trackIndex, filterName, enabled);
        }
        if (enabled) {
            auto it = filters.find(filterName);
            if (it != filters.end()) {
                // Push unique ID for this filter to ensure parameter sliders are unique
                ImGui::PushID((filterName + std::to_string(trackIndex)).c_str());
                for (const auto& [paramId, param] : it->second.parameters) {
                    float value = param.value;
                    bool changed = false;

                    // Use appropriate control based on parameter type
                    switch (param.type) {
                        case AudioTester::ParameterType::BOOL: {
                            bool boolValue = value > 0.5f;
                            if (ImGui::Checkbox(param.name.c_str(), &boolValue)) {
                                value = boolValue ? 1.0f : 0.0f;
                                changed = true;
                            }
                            break;
                        }
                        case AudioTester::ParameterType::INT: {
                            int intValue = static_cast<int>(value);
                            if (ImGui::SliderInt(param.name.c_str(), &intValue,
                                                 static_cast<int>(param.min),
                                                 static_cast<int>(param.max))) {
                                value = static_cast<float>(intValue);
                                changed = true;
                            }
                            break;
                        }
                        case AudioTester::ParameterType::FLOAT:
                        default: {
                            if (ImGui::SliderFloat(param.name.c_str(), &value, param.min,
                                                   param.max)) {
                                changed = true;
                            }
                            break;
                        }
                    }

                    if (changed) {
                        m_controller->setTrackFilterParameter(trackIndex, filterName, paramId,
                                                              value);
                    }
                }
                ImGui::PopID();
            }
        }
    }
}

void TesterView::RenderBusControls() {
    const auto& state = m_controller->getState();
    const auto& audioSystem = m_controller->getAudioSystem();

    ImGui::Begin("Bus Controls");

    // Bus volume
    float busVolume = state.busVolume;
    if (ImGui::SliderFloat("Bus Volume", &busVolume, 0.0f, 1.0f)) {
        m_controller->setBusVolume(busVolume);
    }

    ImGui::Separator();
    ImGui::Text("Bus FX");
    const auto& busFilters = audioSystem.getBusFilters();

    for (const auto& filterName : AudioTester::AudioSystem::AVAILABLE_FILTERS) {
        bool enabled = audioSystem.isBusFilterEnabled(filterName);
        if (ImGui::Checkbox(filterName.c_str(), &enabled)) {
            m_controller->setBusFilterEnabled(filterName, enabled);
        }
        if (enabled) {
            auto it = busFilters.find(filterName);
            if (it != busFilters.end()) {
                // Push unique ID for this bus filter to ensure parameter sliders are unique
                ImGui::PushID((filterName + "bus").c_str());
                for (const auto& [paramId, param] : it->second.parameters) {
                    float value = param.value;
                    bool changed = false;

                    // Use appropriate control based on parameter type
                    switch (param.type) {
                        case AudioTester::ParameterType::BOOL: {
                            bool boolValue = value > 0.5f;
                            if (ImGui::Checkbox(param.name.c_str(), &boolValue)) {
                                value = boolValue ? 1.0f : 0.0f;
                                changed = true;
                            }
                            break;
                        }
                        case AudioTester::ParameterType::INT: {
                            int intValue = static_cast<int>(value);
                            if (ImGui::SliderInt(param.name.c_str(), &intValue,
                                                 static_cast<int>(param.min),
                                                 static_cast<int>(param.max))) {
                                value = static_cast<float>(intValue);
                                changed = true;
                            }
                            break;
                        }
                        case AudioTester::ParameterType::FLOAT:
                        default: {
                            if (ImGui::SliderFloat(param.name.c_str(), &value, param.min,
                                                   param.max)) {
                                changed = true;
                            }
                            break;
                        }
                    }

                    if (changed) {
                        m_controller->setBusFilterParameter(filterName, paramId, value);
                    }
                }
                ImGui::PopID();
            }
        }
    }

    ImGui::End();
}

void TesterView::RenderControlsWindow() {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_None);

    // Track if this window is focused for keyboard input routing
    m_controlsWindowWasFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    // Keyboard Shortcuts Section
    ImGui::Text("Keyboard Shortcuts");
    ImGui::Separator();
    ImGui::Text("Space:  Toggle Play/Pause");
    ImGui::Text("1-9:    Toggle tracks 1-9 (active window)");
    ImGui::Text("0:      Toggle track 10 (active window)");

    // Show which window is currently active for number keys
    const char* activeWindowName = (m_activeWindow == ActiveWindow::MAIN) ? "Main" : "Controls";
    ImGui::Text("Active window: %s", activeWindowName);

    ImGui::Separator();

    // Tempo Control Section
    ImGui::Text("Tempo Control");
    ImGui::Separator();

    if (m_controller) {
        // Sync UI state with controller state only when not actively editing
        bool uiIsDragging = ImGui::IsAnyItemActive();
        if (!uiIsDragging) {
            m_masterTempoUI = m_controller->getMasterTempo();
        }

        // Playback Speed slider (tape-style, affects pitch)
        if (ImGui::SliderFloat("Tape Speed", &m_masterTempoUI, 0.1f, 4.0f, "%.2fx")) {
            m_controller->setMasterTempo(m_masterTempoUI);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Tape-style speed control (affects pitch)");
        }

        // Granular Tempo slider (pitch-preserving)
        float granularTempo = m_controller->getGranularTempo();
        if (ImGui::SliderFloat("Granular Tempo", &granularTempo, 0.5f, 2.0f, "%.2fx")) {
            m_controller->setGranularTempo(granularTempo);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            float latency = m_controller->getGranularLatencyMs();
            ImGui::SetTooltip("Pitch-preserving tempo stretching\nLatency: %.1f ms", latency);
        }

        ImGui::Separator();

        // Help text
        ImGui::TextWrapped("Tape Speed: Classic tape-style speed control (changes pitch).\n"
                           "Future: Granular pitch-preserving tempo and pitch shifting will be "
                           "implemented outside SoLoud's filter system.");
    }

    ImGui::End();
}

void TesterView::UpdateActiveWindow() {
    // Update active window based on which window was focused during rendering

    if (m_mainWindowWasFocused) {
        m_activeWindow = ActiveWindow::MAIN;
    } else if (m_controlsWindowWasFocused) {
        // For now, controls window acts like main window for keyboard shortcuts
        m_activeWindow = ActiveWindow::MAIN;
    }

    // Future: When secondary window is implemented
    // if (m_secondaryWindowWasFocused) {
    //     m_activeWindow = ActiveWindow::SECONDARY;
    // }

    // If no window is focused, keep the current active window
    // This ensures number keys continue to work on the last focused window
    // even when the user isn't actively clicking in windows
}

void TesterView::handleNumberKeyPress(int keyNumber) {
    if (!m_controller) {
        return;
    }

    // Always operate on the main window's tracks for now
    // When secondary window is implemented, this will route based on m_activeWindow
    const auto& state = m_controller->getState();
    size_t trackCount = state.getTrackCount();

    // Map number keys to track indices
    // 1-9 maps to tracks 0-8, 0 maps to track 9
    size_t trackIndex;
    if (keyNumber >= 1 && keyNumber <= 9) {
        trackIndex = keyNumber - 1; // 1->0, 2->1, ..., 9->8
    } else if (keyNumber == 0) {
        trackIndex = 9; // 0->9
    } else {
        return; // Invalid key
    }

    // Only toggle if the track exists
    if (trackIndex < trackCount) {
        // Route to appropriate window's tracks based on active window
        switch (m_activeWindow) {
            case ActiveWindow::MAIN:
                // Main window tracks (current implementation)
                {
                    const auto& track = state.getTrack(trackIndex);
                    m_controller->setTrackActive(trackIndex, !track.active);
                }
                break;

            case ActiveWindow::SECONDARY:
                // Future: Secondary window tracks
                // Would operate on a different track set or different controller instance
                break;
        }
    }
}

void TesterView::cleanup() {
    // ImGui cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}