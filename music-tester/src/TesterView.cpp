#include "TesterView.h"
#include "Logger.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h" g
#include <SDL2/SDL_opengl.h>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>

TesterView::TesterView() {
    // Initialize default directory
    InitializeDefaultDirectory();
    // Set directory input to default directory
    strncpy(m_dirInput, m_defaultDirectory.c_str(), DIR_INPUT_SIZE);
    m_dirInput[DIR_INPUT_SIZE - 1] = '\0';

    // Initialize tempo UI state
    m_masterTempoUI = 1.0f;
}

void TesterView::InitializeDefaultDirectory() {
    // Get user's Music folder
    const char* musicDir = nullptr;

#ifdef __APPLE__
    // On macOS, use the Music folder in the user's home directory
    const char* homeDir = getenv("HOME");
    if (homeDir) {
        m_defaultDirectory = std::string(homeDir) + "/Music/Dynamix";
    } else {
        m_defaultDirectory = "./Music/Dynamix";
    }
#elif defined(_WIN32)
    // On Windows, use the Music folder
    const char* userProfile = getenv("USERPROFILE");
    if (userProfile) {
        m_defaultDirectory = std::string(userProfile) + "\\Music\\Dynamix";
    } else {
        m_defaultDirectory = ".\\Music\\Dynamix";
    }
#else
    // On Linux/Unix, use the Music folder in the user's home directory
    const char* homeDir = getenv("HOME");
    if (homeDir) {
        m_defaultDirectory = std::string(homeDir) + "/Music/Dynamix";
    } else {
        m_defaultDirectory = "./Music/Dynamix";
    }
#endif

    // Create the directory if it doesn't exist
    if (!m_defaultDirectory.empty()) {
        std::filesystem::create_directories(m_defaultDirectory);
    }
}

void TesterView::DiscoverAvailableProjects() {
    m_availableProjects.clear();

    try {
        std::filesystem::path dynamixPath(m_defaultDirectory);
        if (!std::filesystem::exists(dynamixPath) || !std::filesystem::is_directory(dynamixPath)) {
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(dynamixPath)) {
            if (entry.is_directory()) {
                std::string projectName = entry.path().filename().string();

                // Check if this directory contains a _master.json file (indicating it's a project)
                std::filesystem::path masterJsonPath = entry.path() / "_master.json";
                if (std::filesystem::exists(masterJsonPath)) {
                    m_availableProjects.push_back(projectName);
                }
            }
        }

        // Sort projects alphabetically
        std::sort(m_availableProjects.begin(), m_availableProjects.end());

        // Update current project based on current directory
        std::string currentDir = m_controller->getCurrentDirectory();
        if (!currentDir.empty()) {
            std::filesystem::path currentPath(currentDir);
            std::string currentProjectName = currentPath.filename().string();

            // Check if current directory is a project
            std::filesystem::path masterJsonPath = currentPath / "_master.json";
            if (std::filesystem::exists(masterJsonPath)) {
                m_currentProject = currentProjectName;
            }
        }

        // Set current project if not set
        if (m_currentProject.empty() && !m_availableProjects.empty()) {
            m_currentProject = m_availableProjects[0];
            // Auto-load the first project
            std::string projectPath = m_defaultDirectory + "/" + m_currentProject;
            m_controller->setMusicDirectory(projectPath);

            // Auto-select the first song in the project
            const auto* songManager = m_controller->getSongManager();
            if (songManager) {
                const auto& songs = songManager->getSongs();
                if (!songs.empty()) {
                    // Use folder name instead of song name
                    std::string folderName = songs[0].folderPath.filename().string();
                    m_controller->setCurrentSong(folderName);
                    // Clear last triggered events when auto-loading first song
                    ClearLastTriggeredEvents();
                }
            }
        }
    } catch (const std::exception& e) {
        // Handle errors gracefully
        std::cout << "Error discovering projects: " << e.what() << std::endl;
    }
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

    // Render the menu bar
    RenderMenuBar();

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
    ImGui::Begin(GetWindowTitle().c_str());

    // Track if this window is focused for keyboard input routing
    m_mainWindowWasFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    RenderTrackControls();
    ImGui::Separator();
    RenderBusControls();

    // Add PLUS icon at the bottom for adding .ogg files to the current song
    ImGui::Separator();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 40);

    std::string currentSong = m_controller->getCurrentSong();
    if (!currentSong.empty()) {
        if (ImGui::Button("+", ImVec2(30, 30))) {
            m_showOggFileDialog = true;
            m_currentOggBrowserPath = m_defaultDirectory;
            RefreshOggBrowserEntries();
        }
        ImGui::SameLine();
        ImGui::Text("Add .ogg file to '%s'", currentSong.c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Click the + button to browse for .ogg files and add them to this song");
        }
    } else {
        ImGui::TextDisabled("No song loaded - load a song to add tracks");
    }

    // Show dialogs
    if (m_showNewMasterDialog) {
        RenderNewMasterDialog();
    }
    if (m_showNewSongDialog) {
        RenderNewSongDialog();
    }
    if (m_showFileDialog) {
        RenderFileDialog();
    }
    if (m_showOggFileDialog) {
        RenderOggFileDialog();
    }

    ImGui::End();
}

void TesterView::RenderTrackControls() {
    // Get tracks directly from the folder contents
    std::string currentSong = m_controller->getCurrentSong();
    if (currentSong.empty()) {
        ImGui::Text("No song loaded");
        return;
    }

    // Get the song folder path
    std::filesystem::path songFolderPath = m_controller->getCurrentSongFolderPath();
    if (songFolderPath.empty()) {
        ImGui::Text("No song folder found");
        return;
    }

    // Discover tracks in the folder (this is the source of truth)
    std::vector<std::string> trackFiles;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(songFolderPath)) {
            if (entry.is_regular_file()) {
                auto filepath = entry.path().string();
                if (m_controller->isSupportedFile(filepath)) {
                    trackFiles.push_back(entry.path().filename().string());
                }
            }
        }
    } catch (const std::exception& e) {
        ImGui::Text("Error reading folder: %s", e.what());
        return;
    }

    // Sort tracks alphabetically
    std::sort(trackFiles.begin(), trackFiles.end());

    ImGui::Text("Tracks (%d):", static_cast<int>(trackFiles.size()));
    for (size_t i = 0; i < trackFiles.size(); i++) {
        const auto& trackFile = trackFiles[i];
        ImGui::PushID(static_cast<int>(i));

        // Show keyboard shortcut for track (volume control)
        std::string keyText = (i < 9) ? std::to_string(i + 1) : "0";
        ImGui::Text("[%s] %s", keyText.c_str(), trackFile.c_str());

        // Delete button for track (positioned to the right)
        ImGui::SameLine();
        std::string trackDeleteId = "track_delete_" + trackFile;
        if (RenderDeleteButton(trackDeleteId, trackFile.c_str())) {
            // Track was deleted
            m_controller->removeTrackFromCurrentSong(trackFile);
        }

        // Volume slider (primary control for enable/disable)
        // Get volume from AudioState if available, otherwise default to 1.0
        const auto& state = m_controller->getState();
        float volume = 1.0f;
        if (i < state.getTrackCount()) {
            volume = state.getTrack(i).volume;
        }

        if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f)) {
            m_controller->setTrackVolume(i, volume);
        }

        // Collapsible filter controls (start collapsed)
        if (ImGui::TreeNodeEx("Effects", ImGuiTreeNodeFlags_None)) {
            drawFilterControls(i);
            ImGui::TreePop();
        }

        ImGui::PopID();
    }
}

void TesterView::drawFilterControls(size_t trackIndex) {
    const auto& audioSystem = m_controller->getAudioSystem();

    // Get fresh filter state on every frame to ensure UI sync
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

            // Get fresh filter state after any potential changes
            const auto& currentFilters = audioSystem.getFilters(trackIndex);
            auto it = currentFilters.find(filterName);
            bool filterExists = (it != currentFilters.end());

            if (filterExists) {
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
                float wetValue = 0.0f;
                ImGui::PushStyleColor(ImGuiCol_FrameBg,
                                      ImVec4(0.6f, 0.2f, 0.2f, 0.4f)); // Red for disabled
                if (ImGui::SliderFloat("wet", &wetValue, 0.0f, 1.0f)) {
                    m_controller->setTrackFilterParameter(trackIndex, filterName, 0, wetValue);
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

            // Get fresh filter state after any potential changes
            const auto& currentBusFilters = audioSystem.getBusFilters();
            auto it = currentBusFilters.find(filterName);
            bool filterExists = (it != currentBusFilters.end());

            if (filterExists) {
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
            std::string buttonText = "+ " + song.folderPath.filename().string() + " Event";
            if (ImGui::Button(buttonText.c_str())) {
                m_eventCreationType = EventCreationType::SONG;
                m_targetSongName = song.folderPath.filename().string();
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

                // Highlight if this is the last triggered master event
                bool isLastTriggered = (event.name == m_lastTriggeredMasterEvent);

                // Draw background highlight if this is the last triggered event
                if (isLastTriggered) {
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        ImGui::GetCursorScreenPos(),
                        ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() - 20,
                               ImGui::GetCursorScreenPos().y +
                                   ImGui::GetTextLineHeightWithSpacing()),
                        ImGui::ColorConvertFloat4ToU32(
                            ImVec4(1.0f, 1.0f, 0.8f, 0.3f)) // Light yellow
                    );
                }

                // Event name and trigger button
                if (ImGui::Button(("Trigger##" + event.name).c_str())) {
                    m_controller->triggerMasterEvent(event.name);
                    SetLastTriggeredMasterEvent(event.name);
                }
                ImGui::SameLine();

                // Event text with tooltip
                ImGui::Text("%s (%.1fs fade)", event.name.c_str(), event.fadeTime);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Master Event: %s", event.name.c_str());
                    ImGui::Text("Fade Time: %.1f seconds", event.fadeTime);
                    ImGui::Text("Master Tempo: %.2fx", event.state.masterTempo);
                    ImGui::Text("Granular Tempo: %.2fx", event.state.granularTempo);
                    ImGui::Text("Volume: %.2f", event.state.volume);
                    ImGui::Text("Effects: %d", static_cast<int>(event.state.effects.size()));
                    if (isLastTriggered) {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Last Triggered");
                    }
                    ImGui::EndTooltip();
                }

                // Delete button (separate, no interference with event tooltip)
                ImGui::SameLine();
                std::string deleteId = "master_" + event.name;
                if (RenderDeleteButton(deleteId, event.name.c_str())) {
                    // Event deletion confirmed
                    m_controller->deleteMasterEvent(event.name);
                    // Clear tracking if this was the last triggered event
                    if (isLastTriggered) {
                        ClearLastTriggeredEvents();
                    }
                }

                // Save button (disk icon) - new feature
                ImGui::SameLine();
                std::string saveId = "save_master_" + event.name;
                if (RenderSaveButton(saveId, event.name.c_str())) {
                    // Event save confirmed - overwrite with current state
                    bool success = m_controller->overwriteMasterEvent(event.name, event.fadeTime);
                    if (!success) {
                        strcpy(m_errorMessage,
                               ("Failed to overwrite master event '" + event.name + "'").c_str());
                        m_showErrorPopup = true;
                    }
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
                ImGui::PushID(song.folderPath.filename().c_str());

                // Song header - also expanded by default
                bool songOpen = ImGui::TreeNodeEx(song.folderPath.filename().c_str(),
                                                  ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Song: %s", song.folderPath.filename().c_str());
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

                            // Highlight if this is the last triggered song event
                            bool isLastTriggered =
                                (song.folderPath.filename().string() == m_lastTriggeredSongName &&
                                 event.name == m_lastTriggeredSongEvent);

                            // Draw background highlight if this is the last triggered event
                            if (isLastTriggered) {
                                ImGui::GetWindowDrawList()->AddRectFilled(
                                    ImGui::GetCursorScreenPos(),
                                    ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() -
                                               20,
                                           ImGui::GetCursorScreenPos().y +
                                               ImGui::GetTextLineHeightWithSpacing()),
                                    ImGui::ColorConvertFloat4ToU32(
                                        ImVec4(1.0f, 1.0f, 0.8f, 0.3f)) // Light yellow
                                );
                            }

                            // Event trigger button
                            if (ImGui::Button(("Trigger##" + event.name).c_str())) {
                                m_controller->triggerSongEvent(song.folderPath.filename().string(),
                                                               event.name);
                                SetLastTriggeredSongEvent(song.folderPath.filename().string(),
                                                          event.name);
                            }
                            ImGui::SameLine();

                            // Event text with tooltip
                            ImGui::Text("%s (%.1fs fade)", event.name.c_str(), event.fadeTime);
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
                                if (isLastTriggered) {
                                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
                                                       "Last Triggered");
                                }
                                ImGui::EndTooltip();
                            }

                            // Delete button (separate, no interference with event tooltip)
                            ImGui::SameLine();
                            std::string deleteId =
                                "song_" + song.folderPath.filename().string() + "_" + event.name;
                            if (RenderDeleteButton(deleteId, event.name.c_str())) {
                                // Event deletion confirmed
                                m_controller->deleteSongEvent(song.folderPath.filename().string(),
                                                              event.name);
                                // Clear tracking if this was the last triggered event
                                if (isLastTriggered) {
                                    ClearLastTriggeredEvents();
                                }
                            }

                            // Save button (disk icon) - new feature
                            ImGui::SameLine();
                            std::string saveId = "save_song_" +
                                                 song.folderPath.filename().string() + "_" +
                                                 event.name;
                            if (RenderSaveButton(saveId, event.name.c_str())) {
                                // Event save confirmed - overwrite with current state
                                bool success = m_controller->overwriteSongEvent(
                                    song.folderPath.filename().string(), event.name,
                                    event.fadeTime);
                                if (!success) {
                                    strcpy(m_errorMessage,
                                           ("Failed to overwrite song event '" + event.name +
                                            "' in song '" + song.folderPath.filename().string() +
                                            "'")
                                               .c_str());
                                    m_showErrorPopup = true;
                                }
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
                    // Check for duplicate before creating
                    if (m_controller->hasMasterEvent(m_newEventName)) {
                        strcpy(m_errorMessage,
                               ("Master event '" + std::string(m_newEventName) +
                                "' already exists. Use the save button to overwrite.")
                                   .c_str());
                        m_showErrorPopup = true;
                    } else {
                        success =
                            m_controller->createMasterEvent(m_newEventName, m_newEventFadeTime);
                        if (success) {
                            printf("SUCCESS: Created master event '%s' with fade time %.1f\n",
                                   m_newEventName, m_newEventFadeTime);
                        } else {
                            printf("FAILED: Could not create master event '%s'\n", m_newEventName);
                        }
                    }
                } else {
                    // Check for duplicate before creating
                    if (m_controller->hasSongEvent(m_targetSongName, m_newEventName)) {
                        strcpy(m_errorMessage, ("Song event '" + std::string(m_newEventName) +
                                                "' already exists in song '" + m_targetSongName +
                                                "'. Use the save button to overwrite.")
                                                   .c_str());
                        m_showErrorPopup = true;
                    } else {
                        success = m_controller->createSongEvent(m_targetSongName, m_newEventName,
                                                                m_newEventFadeTime);
                        if (success) {
                            printf("SUCCESS: Created song event '%s' for song '%s' with fade time "
                                   "%.1f\n",
                                   m_newEventName, m_targetSongName.c_str(), m_newEventFadeTime);
                        } else {
                            printf("FAILED: Could not create song event '%s' for song '%s'\n",
                                   m_newEventName, m_targetSongName.c_str());
                        }
                    }
                }

                if (success) {
                    ImGui::CloseCurrentPopup();
                    m_showCreateEventDialog = false;
                } else if (!m_showErrorPopup) {
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

    // Show Error Popup
    if (m_showErrorPopup) {
        ImGui::OpenPopup("Error");
        ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_FirstUseEver);

        if (ImGui::BeginPopupModal("Error", &m_showErrorPopup)) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error");
            ImGui::Separator();
            ImGui::TextWrapped("%s", m_errorMessage);
            ImGui::Separator();

            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
                m_showErrorPopup = false;
            }

            ImGui::EndPopup();
        }
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

bool TesterView::RenderHoldActionButton(const std::string& actionId, const char* buttonText,
                                        const char* tooltipText, HoldActionType actionType,
                                        const ImVec4& textColor, const ImVec4& progressColor,
                                        const ImVec4& bgColor) {
    ImGui::PushID(("hold_action_" + actionId).c_str());

    // Check if this is the button being held
    bool isThisButton = (m_holdActionState.actionId == actionId);

    // Set up text color and transparent background
    ImGui::PushStyleColor(ImGuiCol_Text, textColor);
    ImGui::PushStyleColor(ImGuiCol_Button,
                          ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent background
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent hover
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent active

    // Render the button
    bool buttonPressed = ImGui::Button(buttonText, ImVec2(20, 20));

    // Get button position for drawing progress circle
    ImVec2 buttonMin = ImGui::GetItemRectMin();
    ImVec2 buttonMax = ImGui::GetItemRectMax();
    ImVec2 center = ImVec2((buttonMin.x + buttonMax.x) * 0.5f, (buttonMin.y + buttonMax.y) * 0.5f);

    // Handle mouse interaction
    bool isHovered = ImGui::IsItemHovered();
    bool isPressed = ImGui::IsItemActive();

    // Calculate delta time using ImGui's built-in delta time
    float deltaTime = ImGui::GetIO().DeltaTime;

    if (isPressed && isHovered) {
        if (!m_holdActionState.isHolding || !isThisButton) {
            // Start holding
            m_holdActionState.actionId = actionId;
            m_holdActionState.holdTime = 0.0f;
            m_holdActionState.isHolding = true;
            m_holdActionState.hasTriggered = false; // Reset trigger flag
            m_holdActionState.actionType = actionType;
        } else {
            // Continue holding
            m_holdActionState.holdTime += deltaTime;
        }
    } else {
        // Released or not hovering
        if (isThisButton) {
            m_holdActionState.isHolding = false;
            m_holdActionState.holdTime = 0.0f;
            m_holdActionState.hasTriggered = false; // Reset trigger flag
            m_holdActionState.actionId = "";
        }
    }

    // Draw progress circle if holding
    if (isThisButton && m_holdActionState.isHolding) {
        float progress = m_holdActionState.holdTime / m_holdActionState.HOLD_DURATION;
        progress = std::min(progress, 1.0f);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        float radius = 9.0f;

        // Draw background circle
        ImU32 bgColorU32 =
            IM_COL32(static_cast<int>(bgColor.x * 255), static_cast<int>(bgColor.y * 255),
                     static_cast<int>(bgColor.z * 255), static_cast<int>(bgColor.w * 255));
        drawList->AddCircleFilled(center, radius, bgColorU32);

        // Draw progress pie slice
        if (progress > 0.0f) {
            ImU32 progressColorU32 = IM_COL32(
                static_cast<int>(progressColor.x * 255), static_cast<int>(progressColor.y * 255),
                static_cast<int>(progressColor.z * 255), static_cast<int>(progressColor.w * 255));
            float startAngle = -M_PI * 0.5f; // Start at top
            float endAngle = startAngle + (2.0f * M_PI * progress);

            // Draw pie slice
            ImVec2 points[32];
            int numPoints = (int)(progress * 30) + 2; // More points for smoother circle
            points[0] = center;                       // Center point

            for (int i = 1; i < numPoints; i++) {
                float angle = startAngle + (endAngle - startAngle) * (i - 1) / (numPoints - 2);
                points[i] =
                    ImVec2(center.x + cosf(angle) * radius, center.y + sinf(angle) * radius);
            }

            drawList->AddConvexPolyFilled(points, numPoints, progressColorU32);
        }

        // Check if hold is complete and hasn't been triggered yet
        if (progress >= 1.0f && !m_holdActionState.hasTriggered) {
            // Mark as triggered to prevent multiple actions
            m_holdActionState.hasTriggered = true;

            ImGui::PopStyleColor(4);
            ImGui::PopID();
            return true; // Action should happen
        }
    }

    // Show tooltip on hover
    if (isHovered) {
        ImGui::SetTooltip("%s", tooltipText);
    }

    ImGui::PopStyleColor(4);
    ImGui::PopID();
    return false; // No action
}

bool TesterView::RenderDeleteButton(const std::string& eventId, const char* eventName) {
    return RenderHoldActionButton(
        eventId, "X", ("Hold for 1 second to delete \"" + std::string(eventName) + "\"").c_str(),
        HoldActionType::DELETE, ImVec4(0.9f, 0.2f, 0.2f, 1.0f), // Red text
        ImVec4(0.9f, 0.2f, 0.2f, 1.0f),                         // Red progress
        ImVec4(0.9f, 0.2f, 0.2f, 0.3f)                          // Light red background
    );
}

bool TesterView::RenderSaveButton(const std::string& eventId, const char* eventName) {
    return RenderHoldActionButton(
        eventId, "S",
        ("Hold for 1 second to save (overwrite) \"" + std::string(eventName) + "\"").c_str(),
        HoldActionType::SAVE, ImVec4(0.2f, 0.8f, 0.2f, 1.0f), // Green text
        ImVec4(0.2f, 0.8f, 0.2f, 1.0f),                       // Green progress
        ImVec4(0.2f, 0.8f, 0.2f, 0.3f)                        // Light green background
    );
}

void TesterView::RenderNewMasterDialog() {
    ImGui::OpenPopup("New Master Directory");
    ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal("New Master Directory", &m_showNewMasterDialog)) {
        ImGui::Text("Create new master directory:");
        ImGui::Separator();

        ImGui::Text("Directory name:");
        ImGui::InputText("##master_name", m_newMasterName, sizeof(m_newMasterName));

        ImGui::Separator();

        if (ImGui::Button("Create")) {
            if (strlen(m_newMasterName) > 0) {
                bool success = m_controller->createNewMasterDirectory(m_newMasterName);
                if (success) {
                    ImGui::CloseCurrentPopup();
                    m_showNewMasterDialog = false;
                } else {
                    strcpy(m_errorMessage, "Failed to create master directory");
                    m_showErrorPopup = true;
                }
            } else {
                strcpy(m_errorMessage, "Directory name is required");
                m_showErrorPopup = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
            m_showNewMasterDialog = false;
        }

        ImGui::EndPopup();
    }
}

void TesterView::RenderNewSongDialog() {
    ImGui::OpenPopup("New Song Folder");
    ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal("New Song Folder", &m_showNewSongDialog)) {
        ImGui::Text("Create new song folder:");
        ImGui::Separator();

        ImGui::Text("Song name:");
        ImGui::InputText("##song_name", m_newSongName, sizeof(m_newSongName));

        ImGui::Separator();

        if (ImGui::Button("Create")) {
            if (strlen(m_newSongName) > 0) {
                bool success = m_controller->createNewSongFolder(m_newSongName);
                if (success) {
                    ImGui::CloseCurrentPopup();
                    m_showNewSongDialog = false;
                } else {
                    strcpy(m_errorMessage, "Failed to create song folder");
                    m_showErrorPopup = true;
                }
            } else {
                strcpy(m_errorMessage, "Song name is required");
                m_showErrorPopup = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
            m_showNewSongDialog = false;
        }

        ImGui::EndPopup();
    }
}

void TesterView::RenderFileDialog() {
    ImGui::OpenPopup("File Browser");
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal("File Browser", &m_showFileDialog)) {
        // Initialize browser path if empty
        if (m_currentBrowserPath.empty()) {
            m_currentBrowserPath = m_defaultDirectory;
            RefreshBrowserEntries();
        }

        // Path display and navigation
        ImGui::Text("Current Path: %s", m_currentBrowserPath.c_str());

        if (ImGui::Button("Go Up")) {
            std::filesystem::path currentPath(m_currentBrowserPath);
            if (currentPath.has_parent_path()) {
                m_currentBrowserPath = currentPath.parent_path().string();
                RefreshBrowserEntries();
                m_selectedEntry = -1;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Home")) {
            m_currentBrowserPath = m_defaultDirectory;
            RefreshBrowserEntries();
            m_selectedEntry = -1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            RefreshBrowserEntries();
        }

        ImGui::Separator();

        // Filter input
        ImGui::Text("Filter:");
        ImGui::SameLine();
        if (ImGui::InputText("##filter", m_browserFilter, sizeof(m_browserFilter))) {
            RefreshBrowserEntries();
        }

        ImGui::Separator();

        // File/directory list
        ImGui::BeginChild("##browser_list", ImVec2(0, 250), true);

        for (int i = 0; i < static_cast<int>(m_browserEntries.size()); ++i) {
            const auto& entry = m_browserEntries[i];
            std::string displayName = entry.filename().string();

            // Apply filter
            if (strlen(m_browserFilter) > 0) {
                std::string filter(m_browserFilter);
                std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
                std::string lowerName = displayName;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                if (lowerName.find(filter) == std::string::npos) {
                    continue;
                }
            }

            // Selectable item
            bool isSelected = (m_selectedEntry == i);
            if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                m_selectedEntry = i;
            }

            // Double-click to navigate
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                if (std::filesystem::is_directory(entry)) {
                    m_currentBrowserPath = entry.string();
                    RefreshBrowserEntries();
                    m_selectedEntry = -1;
                }
            }

            // Show icon or indicator
            ImGui::SameLine();
            if (std::filesystem::is_directory(entry)) {
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "[DIR]");
            } else {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[FILE]");
            }
        }

        ImGui::EndChild();

        ImGui::Separator();

        // Action buttons
        if (ImGui::Button("Select Directory")) {
            if (m_selectedEntry >= 0 &&
                m_selectedEntry < static_cast<int>(m_browserEntries.size())) {
                const auto& selectedEntry = m_browserEntries[m_selectedEntry];
                if (std::filesystem::is_directory(selectedEntry)) {
                    std::string selectedPath = selectedEntry.string();
                    strncpy(m_dirInput, selectedPath.c_str(), DIR_INPUT_SIZE);
                    m_dirInput[DIR_INPUT_SIZE - 1] = '\0';
                    m_controller->setMusicDirectory(selectedPath);
                    ImGui::CloseCurrentPopup();
                    m_showFileDialog = false;
                }
            } else {
                // Use current path
                strncpy(m_dirInput, m_currentBrowserPath.c_str(), DIR_INPUT_SIZE);
                m_dirInput[DIR_INPUT_SIZE - 1] = '\0';
                m_controller->setMusicDirectory(m_currentBrowserPath);
                ImGui::CloseCurrentPopup();
                m_showFileDialog = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
            m_showFileDialog = false;
        }

        ImGui::EndPopup();
    }
}

void TesterView::RefreshBrowserEntries() {
    m_browserEntries.clear();

    try {
        std::filesystem::path currentPath(m_currentBrowserPath);
        if (std::filesystem::exists(currentPath) && std::filesystem::is_directory(currentPath)) {
            for (const auto& entry : std::filesystem::directory_iterator(currentPath)) {
                // Skip hidden files on Unix-like systems
                std::string filename = entry.path().filename().string();
                if (filename.empty() || filename[0] == '.') {
                    continue;
                }
                m_browserEntries.push_back(entry.path());
            }

            // Sort entries: directories first, then files
            std::sort(m_browserEntries.begin(), m_browserEntries.end(),
                      [](const std::filesystem::path& a, const std::filesystem::path& b) {
                          bool aIsDir = std::filesystem::is_directory(a);
                          bool bIsDir = std::filesystem::is_directory(b);
                          if (aIsDir != bIsDir) {
                              return aIsDir > bIsDir; // Directories first
                          }
                          return a.filename().string() < b.filename().string(); // Alphabetical
                      });
        }
    } catch (const std::exception& e) {
        // Handle errors gracefully
        m_browserEntries.clear();
    }
}

void TesterView::RenderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        // File menu
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Master")) {
                m_showNewMasterDialog = true;
                strcpy(m_newMasterName, "");
            }
            if (ImGui::MenuItem("New Song")) {
                m_showNewSongDialog = true;
                strcpy(m_newSongName, "");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Browse...")) {
                m_showFileDialog = true;
            }
            if (ImGui::MenuItem("Load Current")) {
                m_controller->setMusicDirectory(m_dirInput);
            }
            ImGui::EndMenu();
        }

        // Unified Project/Song dropdown
        RenderProjectSongDropdown();

        // Directory input in menu bar
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
        ImGui::Text("Directory:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(300.0f);
        if (ImGui::InputText("##dir_menu", m_dirInput, DIR_INPUT_SIZE,
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            m_controller->setMusicDirectory(m_dirInput);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enter directory path and press Enter to load tracks and songs");
        }

        // Playback controls in menu bar
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
        const auto& state = m_controller->getState();
        if (ImGui::Button(state.globalPlaying ? "Pause" : "Play")) {
            m_controller->toggleGlobalPlayback();
        }

        ImGui::EndMainMenuBar();
    }
}

std::string TesterView::GetWindowTitle() {
    std::string currentSong = m_controller->getCurrentSong();
    if (!currentSong.empty()) {
        return "Dynamix - " + currentSong;
    } else {
        return "Dynamix - Music Tester";
    }
}

void TesterView::RenderOggFileDialog() {
    ImGui::OpenPopup("Add .ogg File to Song");
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal("Add .ogg File to Song", &m_showOggFileDialog)) {
        // Initialize browser path if empty
        if (m_currentOggBrowserPath.empty()) {
            m_currentOggBrowserPath = m_defaultDirectory;
            RefreshOggBrowserEntries();
        }

        std::string currentSong = m_controller->getCurrentSong();
        ImGui::Text("Adding .ogg file to song: %s", currentSong.c_str());
        ImGui::Separator();

        // Path display and navigation
        ImGui::Text("Current Path: %s", m_currentOggBrowserPath.c_str());

        if (ImGui::Button("Go Up")) {
            std::filesystem::path currentPath(m_currentOggBrowserPath);
            if (currentPath.has_parent_path()) {
                m_currentOggBrowserPath = currentPath.parent_path().string();
                RefreshOggBrowserEntries();
                m_selectedOggEntry = -1;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Home")) {
            m_currentOggBrowserPath = m_defaultDirectory;
            RefreshOggBrowserEntries();
            m_selectedOggEntry = -1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            RefreshOggBrowserEntries();
        }

        ImGui::Separator();

        // Filter input
        ImGui::Text("Filter:");
        ImGui::SameLine();
        if (ImGui::InputText("##ogg_filter", m_oggBrowserFilter, sizeof(m_oggBrowserFilter))) {
            RefreshOggBrowserEntries();
        }

        ImGui::Separator();

        // File list (only .ogg files)
        ImGui::BeginChild("##ogg_browser_list", ImVec2(0, 250), true);

        for (int i = 0; i < static_cast<int>(m_oggBrowserEntries.size()); ++i) {
            const auto& entry = m_oggBrowserEntries[i];
            std::string displayName = entry.filename().string();

            // Apply filter
            if (strlen(m_oggBrowserFilter) > 0) {
                std::string filter(m_oggBrowserFilter);
                std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
                std::string lowerName = displayName;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                if (lowerName.find(filter) == std::string::npos) {
                    continue;
                }
            }

            // Selectable item
            bool isSelected = (m_selectedOggEntry == i);
            if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                m_selectedOggEntry = i;
            }

            // Double-click to navigate into directories
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                if (std::filesystem::is_directory(entry)) {
                    m_currentOggBrowserPath = entry.string();
                    RefreshOggBrowserEntries();
                    m_selectedOggEntry = -1;
                }
            }

            // Show icon or indicator
            ImGui::SameLine();
            if (std::filesystem::is_directory(entry)) {
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "[DIR]");
            } else {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "[OGG]");
            }
        }

        ImGui::EndChild();

        ImGui::Separator();

        // Action buttons
        if (ImGui::Button("Add Selected File")) {
            // Debug logging
            std::cout << "Add Selected File button clicked!" << std::endl;
            std::cout << "Selected entry: " << m_selectedOggEntry << std::endl;
            std::cout << "Browser entries size: " << m_oggBrowserEntries.size() << std::endl;

            if (m_selectedOggEntry >= 0 &&
                m_selectedOggEntry < static_cast<int>(m_oggBrowserEntries.size())) {
                const auto& selectedEntry = m_oggBrowserEntries[m_selectedOggEntry];
                std::cout << "Selected entry path: " << selectedEntry.string() << std::endl;

                if (std::filesystem::is_regular_file(selectedEntry)) {
                    std::string sourcePath = selectedEntry.string();
                    std::string filename = selectedEntry.filename().string();
                    std::cout << "Source path: " << sourcePath << std::endl;
                    std::cout << "Filename: " << filename << std::endl;

                    // Check if file already exists in the song folder
                    std::filesystem::path songFolderPath = m_controller->getCurrentSongFolderPath();
                    std::filesystem::path destPath = songFolderPath / filename;
                    std::cout << "Destination path: " << destPath.string() << std::endl;

                    if (std::filesystem::exists(destPath)) {
                        // File already exists - show specific error
                        std::cout << "File already exists!" << std::endl;
                        strcpy(m_errorMessage,
                               ("File '" + filename + "' already exists in this song").c_str());
                        m_showErrorPopup = true;
                    } else {
                        // Try to copy the file
                        std::cout << "Attempting to copy file..." << std::endl;
                        if (CopyOggFileToSong(sourcePath, currentSong)) {
                            std::cout << "Copy successful!" << std::endl;
                            // Add to current event
                            if (!m_controller->addTrackToCurrentEvent(filename)) {
                                std::cout
                                    << "Failed to add track to current event or already present."
                                    << std::endl;
                            }
                            ImGui::CloseCurrentPopup();
                            m_showOggFileDialog = false;
                            // Don't reload the directory - this causes audio system state reset
                            // The track is already added to the system via addTrackToCurrentEvent
                        } else {
                            std::cout << "Copy failed!" << std::endl;
                            strcpy(m_errorMessage, "Failed to copy .ogg file to song folder");
                            m_showErrorPopup = true;
                        }
                    }
                } else {
                    std::cout << "Selected entry is not a regular file" << std::endl;
                }
            } else {
                std::cout << "No file selected or invalid selection" << std::endl;
                strcpy(m_errorMessage, "Please select a .ogg file to add");
                m_showErrorPopup = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
            m_showOggFileDialog = false;
        }

        ImGui::EndPopup();
    }
}

void TesterView::RefreshOggBrowserEntries() {
    m_oggBrowserEntries.clear();

    std::cout << "Refreshing .ogg browser entries from: " << m_currentOggBrowserPath << std::endl;

    try {
        std::filesystem::path currentPath(m_currentOggBrowserPath);
        if (std::filesystem::exists(currentPath) && std::filesystem::is_directory(currentPath)) {
            for (const auto& entry : std::filesystem::directory_iterator(currentPath)) {
                // Skip hidden files on Unix-like systems
                std::string filename = entry.path().filename().string();
                if (filename.empty() || filename[0] == '.') {
                    continue;
                }

                // Only show directories and .ogg files
                if (std::filesystem::is_directory(entry)) {
                    m_oggBrowserEntries.push_back(entry.path());
                    std::cout << "Found directory: " << filename << std::endl;
                } else if (std::filesystem::is_regular_file(entry)) {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".ogg") {
                        m_oggBrowserEntries.push_back(entry.path());
                        std::cout << "Found .ogg file: " << filename << std::endl;
                    }
                }
            }

            std::cout << "Total entries found: " << m_oggBrowserEntries.size() << std::endl;

            // Sort entries: directories first, then files
            std::sort(m_oggBrowserEntries.begin(), m_oggBrowserEntries.end(),
                      [](const std::filesystem::path& a, const std::filesystem::path& b) {
                          bool aIsDir = std::filesystem::is_directory(a);
                          bool bIsDir = std::filesystem::is_directory(b);
                          if (aIsDir != bIsDir) {
                              return aIsDir > bIsDir; // Directories first
                          }
                          return a.filename().string() < b.filename().string(); // Alphabetical
                      });
        } else {
            std::cout << "Path does not exist or is not a directory: " << m_currentOggBrowserPath
                      << std::endl;
        }
    } catch (const std::exception& e) {
        // Handle errors gracefully
        std::cout << "Exception in RefreshOggBrowserEntries: " << e.what() << std::endl;
        m_oggBrowserEntries.clear();
    }
}

bool TesterView::CopyOggFileToSong(const std::string& sourcePath, const std::string& songName) {
    try {
        // Get the song folder path
        std::filesystem::path songFolderPath = m_controller->getCurrentSongFolderPath();

        if (!std::filesystem::exists(songFolderPath) ||
            !std::filesystem::is_directory(songFolderPath)) {
            return false;
        }

        // Get the source file name
        std::filesystem::path sourceFilePath(sourcePath);
        std::string filename = sourceFilePath.filename().string();

        // Create the destination path
        std::filesystem::path destPath = songFolderPath / filename;

        // Check if file already exists - if so, don't allow the copy
        if (std::filesystem::exists(destPath)) {
            return false; // Signal that file already exists
        }

        // Copy the file
        std::filesystem::copy_file(sourcePath, destPath);

        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void TesterView::cleanup() {
    // ImGui cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void TesterView::RenderProjectSongDropdown() {
    // Discover projects if needed
    if (m_availableProjects.empty()) {
        DiscoverAvailableProjects();
    }

    // Get current song
    std::string currentSong = m_controller->getCurrentSong();

    // Project dropdown
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
    ImGui::Text("Project:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200.0f);

    std::string projectDisplayText = m_currentProject.empty() ? "No Project" : m_currentProject;
    if (ImGui::BeginCombo("##project", projectDisplayText.c_str())) {
        for (const auto& project : m_availableProjects) {
            bool isSelected = (project == m_currentProject);

            if (ImGui::Selectable(("📁 " + project).c_str(), isSelected)) {
                m_currentProject = project;
                // Load the project directory
                std::string projectPath = m_defaultDirectory + "/" + project;
                m_controller->setMusicDirectory(projectPath);

                // Auto-select the first song in the new project
                const auto* songManager = m_controller->getSongManager();
                if (songManager) {
                    const auto& songs = songManager->getSongs();
                    if (!songs.empty()) {
                        // Use folder name instead of song name
                        std::string folderName = songs[0].folderPath.filename().string();
                        m_controller->setCurrentSong(folderName);
                        // Clear last triggered events when switching projects
                        ClearLastTriggeredEvents();
                    }
                }
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        if (m_availableProjects.empty()) {
            ImGui::TextDisabled("No projects found");
        }

        ImGui::EndCombo();
    }

    // Song dropdown
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    ImGui::Text("Song:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200.0f);

    std::string songDisplayText = currentSong.empty() ? "No Song" : currentSong;
    if (ImGui::BeginCombo("##song", songDisplayText.c_str())) {
        const auto* songManager = m_controller->getSongManager();
        if (songManager) {
            const auto& songs = songManager->getSongs();

            for (const auto& song : songs) {
                // Use folder name instead of song name for consistency
                std::string folderName = song.folderPath.filename().string();
                bool songSelected = (folderName == currentSong);

                if (ImGui::Selectable(("🎵 " + folderName).c_str(), songSelected)) {
                    m_controller->setCurrentSong(folderName);
                    // Clear last triggered events when switching songs
                    ClearLastTriggeredEvents();
                }

                if (songSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            if (songs.empty()) {
                ImGui::TextDisabled("No songs in project");
            }
        } else {
            ImGui::TextDisabled("No song manager");
        }

        ImGui::EndCombo();
    }

    // Refresh button
    ImGui::SameLine();
    if (ImGui::Button("🔄")) {
        DiscoverAvailableProjects();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Refresh project list");
    }
}

// Event tracking methods implementation
void TesterView::SetLastTriggeredMasterEvent(const std::string& eventName) {
    m_lastTriggeredMasterEvent = eventName;
    // Don't clear song event tracking - allow both to be highlighted independently
}

void TesterView::SetLastTriggeredSongEvent(const std::string& songName,
                                           const std::string& eventName) {
    m_lastTriggeredSongEvent = eventName;
    m_lastTriggeredSongName = songName;
    // Don't clear master event tracking - allow both to be highlighted independently
}

void TesterView::ClearLastTriggeredEvents() {
    m_lastTriggeredMasterEvent = "";
    m_lastTriggeredSongEvent = "";
    m_lastTriggeredSongName = "";
}