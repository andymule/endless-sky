#include "TesterView.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <SDL2/SDL_opengl.h>
#include <cstring>
#include <iostream>

TesterView::TesterView() {
    // Initialize UI with default directory
    strncpy(m_dirInput, "sound_staging", DIR_INPUT_SIZE);
    m_dirInput[DIR_INPUT_SIZE - 1] = '\0';
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
        std::cerr << "Failed to initialize ImGui SDL2 backend" << std::endl;
        return false;
    }

    const char* glsl_version = "#version 150";
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        std::cerr << "Failed to initialize ImGui OpenGL3 backend" << std::endl;
        return false;
    }

    return true;
}

void TesterView::ProcessEvents(const SDL_Event& event) {
    ImGui_ImplSDL2_ProcessEvent(&event);

    // Handle spacebar for play/pause
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
        if (m_controller) {
            m_controller->toggleGlobalPlayback();
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
        ImGui::Text("%s", track.name.c_str());

        // Volume slider
        float volume = track.volume;
        if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f)) {
            m_controller->setTrackVolume(i, volume);
        }

        // Filter controls
        drawFilterControls(i);

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

void TesterView::cleanup() {
    // ImGui cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}