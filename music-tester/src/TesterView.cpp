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

    // Render Events window if enabled
    if (m_showEventsWindow) {
        RenderEventsWindow();
    }

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
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Load tracks and songs from directory (automatically loads both tracks and events)");
    }
}

void TesterView::RenderGlobalControls() {
    const auto& state = m_controller->getState();

    if (ImGui::Button(state.globalPlaying ? "Stop" : "Play")) {
        m_controller->toggleGlobalPlayback();
    }

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

        // Show keyboard shortcut for track (volume control)
        std::string keyText = (i < 9) ? std::to_string(i + 1) : "0";
        ImGui::Text("[%s] %s", keyText.c_str(), track.name.c_str());

        // Volume slider (primary control for enable/disable)
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
        // Check if filter is currently enabled (based on wet parameter > 0)
        bool effectivelyEnabled = audioSystem.isFilterEnabled(trackIndex, filterName);

        // Create unique ID for this filter's expanded state
        std::string expandedId = "expand_" + filterName + "_" + std::to_string(trackIndex);

        // Get or initialize expanded state
        static std::map<std::string, bool> expandedStates;
        bool& isExpanded = expandedStates[expandedId];

        // Show dropdown arrow with effect name and status
        ImGui::PushID(filterName.c_str());

        // Color the arrow based on effect enabled state
        if (effectivelyEnabled) {
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  ImVec4(0.4f, 0.8f, 0.4f, 1.0f)); // Green when enabled
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  ImVec4(0.6f, 0.6f, 0.6f, 1.0f)); // Gray when disabled
        }

        // Dropdown arrow (TreeNode style but manual)
        if (ImGui::ArrowButton("##arrow", isExpanded ? ImGuiDir_Down : ImGuiDir_Right)) {
            isExpanded = !isExpanded;
        }
        ImGui::PopStyleColor();

        // Effect name on same line
        ImGui::SameLine();
        ImGui::Text("%s%s", filterName.c_str(), effectivelyEnabled ? " (ON)" : " (OFF)");

        // Show parameters when expanded
        if (isExpanded) {
            ImGui::Indent();

            auto it = filters.find(filterName);
            if (it != filters.end()) {
                // Filter exists, show all parameters
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
                            // Highlight wet parameter for easy identification
                            if (paramId == 0 && filterName != "dcremoval") {
                                ImGui::PushStyleColor(ImGuiCol_FrameBg,
                                                      value > 0.0f
                                                          ? ImVec4(0.2f, 0.6f, 0.2f, 0.4f)
                                                          : ImVec4(0.6f, 0.2f, 0.2f, 0.4f));
                            }

                            if (ImGui::SliderFloat(param.name.c_str(), &value, param.min,
                                                   param.max)) {
                                changed = true;
                            }

                            if (paramId == 0 && filterName != "dcremoval") {
                                ImGui::PopStyleColor();
                                // Add tooltip for wet parameter
                                if (ImGui::IsItemHovered()) {
                                    ImGui::SetTooltip(
                                        "Wet parameter: Controls effect enable/disable.\n"
                                        "0.0 = effect disabled, >0.0 = effect enabled");
                                }
                            }
                            break;
                        }
                    }

                    if (changed) {
                        m_controller->setTrackFilterParameter(trackIndex, filterName, paramId,
                                                              value);
                    }
                }
            } else if (filterName != "dcremoval") {
                // Filter doesn't exist yet, show wet parameter to enable it
                float wetValue = 0.0f;
                ImGui::PushStyleColor(ImGuiCol_FrameBg,
                                      ImVec4(0.6f, 0.2f, 0.2f, 0.4f)); // Red for disabled
                if (ImGui::SliderFloat("wet", &wetValue, 0.0f, 1.0f)) {
                    m_controller->setTrackFilterParameter(trackIndex, filterName, 0, wetValue);
                }
                ImGui::PopStyleColor();
            } else {
                // DCRemoval has no wet parameter, show enable option
                ImGui::TextDisabled("(Effect disabled - click to enable)");
                if (ImGui::Button("Enable DCRemoval")) {
                    // DCRemoval default parameter
                    m_controller->setTrackFilterParameter(trackIndex, filterName, 0, 0.1f);
                }
            }

            ImGui::Unindent();
        }

        ImGui::PopID();
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
        // Check if filter is currently enabled (based on wet parameter > 0)
        bool effectivelyEnabled = audioSystem.isBusFilterEnabled(filterName);

        // Create unique ID for this filter's expanded state
        std::string expandedId = "expand_bus_" + filterName;

        // Get or initialize expanded state
        static std::map<std::string, bool> expandedStates;
        bool& isExpanded = expandedStates[expandedId];

        // Show dropdown arrow with effect name and status
        ImGui::PushID(filterName.c_str());

        // Color the arrow based on effect enabled state
        if (effectivelyEnabled) {
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  ImVec4(0.4f, 0.8f, 0.4f, 1.0f)); // Green when enabled
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  ImVec4(0.6f, 0.6f, 0.6f, 1.0f)); // Gray when disabled
        }

        // Dropdown arrow (TreeNode style but manual)
        if (ImGui::ArrowButton("##arrow", isExpanded ? ImGuiDir_Down : ImGuiDir_Right)) {
            isExpanded = !isExpanded;
        }
        ImGui::PopStyleColor();

        // Effect name on same line
        ImGui::SameLine();
        ImGui::Text("%s%s", filterName.c_str(), effectivelyEnabled ? " (ON)" : " (OFF)");

        // Show parameters when expanded
        if (isExpanded) {
            ImGui::Indent();

            auto it = busFilters.find(filterName);
            if (it != busFilters.end()) {
                // Filter exists, show all parameters
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
                            // Highlight wet parameter for easy identification
                            if (paramId == 0 && filterName != "dcremoval") {
                                ImGui::PushStyleColor(ImGuiCol_FrameBg,
                                                      value > 0.0f
                                                          ? ImVec4(0.2f, 0.6f, 0.2f, 0.4f)
                                                          : ImVec4(0.6f, 0.2f, 0.2f, 0.4f));
                            }

                            if (ImGui::SliderFloat(param.name.c_str(), &value, param.min,
                                                   param.max)) {
                                changed = true;
                            }

                            if (paramId == 0 && filterName != "dcremoval") {
                                ImGui::PopStyleColor();
                                // Add tooltip for wet parameter
                                if (ImGui::IsItemHovered()) {
                                    ImGui::SetTooltip(
                                        "Wet parameter: Controls effect enable/disable.\n"
                                        "0.0 = effect disabled, >0.0 = effect enabled");
                                }
                            }
                            break;
                        }
                    }

                    if (changed) {
                        m_controller->setBusFilterParameter(filterName, paramId, value);
                    }
                }
            } else if (filterName != "dcremoval") {
                // Filter doesn't exist yet, show wet parameter to enable it

                float wetValue = 0.0f;
                ImGui::PushStyleColor(ImGuiCol_FrameBg,
                                      ImVec4(0.6f, 0.2f, 0.2f, 0.4f)); // Red for disabled
                if (ImGui::SliderFloat("wet", &wetValue, 0.0f, 1.0f)) {
                    m_controller->setBusFilterParameter(filterName, 0, wetValue);
                }
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Set wet > 0.0 to enable this effect");
                }
            } else {
                // DCRemoval has no wet parameter, show enable option
                ImGui::TextDisabled("(Effect disabled - click to enable)");
                if (ImGui::Button("Enable DCRemoval")) {
                    // DCRemoval default parameter
                    m_controller->setBusFilterParameter(filterName, 0, 0.1f);
                }
            }

            ImGui::Unindent();
        }

        ImGui::PopID();
    }

    ImGui::End();
}

void TesterView::RenderControlsWindow() {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_None);

    // Track if this window is focused for keyboard input routing
    m_controlsWindowWasFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

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

    // Only control if the track exists
    if (trackIndex < trackCount) {
        // Route to appropriate window's tracks based on active window
        switch (m_activeWindow) {
            case ActiveWindow::MAIN:
                // Main window tracks - toggle volume between 0.0 and 1.0
                {
                    const auto& track = state.getTrack(trackIndex);
                    float newVolume = (track.volume > 0.5f) ? 0.0f : 1.0f;
                    m_controller->setTrackVolume(trackIndex, newVolume);
                }
                break;

            case ActiveWindow::SECONDARY:
                // Future: Secondary window tracks
                // Would operate on a different track set or different controller instance
                break;
        }
    }
}

void TesterView::RenderEventsWindow() {
    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Events", &m_showEventsWindow);

    if (!m_controller) {
        ImGui::Text("No controller available");
        ImGui::End();
        return;
    }

    // Toggle Events window visibility from main menu
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Show Events", nullptr, &m_showEventsWindow);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::Text("Event-Driven Music System");
    ImGui::Separator();

    // Create Event buttons for different types
    const auto* songManager = m_controller->getSongManager();
    if (songManager) {
        // Master Event creation button
        if (ImGui::Button("+ Master Event")) {
            m_eventCreationType = EventCreationType::MASTER;
            m_targetSongName = "";
            m_showCreateEventDialog = true;
            strcpy(m_newEventName, "");
            m_newEventFadeTime = 1.0f;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(Global effects & tempo)");

        // Song Event creation buttons
        const auto& songs = songManager->getSongs();
        for (const auto& song : songs) {
            std::string buttonText = "+ " + song.name + " Event";
            if (ImGui::Button(buttonText.c_str())) {
                m_eventCreationType = EventCreationType::SONG;
                m_targetSongName = song.name;
                m_showCreateEventDialog = true;
                strcpy(m_newEventName, "");
                m_newEventFadeTime = 1.0f;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(Song tracks & tempo)");
        }

        if (songs.empty()) {
            ImGui::TextDisabled("Load songs to create song events");
        }
    }

    ImGui::Separator();

    // Render Master Events
    RenderMasterEvents();

    ImGui::Separator();

    // Render Song Events
    RenderSongEvents();

    // Show Create Event Dialog
    if (m_showCreateEventDialog) {
        ShowCreateEventDialog();
    }

    ImGui::End();
}

void TesterView::RenderMasterEvents() {
    const auto* songManager = m_controller->getSongManager();
    if (!songManager) {
        return;
    }

    const auto& masterBus = songManager->getMasterBus();

    if (ImGui::CollapsingHeader("Master Events", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Debug info
        ImGui::Text("Master Bus Events Count: %d", static_cast<int>(masterBus.events.size()));

        if (masterBus.events.empty()) {
            ImGui::TextDisabled("No master events loaded");
            ImGui::Text("Load a directory with _master.json to see master events");
        } else {
            ImGui::Text("Master Bus: %s", masterBus.name.c_str());

            for (const auto& event : masterBus.events) {
                ImGui::PushID(("master_" + event.name).c_str());

                // Event name and trigger button
                if (ImGui::Button(("Trigger##" + event.name).c_str())) {
                    m_controller->triggerMasterEvent(event.name);
                }
                ImGui::SameLine();
                ImGui::Text("%s (%.1fs fade)", event.name.c_str(), event.fadeTime);

                // Show event details in a tooltip
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Master Event: %s", event.name.c_str());
                    ImGui::Text("Fade Time: %.1f seconds", event.fadeTime);
                    ImGui::Text("Master Tempo: %.2fx", event.state.masterTempo);
                    ImGui::Text("Granular Tempo: %.2fx", event.state.granularTempo);
                    ImGui::Text("Volume: %.2f", event.state.volume);
                    ImGui::Text("Effects: %d", static_cast<int>(event.state.effects.size()));
                    ImGui::EndTooltip();
                }

                ImGui::PopID();
            }
        }
    }
}

void TesterView::RenderSongEvents() {
    const auto* songManager = m_controller->getSongManager();
    if (!songManager) {
        return;
    }

    const auto& songs = songManager->getSongs();

    if (ImGui::CollapsingHeader("Song Events", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (songs.empty()) {
            ImGui::TextDisabled("No songs loaded");
            ImGui::Text("Load a directory with song folders containing _song.json");
        } else {
            for (const auto& song : songs) {
                ImGui::PushID(song.name.c_str());

                // Song header
                bool songOpen = ImGui::TreeNode(song.name.c_str());
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Song: %s", song.name.c_str());
                    ImGui::Text("Events: %d", static_cast<int>(song.events.size()));
                    ImGui::Text("Path: %s", song.folderPath.string().c_str());
                    ImGui::EndTooltip();
                }

                if (songOpen) {
                    if (song.events.empty()) {
                        ImGui::TextDisabled("  No events in this song");
                    } else {
                        for (const auto& event : song.events) {
                            ImGui::PushID(event.name.c_str());

                            // Event trigger button
                            if (ImGui::Button(("Trigger##" + event.name).c_str())) {
                                m_controller->triggerSongEvent(song.name, event.name);
                            }
                            ImGui::SameLine();
                            ImGui::Text("%s (%.1fs fade)", event.name.c_str(), event.fadeTime);

                            // Show event details in a tooltip
                            if (ImGui::IsItemHovered()) {
                                ImGui::BeginTooltip();
                                ImGui::Text("Event: %s", event.name.c_str());
                                ImGui::Text("Fade Time: %.1f seconds", event.fadeTime);
                                ImGui::Text("Master Tempo: %.2fx", event.state.masterTempo);
                                ImGui::Text("Granular Tempo: %.2fx", event.state.granularTempo);
                                ImGui::Text("Tracks: %d",
                                            static_cast<int>(event.state.tracks.size()));

                                // Show track details
                                for (size_t i = 0; i < event.state.tracks.size(); ++i) {
                                    const auto& track = event.state.tracks[i];
                                    ImGui::Text("  %d: %s (vol:%.2f, %s)", static_cast<int>(i),
                                                track.file.c_str(), track.volume,
                                                track.volume > 0.0f ? "audible" : "silent");
                                }
                                ImGui::EndTooltip();
                            }

                            ImGui::PopID();
                        }
                    }
                    ImGui::TreePop();
                }

                ImGui::PopID();
            }
        }
    }
}

void TesterView::ShowCreateEventDialog() {
    ImGui::OpenPopup("Create Event");
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal("Create Event", &m_showCreateEventDialog)) {
        // Show which type of event we're creating
        if (m_eventCreationType == EventCreationType::MASTER) {
            ImGui::Text("Create new MASTER event from current state");
            ImGui::TextDisabled("This will affect global effects and master tempo");
        } else {
            ImGui::Text("Create new SONG event from current state");
            ImGui::TextDisabled("Song: %s", m_targetSongName.c_str());
            ImGui::TextDisabled("This will affect track volumes, effects, and song tempo");
        }
        ImGui::Separator();

        ImGui::InputText("Event Name", m_newEventName, sizeof(m_newEventName));
        ImGui::SliderFloat("Fade Time", &m_newEventFadeTime, 0.0f, 10.0f, "%.1f seconds");

        ImGui::Separator();
        ImGui::Text("Current State Preview:");

        // Show current state that would be captured
        if (m_controller) {
            const auto& state = m_controller->getState();
            if (m_eventCreationType == EventCreationType::MASTER) {
                ImGui::Text("Master Tempo: %.2fx", m_controller->getMasterTempo());
                ImGui::Text("Granular Tempo: %.2fx", m_controller->getGranularTempo());
                ImGui::Text("Bus Volume: %.2f", state.busVolume);
                ImGui::Text("Bus Effects: (will be captured)");
            } else {
                ImGui::Text("Tracks: %d", static_cast<int>(state.getTrackCount()));
                ImGui::Text("Master Tempo: %.2fx", m_controller->getMasterTempo());
                ImGui::Text("Granular Tempo: %.2fx", m_controller->getGranularTempo());
                ImGui::Text("Track Effects: (will be captured)");
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Create Event")) {
            if (strlen(m_newEventName) > 0) {
                bool success = false;

                if (m_eventCreationType == EventCreationType::MASTER) {
                    success = m_controller->createMasterEvent(m_newEventName, m_newEventFadeTime);
                    if (success) {
                        printf("SUCCESS: Created master event '%s' with fade time %.1f\n",
                               m_newEventName, m_newEventFadeTime);
                    } else {
                        printf("FAILED: Could not create master event '%s'\n", m_newEventName);
                    }
                } else {
                    success = m_controller->createSongEvent(m_targetSongName, m_newEventName,
                                                            m_newEventFadeTime);
                    if (success) {
                        printf(
                            "SUCCESS: Created song event '%s' for song '%s' with fade time %.1f\n",
                            m_newEventName, m_targetSongName.c_str(), m_newEventFadeTime);
                    } else {
                        printf("FAILED: Could not create song event '%s' for song '%s'\n",
                               m_newEventName, m_targetSongName.c_str());
                    }
                }

                if (success) {
                    ImGui::CloseCurrentPopup();
                    m_showCreateEventDialog = false;
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create event");
                }
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Event name required");
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
            m_showCreateEventDialog = false;
        }

        ImGui::EndPopup();
    }
}

AudioTester::StateSnapshot TesterView::CaptureCurrentState() {
    if (!m_controller) {
        return AudioTester::StateSnapshot{};
    }

    AudioTester::StateSnapshot snapshot;
    snapshot.masterTempo = m_controller->getMasterTempo();
    snapshot.granularTempo = m_controller->getGranularTempo();

    const auto& state = m_controller->getState();

    // Capture track states
    for (size_t i = 0; i < state.getTrackCount(); ++i) {
        const auto& track = state.getTrack(i);

        AudioTester::TrackStateExtended trackState;
        trackState.file = track.name; // Use name as file reference
        trackState.volume = track.volume;
        // Note: active field removed - tracks are always active, use volume for enable/disable
        // TODO: Capture effect states from AudioSystem

        snapshot.tracks.push_back(trackState);
    }

    return snapshot;
}

void TesterView::cleanup() {
    // ImGui cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}